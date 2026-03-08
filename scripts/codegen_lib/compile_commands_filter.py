"""
compile_commands_filter
=======================
Filters a compile_commands.json entry list down to only the entries whose
transitively included project headers have changed since the last run.
"""

from __future__ import annotations

import hashlib
import os
import pickle
import re
import shlex
from pathlib import Path
from typing import Optional

__all__ = ["Cache", "filter_changed"]

_INCLUDE_RE = re.compile(
    r'^\s*#\s*include\s*(?:"([^"]+)"|<([^>]+)>)',
    re.MULTILINE,
)

_HEADER_SUFFIXES = {".h", ".hpp", ".hxx", ".h++", ".hh"}


def _is_relative_to(child: Path, parent: Path) -> bool:
    try:
        child.relative_to(parent)
        return True
    except ValueError:
        return False


def _extract_include_dirs(compiler_args: list[str], excluded: frozenset[Path]) -> list[Path]:
    """Return all -I / /I directories present in *compiler_args* as resolved Paths."""
    dirs: list[Path] = []
    i = 0
    while i < len(compiler_args):
        a = compiler_args[i]
        path_str: Optional[str] = None

        if a in ("-I", "/I"):                    # separate token: -I /some/path
            if i + 1 < len(compiler_args):
                path_str = compiler_args[i + 1]
                i += 1
        elif a.startswith("-I") and len(a) > 2:  # combined: -I/some/path
            path_str = a[2:]
        elif a.startswith("/I") and len(a) > 2:  # MSVC combined: /Isome\path
            path_str = a[2:]

        if path_str is not None:
            p = Path(path_str)
            if p.is_dir() and not any(_is_relative_to(p, x) for x in excluded):
                dirs.append(p)
        i += 1
    return dirs


def _resolve_include(
    include_path: str,
    is_angle: bool,
    current_dir: Path,
    include_dirs: list[Path],
) -> Optional[Path]:
    candidates: list[Path] = []
    if not is_angle:
        candidates.append(current_dir / include_path)
    for d in include_dirs:
        candidates.append(d / include_path)

    for c in candidates:
        try:
            resolved = c.resolve()
            if resolved.exists():
                return resolved
        except OSError:
            pass
    return None


def _transitive_headers(
    source: Path,
    include_dirs: list[Path],
    project_root: Path,
    exclude_roots: frozenset[Path],
    resolve_cache: dict[tuple[str, bool, Path], Optional[Path]],
    closure_cache: dict[Path, frozenset[Path]],
) -> frozenset[Path]:
    def _scan(file: Path) -> frozenset[Path]:
        if file in closure_cache:
            return closure_cache[file]

        # Insert a sentinel immediately to break include cycles before reading
        # the file, so a cycle A->B->A terminates without infinite recursion.
        closure_cache[file] = frozenset()

        try:
            text = file.read_text(encoding="utf-8-sig", errors="replace")
        except OSError:
            return frozenset()

        direct: set[Path] = set()
        for m in _INCLUDE_RE.finditer(text):
            quoted, angle = m.group(1), m.group(2)
            include_str = quoted or angle
            is_angle = angle is not None

            cache_key = (include_str, is_angle, file.parent)
            if cache_key not in resolve_cache:
                resolve_cache[cache_key] = _resolve_include(
                    include_str, is_angle, file.parent, include_dirs
                )
            resolved = resolve_cache[cache_key]

            if resolved is None:
                continue
            if resolved.suffix.lower() not in _HEADER_SUFFIXES:
                continue
            if not _is_relative_to(resolved, project_root):
                continue  # outside the project tree
            if any(_is_relative_to(resolved, ex) for ex in exclude_roots):
                continue  # inside an excluded subtree (e.g. ThirdParty)

            direct.add(resolved)

        # Full closure: direct includes plus their own transitive closures.
        closure: set[Path] = set()
        for h in direct:
            closure.add(h)
            closure.update(_scan(h))

        result = frozenset(closure)
        closure_cache[file] = result
        return result

    return _scan(source)


def _parse_args(entry: dict) -> tuple[Path, list[str]]:
    if "command" in entry:
        parts = shlex.split(entry["command"], posix=(os.name != "nt"))
    else:
        parts = list(entry["arguments"])

    source = Path(entry["file"]).resolve()

    # Flags whose *following* token is an output path to be dropped.
    _SKIP_NEXT: frozenset[str] = frozenset({
        "-o", "-MF", "-MT", "-MQ",   # GCC / Clang
        "/Fo", "/Fd", "/MF",         # MSVC (space-separated form)
    })
    # Flag prefixes whose entire token is an output specifier to be dropped.
    _SKIP_PREFIX: tuple[str, ...] = ("/Fo", "/Fd")

    args: list[str] = []
    skip_next = False
    for tok in parts[1:]:   # parts[0] is the compiler executable
        if skip_next:
            skip_next = False
            continue
        if tok in _SKIP_NEXT:
            skip_next = True
            continue
        if any(tok.startswith(p) for p in _SKIP_PREFIX):
            continue
        if Path(tok) == source:   # drop the source file itself
            continue
        args.append(tok)

    return source, args


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

class Cache:
    """
    Persistent store of per-file SHA-256 hashes used to detect changes.

    The cache maps each absolute file path (normalised to a forward-slash
    POSIX string, lower-cased on Windows) to the SHA-256 hex digest seen on
    the previous run.

    Parameters
    ----------
    path:
        File path where the cache is persisted.  Passed to :meth:`load` and
        :meth:`save`.  The default ``".ccf_cache"`` works for most projects;
        add it to ``.gitignore``.
    """

    def __init__(self) -> None:
        self._hashes: dict[str, str] = {}
        self._is_valid = False
        self._path: Optional[Path] = None

    # ------------------------------------------------------------------
    # Construction

    @classmethod
    def load(cls, path: str | Path = ".ccf_cache") -> "Cache":
        """
        Load a previously saved cache from *path*, or return an empty cache
        if the file does not exist or cannot be read.
        """
        cache = cls()
        cache._path = Path(path)
        if cache._path.exists():
            try:
                with cache._path.open("rb") as f:
                    data = pickle.load(f)
                    assert(isinstance(data, dict))
                    cache._is_valid = True
                    cache._hashes = data

                    for key in cache._hashes.keys():
                        if not Path(key).exists():
                            cache._is_valid = False
                            cache._hashes = {}
                            break
            except Exception:
                cache._hashes = {}
        return cache

    def is_valid(self) -> bool:
        return self._is_valid

    # ------------------------------------------------------------------
    # Persistence

    def save(self, path: Optional[str | Path] = None) -> None:
        """
        Write the cache to *path*.  If *path* is omitted the path given to
        :meth:`load` is used.  Raises ``ValueError`` if no path is known.
        """
        target = Path(path) if path is not None else self._path
        if target is None:
            raise ValueError(
                "No cache path known -- pass a path to save() or use Cache.load(path)."
            )
        with target.open("wb") as f:
            pickle.dump(self._hashes, f)

    # ------------------------------------------------------------------
    # Internal use by filter_changed

    def _key(self, p: Path) -> str:
        """Stable, case-normalised string key for *p*."""
        return p.as_posix().lower() if os.name == "nt" else p.as_posix()

    def _compute_digests(self, paths: list[Path]) -> dict[str, str]:
        """
        Hash every path in *paths* and return a ``{cache_key: digest}`` dict.

        Does **not** touch ``self._hashes``; the caller must collect digests
        for *all* entries before comparing or committing anything, so that a
        header shared by multiple entries is always compared against the digest
        from the previous run rather than one written for an earlier entry in
        the same run.
        """
        result: dict[str, str] = {}
        for p in paths:
            try:
                result[self._key(p)] = _sha256(p)
            except OSError:
                pass
        return result

    def _is_changed(self, digests: dict[str, str]) -> bool:
        """Return ``True`` if any digest in *digests* differs from the stored value."""
        return any(
            self._hashes.get(key) != digest
            for key, digest in digests.items()
        )

    def _commit(self, digests: dict[str, str]) -> None:
        """Write *digests* into the cache, overwriting any previous values."""
        self._hashes.update(digests)


# ---------------------------------------------------------------------------
# Hashing
# ---------------------------------------------------------------------------

def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


# ---------------------------------------------------------------------------
# Public filtering function
# ---------------------------------------------------------------------------

def filter_changed(
    commands: list[dict],
    cache: Cache,
    project_root: Optional[str | Path] = None,
    exclude_dirs: Optional[list[str | Path]] = None,
) -> list[dict]:
    """
    Return the subset of *commands* that need to be re-processed.

    An entry is considered changed if **any** of the following are true:

    * The source file (``.cpp``) itself has changed.
    * Any transitively ``#include``-reachable header inside *project_root*
      (and not under an excluded directory) has changed.
    """
    root = Path(project_root).resolve() if project_root is not None else Path.cwd().resolve()

    # Resolve excluded directories once up front.
    excluded: frozenset[Path] = frozenset(
        (root / Path(d)).resolve() for d in (exclude_dirs or [])
    )

    # Caches shared across all entries in this call.
    resolve_cache: dict[tuple[str, bool, Path], Optional[Path]] = {}
    closure_cache: dict[Path, frozenset[Path]] = {}

    entry_files: list[tuple[dict, list[Path]]] = []
    all_digests: dict[str, str] = {}

    for entry in commands:
        source, args = _parse_args(entry)
        include_dirs = _extract_include_dirs(args, excluded)
        headers = _transitive_headers(
            source, include_dirs, root, excluded, resolve_cache, closure_cache
        )
        files = [source, *sorted(headers)]
        entry_files.append((entry, files))
        all_digests.update(cache._compute_digests(files))

    # Compare all fresh digests against the previous run's values, then commit.
    changed = [
        entry for entry, files in entry_files
        if cache._is_changed(cache._compute_digests(files))
    ]
    cache._commit(all_digests)
    return changed

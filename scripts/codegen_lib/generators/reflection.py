from __future__ import annotations
from pathlib import Path
from jinja2 import Environment
import subprocess
import time

from ..model import ReflectedType


class ReflectionGenerator:
    def __init__(self, env: Environment, templates_dir: Path, clang_format_path: Path, clang_format_style: Path):
        self._templates_dir = templates_dir
        self._clang_format_path = clang_format_path
        self._clang_format_style = clang_format_style

        self._template = env.get_template("ReflectedType.cpp")

    def generate(self, reflected_types: list[ReflectedType], modules: dict[Path, tuple[str, str]]) -> None:
        types_by_module = {path: [] for path in modules}
        for t in reflected_types:
            types_by_module.setdefault(t.module_path, []).append(t)

        common_header = (self._templates_dir / "CommonGeneratedHeader.h").read_text()

        for module_path, types in types_by_module.items():
            name, include_prefix = modules[module_path]
            anchor = f"CallLinkerAnchor_{name}"
            header = module_path / "Reflection.gen.h"
            if include_prefix:
                header = module_path / "Public" / include_prefix / "Reflection.gen.h"
            header.parent.mkdir(parents=True, exist_ok=True)
            header_code = (
                common_header + "\n#pragma once\n\nnamespace FE\n{\n"
                + f"    //! @brief Retain {name}'s generated reflection in statically linked programs.\n"
                + f"    void {anchor}();\n}} // namespace FE\n"
            )
            self._write_generated(header, header_code)

            includes = "".join(f"#include <{h.as_posix()}>\n" for h in sorted({t.header_path for t in types}))
            code = common_header + "\n#include <Core/RTTI/ReflectionContext.h>\n\n" + includes
            code += f"\n\nnamespace FE\n{{\n    void {anchor}() {{}}\n}} // namespace FE\n\n\n"
            for t in sorted(types, key=lambda t: t.id):
                code += self._template.render(type=t) + "\n\n\n"
            self._write_generated(module_path / "Reflection.gen.cpp", code)

    def _write_generated(self, path: Path, code: str) -> None:
        # libclang may still map existing generated files. Replace formatted output rather than truncating a mapped file.
        temporary = path.with_name(path.stem + ".tmp" + path.suffix)
        temporary.write_text(code, encoding="utf-8", newline="\n")
        self._run_clang_format(temporary)
        if path.exists() and path.read_bytes() == temporary.read_bytes():
            temporary.unlink()
        else:
            # Windows indexers can briefly hold the destination without sharing delete access.
            for attempt in range(8):
                try:
                    temporary.replace(path)
                    break
                except PermissionError:
                    if attempt == 7:
                        temporary.unlink()
                        raise
                    time.sleep(0.1 * (attempt + 1))


    def _run_clang_format(self, file_path: Path) -> None:
        subprocess.run(
            [
                str(self._clang_format_path),
                "-i",
                str(file_path),
                f"-style=file:{self._clang_format_style.as_posix()}",
            ],
            check=True,
        )

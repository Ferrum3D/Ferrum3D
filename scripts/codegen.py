from dataclasses import dataclass
from pathlib import Path
import json
import os
import re
import shlex
import time

from jinja2 import Environment, FileSystemLoader, select_autoescape

from codegen_lib.clang_parser import ParseConfig, parse_file
from codegen_lib.generators.reflection import ReflectionGenerator
from codegen_lib.gpudb_parser import generate_gpudb

script_path = os.path.abspath(__file__)
script_directory = os.path.dirname(script_path)
os.chdir(script_directory)

PROJECT_DIR = Path.cwd().parent.absolute()
LLVM_DIR = PROJECT_DIR / "ThirdParty/llvm"

HEADER_SUFFIXES = {".h", ".hh", ".hpp", ".hxx", ".h++"}
INCLUDE_PATTERN = re.compile(r'^\s*#\s*include\s*(?:"([^"]+)"|<([^>]+)>)', re.MULTILINE)


@dataclass(frozen=True)
class Project:
    name: str
    root: Path


REFLECTION_PROJECTS = [
    Project("Core", PROJECT_DIR / "FerrumCore"),
]


def _is_relative_to(path: Path, parent: Path) -> bool:
    try:
        path.relative_to(parent)
        return True
    except ValueError:
        return False


def _split_command(entry: dict) -> list[str]:
    if "arguments" in entry:
        return list(entry["arguments"])
    return [token.strip('"') for token in shlex.split(entry["command"], posix=False)]


def _extract_compiler_args(entry: dict) -> list[str]:
    eastl_config = PROJECT_DIR / "FerrumCore/Private/Core/Base/EASTLConfig.h"
    compiler_args = [
        "-x",
        "c++",
        "-std=c++20",
        f'-DEASTL_USER_CONFIG_HEADER="{eastl_config.as_posix()}"',
    ]
    command_tokens = _split_command(entry)
    index = 1
    while index < len(command_tokens):
        argument = command_tokens[index]
        if argument in ("-D", "/D", "-I", "/I", "-external:I"):
            index += 1
            if index < len(command_tokens):
                prefix = "-D" if argument in ("-D", "/D") else "-I"
                compiler_args.append(prefix + command_tokens[index])
        elif argument.startswith("-external:I"):
            compiler_args.append("-I" + argument[len("-external:I"):])
        elif argument.startswith(("-D", "/D")):
            compiler_args.append("-D" + argument[2:])
        elif argument.startswith(("-I", "/I")):
            compiler_args.append("-I" + argument[2:])
        index += 1

    return compiler_args


def _extract_include_dirs(compiler_args: list[str]) -> list[Path]:
    include_dirs = []
    index = 0
    while index < len(compiler_args):
        argument = compiler_args[index]
        include_dir = None
        if argument in ("-I", "/I", "-external:I"):
            index += 1
            if index < len(compiler_args):
                include_dir = compiler_args[index]
        elif argument.startswith("-external:I"):
            include_dir = argument[len("-external:I"):]
        elif argument.startswith(("-I", "/I")):
            include_dir = argument[2:]
        if include_dir:
            include_dirs.append(Path(include_dir).resolve())
        index += 1

    return include_dirs


def _find_project_command(commands: list[dict], project: Project) -> dict:
    for entry in commands:
        source_path = Path(entry["file"]).resolve()
        if (
            _is_relative_to(source_path, project.root)
            and not _is_relative_to(source_path, project.root / "Tests")
            and source_path.suffix == ".cpp"
            and not source_path.name.endswith(".gen.cpp")
        ):
            return entry
    raise RuntimeError(f"No compile command found for project {project.name}")


def _find_headers(project: Project) -> list[Path]:
    tests_dir = project.root / "Tests"
    return sorted(
        path.resolve()
        for path in project.root.rglob("*")
        if path.is_file()
        and path.suffix.lower() in HEADER_SUFFIXES
        and not _is_relative_to(path, tests_dir)
    )


def _resolve_include(include_path: str, current_dir: Path, include_dirs: list[Path]) -> Path | None:
    for root in [current_dir, *include_dirs]:
        candidate = (root / include_path).resolve()
        if candidate.is_file():
            return candidate
    return None


def _sort_headers_by_dependencies(headers: list[Path], include_dirs: list[Path]) -> list[Path]:
    header_set = set(headers)
    dependencies = {}
    for header in headers:
        contents = header.read_text(encoding="utf-8-sig", errors="replace")
        dependencies[header] = {
            resolved
            for match in INCLUDE_PATTERN.finditer(contents)
            if (resolved := _resolve_include(match.group(1) or match.group(2), header.parent, include_dirs)) in header_set
        }

    sorted_headers = []
    visiting = set()
    visited = set()

    def visit(header: Path) -> None:
        if header in visited or header in visiting:
            return
        visiting.add(header)
        for dependency in sorted(dependencies[header]):
            visit(dependency)
        visiting.remove(header)
        visited.add(header)
        sorted_headers.append(header)

    for header in headers:
        visit(header)
    return sorted_headers


def _generate_synthetic_tu(headers: list[Path]) -> str:
    return "\n".join(f'#include "{header.as_posix()}"' for header in headers) + "\n"


def main():
    start_time = time.perf_counter()

    templates_dir = Path("./templates")
    clang_format_path = LLVM_DIR / "clang-format.exe"
    clang_format_style = Path("../.clang-format")

    env = Environment(loader=FileSystemLoader(str(templates_dir)), autoescape=select_autoescape())

    generate_gpudb(
        env=env,
        templates_dir=templates_dir,
        project_root=PROJECT_DIR,
        clang_format_path=clang_format_path,
        clang_format_style=clang_format_style
    )

    compile_commands_path = Path("../cmake-build/windows-codegen/compile_commands.json")
    with open(compile_commands_path, "r") as f:
        commands = json.load(f)

    reflected_types_dict = {}
    parse_config = ParseConfig(project_dir=PROJECT_DIR, llvm_dir=LLVM_DIR)
    for project in REFLECTION_PROJECTS:
        command = _find_project_command(commands, project)
        compiler_args = _extract_compiler_args(command)
        headers = _sort_headers_by_dependencies(_find_headers(project), _extract_include_dirs(compiler_args))
        synthetic_tu = _generate_synthetic_tu(headers)
        synthetic_tu_path = project.root / "Reflection.synthetic.cpp"
        reflected_types_dict.update(parse_file(synthetic_tu_path, synthetic_tu, compiler_args, parse_config) or {})

    reflected_types = list(reflected_types_dict.values())
    for t in reflected_types:
        same_id = [x for x in reflected_types if x.id == t.id]
        ok = len(same_id) == 1 and same_id[0] is t
        if not ok:
            raise Exception(f'These types have the same id: {", ".join(x.qualified_name for x in same_id)}')

    generator = ReflectionGenerator(
        env=env,
        templates_dir=templates_dir,
        clang_format_path=clang_format_path,
        clang_format_style=clang_format_style,
    )
    generator.generate(reflected_types)

    end_time = time.perf_counter()
    print(f'Completed in {end_time - start_time:.2f} seconds')


if __name__ == "__main__":
    main()

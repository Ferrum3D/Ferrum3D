from __future__ import annotations

from pathlib import Path
import subprocess

from jinja2 import Environment, FileSystemLoader, select_autoescape

from ..model import ReflectedType


class ReflectionGenerator:
    def __init__(self, templates_dir: Path, clang_format_path: Path, clang_format_style: Path):
        self._templates_dir = templates_dir
        self._clang_format_path = clang_format_path
        self._clang_format_style = clang_format_style

        env = Environment(loader=FileSystemLoader(str(templates_dir)), autoescape=select_autoescape())
        self._template = env.get_template("ReflectedType.cpp")

    def generate(self, reflected_types: list[ReflectedType]) -> None:
        types_by_module = {}
        for t in reflected_types:
            types_by_module.setdefault(t.module_path, []).append(t)

        common_header = (self._templates_dir / "CommonGeneratedHeader.h").read_text()

        for module_path, types in types_by_module.items():
            headers = {t.header_path for t in types}

            generated_file = module_path / "Reflection.gen.cpp"
            with open(generated_file, "w") as f:
                f.write(common_header)
                f.write("\n")

                for h in headers:
                    f.write(f"#include <{h.as_posix()}>\n")
                f.write("\n\n\n")

                for t in types:
                    code = self._template.render(type=t)
                    f.write(code)
                    f.write("\n\n\n")

            self._run_clang_format(generated_file)

    def _run_clang_format(self, file_path: Path) -> None:
        subprocess.run(
            [
                str(self._clang_format_path),
                "-i",
                str(file_path),
                f"-style=file:{self._clang_format_style.as_posix()}",
            ]
        )

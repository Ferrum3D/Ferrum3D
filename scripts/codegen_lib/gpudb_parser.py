import os
import re
import shutil
import subprocess
from pathlib import Path
from typing import NoReturn
from jinja2 import Environment, FileSystemLoader, select_autoescape


GRAPHICS_DIR = Path("Modules/Graphics/Framework")
CPP_DIR_RELATIVE = Path("Public/Graphics/Tables")
HLSL_DIR_RELATIVE = Path("Shaders/Shaders/Tables")
CPP_DIR = GRAPHICS_DIR / CPP_DIR_RELATIVE
HLSL_DIR = GRAPHICS_DIR / HLSL_DIR_RELATIVE
GPUDB_DIR = GRAPHICS_DIR / "Gpudb"


class TableColumn:
    def __init__(self, name: str, typename: str) -> None:
        self.name = name
        self.typename = typename


class Table:
    def __init__(self, name: str, includes: list[str], columns: list[TableColumn]) -> None:
        self.name = name
        self.includes = includes
        self.columns = columns

        self.element_count_per_page = f'DB::kTablePageSize / ({' + '.join(f'sizeof({x.typename})' for x in self.columns)})'

        self.offsets = {}
        if len(columns) > 0:
            self.offsets[columns[0].name] = '0'

        for i in range(1, len(columns)):
            self.offsets[columns[i].name] = f'kOffset_{columns[i - 1].name} + sizeof({columns[i - 1].typename}) * kElementCountPerPage'


def _syntax_error(line: str) -> NoReturn:
    raise Exception(f'Syntax error: {line}')


def _parse_file(filename: Path, tables: list[Table]):
    includes = []
    with open(filename) as f:
        while True:
            line = f.readline() # do not strip because we need to keep newline characters
            if line == '':
                break

            line = line.strip()
            if line == '':
                continue

            if line.startswith('#include <') and line.endswith('>'):
                includes.append(line)
                continue

            m = re.match(r'table\s+([A-Za-z0-9_]+)', line)
            if not m:
                _syntax_error(line)

            table_name = m.group(1)
            columns = []

            line = f.readline().strip()
            if line != '{':
                _syntax_error(line)

            line = f.readline()
            while line.strip() != '}':
                if line == '':
                    raise Exception("Unexpected EOF")
                
                line = line.strip()
                m = re.match(r'(\S+)\s+([A-Za-z0-9_]+)', line)
                if not m:
                    _syntax_error(line)

                column_type = m.group(1)
                column_name = m.group(2)
                columns.append(TableColumn(column_name, column_type))
                line = f.readline()

            tables.append(Table(table_name, includes, columns))
            includes = []


def _generate_cpp(env: Environment, table: Table) -> str:
    template = env.get_template("GpuDatabaseTable.h")
    return template.render(table=table) + '\n'


def _generate_hlsl(env: Environment, table: Table) -> str:
    template = env.get_template("GpuDatabaseTable.hlsli")
    return template.render(table=table) + '\n'


def _clear_folder(folder: Path):
    if folder.exists():
        shutil.rmtree(folder)

    folder.mkdir(parents=True, exist_ok=True)


def _generate_cmake(tables: list[Table], basedir: str, ext: str, varname: str) -> str:
    text = f'set({varname}\n'
    text += f'    {basedir}/Forwards.{ext}\n'
    for table in tables:
        text += f'    {basedir}/{table.name}.{ext}\n'
    text += ')\n'
    return text


def _generate_forwards_cpp(tables: list[Table]) -> str:
    text = '#pragma once\n\nnamespace FE::Graphics\n{\n'
    for table in tables:
        text += f'    struct {table.name};\n'
    text += '} // namespace FE::Graphics\n'
    return text


def _generate_forwards_hlsl(tables: list[Table]) -> str:
    text = '#pragma once\n\n'
    for table in tables:
        text += f'struct {table.name};\n'
    return text


def _run_clang_format(clang_format_path: Path, clang_format_style: Path, file_path: Path) -> None:
    subprocess.run(
        [
            str(clang_format_path),
            "-i",
            str(file_path),
            f"-style=file:{clang_format_style.as_posix()}",
        ]
    )


def generate_gpudb(env: Environment, templates_dir: Path, project_root: Path, clang_format_path: Path, clang_format_style: Path):
    cpp_dir = project_root / CPP_DIR
    hlsl_dir = project_root / HLSL_DIR

    common_header = (templates_dir / "CommonGeneratedHeader.h").read_text() + '\n'

    _clear_folder(cpp_dir)
    _clear_folder(hlsl_dir)

    gpudb_dir = project_root / GPUDB_DIR
    tables: list[Table] = []
    for item in gpudb_dir.rglob('*.gpudb'):
        if item.is_file():
            _parse_file(item, tables)

    cpp_forwards = common_header + _generate_forwards_cpp(tables)
    hlsl_forwards = common_header + _generate_forwards_hlsl(tables)
    (cpp_dir / 'Forwards.h').write_text(cpp_forwards, newline='\n')
    (hlsl_dir / 'Forwards.hlsli').write_text(hlsl_forwards, newline='\n')

    for table in tables:
        cpp_code = common_header + _generate_cpp(env, table)
        cpp_code_path = cpp_dir / f'{table.name}.h'
        cpp_code_path.write_text(cpp_code, newline='\n')
        _run_clang_format(clang_format_path, clang_format_style, cpp_code_path)

        hlsl_code = common_header + _generate_hlsl(env, table)
        hlsl_code_path = hlsl_dir / f'{table.name}.hlsli'
        hlsl_code_path.write_text(hlsl_code, newline='\n')
        _run_clang_format(clang_format_path, clang_format_style, hlsl_code_path)

    cpp_cmake = _generate_cmake(tables, CPP_DIR_RELATIVE.as_posix(), 'h', 'GPUDB_TABLE_SOURCES')
    hlsl_cmake = _generate_cmake(tables, HLSL_DIR_RELATIVE.as_posix(), 'hlsli', 'GPUDB_TABLE_SHADERS')

    (cpp_dir / 'TableList.cmake').write_text(cpp_cmake, newline='\n')
    (hlsl_dir / 'TableList.cmake').write_text(hlsl_cmake, newline='\n')


if __name__ == "__main__":
    script_path = os.path.abspath(__file__)
    script_directory = os.path.join(os.path.dirname(script_path), '..')
    os.chdir(script_directory)

    env = Environment(loader=FileSystemLoader('./templates'), autoescape=select_autoescape())

    tables = []
    _parse_file(Path("./codegen_lib/test.gpudb"), tables)

    for table in tables:
        s = _generate_cpp(env, table)
        print(s)

    for table in tables:
        s = _generate_hlsl(env, table)
        print(s)

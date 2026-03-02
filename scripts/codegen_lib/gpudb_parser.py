import os
import re
from pathlib import Path
from typing import NoReturn
from jinja2 import Environment, FileSystemLoader, select_autoescape

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


def _parse_file(filename: str, tables: list[Table]):
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
    return template.render(table=table)


def _generate_hlsl(env: Environment, table: Table) -> str:
    template = env.get_template("GpuDatabaseTable.hlsli")
    return template.render(table=table)


def generate_gpudb(env: Environment, project_root: Path, clang_format_path: Path, clang_format_style: Path):
    pass


if __name__ == "__main__":
    script_path = os.path.abspath(__file__)
    script_directory = os.path.join(os.path.dirname(script_path), '..')
    os.chdir(script_directory)

    env = Environment(loader=FileSystemLoader('./templates'), autoescape=select_autoescape())

    tables = []
    _parse_file("./codegen_lib/test.gpudb", tables)
    
    for table in tables:
        s = _generate_cpp(env, table)
        print(s)

    for table in tables:
        s = _generate_hlsl(env, table)
        print(s)

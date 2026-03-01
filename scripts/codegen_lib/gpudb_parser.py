import os
import re
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


def syntax_error(line: str) -> NoReturn:
    raise Exception(f'Syntax error: {line}')


def parse_file(filename: str, tables: list[Table]):
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
                syntax_error(line)

            table_name = m.group(1)
            columns = []

            line = f.readline().strip()
            if line != '{':
                syntax_error(line)

            line = f.readline()
            while line.strip() != '}':
                if line == '':
                    raise Exception("Unexpected EOF")
                
                line = line.strip()
                m = re.match(r'(\S+)\s+([A-Za-z0-9_]+)', line)
                if not m:
                    syntax_error(line)

                column_type = m.group(1)
                column_name = m.group(2)
                columns.append(TableColumn(column_name, column_type))
                line = f.readline()

            tables.append(Table(table_name, includes, columns))
            includes = []


def generate(env: Environment, table: Table) -> str:
    header_template = env.get_template("GpuDatabaseTable.h")
    return header_template.render(table=table)


if __name__ == "__main__":
    script_path = os.path.abspath(__file__)
    script_directory = os.path.join(os.path.dirname(script_path), '..')
    os.chdir(script_directory)

    env = Environment(loader=FileSystemLoader('./templates'), autoescape=select_autoescape())

    tables = []
    parse_file("./codegen_lib/test.gpudb", tables)
    
    for table in tables:
        s = generate(env, table)
        print(s)

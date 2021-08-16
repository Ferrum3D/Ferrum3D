from scripts.common.ferrum_files import get_ferrum_files
import uuid
import re


class RttiDeclaration:
    def __init__(self, line_number, file_name, uuid_value) -> None:
        self.line_number = line_number
        self.uuid_value = uuid_value
        self.filename = file_name
        pass

    def __eq__(self, value) -> bool:
        return value.uuid_value == self.uuid_value


filenames = get_ferrum_files()

declarations = []
error_count = 0

pattern = re.compile(r'FE_(CLASS|STRUCT)_RTTI\([\w\d]+,\s+"([A-Z0-9\-]+)"\)')


def proc_line(line_text, line_number, file_name, uuid_str):
    global error_count
    uuid_value = uuid.UUID(uuid_str)
    decl = RttiDeclaration(line_number, file_name, uuid_value)
    try:
        prev_idx = declarations.index(decl)
        prev = declarations[prev_idx]
        print('=' * 80)
        print("Validation error at line {} of file {}: "
              + "the specified UUID was declared twice".format(line_number, file_name))
        print("Note: First declaration was at line {} of file {}".format(prev.line_number, prev.filename))
        print("Note: The UUID was " + uuid_str)
        print("Note: See declaration: {}".format(line_text))
        error_count += 1
    except ValueError:
        pass
    declarations.append(decl)
    pass


def process_files():
    for filename in filenames:
        with open(filename) as f:
            for n, line in enumerate(f):
                search_result = pattern.search(line)
                if search_result:
                    proc_line(line, n, filename, search_result.group(2))

    print("Checked {} files.".format(len(filenames)))
    if error_count == 0:
        print("Type UUID validation finished without errors.")
    else:
        print("Type UUID validation finished, {} errors found.".format(error_count))

from functools import reduce
import glob
import uuid
import re

engine_dirs = [
    "FeAssetCompiler",
    "FeCore",
    "FeModules",
    "FeTestProject"
]

extensions = [ "cpp", "h" ]

filenames = [ glob.glob(d + '/**/*.' + ext, recursive=True) for d in engine_dirs for ext in extensions ]
filenames = reduce(list.__add__, filenames)

class RttiDeclaration:
    def __init__(self, line_number, filename, uuid_value) -> None:
        self.line_number = line_number
        self.uuid_value = uuid_value
        self.filename = filename
        pass
    def __eq__(self, o: object) -> bool:
        return o.uuid_value == self.uuid_value

declarations = []
error_count = 0

def proc_line(line, line_number, filename, uuid_str):
    global error_count
    uuid_value = uuid.UUID(uuid_str)
    decl = RttiDeclaration(line_number, filename, uuid_value)
    try:
        prev_idx = declarations.index(decl)
        prev = declarations[prev_idx]
        print('=' * 80)
        print("Validation error at line {} of file {}: the specified UUID was declared twise".format(line_number, filename))
        print("Note: First declaration was at line {} of file {}".format(prev.line_number, prev.filename))
        print("Note: The UUID was " + uuid_str)
        print("Note: See declaration: {}".format(line))
        error_count += 1
    except(ValueError):
        pass
    declarations.append(decl)
    pass

regex = re.compile(r'FE_(CLASS|STRUCT)_RTTI\([\w\d]+,\s+"([A-Z0-9\-]+)"\)')
for filename in filenames:
    with open(filename) as f:
        for n, line in enumerate(f):
            m = regex.search(line)
            if m:
                proc_line(line, n, filename, m.group(2))

if error_count == 0:
    print("Type UUID validation finished without errors")
else:
    print("Type UUID validation finished, {} errors found".format(error_count))

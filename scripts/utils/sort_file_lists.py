from scripts.common.ferrum_files import get_ferrum_files

filenames = get_ferrum_files(extensions=["txt"], filename="CMakeLists.txt")


def group_files_by_directory(files):
    files_by_directory = {}
    for file in files:
        directory = file[:file.rfind('/')]
        if directory not in files_by_directory:
            files_by_directory[directory] = []
        files_by_directory[directory].append(file)
    return files_by_directory


def not_whitespace(s):
    return s and not s.isspace()


def split_into_lines(lines):
    result = []
    for line in lines:
        result.extend(line.split())
    return result


def formatted_lines(lines):
    lines = split_into_lines(lines)
    lines = [line.strip() for line in lines]
    lines = [line for line in lines if not_whitespace(line)]
    files_by_directory = group_files_by_directory(lines)
    result = []
    for directory in sorted(files_by_directory.keys()):
        result.extend(sorted(files_by_directory[directory]))
        result.append('')
    result = [f'    {line}\n' for line in result]
    return result if not_whitespace(result[-1]) else result[:-1]


def process_files():
    for filename in filenames:
        try:
            with open(filename) as f:
                lines = f.readlines()
            new_lines = []
            begin_index = 0
            found_begin = False
            found_end = False
            for i, line in enumerate(lines):
                if '# FERRUM_SORT_LINES BEGIN' in line:
                    new_lines.extend(lines[:i+1])
                    begin_index = i
                    found_begin = True
                    continue
                if '# FERRUM_SORT_LINES END' in line:
                    new_lines.extend(formatted_lines(lines[begin_index+1:i]))
                    new_lines.extend(lines[i:])
                    found_end = True
                    break
            for i in range(len(new_lines)):
                if new_lines[i].strip() == ')':
                    new_lines[i] = ')\n'
                if not_whitespace(new_lines[i].strip()):
                    continue
                new_lines[i] = '\n'
            if new_lines and (not found_begin or not found_end):
                raise Exception("FERRUM_SORT_LINES BEGIN and FERRUM_SORT_LINES END must be used together")
            if new_lines and found_begin and found_end:
                with open(filename, 'w') as f:
                    f.writelines(new_lines)
        except UnicodeDecodeError:
            print(f'Cannot decode file {filename}')

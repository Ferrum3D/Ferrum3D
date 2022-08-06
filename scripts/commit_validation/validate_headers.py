from scripts.common.ferrum_files import get_ferrum_files
from termcolor import cprint

filenames = get_ferrum_files(extensions=["h", "cpp"])
error_count = 0


def process_files():
    global error_count
    for filename in filenames:
        try:
            with open(filename, encoding='utf-8') as f:
                if "#pragma once" not in f.readline():
                    if filename.endswith(".h"):
                        print('=' * 80)
                        cprint('Validation error ', 'red', end='')
                        print(f"in file {filename}: No \"#pragma once\" in header")
                        error_count += 1
                else:
                    if filename.endswith(".cpp"):
                        print('=' * 80)
                        cprint('Validation error ', 'red', end='')
                        print(f"in file {filename}: \"#pragma once\" in cpp file")
                        error_count += 1
        except UnicodeDecodeError:
            print(f'Cannot decode file {filename}')

    print("Checked {} files.".format(len(filenames)))
    if error_count == 0:
        print("Header validation finished without errors.")
    else:
        print(f"Header validation finished, {error_count} errors found.")

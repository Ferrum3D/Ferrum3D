from scripts.common.ferrum_files import get_ferrum_files

filenames = get_ferrum_files(extensions=["h"])
error_count = 0


def process_files():
    global error_count
    for filename in filenames:
        with open(filename) as f:
            if "#pragma once" not in f.readline():
                print('=' * 80)
                print("Validation error in file {}: No \"#pragma once\" in header".format(filename))
                error_count += 1

    print("Checked {} files.".format(len(filenames)))
    if error_count == 0:
        print("Header validation finished without errors.")
    else:
        print("Header validation finished, {} errors found.".format(error_count))

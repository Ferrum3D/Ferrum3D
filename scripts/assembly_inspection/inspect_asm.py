from scripts.common.common import run_command, get_vs_path
import os
import sys
import re


DEMANGLED_NAME_FILTER = [
    '__cdecl', 'public:', 'private:', 'protected:', 'struct', 'class'
]


def filter_asm_line(line: str):
    pattern = re.compile(r'.*# -- Begin function (.*)')
    match = pattern.match(line)
    if match:
        function_name = match.group(1)
        demangled = run_command(['../../demumble.exe', function_name], return_stdout=True)
        demangled = filter(lambda x: x not in DEMANGLED_NAME_FILTER, demangled.split())
        return f'"{" ".join(demangled)}":\n'
    if any(x in line for x in ('\t.', '# -- End function')):
        return None
    if any(line.startswith(x) for x in ('# %', '"', '.')):
        return None
    return line


def run_clang_command(filename, output):
    clang = 'clang++'
    if sys.platform == 'win32':
        clang = get_vs_path() + r'\VC\Tools\Llvm\x64\bin' + os.sep + clang + '.exe'
    print(clang)
    args = [
        clang,
        '-O3',
        '-S', '-masm=intel',
        '-std=c++17', '-m64', '-msse4.1',
        '-DFE_SSE41_SUPPORTED=1', '-DFE_SSE3_SUPPORTED=1', '-DFE_FINLINE=__cdecl',
        filename,
        # TODO: generalize include directory for different projects
        f'-I..{os.sep}FerrumCore',
        '-o', output
    ]
    code = run_command(args)
    print(f'Successfully generated assembly code for file' if code
          else f'Failed to generate assembly code for file')
    return code


def inspect_assembly(filename: str):
    source_file = filename
    if filename.endswith('.h'):
        source_file = f'{filename[:-2]}_temp_copy.cpp'
        output_file = f'{filename[:-2]}.asm'
        with open(filename) as f:
            code = [line for line in f if '#pragma once' not in line]
        with open(source_file, 'w') as f:
            f.writelines(code)
        del code
    elif filename.endswith('.cpp'):
        output_file = f'{filename[:-4]}.asm'
    else:
        print('Only C++ source files and headers are allowed')
        return False

    success = run_clang_command(source_file, output_file)
    if source_file != filename:
        os.remove(source_file)

    with open(output_file) as f:
        assembly_code = f.readlines()
    assembly_code = filter(None, map(filter_asm_line, assembly_code))
    with open(output_file, 'w') as f:
        f.writelines(assembly_code)
    return success

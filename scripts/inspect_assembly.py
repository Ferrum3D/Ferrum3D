from scripts.assembly_inspection.inspect_asm import inspect_assembly
import os


if __name__ == '__main__':
    inspect_assembly('..' + os.sep + input('Enter file name: '))

from scripts.common.common import run_command
import sys


def run_cmake_command(config_name: str):
    args = [
        'cmake',
        '-B', f'Build{config_name.capitalize()}',
        '-S', '.',
        f'-DCMAKE_BUILD_TYPE={config_name.capitalize()}'
    ]
    if sys.platform == 'win32':
        args += '-A', 'x64'
        args += '-G', 'Visual Studio 16 2019'
    code = run_command(args)
    print(f'Successfully generated build files for {config_name.capitalize()}' if code
          else f'Failed to generate build files for {config_name.capitalize()}')
    return code


def main():
    print('Running Ferrum3D Engine setup...')
    msg = 'succeeded' if all(map(run_cmake_command, ['Debug', 'Release'])) else 'failed'
    print(f'Project setup {msg}')


if __name__ == '__main__':
    main()

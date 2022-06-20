from scripts.common.common import run_command
import sys
import time
from termcolor import cprint


def get_cmake_version():
    args = ['cmake', '--version']
    return run_command(args, return_stdout=True).splitlines()[0].split()[2]


def get_cmake_generator():
    cmake_vs_generators = {
        '15': 'Visual Studio 15 2017',
        '16': 'Visual Studio 16 2019',
        '17': 'Visual Studio 17 2022',
    }
    args = [
        r'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe',
        '-latest',
        '-property', 'catalog_productDisplayVersion'
    ]
    v = run_command(args, return_stdout=True)
    if v is None:
        cprint('Error: ', 'red', end='')
        print('Failed to get Visual Studio version.')
        v = input('Enter your Visual Studio version manually (15, 16 or 17): ')
    v = v.split('.')[0]
    if v not in cmake_vs_generators:
        cprint('Error: ', 'red', end='')
        print(f'Visual Studio version {v} is not supported.')
        return None
    return cmake_vs_generators[v]


cmake_generator = ''


def run_dotnet_build(build_type: str):
    args = [
        'dotnet',
        'build',
        'Managed/Ferrum3D/',
        '--configuration', build_type
    ]
    code = run_command(args, hide_output=False)
    if not code:
        cprint('Error: ', 'red', end='')
        print(f'Failed to build project for {build_type.capitalize()}')
    else:
        cprint(f'Successfully built project for {build_type.capitalize()}', 'green')
    return code


def run_cmake_build(build_type: str):
    args = [
        'cmake',
        '--build',
        f'Build{build_type.capitalize()}',
        '--config', build_type
    ]
    code = run_command(args, hide_output=False)
    if not code:
        cprint('Error: ', 'red', end='')
        print(f'Failed to build project for {build_type.capitalize()}')
    else:
        cprint(f'Successfully built project for {build_type.capitalize()}', 'green')
    return code


def run_cmake_setup(config_name: str):
    args = [
        'cmake',
        '-B', f'Build{config_name.capitalize()}',
        '-S', '.',
        f'-D CMAKE_BUILD_TYPE={config_name.capitalize()}'
    ]
    if sys.platform == 'win32':
        # args += '-A', 'x64'
        args += '-G', cmake_generator
    code = run_command(args)
    if not code:
        cprint('Error: ', 'red', end='')
        print(f'Failed to setup project for {config_name.capitalize()}')
    else:
        cprint(f'Successfully setup project for {config_name.capitalize()}', 'green')
    return code


def run_build_stage(name: str, cmake_command, build_types: list):
    cprint('\n============================================================', 'yellow')
    cprint(f'Running project {name}...\n\n', 'yellow')
    success = all(map(cmake_command, build_types))
    if not success:
        sys.exit(1)


def main():
    start_time = time.time()
    build_types = ['Debug', 'Release']

    global cmake_generator
    cprint('Running Ferrum3D Engine setup...', 'yellow')
    cmake_generator = get_cmake_generator()
    if cmake_generator is None:
        sys.exit(1)
    cprint(f'Using CMake generator {cmake_generator}...', 'yellow')
    cmake_version = get_cmake_version()
    if cmake_version is None:
        cprint('Error: ', 'red', end='')
        print('CMake is not installed.')
        sys.exit(1)
    cprint(f'Using CMake version {cmake_version}...', 'yellow')

    cprint('\n============================================================', 'yellow')
    cprint('Running setup and build of C++ project...', 'yellow')
    run_build_stage('setup', run_cmake_setup, build_types)
    run_build_stage('build', run_cmake_build, build_types)

    cprint('\n============================================================', 'yellow')
    cprint('Running setup and build of C# project...', 'yellow')
    run_build_stage('build', run_dotnet_build, build_types)
    cprint('\n============================================================', 'yellow')
    cprint(f'Setup and build finished in {time.time() - start_time:.2f} seconds.', 'green')
    run_command(['Managed/Ferrum3D/Ferrum.Samples.Triangle/bin/Debug/Ferrum.Samples.Triangle.exe'],
                working_directory='Managed/Ferrum3D/Ferrum.Samples.Triangle/bin/Debug/')


if __name__ == '__main__':
    main()

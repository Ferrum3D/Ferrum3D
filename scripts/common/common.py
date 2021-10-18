import subprocess


def get_vs_path():
    # TODO: Can be in a different path, check all possibilities
    return r'C:\Program Files (x86)\Microsoft Visual Studio\2019\Community'


def run_command(arguments, *, return_stdout=False):
    proc = subprocess.run(arguments, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    if proc.returncode:
        print(f'Process "{arguments[0]}" exited with code {proc.returncode}')
        print(f'Stderr:\n{proc.stderr.decode("utf-8")}')
    if not return_stdout:
        return proc.returncode == 0
    else:
        return proc.stdout.decode('utf-8')

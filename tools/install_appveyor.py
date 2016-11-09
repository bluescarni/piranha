import os

def run_unbuffered_command(raw_command, directory):
    import shlex
    from subprocess import Popen, PIPE, STDOUT
    proc = Popen(shlex.split(raw_command), cwd=directory,
                 stdout=PIPE, stderr=STDOUT)
    output = []
    while True:
        line = proc.stdout.readline()
        if not line:
            break
        if VERBOSE:
            print(line)
        output.append(line)
    proc.communicate()
    if proc.returncode:
        raise RuntimeError(output)
    return output

import urllib.request
urllib.request.urlretrieve('https://github.com/bluescarni/binary_deps/raw/master/x86_64-6.2.0-release-posix-seh-rt_v5-rev1.7z', 'mw64.7z')
run_unbuffered_command('7z x -oC:\ mw64.7z > NUL', os.getcwd())

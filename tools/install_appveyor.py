import urllib.request
import os

def run_unbuffered_command(raw_command, directory = None, verbose = True):
    # Helper function to run a command and display optionally its output
    # unbuffered.
    import shlex
    from subprocess import Popen, PIPE, STDOUT
    proc = Popen(shlex.split(raw_command), cwd=directory,
                 stdout=PIPE, stderr=STDOUT)
    output = []
    while True:
        line = proc.stdout.readline()
        if not line:
            break
        if verbose:
            print(str(line,'utf-8'))
        output.append(line)
    proc.communicate()
    if proc.returncode:
        raise RuntimeError(output)
    return output

# Get mingw and set the path.
urllib.request.urlretrieve(r'https://github.com/bluescarni/binary_deps/raw/master/x86_64-6.2.0-release-posix-seh-rt_v5-rev1.7z', 'mw64.7z')
run_unbuffered_command(r'7z x -oC:\\ mw64.7z > NUL', verbose = False)
os.environ['PATH'] = r'C:\\mingw64\\bin;%PATH%'
print(os.environ['PATH'])

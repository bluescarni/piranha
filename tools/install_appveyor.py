import os

def wget(url,out):
    import urllib.request
    print('Downloading "' + url + '" as "' + out + '"')
    urllib.request.urlretrieve(url,out)

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
wget(r'https://github.com/bluescarni/binary_deps/raw/master/x86_64-6.2.0-release-posix-seh-rt_v5-rev1.7z', 'mw64.7z')
run_unbuffered_command(r'7z x -oC:\\ mw64.7z > NUL')
os.environ['PATH'] = r'C:\\mingw64\\bin;'+os.environ['PATH']

# Download deps.
wget(r'https://github.com/bluescarni/binary_deps/raw/master/gmp_mingw_64.7z', 'gmp.7z')
wget(r'https://github.com/bluescarni/binary_deps/raw/master/mpfr_mingw_64.7z', 'mpfr.7z')
wget(r'https://github.com/bluescarni/binary_deps/raw/master/boost_mingw_64.7z', 'boost.7z')
wget(r'https://github.com/bluescarni/binary_deps/raw/master/msgpack_mingw_64.7z', 'msgpack.7z')

# Extract them.
run_unbuffered_command(r'7z x -aoa -oC:\\ gmp.7z > NUL')
run_unbuffered_command(r'7z x -aoa -oC:\\ mpfr.7z > NUL')
run_unbuffered_command(r'7z x -aoa -oC:\\ boost.7z > NUL')
run_unbuffered_command(r'7z x -aoa -oC:\\ msgpack.7z > NUL')

# Set the path so that the precompiled libs can be found.
os.environ['PATH'] = os.environ['PATH'] + r'c:\\local\\lib'

# Build type setup.
BUILD_TYPE = os.environ['BUILD_TYPE']
is_python_build = False
if 'Python' in BUILD_TYPE:
    is_python_build = True
    # Hard code this for the moment, eventually we'll have different choices.
    pinterp = r'c:\\Python35\\python.exe'
    # NOTE: the dep for Python35 is there already, no need to fetch and unpack.
    wget(r'https://bootstrap.pypa.io/get-pip.py','get-pip.py')
    run_unbuffered_command(pinterp + ' get-pip.py')
    pip = r'c:\\Python35\\scripts\\pip'
    run_unbuffered_command(pip + ' install numpy')
    run_unbuffered_command(pip + ' install mpmath')

import os

def wget(url,out):
    import urllib.request
    print('Downloading "' + url + '" as "' + out + '"')
    urllib.request.urlretrieve(url,out)

def run_unbuffered_command(raw_command, directory = None, verbose = True):
    # Helper function to run a command and display optionally its output
    # unbuffered.
    import shlex
    import sys
    from subprocess import Popen, PIPE, STDOUT
    proc = Popen(shlex.split(raw_command), cwd=directory,
                 stdout=PIPE, stderr=STDOUT)
    if verbose:
        output = ''
        while True:
            line = proc.stdout.readline()
            if not line:
                break
            line = str(line[:-1],'utf-8')
            print(line)
            sys.stdout.flush()
            output += line
        proc.communicate()
    else:
        output = str(proc.communicate()[0],'utf-8')
    if proc.returncode:
        raise RuntimeError(output)
    return output

# Get mingw and set the path.
wget(r'https://github.com/bluescarni/binary_deps/raw/master/x86_64-6.2.0-release-posix-seh-rt_v5-rev1.7z', 'mw64.7z')
run_unbuffered_command(r'7z x -oC:\\ mw64.7z', verbose = False)
ORIGINAL_PATH = os.environ['PATH']
os.environ['PATH'] = r'C:\\mingw64\\bin;'+os.environ['PATH']

# Download common deps.
wget(r'https://github.com/bluescarni/binary_deps/raw/master/gmp_mingw_64.7z', 'gmp.7z')
wget(r'https://github.com/bluescarni/binary_deps/raw/master/mpfr_mingw_64.7z', 'mpfr.7z')
wget(r'https://github.com/bluescarni/binary_deps/raw/master/boost_mingw_64.7z', 'boost.7z')
wget(r'https://github.com/bluescarni/binary_deps/raw/master/msgpack_mingw_64.7z', 'msgpack.7z')
# Extract them.
run_unbuffered_command(r'7z x -aoa -oC:\\ gmp.7z', verbose = False)
run_unbuffered_command(r'7z x -aoa -oC:\\ mpfr.7z', verbose = False)
run_unbuffered_command(r'7z x -aoa -oC:\\ boost.7z', verbose = False)
run_unbuffered_command(r'7z x -aoa -oC:\\ msgpack.7z', verbose = False)

# Set the path so that the precompiled libs can be found.
os.environ['PATH'] = os.environ['PATH'] + r';c:\\local\\lib'

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
    run_unbuffered_command(pip + ' install twine')
    twine = r'c:\\Python35\\scripts\\twine'

# Proceed to the build.
os.makedirs('build')
os.chdir('build')

if BUILD_TYPE == 'Python35':
    run_unbuffered_command(r'cmake -G "MinGW Makefiles" ..  -DBUILD_PYRANHA=yes -DCMAKE_BUILD_TYPE=Release -DBoost_LIBRARY_DIR_RELEASE=c:\\local\\lib -DBoost_INCLUDE_DIR=c:\\local\\include -DGMP_INCLUDE_DIR=c:\\local\\include -DGMP_LIBRARIES=c:\\local\\lib\\libgmp.a -DMPFR_INCLUDE_DIR=c:\\local\\include -DMPFR_LIBRARIES=c:\\local\\lib\\libmpfr.a -DPIRANHA_WITH_BZIP2=yes -DBZIP2_INCLUDE_DIR=c:\\local\\include -DBZIP2_LIBRARY_RELEASE=c:\\local\\lib\\libboost_bzip2-mgw62-mt-1_62.dll -DPIRANHA_WITH_MSGPACK=yes -DPIRANHA_WITH_ZLIB=yes -DMSGPACK-C_INCLUDE_DIR=c:\\local\\include -DZLIB_INCLUDE_DIR=c:\\local\\include -DZLIB_LIBRARY_RELEASE=c:\\local\\lib\\libboost_zlib-mgw62-mt-1_62.dll -DBoost_PYTHON_LIBRARY_RELEASE=c:\\local\\lib\\libboost_python3-mgw62-mt-1_62.dll -DPYTHON_EXECUTABLE=C:\\Python35\\python.exe -DPYTHON_LIBRARY=C:\\Python35\\libs\\python35.dll')

run_unbuffered_command(r'cmake --build . --target install')

if is_python_build:
    # Run the Python tests.
    run_unbuffered_command(pinterp + r' -c "import pyranha.test; pyranha.test.run_test_suite()"')
    # Build the wheel.
    import shutil
    os.chdir('wheel')
    shutil.move(r'C:\\Python35\\Lib\\site-packages\\pyranha',r'.')
    DLL_LIST = [_[:-1] for _ in open('mingw_wheel_libs.txt','r').readlines()]
    for _ in DLL_LIST:
        shutil.copy(_,'pyranha')
    run_unbuffered_command(pinterp + r' setup.py bdist_wheel')
    os.environ['PATH'] = ORIGINAL_PATH
    run_unbuffered_command(pip + r' install dist\\' + os.listdir('dist')[0])
    run_unbuffered_command(pinterp + r' -c "import pyranha.test; pyranha.test.run_test_suite()"')
    # if os.environ['APPVEYOR_REPO_BRANCH'] == 'master' or True:
    #     run_unbuffered_command(twine + r' upload --repository-url https://testpypi.python.org/pypi -u bluescarni  dist\\*')

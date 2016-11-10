from setuptools import setup
from setuptools.dist import Distribution
from distutils import util
import sys

NAME = 'pyranha'
VERSION = '@piranha_VERSION@'
DESCRIPTION = 'A computer algebra system for celestial mechanics'
LONG_DESCRIPTION = 'Pyranha is a Python CAS specialised for celestial mechanics applications.'
URL = 'https://github.com/bluescarni/piranha'
AUTHOR = 'Francesco Biscani'
AUTHOR_EMAIL = 'bluescarni@gmail.com'
LICENSE = 'GPLv3+/LGPL3+'
CLASSIFIERS = [
    # How mature is this project? Common values are
    #   3 - Alpha
    #   4 - Beta
    #   5 - Production/Stable
    'Development Status :: 4 - Beta',

    'Operating System :: OS Independent',

    # Indicate who your project is intended for
    'Intended Audience :: Science/Research',
    'Topic :: Scientific/Engineering',
    'Topic :: Scientific/Engineering :: Astronomy',
    'Topic :: Scientific/Engineering :: Mathematics',
    'Topic :: Scientific/Engineering :: Physics',

    # Pick your license as you wish (should match "license" above)
    'License :: OSI Approved :: GNU General Public License v3 or later (GPLv3+)',
    'License :: OSI Approved :: GNU Lesser General Public License v3 or later (LGPLv3+)',

    # Specify the Python versions you support here. In particular, ensure
    # that you indicate whether you support Python 2, Python 3 or both.
    'Programming Language :: Python :: 2',
    'Programming Language :: Python :: 3'
]
KEYWORDS = 'computer_algebra CAS science math physics astronomy'
INSTALL_REQUIRES = ['numpy','mpmath']
PLATFORMS = ['Unix','Windows','OSX']

class BinaryDistribution(Distribution):
    def has_ext_modules(foo):
        return True

# The nambe of the Boost Python library depends on the Python version.
BOOST_PYTHON_LIB = 'libboost_python'+ str(sys.version_info[0]) + '-mgw62-mt-1_62.dll'
DLL_LIST = ['libboost_iostreams-mgw62-mt-1_62.dll',BOOST_PYTHON_LIB,'libboost_serialization-mgw62-mt-1_62.dll','libboost_zlib-mgw62-mt-1_62.dll','libboost_bzip2-mgw62-mt-1_62.dll','libgcc_s_seh-1.dll','libstdc++-6.dll','libwinpthread-1.dll']

setup(name=NAME,
    version=VERSION,
    description=DESCRIPTION,
    long_description=LONG_DESCRIPTION,
    url=URL,
    author=AUTHOR,
    author_email=AUTHOR_EMAIL,
    license=LICENSE,
    classifiers=CLASSIFIERS,
    keywords=KEYWORDS,
    platforms=PLATFORMS,
    install_requires=INSTALL_REQUIRES,
    packages=['pyranha','pyranha._tutorial'],
    package_dir = {
            'pyranha': 'pyranha',
            'pyranha._tutorial': util.convert_path('pyranha/_tutorial')},
    # Include pre-compiled extension
    package_data={'pyranha': ['_core.pyd'] + DLL_LIST},
    distclass=BinaryDistribution)

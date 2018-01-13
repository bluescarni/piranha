Changelog
=========

%%version%% (unreleased)
------------------------

New
~~~

- Initial integration of the mp++ library in piranha (so far affecting
  only the mp_integer class).

- Revamped build system: removed cruft, restructured directory layout,
  adopted modern CMake idioms, added support for package installation,
  separated benchmarks in own dir.

Changes
~~~~~~~

- Bump the minimum python version to 2.7.

- Require Boost >= 1.58 and CMake >= 3.2.

- Remove for the time being all functionality related to polynomial
  division/gcd, including rational functions.

- Updated the copyright date.

Fix
~~~

- Fix missing #undef for an internal macro (`#104 <https://github.com/bluescarni/piranha/pull/104>`__).

- Fix pyranha's compilation against recent Boost versions (`#110 <https://github.com/bluescarni/piranha/pull/110>`__).

- Build system fixes for recent GCC versions (`#110 <https://github.com/bluescarni/piranha/pull/110>`__).

- Fix building with BZip2 support on older CMake versions.

v0.10 (2017-01-15)
------------------

New
~~~

- Add pyranha's binary package for Python 3.6.

- Add advanced option in the build system to enable/disable the
  installation of the header files (enabled by default).

Changes
~~~~~~~

- No changes with respect to 0.9, whose released was botched.

- The GMP library bundled with pyranha's binary packages has been
  updated to the latest version, and it is now built in fat mode.

- In pyrannha, remove the exposition of series with real coefficient
  types.

- Update the msgpack-c version in the binary packages to 2.1.0.

- Update the Boost version in binary packages to 1.63.

- Simplify the math::multiply_accumulate() function to work only on
  arguments of the same type.

- The zero_division_error exception now derives from domain_error rather
  than invalid_argument.

Fix
~~~

- Trigger the release scripts.

- Tentative fix for release script.

 - Various documentation fixes for the latest doxygen version.

- Require GMP >= 5.

- Fix occurrences of potential undefined behaviour when interacting with
  raw storage.

- Fix compilation on CentOS 5, where the CPU_COUNT macro is not
  available.

v0.8 (2016-11-20)
-----------------

New
~~~

- Add version number to pyranha, via the usual '__version__' attribute.

Changes
~~~~~~~

- Remove the dependency on boost timer and chrono in favour of a simple
  timer class based on C++11's <chrono>.

- Use the header-only variant of the boost unit test library.

Fix
~~~

- Fix main pyranha test function not throwing on errors in the test
  suite code.

- Silence an error being raised in Pyranha when latex/dvipng are not
  installed.

- A header file was not being installed.

- Fix a real test for older libc++/clang.

v0.7 (2016-11-13)
-----------------

New
~~~

- Windows packages of pyranha are now built and uploaded automatically
  via appveyor after every push to master.

Changes
~~~~~~~

- Disable the exposition of rational functions in pyranha.

- In pyranha, use square brackets [] instead of round brackets () in the
  syntax that emulates C++ templates.

- In pyranha use std::int_least16_t (exported as 'int16' in the type
  system) rather than short int. Also, remove a few unused types.

Fix
~~~

- Various fixes for pyranha's doctests in Windows.

- Fix pyranha hanging on the shutdown of the python interpreter on
  Windows.

- Fix the installation path of pyranha on Windows.

v0.6 (2016-11-01)
-----------------

New
~~~

- New serialization API.

Changes
~~~~~~~

- Thread binding is now disabled by default, and it can be enabled at
  runtime.

- The thread pool does not use anymore the main thread id in order to
  determine the suggested number of threads to use for a task. Rather,
  it checks whether the calling thread belongs to the pool.

- Reduce the usage of boost::numeric_cast() in favour of
  piranha::safe_cast().

- Change series multiplication and division to behave like coefficient
  mult/div in case of zero operands.

Fix
~~~

- Simplify the exception hierarchy by removing the base_exception class
  in favour of inheriting directly from std exceptions.

- Various safe_cast() improvements: remove dependency from mp_integer,
  introduce specific exception to signal failure, misc implementation
  and doc improvements.

v0.5 (2016-10-05)
-----------------

Fix
~~~

- YACMA_COMPILER_IS_CLANGXX now recognizes correctly AppleClang.

  CMAKE_CXX_COMPILER_ID can sometimes be AppleClang when Mac's version of Clang is used

v0.4 (2016-10-04)
-----------------

Fix
~~~

- Fix changelog generation.

v0.3 (2016-10-04)
-----------------

New
~~~

- Implement static methods to force (un)truncated multiplication,
  regardless of the current global truncation settings.

Changes
~~~~~~~

- Require CMake >= 3.0.0.

- Remove the (unused) is_instance_of type trait.

- Setting a global truncation limit in polynomials now resets the cache
  of natural powers.

  With this change, the behaviour of polynomial exponentiation should always be consistent with the currently active truncation level.

- Use Boost's demangler instead of our own.

Fix
~~~

- Fix build system error when cmake is not run from a git checkout.

- Test compilation fixes for libc++.

- Improve documentation for mp_integer::get_mpz_view().

- Fix documentation of the truncated multiplication method in the
  polynomial multiplier.

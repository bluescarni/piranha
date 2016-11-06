Changelog
=========

%%version%% (unreleased)
------------------------

Changes
~~~~~~~

- In pyranha, use square brackets [] instead of round brackets () in the
  syntax that emulates C++ templates. [Francesco Biscani]

- In pyranha use std::int_least16_t (exported as 'int16' in the type
  system) rather than short int. Also, remove a few unused types.
  [Francesco Biscani]

Fix
~~~

- Fix pyranha hanging on the shutdown of the python interpreter on
  Windows. [Francesco Biscani]

- Fix the installation path of pyranha on Windows. [Francesco Biscani]

v0.6 (2016-11-01)
-----------------

New
~~~

- New serialization API. [Francesco Biscani]

Changes
~~~~~~~

- Thread binding is now disabled by default, and it can be enabled at
  runtime. [Francesco Biscani]

- The thread pool does not use anymore the main thread id in order to
  determine the suggested number of threads to use for a task. Rather,
  it checks whether the calling thread belongs to the pool. [Francesco
  Biscani]

- Reduce the usage of boost::numeric_cast() in favour of
  piranha::safe_cast(). [Francesco Biscani]

- Change series multiplication and division to behave like coefficient
  mult/div in case of zero operands. [Francesco Biscani]

Fix
~~~

- Simplify the exception hierarchy by removing the base_exception class
  in favour of inheriting directly from std exceptions. [Francesco
  Biscani]

- Various safe_cast() improvements: remove dependency from mp_integer,
  introduce specific exception to signal failure, misc implementation
  and doc improvements. [Francesco Biscani]

v0.5 (2016-10-05)
-----------------

Fix
~~~

- YACMA_COMPILER_IS_CLANGXX now recognizes correctly AppleClang. [Isuru
  Fernando]

  CMAKE_CXX_COMPILER_ID can sometimes be AppleClang when Mac's version of Clang is used

v0.4 (2016-10-04)
-----------------

Fix
~~~

- Fix changelog generation. [Francesco Biscani]

v0.3 (2016-10-04)
-----------------

New
~~~

- Implement static methods to force (un)truncated multiplication,
  regardless of the current global truncation settings. [Francesco
  Biscani]

Changes
~~~~~~~

- Require CMake >= 3.0.0. [Francesco Biscani]

- Remove the (unused) is_instance_of type trait. [Francesco Biscani]

- Setting a global truncation limit in polynomials now resets the cache
  of natural powers. [Francesco Biscani]

  With this change, the behaviour of polynomial exponentiation should always be consistent with the currently active truncation level.

- Use Boost's demangler instead of our own. [Francesco Biscani]

Fix
~~~

- Fix build system error when cmake is not run from a git checkout.
  [Francesco Biscani]

- Test compilation fixes for libc++. [Francesco Biscani]

- Improve documentation for mp_integer::get_mpz_view() (fixes #22) [skip
  ci]. [Francesco Biscani]

- Fix documentation of the truncated multiplication method in the
  polynomial multiplier. [Francesco Biscani]



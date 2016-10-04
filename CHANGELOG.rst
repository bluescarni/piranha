Changelog
=========

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



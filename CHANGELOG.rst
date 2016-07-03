Changelog
=========

%%version%% (unreleased)
------------------------

New
~~~

- Implement static methods to force (un)truncated multiplication,
  regardless of the current global truncation settings. [Francesco
  Biscani]

Changes
~~~~~~~

- Setting a global truncation limit in polynomials now resets the cache
  of natural powers. [Francesco Biscani]

  With this change, the behaviour of polynomial exponentiation should always be consistent with the currently active truncation level.

- Use Boost's demangler instead of our own. [Francesco Biscani]

Fix
~~~

- Fix documentation of the truncated multiplication method in the
  polynomial multiplier. [Francesco Biscani]



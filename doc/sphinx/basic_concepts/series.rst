.. _series:

Series
======

The fundamental symbolic objects in Piranha are *series*. A series is a mathematical object of the form

.. math:: \sum_i T_i,

where the :math:`T_i` are the *terms* of the series. A term, in turn, is a mathematical object of the form

.. math:: T_i = C_i \cdot K_i,

where :math:`C_i` is the term's *coefficient*, :math:`K_i` is the term's *key* and ":math:`\cdot`" denotes scalar
multiplication. Coefficients can be zero-tested (that is, it is possible to determine whether a coefficient is zero
or not), and keys are equipped with an equivalence relation (that is, they can be compared for equality).
These properties allow to enforce the following invariants in series objects:

* terms are uniquely identified by their keys (i.e., terms with equivalent keys cannot coexist in the same
  series object);
* no terms with zero coefficient can exist in a series object.


As an example, multivariate
polynomials over :math:`\mathbb{Z}` are represented in Piranha as series in which

* the coefficient is a :cpp:class:`piranha::mp_integer` and
* the key is one of the available monomial representations.

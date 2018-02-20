.. _math_degree:

Degree
======

.. todo::

   Document series specialisations.

*#include <piranha/math/degree.hpp>*

.. cpp:function:: template <typename T> auto piranha::degree(T &&x)
.. cpp:function:: template <typename T> auto piranha::degree(T &&x, const piranha::symbol_fset &s)

   These functions return respectively the total and partial degree of *x*.

   What is meant exactly by *degree* depends on the type of of *x*, but, generally speaking,
   this function is intended to return the *polynomial degree* of a symbolic expression.
   For instance, for an expression such as

   .. math::
      
      3xy^2z - 2xyz + xz

   the total degree should be 4 (i.e., the degree of the term with the highest degree).
   The *partial* degree is the degree when only a subset of all
   symbols in an expression are considered. For the expression above, the partial
   degree when only the symbols :math:`x` and :math:`y` are considered will be 3. The input
   argument *s* indicates which symbols need to be taken into account when computing the
   partial degree.

   The implementations are delegated to the call operators of the :cpp:class:`piranha::degree_impl`
   function object. The bodies of these functions are equivalent respectively to

   .. code-block:: c++

      return piranha::degree_impl<Tp>{}(x);

   and

   .. code-block:: c++

      return piranha::degree_impl<Tp>{}(x, s);

   where ``Tp`` is ``T`` after the removal of reference and cv-qualifiers,
   and *x* is perfectly forwarded to the call operators of :cpp:class:`piranha::degree_impl`.
   If the expressions above are invalid, or if they return a type which does not satisfy the
   :cpp:concept:`piranha::Returnable` concept,
   then these functions will be disabled (i.e., they will not participate in overload resolution).

   See the :ref:`implementation <math_degree_impls>` section below for more details about the available
   specialisations.

   :param x: the input argument.
   :param s: the list of symbols that will be considered in the computation of the partial degree.

   :return: the total or partial degree of *x*.

   :exception unspecified: any exception thrown by the call operator of :cpp:class:`piranha::degree_impl`.

Concepts
--------

.. cpp:concept:: template <typename T> piranha::IsDegreeType

   This concept is satisfied if both overloads of :cpp:func:`piranha::degree()` can be called
   with a first argument of type ``T``. Specifically, this concept will be satisfied if both

   .. code-block:: c++

      piranha::degree(x)

   and

   .. code-block:: c++

      piranha::degree(x, s)

   are valid expressions, where ``x`` is a reference to const ``T`` and ``s`` a reference to const
   :cpp:type:`piranha::symbol_fset`.

.. _math_degree_impls:

Implementations
---------------

.. cpp:class:: template <typename T> piranha::degree_impl

   Unspecialised version of the function object implementing :cpp:func:`piranha::degree()`.

   This default implementation does not define any call operator, and thus no default implementation
   of :cpp:func:`piranha::degree()` is available.

.. _key_key_is_zero:

Detect zero keys
================

*#include <piranha/key/key_is_zero.hpp>*

.. cpp:function:: template <typename T> bool piranha::key_is_zero(const T &x, const piranha::symbol_fset &s)

   This function returns ``true`` if ``x`` is equal to zero, ``false`` otherwise.

   The implementation is delegated to the call operator of the :cpp:class:`piranha::is_zero_impl` function object.
   The body of this function is equivalent to:

   .. code-block:: c++

      return piranha::is_zero_impl<T>{}(x);

   If the expression above is invalid, or if it returns a type which is not
   :cpp:concept:`convertible <piranha::Convertible>` to ``bool``,
   then this function will be disabled (i.e., it will not participate in overload resolution).

   Piranha provides specialisations of :cpp:class:`piranha::is_zero_impl` for the following types:

   * any type for which the expression ``x == T(0)`` is valid (the default implementation),
   * all C++ complex types,
   * all :cpp:class:`mppp::integer <mppp::integer>` types (including :cpp:type:`piranha::integer`),
   * all :cpp:class:`mppp::rational <mppp::rational>` types (including :cpp:type:`piranha::rational`),
   * :cpp:class:`mppp::real <mppp::real>`.
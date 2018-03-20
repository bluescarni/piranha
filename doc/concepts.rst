.. _concepts:

General concepts and type traits
================================

.. note::

   Generic functions and classes in piranha support `concepts <https://en.wikipedia.org/wiki/Concepts_(C%2B%2B)>`__
   to constrain the types with which they can be used. C++ concepts are not (yet) part of the standard, and they are
   currently available only in GCC 6 and later (with the ``-fconcepts`` compilation flag). When used with compilers which do not
   support concepts natively, piranha will employ a concept emulation layer in order to provide the same functionality as native
   C++ concepts.

   Since the syntax of native C++ concepts is clearer than that of the concept emulation layer, the piranha documentation describes
   and refers to concepts in their native C++ form. As long as concepts are not part of the C++ standard, piranha's concepts
   will be subject to breaking changes, and they should not be regarded as part of the public piranha API.

*#include <piranha/type_traits.hpp>*

Type categories
---------------

.. cpp:concept:: template <typename T> piranha::CppArithmetic

   This concept is satisfied if ``T`` is a C++ arithmetic type, or a cv-qualified version thereof,
   as established by the standard ``std::is_arithmetic`` type trait.

.. cpp:concept:: template <typename T> piranha::CppIntegral

   This concept is satisfied if ``T`` is a C++ integral type, or a cv-qualified version thereof,
   as established by the standard ``std::is_integral`` type trait.

.. cpp:concept:: template <typename T> piranha::CppFloatingPoint

   This concept is satisfied if ``T`` is a C++ floating-point type, or a cv-qualified version thereof,
   as established by the standard ``std::is_floating_point`` type trait.

.. cpp:concept:: template <typename T> piranha::CppComplex

   This concept is satisfied if ``T`` is a C++ complex floating-point type, or a cv-qualified version thereof.

.. cpp:concept:: template <typename T> piranha::StringType

   This concept is satisfied if ``T`` is a string-like entity, or a cv-qualified version thereof. The types satisfying
   this concept are listed in the documentation of :cpp:concept:`mppp::StringType <mppp::StringType>`.

Type properties
---------------

.. cpp:concept:: template <typename T, typename... Args> piranha::Constructible

   This concept is satisfied if an instance of type ``T`` can be constructed from a set of arguments
   of types ``Args...``, as established by the standard ``std::is_constructible`` type trait.

.. cpp:concept:: template <typename T> piranha::DefaultConstructible

   This concept is satisfied if an instance of type ``T`` can be constructed from an empty set of arguments,
   as established by the standard ``std::is_default_constructible`` type trait.

.. cpp:concept:: template <typename From, typename To> piranha::Convertible

   This concept is satisfied if an instance of type ``From`` can be converted to the type ``To``,
   as established by the standard ``std::is_convertible`` type trait.

.. cpp:concept:: template <typename T> piranha::Returnable

   This concept is satisfied if instances of type ``T`` can be returned from a function.
   Specifically, this concept is satisfied if ``T`` is either ``void``, copy-constructible, or
   move-constructible.

.. cpp:concept:: template <typename T, typename... Args> piranha::Same

   This concept is satisfied if ``T`` and ``Args...`` are all the same type.

.. cpp:concept:: template <typename T, typename U = T> piranha::Swappable

   If at least C++17 is being used, this concept is equivalent to the ``std::is_swappable_with``
   type trait. That is, the concept is satisfied if the expressions

   .. code-block:: c++

      swap(std::declval<T>(), std::declval<U>())

   and

   .. code-block:: c++

      swap(std::declval<U>(), std::declval<T>())

   are both well-formed in unevaluated context after ``using std::swap``.

   Before C++17, an emulation of the behaviour of ``std::is_swappable_with`` is implemented.

Arithmetic and logical operators
--------------------------------

.. cpp:concept:: template <typename T> piranha::Preincrementable

   This concept is satisfied if the type ``T`` supports the pre-increment operator.

   Specifically, this concept is satisfied if

   .. code-block:: c++

      ++std::declval<T>()

   is a valid expression.

.. cpp:concept:: template <typename T> piranha::Postincrementable

   This concept is satisfied if the type ``T`` supports the post-increment operator.

   Specifically, this concept is satisfied if

   .. code-block:: c++

      std::declval<T>()++

   is a valid expression.

.. cpp:concept:: template <typename T, typename U = T> piranha::Addable

   This concept is satisfied if instances of ``T`` can be added to instances of ``U``.

   Specifically, this concept is satisfied if

   .. code-block:: c++

      std::declval<T>() + std::declval<U>()

   is a valid expression.

.. cpp:concept:: template <typename T, typename U = T> piranha::EqualityComparable

   This concept is satisfied if instances of type ``T`` can be compared to instances of type
   ``U`` via the equality and inequality operators.

   Specifically, this concept is satisfied if

   .. code-block:: c++

      std::declval<T>() == std::declval<U>()

   and

   .. code-block:: c++

      std::declval<T>() != std::declval<U>()

   are valid expressions whose types are :cpp:concept:`convertible <piranha::Convertible>` to ``bool``.

Iterators
---------

.. cpp:concept:: template <typename T> piranha::Iterator

   This concept is satisfied if ``T`` fulfills all the compile-time requirements specified by the C++ standard
   for iterator types.

.. cpp:concept:: template <typename T> piranha::InputIterator

   This concept is satisfied if ``T`` fulfills all the compile-time requirements specified by the C++ standard
   for input iterator types.

.. cpp:concept:: template <typename T, typename U> piranha::OutputIterator

   This concept is satisfied if ``T`` fulfills all the compile-time requirements specified by the C++ standard
   for output iterator types to which lvalues of type ``U`` can be assigned.

.. cpp:concept:: template <typename T> piranha::ForwardIterator

   This concept is satisfied if ``T`` fulfills all the compile-time requirements specified by the C++ standard
   for forward iterator types.

.. cpp:concept:: template <typename T> piranha::MutableForwardIterator

   This concept is satisfied if ``T`` is a mutable :cpp:concept:`forward iterator <piranha::ForwardIterator>`.

.. cpp:concept:: template <typename T> piranha::InputRange

   This concept is satisfied if the expressions

   .. code-block:: c++

      begin(std::declval<T>())

   and

   .. code-block:: c++

      end(std::declval<T>())

   are both well-formed in unevaluated context after ``using std::begin`` and ``using std::end``, and they yield
   the same type satisfying the :cpp:concept:`piranha::InputIterator` concept.

.. cpp:concept:: template <typename T> piranha::ForwardRange

   This concept is satisfied if the expressions

   .. code-block:: c++

      begin(std::declval<T>())

   and

   .. code-block:: c++

      end(std::declval<T>())

   are both well-formed in unevaluated context after ``using std::begin`` and ``using std::end``, and they yield
   the same type satisfying the :cpp:concept:`piranha::ForwardIterator` concept.

.. cpp:concept:: template <typename T> piranha::MutableForwardRange

   This concept is satisfied if the expressions

   .. code-block:: c++

      begin(std::declval<T>())

   and

   .. code-block:: c++

      end(std::declval<T>())

   are both well-formed in unevaluated context after ``using std::begin`` and ``using std::end``, and they yield
   the same type satisfying the :cpp:concept:`piranha::MutableForwardIterator` concept.

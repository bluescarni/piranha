.. _symbol_management:

Symbol management
=================

Types
-----

.. cpp:type:: piranha::symbol_fset = boost::container::flat_set<std::string>

   This data structure represents a collection of symbols as a set of strings ordered
   lexicographically.

   .. seealso::

      http://www.boost.org/doc/libs/1_66_0/doc/html/boost/container/flat_set.html

.. cpp:type:: piranha::symbol_idx = piranha::symbol_fset::size_type

   A C++ unsigned integral type representing indices into a :cpp:type:`piranha::symbol_fset`.

.. cpp:type:: piranha::symbol_idx_fset =  boost::container::flat_set<piranha::symbol_idx>

   This data structure represents an ordered set of indices into a :cpp:type:`piranha::symbol_fset`.

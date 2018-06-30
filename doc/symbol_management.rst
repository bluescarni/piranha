.. _symbol_management:

Symbol management
=================

Types
-----

.. cpp:type:: piranha::symbol_fset = boost::container::flat_set<std::string>

   This data structure represents a collection of symbols as a set of strings ordered
   lexicographically.

   This data structure is implemented on top of the
   `boost::flat_set <https://www.boost.org/doc/libs/release/doc/html/boost/container/flat_set.html>`__ class.

.. cpp:type:: piranha::symbol_idx = piranha::symbol_fset::size_type

   A C++ unsigned integral type representing indices into a :cpp:type:`piranha::symbol_fset`.

.. cpp:type:: piranha::symbol_idx_fset =  boost::container::flat_set<piranha::symbol_idx>

   This data structure represents an ordered set of :cpp:type:`indices <piranha::symbol_idx>`
   into a :cpp:type:`piranha::symbol_fset`.

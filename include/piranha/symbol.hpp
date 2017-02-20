/* Copyright 2009-2017 Francesco Biscani (bluescarni@gmail.com)

This file is part of the Piranha library.

The Piranha library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The Piranha library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the Piranha library.  If not,
see https://www.gnu.org/licenses/. */

#ifndef PIRANHA_SYMBOL_HPP
#define PIRANHA_SYMBOL_HPP

#include <atomic>
#include <cstddef>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_set>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/atomic_lock_guard.hpp>

namespace piranha
{

namespace detail
{

template <typename = void>
struct base_symbol {
    static std::atomic_flag m_mutex;
    static std::unordered_set<std::string> m_symbol_list;
};

template <typename T>
std::atomic_flag base_symbol<T>::m_mutex = ATOMIC_FLAG_INIT;

template <typename T>
std::unordered_set<std::string> base_symbol<T>::m_symbol_list;
}

/// Literal symbol class.
/**
 * This class represents a symbolic variable uniquely identified by its name. Symbol instances
 * are tracked in a global list for the duration of the program, so that different instances of symbols with the same
 * name are always referring to the same underlying object.
 *
 * The methods of this class, unless specified otherwise, are thread-safe: it is possible to create, access and operate
 * on objects of this class concurrently from multiple threads
 *
 * ## Exception safety guarantee ##
 *
 * This class provides the strong exception safety guarantee for all operations.
 *
 * ## Move semantics ##
 *
 * Move construction and move assignment will not alter the original object.
 */
class symbol : private detail::base_symbol<>
{
public:
    /// Constructor from name.
    /**
     * Construct a symbol with name \p name.
     *
     * @param name name of the symbol.
     *
     * @throws unspecified any exception thrown by <tt>std::unordered_set::insert()</tt>.
     */
    explicit symbol(const std::string &name) : m_ptr(get_pointer(name))
    {
    }
    /// Defaulted copy constructor.
    symbol(const symbol &) = default;
    /// Defaulted move constructor.
    symbol(symbol &&) = default;
    /// Copy assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     */
    symbol &operator=(const symbol &other) = default;
    /// Defaulted move assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     */
    symbol &operator=(symbol &&other) = default;
    /// Defaulted destructor.
    ~symbol() = default;
    /// Name getter.
    /**
     * @return const reference to the name of the symbol.
     */
    const std::string &get_name() const
    {
        return *m_ptr;
    }
    /// Equality operator.
    /**
     * @param other equality argument.
     *
     * @return true if \p other refers to the same underlying object as \p this, false otherwise.
     */
    bool operator==(const symbol &other) const
    {
        return m_ptr == other.m_ptr;
    }
    /// Inequality operator.
    /**
     * @param other inequality argument.
     *
     * @return negation of operator==().
     */
    bool operator!=(const symbol &other) const
    {
        return !(*this == other);
    }
    /// Less-than comparison.
    /**
     * This method will compare lexicographically the names of the two symbols.
     *
     * @param other comparison argument.
     *
     * @return <tt>this->get_name() < other.get_name()</tt>.
     */
    bool operator<(const symbol &other) const
    {
        return get_name() < other.get_name();
    }
    /// Hash value.
    /**
     * @return a hash value for the symbol.
     */
    std::size_t hash() const
    {
        return std::hash<std::string const *>()(m_ptr);
    }
    /// Overload output stream operator for piranha::symbol.
    /**
     * Will direct to \p os a human-readable description of \p s.
     *
     * @param os output stream.
     * @param s piranha::symbol to be sent to stream.
     *
     * @return reference to \p os.
     */
    friend std::ostream &operator<<(std::ostream &os, const symbol &s)
    {
        os << "name = \'" << s.get_name() << "\'";
        return os;
    }

private:
    static std::string const *get_pointer(const std::string &name)
    {
        detail::atomic_lock_guard lock(m_mutex);
        auto it = m_symbol_list.find(name);
        if (it == m_symbol_list.end()) {
            // If symbol is not present in the list, add it.
            auto retval = m_symbol_list.insert(name);
            piranha_assert(retval.second);
            it = retval.first;
        }
        // Final check, just for peace of mind.
        piranha_assert(it != m_symbol_list.end());
        // Extract and return the pointer.
        // NOTE: this should be alright: the standard containers' iterators should return references
        // and std::string shouldn't have operator&() overloaded.
        return &*it;
    }

private:
    std::string const *m_ptr;
};
}

namespace std
{

/// Specialisation of \p std::hash for piranha::symbol.
template <>
struct hash<piranha::symbol> {
    /// Result type.
    typedef size_t result_type;
    /// Argument type.
    typedef piranha::symbol argument_type;
    /// Hash operator.
    /**
     * @param s piranha::symbol whose hash value will be returned.
     *
     * @return piranha::symbol::hash().
     */
    result_type operator()(const argument_type &s) const
    {
        return s.hash();
    }
};
}

#endif

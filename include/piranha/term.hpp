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

#ifndef PIRANHA_TERM_HPP
#define PIRANHA_TERM_HPP

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

#include <piranha/detail/init.hpp>
#include <piranha/is_cf.hpp>
#include <piranha/is_key.hpp>
#include <piranha/math.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

/// Term class.
/**
 * This class represents series terms, which are parametrised over a coefficient type \p Cf and a key type
 * \p Key. One mutable coefficient instance and one key instance are the only data members and they can be accessed
 * directly.
 *
 * ## Type requirements ##
 *
 * - \p Cf must satisfy piranha::is_cf.
 * - \p Key must satisfy piranha::is_key.
 *
 * ## Exception safety guarantee ##
 *
 * Unless otherwise specified, this class provides the strong exception safety guarantee for all operations.
 *
 * ## Move semantics ##
 *
 * Move semantics is equivalent to the data members' move semantics.
 */
template <typename Cf, typename Key>
class term
{
    PIRANHA_TT_CHECK(is_cf, Cf);
    PIRANHA_TT_CHECK(is_key, Key);
    // Enabler for the generic binary ctor.
    template <typename T, typename U>
    using binary_ctor_enabler
        = enable_if_t<conjunction<std::is_constructible<Cf, T &&>, std::is_constructible<Key, U &&>>::value, int>;

public:
    /// Alias for the coefficient type.
    using cf_type = Cf;
    /// Alias for the key type.
    using key_type = Key;
    /// Default constructor.
    /**
     * The default constructor will explicitly call the default constructors of <tt>Cf</tt> and <tt>Key</tt>.
     *
     * @throws unspecified any exception thrown by the default constructors of \p Cf and \p Key.
     */
    term() : m_cf(), m_key() {}
    /// Default copy constructor.
    /**
     * @throws unspecified any exception thrown by the copy constructors of \p Cf and \p Key.
     */
    term(const term &) = default;
    /// Defaulted move constructor.
    term(term &&) = default;
    /// Constructor from generic coefficient and key.
    /**
     * \note
     * This constructor is activated only if \p Cf and \p Key are constructible from \p T and \p U respectively.
     *
     * This constructor will forward perfectly \p cf and \p key to construct coefficient and key.
     *
     * @param cf argument used for the construction of the coefficient.
     * @param key argument used for the construction of the key.
     *
     * @throws unspecified any exception thrown by the constructors of \p Cf and \p Key.
     */
    template <typename T, typename U, binary_ctor_enabler<T, U> = 0>
    explicit term(T &&cf, U &&key) : m_cf(std::forward<T>(cf)), m_key(std::forward<U>(key))
    {
    }
    /// Destructor.
    ~term()
    {
        PIRANHA_TT_CHECK(is_container_element, term);
    }
    /// Copy assignment operator.
    /**
     * The copy assignment operator offers the basic exception safety guarantee.
     *
     * @param other assignment argument.
     *
     * @return reference to \p this.
     *
     * @throws unspecified any exception thrown by the assignment operators of \p Cf and \p Key.
     */
    term &operator=(const term &other) = default;
    /// Move assignment operator.
    /**
     * @param other assignment argument.
     *
     * @return reference to \p this.
     */
    term &operator=(term &&other) noexcept
    {
        m_cf = std::move(other.m_cf);
        m_key = std::move(other.m_key);
        return *this;
    }
    /// Equality operator.
    /**
     * Equivalence of terms is defined by the equivalence of their keys.
     *
     * @param other comparison argument.
     *
     * @return <tt>m_key == other.m_key</tt>.
     *
     * @throws unspecified any exception thrown by the equality operator of \p Key.
     */
    bool operator==(const term &other) const
    {
        return m_key == other.m_key;
    }
    /// Hash value.
    /**
     * The term's hash value is given by its key's hash value.
     *
     * @return hash value of \p m_key as calculated via a default-constructed instance of \p std::hash.
     *
     * @throws unspecified any exception thrown by the <tt>std::hash</tt> specialisation of \p Key.
     */
    std::size_t hash() const
    {
        return std::hash<key_type>{}(m_key);
    }
    /// Compatibility test.
    /**
     * @param args reference piranha::symbol_fset.
     *
     * @return the output of the key's <tt>is_compatible()</tt> method.
     */
    bool is_compatible(const symbol_fset &args) const noexcept
    {
        return m_key.is_compatible(args);
    }
    /// Zero test.
    /**
     * @param args reference piranha::symbol_fset.
     *
     * @return \p true if either the key's <tt>is_zero()</tt> method or piranha::math::is_zero() on the coefficient
     * return \p true, \p false otherwise.
     */
    bool is_zero(const symbol_fset &args) const
    {
        return math::is_zero(m_cf) || m_key.is_zero(args);
    }
    /// Coefficient member.
    mutable Cf m_cf;
    /// Key member.
    Key m_key;
};

/// Specialisation of piranha::enable_noexcept_checks for piranha::term.
/**
 * The value of the type trait is set to \p true if both the coefficient and key types satisfy
 * piranha::enable_noexcept_checks. Otherwise, the value of the type trait is set to \p false.
 */
template <typename Cf, typename Key>
struct enable_noexcept_checks<term<Cf, Key>> {
private:
    static const bool implementation_defined
        = conjunction<enable_noexcept_checks<Cf>, enable_noexcept_checks<Key>>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename Cf, typename Key>
const bool enable_noexcept_checks<term<Cf, Key>>::value;
}

namespace std
{

/// Specialisation of \p std::hash for piranha::term.
template <typename Cf, typename Key>
struct hash<piranha::term<Cf, Key>> {
    /// The result type.
    using result_type = size_t;
    /// The argument type.
    using argument_type = piranha::term<Cf, Key>;
    /// Hash operator.
    /**
     * @param a the argument whose hash value will be computed.
     *
     * @return a hash value for \p a computed via piranha::term::hash().
     */
    result_type operator()(const argument_type &a) const
    {
        return a.hash();
    }
};
}

#endif

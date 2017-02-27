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

#ifndef PIRANHA_ARRAY_KEY_HPP
#define PIRANHA_ARRAY_KEY_HPP

#include <algorithm>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/math.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/small_vector.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

/// Array key.
/**
 * This class represents an array-like dense sequence of instances of type \p T. Interface and semantics
 * mimic those of \p std::vector. The underlying container used to store the elements is piranha::small_vector.
 * The template argument \p S is passed down as second argument to piranha::small_vector in the definition
 * of the internal container.
 *
 * This class is intended as a base class for the implementation of a key type, but it does *not* satisfy
 * all the requirements specified in piranha::is_key.
 *
 * ## Type requirements ##
 *
 * - \p T must satisfy the following requirements:
 *   - it must be suitable for use in piranha::small_vector as a value type,
 *   - it must be constructible from \p int,
 *   - it must be less-than comparable and equality-comparable,
 *   - it must be hashable,
 *   - it must satisfy piranha::has_is_zero,
 * - \p Derived must derive from piranha::array_key of \p T and \p Derived,
 * - \p Derived must satisfy the piranha::is_container_element type trait,
 * - \p S must be suitable as second template argument to piranha::small_vector.
 *
 * ## Exception safety guarantee ##
 *
 * Unless otherwise specified, this class provides the strong exception safety guarantee for all operations.
 *
 * ## Move semantics ##
 *
 * Move semantics is equivalent to the move semantics of piranha::small_vector.
 */
template <typename T, typename Derived, typename S = std::integral_constant<std::size_t, 0u>>
class array_key
{
    // NOTE: the general idea about requirements here is that these are the bare minimum
    // to make a simple array_key suitable as key in a piranha::series. There are additional
    // requirements which are optional and checked in the member functions.
    PIRANHA_TT_CHECK(std::is_constructible, T, int);
    PIRANHA_TT_CHECK(is_less_than_comparable, T);
    PIRANHA_TT_CHECK(is_equality_comparable, T);
    PIRANHA_TT_CHECK(is_hashable, T);
    PIRANHA_TT_CHECK(has_is_zero, T);

public:
    /// The internal container type.
    using container_type = small_vector<T, S>;
    /// Value type.
    using value_type = typename container_type::value_type;
    /// Iterator type.
    using iterator = typename container_type::iterator;
    /// Const iterator type.
    using const_iterator = typename container_type::const_iterator;
    /// Size type.
    using size_type = typename container_type::size_type;
    /// Defaulted default constructor.
    /**
     * The constructed object will be empty.
     */
    array_key() = default;
    /// Defaulted copy constructor.
    /**
     * @throws unspecified any exception thrown by the copy constructor of piranha::small_vector.
     */
    array_key(const array_key &) = default;
    /// Defaulted move constructor.
    array_key(array_key &&) = default;

private:
    // Enabler for constructor from init list.
    template <typename U>
    using init_list_enabler =
        typename std::enable_if<std::is_constructible<container_type, std::initializer_list<U>>::value, int>::type;

public:
    /// Constructor from initializer list.
    /**
     * \note
     * This constructor is enabled only if piranha::small_vector is constructible from \p list.
     *
     * \p list will be forwarded to construct the internal piranha::small_vector.
     *
     * @param list initializer list.
     *
     * @throws unspecified any exception thrown by the corresponding constructor of piranha::small_vector.
     */
    template <typename U, init_list_enabler<U> = 0>
    explicit array_key(std::initializer_list<U> list) : m_container(list)
    {
    }
    /// Constructor from piranha::symbol_fset.
    /**
     * The key will be created with a number of variables equal to <tt>args.size()</tt>
     * and filled with elements constructed from the integral constant 0.
     *
     * @param args piranha::symbol_fset used for construction.
     *
     * @throws unspecified any exception thrown by:
     * - piranha::small_vector::push_back(),
     * - the construction of instances of type \p value_type from the integral constant 0.
     */
    explicit array_key(const symbol_fset &args)
    {
        // NOTE: the back inserter transforms the assignment into a push_back() operation.
        std::fill_n(std::back_inserter(m_container), args.size(), value_type(0));
    }

private:
    // Enabler for generic ctor.
    template <typename U>
    using generic_ctor_enabler = typename std::enable_if<has_safe_cast<value_type, U>::value, int>::type;

public:
    /// Constructor from piranha::array_key parametrized on a generic type.
    /**
     * \note
     * This constructor is enabled only if \p U can be cast safely to the value type.
     *
     * Generic constructor for use in piranha::series, when inserting a term of different type.
     * The internal container will be initialised with the contents of the internal container of \p x (possibly
     * converting the individual contained values through piranha::safe_cast(),
     * if the values are of different type). If the size of \p x is different from the size of \p args, a runtime error
     * will be produced.
     *
     * @param other construction argument.
     * @param args reference piranha::symbol_fset.
     *
     * @throws std::invalid_argument if the sizes of \p x and \p args differ.
     * @throws unspecified any exception thrown by:
     * - piranha::small_vector::push_back(),
     * - piranha::safe_cast().
     */
    template <typename U, typename Derived2, typename S2, generic_ctor_enabler<U> = 0>
    explicit array_key(const array_key<U, Derived2, S2> &other, const symbol_fset &args)
    {
        if (unlikely(other.size() != args.size())) {
            piranha_throw(std::invalid_argument, "inconsistent sizes in the generic array_key constructor: the array "
                                                 "has a size of "
                                                     + std::to_string(other.size()) + ", the symbol set has a size of "
                                                     + std::to_string(args.size()));
        }
        std::transform(other.begin(), other.end(), std::back_inserter(m_container),
                       [](const U &x) { return safe_cast<value_type>(x); });
    }
    /// Trivial destructor.
    ~array_key()
    {
        PIRANHA_TT_CHECK(is_container_element, Derived);
        PIRANHA_TT_CHECK(std::is_base_of, array_key, Derived);
    }
    /// Copy assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     *
     * @throws unspecified any exception thrown by the assignment operator of piranha::small_vector.
     */
    array_key &operator=(const array_key &other) = default;
    /// Move assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     */
    array_key &operator=(array_key &&other) = default;
    /// Begin iterator.
    /**
     * @return iterator to the first element of the internal container.
     */
    iterator begin()
    {
        return m_container.begin();
    }
    /// End iterator.
    /**
     * @return iterator one past the last element of the internal container.
     */
    iterator end()
    {
        return m_container.end();
    }
    /// Begin const iterator.
    /**
     * @return const iterator to the first element of the internal container.
     */
    const_iterator begin() const
    {
        return m_container.begin();
    }
    /// End const iterator.
    /**
     * @return const iterator one past the last element of the internal container.
     */
    const_iterator end() const
    {
        return m_container.end();
    }
    /// Size.
    /**
     * @return number of elements stored within the container.
     */
    size_type size() const
    {
        return m_container.size();
    }
    /// Resize the internal array container.
    /**
     * Equivalent to piranha::small_vector::resize().
     *
     * @param new_size desired new size for the internal container.
     *
     * @throws unspecified any exception thrown by piranha::small_vector::resize().
     */
    void resize(const size_type &new_size)
    {
        m_container.resize(new_size);
    }
    /// Element access.
    /**
     * *Preconditions*:  <tt>i < size()</tt>.
     *
     * @param i index of the element to be accessed.
     *
     * @return reference to the element of the container at index \p i.
     */
    value_type &operator[](const size_type &i)
    {
        return m_container[i];
    }
    /// Const element access.
    /**
     * *Preconditions*:  <tt>i < size()</tt>.
     *
     * @param i index of the element to be accessed.
     *
     * @return const reference to the element of the container at index \p i.
     */
    const value_type &operator[](const size_type &i) const
    {
        return m_container[i];
    }
    /// Hash value.
    /**
     * @return hash value of the key, computed via piranha::small_vector::hash().
     *
     * @throws unspecified any exception thrown by piranha::small_vector::hash().
     */
    std::size_t hash() const
    {
        return m_container.hash();
    }
    /// Move-add element at the end.
    /**
     * Move-add \p x at the end of the internal container.
     *
     * @param x element to be added to the internal container.
     *
     * @throws unspecified any exception thrown by piranha::small_vector::push_back().
     */
    void push_back(value_type &&x)
    {
        m_container.push_back(std::move(x));
    }
    /// Copy-add element at the end.
    /**
     * Copy-add \p x at the end of the internal container.
     *
     * @param x element to be added to the internal container.
     *
     * @throws unspecified any exception thrown by piranha::small_vector::push_back().
     */
    void push_back(const value_type &x)
    {
        m_container.push_back(x);
    }
    /// Equality operator.
    /**
     * @param other comparison argument.
     *
     * @return the result of piranha::small_vector::operator==().
     *
     * @throws unspecified any exception thrown by piranha::small_vector::operator==().
     */
    bool operator==(const array_key &other) const
    {
        return m_container == other.m_container;
    }
    /// Inequality operator.
    /**
     * @param other comparison argument.
     *
     * @return the negation of operator==().
     *
     * @throws unspecified any exception thrown by the equality operator.
     */
    bool operator!=(const array_key &other) const
    {
        return !operator==(other);
    }
    /// Identify symbols that can be trimmed.
    /**
     * This method is used in piranha::series::trim(). The input parameter \p candidates
     * contains a set of symbol indices in \p args that are candidates for elimination. The method will remove
     * from \p candidates those indices whose corresponding element in \p this is not zero.
     *
     * @param candidates set of candidate indices for elimination.
     * @param args reference symbol set.
     *
     * @throws std::invalid_argument if the size of \p this differs from the size of \p args.
     * @throws unspecified any exception thrown by piranha::math::is_zero() or the <tt>erase()</tt> method
     * of piranha::symbol_idx_uset.
     */
    void trim_identify(symbol_idx_uset &candidates, const symbol_fset &args) const
    {
        if (unlikely(m_container.size() != args.size())) {
            piranha_throw(std::invalid_argument, "invalid arguments set for trim_identify(): the array "
                                                 "has a size of "
                                                     + std::to_string(m_container.size())
                                                     + ", the symbol set has a size of " + std::to_string(args.size()));
        }
        for (size_type i = 0; i < m_container.size(); ++i) {
            if (!math::is_zero(m_container[i])) {
                candidates.erase(static_cast<symbol_idx_uset::value_type>(i));
            }
        }
    }
    /// Trim.
    /**
     * This method will return a copy of \p this without the elements at the indices specified by \p trim_idx.
     *
     * @param trim_idx indices of the elements which will be removed.
     * @param args reference symbol set.
     *
     * @return a trimmed copy of \p this.
     *
     * @throws std::invalid_argument if the size of \p this differs from the size of \p args.
     * @throws unspecified any exception thrown by push_back().
     */
    Derived trim(const symbol_idx_fset &trim_idx, const symbol_fset &args) const
    {
        if (unlikely(m_container.size() != args.size())) {
            piranha_throw(std::invalid_argument, "invalid arguments set for trim(): the array "
                                                 "has a size of "
                                                     + std::to_string(m_container.size())
                                                     + ", the symbol set has a size of " + std::to_string(args.size()));
        }
        auto idx_it = trim_idx.begin();
        const auto idx_end = trim_idx.end();
        Derived retval;
        for (size_type i = 0; i < m_container.size(); ++i) {
            if (idx_it != idx_end && i == *idx_it) {
                ++idx_it;
            } else {
                retval.push_back(m_container[i]);
            }
        }
        return retval;
    }

private:
    // Enabler for addition and subtraction.
    template <typename U>
    using add_t = decltype(std::declval<U const &>().add(std::declval<U &>(), std::declval<U const &>()));
    template <typename U>
    using sub_t = decltype(std::declval<U const &>().sub(std::declval<U &>(), std::declval<U const &>()));
    template <typename U>
    using add_enabler = enable_if_t<is_detected<add_t, U>::value, int>;
    template <typename U>
    using sub_enabler = enable_if_t<is_detected<sub_t, U>::value, int>;

public:
    /// Vector add.
    /**
     * \note
     * This method is enabled only if the call to piranha::small_vector::add()
     * is well-formed.
     *
     * Equivalent to calling piranha::small_vector::add() on the internal containers of \p this
     * and of the arguments.
     *
     * @param retval piranha::array_key that will hold the result of the addition.
     * @param other piranha::array_key that will be added to \p this.
     *
     * @throws unspecified any exception thrown by piranha::small_vector::add().
     */
    template <typename U = container_type, add_enabler<U> = 0>
    void vector_add(array_key &retval, const array_key &other) const
    {
        m_container.add(retval.m_container, other.m_container);
    }
    /// Vector sub.
    /**
     * \note
     * This method is enabled only if the call to piranha::small_vector::sub()
     * is well-formed.
     *
     * Equivalent to calling piranha::small_vector::sub() on the internal containers of \p this
     * and of the arguments.
     *
     * @param retval piranha::array_key that will hold the result of the subtraction.
     * @param other piranha::array_key that will be subtracted from \p this.
     *
     * @throws unspecified any exception thrown by piranha::small_vector::sub().
     */
    template <typename U = container_type, sub_enabler<U> = 0>
    void vector_sub(array_key &retval, const array_key &other) const
    {
        m_container.sub(retval.m_container, other.m_container);
    }
    /// Merge symbols.
    /**
     * This method will return a copy of \p this in which the value 0 has been inserted
     * at the positions corresponding to the symbols appearing in \p new_args and missing from \p orig_args.
     * For instance, given a piranha::array_key containing the values <tt>[1,2,3,4]</tt>, a symbol set
     * \p orig_args containing <tt>["c","e","g","h"]</tt> and a symbol set \p new_args containing
     * <tt>["a","b","c","d","e","f",g","h","i"]</tt>, the output of this method will be
     * <tt>[0,0,1,0,2,0,3,4,0]</tt>.
     *
     * *Preconditions*: all symbols in \p orig_args must be present in \p new_args.
     *
     * @param orig_args current reference symbol set for \p this.
     * @param new_args new symbol set.
     *
     * @return a \p Derived instance resulting from merging \p new_args into \p this.
     *
     * @throws std::invalid_argument in the following cases:
     * - the size of \p this is different from the size of \p orig_args,
     * - the size of \p new_args is not greater than the size of \p orig_args.
     * @throws unspecified any exception thrown by:
     * - piranha::small_vector::push_back(),
     * - the construction of instances of type \p value_type from the integral constant 0.
     */
    Derived merge_symbols(const symbol_fset &orig_args, const symbol_fset &new_args) const
    {
        Derived retval;
        retval.m_container = vector_key_merge_symbols(m_container, orig_args, new_args);
        return retval;
    }

protected:
    /// Internal container.
    container_type m_container;

public:
    /// Get size, begin and end iterator (const version).
    /**
     * @return the output of the piranha::small_vector::size_begin_end() method called
     * on the internal piranha::small_vector container.
     */
    auto size_begin_end() const -> decltype(m_container.size_begin_end())
    {
        return m_container.size_begin_end();
    }
    /// Get size, begin and end iterator.
    /**
     * @return the output of the piranha::small_vector::size_begin_end() method called
     * on the internal piranha::small_vector container.
     */
    auto size_begin_end() -> decltype(m_container.size_begin_end())
    {
        return m_container.size_begin_end();
    }
};
}

namespace std
{

/// Specialisation of \p std::hash for piranha::array_key.
template <typename T, typename Derived, typename S>
struct hash<piranha::array_key<T, Derived, S>> {
    /// Result type.
    typedef size_t result_type;
    /// Argument type.
    typedef piranha::array_key<T, Derived, S> argument_type;
    /// Hash operator.
    /**
     * @param a piranha::array_key whose hash value will be returned.
     *
     * @return piranha::array_key::hash().
     */
    result_type operator()(const argument_type &a) const
    {
        return a.hash();
    }
};
}

#endif

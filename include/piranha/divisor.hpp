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

#ifndef PIRANHA_DIVISOR_HPP
#define PIRANHA_DIVISOR_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/cf_mult_impl.hpp>
#include <piranha/detail/divisor_series_fwd.hpp>
#include <piranha/detail/prepare_for_print.hpp>
#include <piranha/detail/series_fwd.hpp>
#include <piranha/detail/vector_merge_args.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/hash_set.hpp>
#include <piranha/is_cf.hpp>
#include <piranha/is_key.hpp>
#include <piranha/math.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/pow.hpp>
#include <piranha/s11n.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/small_vector.hpp>
#include <piranha/symbol_set.hpp>
#include <piranha/term.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

// Fwd declaration.
template <typename>
class divisor;
}

// Implementation of the Boost s11n api.
namespace boost
{
namespace serialization
{

template <typename Archive, typename T>
inline void save(Archive &ar, const piranha::boost_s11n_key_wrapper<piranha::divisor<T>> &k, unsigned)
{
    if (unlikely(!k.key().is_compatible(k.ss()))) {
        piranha_throw(std::invalid_argument, "an invalid symbol_set was passed as an argument during the "
                                             "Boost serialization of a divisor");
    }
    piranha::boost_save(ar, k.key().m_container);
}

template <typename Archive, typename T>
inline void load(Archive &ar, piranha::boost_s11n_key_wrapper<piranha::divisor<T>> &k, unsigned)
{
    try {
        piranha::boost_load(ar, k.key().m_container);
        if (unlikely(!k.key().destruction_checks())) {
            piranha_throw(std::invalid_argument, "the divisor loaded from a Boost archive failed internal "
                                                 "consistency checks");
        }
        if (unlikely(!k.key().is_compatible(k.ss()))) {
            piranha_throw(std::invalid_argument, "the divisor loaded from a Boost archive is not compatible "
                                                 "with the supplied symbol set");
        }
    } catch (...) {
        k.key().m_container = typename piranha::divisor<T>::container_type{};
        throw;
    }
}

template <typename Archive, typename T>
inline void serialize(Archive &ar, piranha::boost_s11n_key_wrapper<piranha::divisor<T>> &k, unsigned version)
{
    split_free(ar, k, version);
}
}
}

namespace piranha
{

inline namespace impl
{

// Pair vector-exponent, with the exponent made mutable. For use in the divisor class.
template <typename T>
struct divisor_p_type {
    using v_type = small_vector<T>;
    divisor_p_type() = default;
    divisor_p_type(const divisor_p_type &) = default;
    divisor_p_type(divisor_p_type &&) = default;
    explicit divisor_p_type(const v_type &v_, const T &e_) : v(v_), e(e_)
    {
    }
    explicit divisor_p_type(v_type &&v_, T &&e_) : v(std::move(v_)), e(std::move(e_))
    {
    }
    ~divisor_p_type() = default;
    divisor_p_type &operator=(const divisor_p_type &) = default;
    divisor_p_type &operator=(divisor_p_type &&) = default;
    bool operator==(const divisor_p_type &other) const
    {
        return v == other.v;
    }
    // Serialization support.
    template <class Archive>
    void save(Archive &ar, unsigned) const
    {
        boost_save(ar, v);
        boost_save(ar, e);
    }
    template <class Archive>
    void load(Archive &ar, unsigned)
    {
        boost_load(ar, v);
        boost_load(ar, e);
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
    // Members.
    v_type v;
    mutable T e;
};
}

// Serialization methods for the divisor's pair type. Not documented because they are implementation details.
template <typename Archive, typename T>
struct boost_save_impl<Archive, divisor_p_type<T>,
                       enable_if_t<conjunction<has_boost_save<Archive, T>,
                                               has_boost_save<Archive, typename divisor_p_type<T>::v_type>>::value>>
    : boost_save_via_boost_api<Archive, divisor_p_type<T>> {
};

template <typename Archive, typename T>
struct boost_load_impl<Archive, divisor_p_type<T>,
                       enable_if_t<conjunction<has_boost_load<Archive, T>,
                                               has_boost_load<Archive, typename divisor_p_type<T>::v_type>>::value>>
    : boost_load_via_boost_api<Archive, divisor_p_type<T>> {
};

#if defined(PIRANHA_WITH_MSGPACK)

template <typename Stream, typename T>
struct msgpack_pack_impl<Stream, divisor_p_type<T>,
                         enable_if_t<conjunction<is_msgpack_stream<Stream>, has_msgpack_pack<Stream, T>,
                                                 has_msgpack_pack<Stream,
                                                                  typename divisor_p_type<T>::v_type>>::value>> {
    void operator()(msgpack::packer<Stream> &pk, const divisor_p_type<T> &p, msgpack_format f) const
    {
        pk.pack_array(2);
        msgpack_pack(pk, p.v, f);
        msgpack_pack(pk, p.e, f);
    }
};

template <typename T>
struct msgpack_convert_impl<divisor_p_type<T>,
                            enable_if_t<conjunction<has_msgpack_convert<T>,
                                                    has_msgpack_convert<typename divisor_p_type<T>::v_type>>::value>> {
    void operator()(divisor_p_type<T> &p, const msgpack::object &o, msgpack_format f) const
    {
        std::array<msgpack::object, 2> tmp;
        o.convert(tmp);
        msgpack_convert(p.v, tmp[0], f);
        msgpack_convert(p.e, tmp[1], f);
    }
};

#endif

/// Divisor class.
/**
 * This class is used to represent keys of the form
 * \f[
 * \prod_j\frac{1}{\left(a_{0,j}x_0+a_{1,j}x_1+\ldots+a_{n,j}x_n\right)^{e_j}},
 * \f]
 * where \f$ a_{i,j} \f$ are integers, \f$ x_i \f$ are symbols, and \f$ e_j \f$ are positive integers. The type
 * of \f$ a_{i,j} \f$ and \f$ e_j \f$ is \p T. The terms of the product are stored in a piranha::hash_set and
 * they are guaranteed to be in a canonical form defined by the following properties:
 * - if \p T is a C++ integral type, the values of \f$ a_{i,j} \f$ and \f$ e_j \f$ are within implementation-defined
 * ranges,
 * - \f$ e_j \f$ is always strictly positive,
 * - the first nonzero \f$ a_{i,j} \f$ in each term is positive,
 * - the \f$ a_{i,j} \f$ in each term have no non-unitary common divisor.
 *
 * ## Type requirements ##
 *
 * \p T must be either a C++ signed integral type or an instance of piranha::mp_integer.
 *
 * ## Exception safety guarantee ##
 *
 * Unless otherwise specified, this class provides the strong exception safety guarantee for all operations.
 *
 * ## Move semantics ##
 *
 * Move semantics is equivalent to the move semantics of piranha::hash_set.
 */
// NOTE: if we ever make this completely generic on T, remember there are some hard-coded assumptions. E.g.,
// is_zero must be available in split().
// NOTE: the implementation defined range restriction is needed in order to make the gcd computations safe.
template <typename T>
class divisor
{
    static_assert((std::is_signed<T>::value && std::is_integral<T>::value) || detail::is_mp_integer<T>::value,
                  "The value type must be a signed integer or an mp_integer");
    // Make friend with the divisor series.
    template <typename, typename>
    friend class divisor_series;

public:
    /// Alias for \p T.
    using value_type = T;

private:
    using p_type = divisor_p_type<value_type>;
    using v_type = typename p_type::v_type;
    // Hasher for the pair type.
    struct p_type_hasher {
        std::size_t operator()(const p_type &p) const
        {
            return p.v.hash();
        }
    };

public:
    /// Underlying container type.
    using container_type = hash_set<p_type, p_type_hasher>;

private:
    // Canonical term: the first nonzero element is positive and all the gcd of all elements is 1 or -1.
    // NOTE: this also includes the check for all zero elements, as gcd(0,0,...,0) = 0.
    static bool term_is_canonical(const p_type &p)
    {
        bool first_nonzero_found = false;
        value_type cd(0);
        for (const auto &n : p.v) {
            if (!first_nonzero_found && !math::is_zero(n)) {
                if (n < 0) {
                    return false;
                }
                first_nonzero_found = true;
            }
            // NOTE: gcd(0,n) == n (or +-n, in our case) for all n, zero included.
            math::gcd3(cd, cd, n);
        }
        if (cd != 1 && cd != -1) {
            return false;
        }
        return true;
    }
    // Range check on a term - meaningful only if T is a C++ integral type.
    template <typename U = T, typename std::enable_if<std::is_integral<U>::value, int>::type = 0>
    static bool term_range_check(const p_type &p)
    {
        return std::all_of(p.v.begin(), p.v.end(), [](const value_type &x) {
            return x >= -detail::safe_abs_sint<value_type>::value && x <= detail::safe_abs_sint<value_type>::value;
        });
    }
    template <typename U = T, typename std::enable_if<!std::is_integral<U>::value, int>::type = 0>
    static bool term_range_check(const p_type &)
    {
        return true;
    }
    bool destruction_checks() const
    {
        const auto it_f = m_container.end();
        auto it = m_container.begin();
        const typename v_type::size_type v_size = (it == it_f) ? 0u : it->v.size();
        for (; it != it_f; ++it) {
            // Check: the exponent must be greater than zero.
            if (it->e <= 0) {
                return false;
            }
            // Check: range.
            if (!term_range_check(*it)) {
                return false;
            }
            // Check: canonical.
            if (!term_is_canonical(*it)) {
                return false;
            }
            // Check: all vectors have the same size.
            if (it->v.size() != v_size) {
                return false;
            }
        }
        return true;
    }
    // Insertion machinery.
    template <typename Term>
    void insertion_impl(Term &&term)
    {
        // Handle the case of a table with no buckets.
        if (unlikely(!m_container.bucket_count())) {
            m_container._increase_size();
        }
        // Try to locate the term.
        auto bucket_idx = m_container._bucket(term);
        const auto it = m_container._find(term, bucket_idx);
        if (it == m_container.end()) {
            // New term.
            if (unlikely(m_container.size() == std::numeric_limits<size_type>::max())) {
                piranha_throw(std::overflow_error, "maximum number of elements reached");
            }
            // Term is new. Handle the case in which we need to rehash because of load factor.
            if (unlikely(static_cast<double>(m_container.size() + size_type(1u))
                             / static_cast<double>(m_container.bucket_count())
                         > m_container.max_load_factor())) {
                m_container._increase_size();
                // We need a new bucket index in case of a rehash.
                bucket_idx = m_container._bucket(term);
            }
            // Actually perform the insertion and finish by updating the size.
            m_container._unique_insert(std::forward<Term>(term), bucket_idx);
            m_container._update_size(m_container.size() + size_type(1u));
        } else {
            // Existing term - update the exponent.
            update_exponent(it->e, term.e);
        }
    }
    template <typename U = T, typename std::enable_if<std::is_integral<U>::value, int>::type = 0>
    static void update_exponent(value_type &a, const value_type &b)
    {
        // Check the range when the exponent is an integral type.
        piranha_assert(a > 0);
        piranha_assert(b > 0);
        // NOTE: this is safe as we require b to be a positive value.
        if (unlikely(a > std::numeric_limits<value_type>::max() - b)) {
            piranha_throw(std::invalid_argument, "overflow in the computation of the exponent of a divisor term");
        }
        a = static_cast<value_type>(a + b);
    }
    template <typename U = T, typename std::enable_if<!std::is_integral<U>::value, int>::type = 0>
    static void update_exponent(value_type &a, const value_type &b)
    {
        // For integer exponents, just do normal addition.
        a += b;
    }
    // Enabler for insertion.
    template <typename It, typename Exponent>
    using insert_enabler =
        typename std::enable_if<is_input_iterator<It>::value
                                    && has_safe_cast<value_type, typename std::iterator_traits<It>::value_type>::value
                                    && has_safe_cast<value_type, Exponent>::value,
                                int>::type;
    // Evaluation utilities.
    // NOTE: here we are not actually requiring that the eval_type has to be destructible, copyable/movable, etc.
    // We just assume it is a sane type of some kind.
    template <typename U>
    using eval_sum_type = decltype(std::declval<const value_type &>() * std::declval<const U &>());
    template <typename U>
    using eval_type_
        = decltype(math::pow(std::declval<const eval_sum_type<U> &>(), std::declval<const value_type &>()));
    template <typename U>
    using eval_type =
        typename std::enable_if<std::is_constructible<eval_type_<U>, int>::value
                                    && is_divisible_in_place<eval_type_<U>>::value
                                    && std::is_constructible<eval_sum_type<U>, int>::value
                                    && is_addable_in_place<eval_sum_type<U>>::value && detail::is_pmappable<U>::value,
                                eval_type_<U>>::type;
    // Multiplication utilities.
    template <typename Cf>
    using multiply_enabler = typename std::enable_if<detail::true_tt<detail::cf_mult_enabler<Cf>>::value, int>::type;

public:
    /// Arity of the multiply() method.
    static const std::size_t multiply_arity = 1u;
    /// Size type.
    /**
     * It corresponds to the size type of the internal container.
     */
    using size_type = typename container_type::size_type;
    /// Defaulted default constructor.
    /**
     * This constructor will initialise an empty divisor.
     */
    divisor() = default;
    /// Defaulted copy constructor.
    divisor(const divisor &) = default;
    /// Defaulted move constructor.
    divisor(divisor &&) = default;
    /// Converting constructor.
    /**
     * This constructor is used in the generic constructor of piranha::series. It is equivalent
     * to a copy constructor with extra checking.
     *
     * @param other construction argument.
     * @param args reference symbol set.
     *
     * @throws std::invalid_argument if \p other is not compatible with \p args.
     * @throws unspecified any exception thrown by the copy constructor.
     */
    explicit divisor(const divisor &other, const symbol_set &args) : m_container(other.m_container)
    {
        if (unlikely(!is_compatible(args))) {
            piranha_throw(std::invalid_argument, "the constructed divisor is incompatible with the "
                                                 "input symbol set");
        }
    }
    /// Constructor from piranha::symbol_set.
    /**
     * Equivalent to the default constructor.
     */
    explicit divisor(const symbol_set &)
    {
    }
    /// Trivial destructor.
    ~divisor()
    {
        piranha_assert(destruction_checks());
        PIRANHA_TT_CHECK(is_key, divisor);
    }
    /// Copy assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     *
     * @throws unspecified any exception thrown by the assignment operator of piranha::hash_set.
     */
    divisor &operator=(const divisor &other) = default;
    /// Move assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     */
    divisor &operator=(divisor &&other) = default;
    /// Create and insert a term from range and exponent.
    /**
     * \note
     * This method is enabled only if:
     * - \p It is an input iterator,
     * - the value type of \p It can be safely cast to piranha::divisor::value_type,
     * - \p Exponent can be safely cast to piranha::divisor::value_type.
     *
     * This method will construct and insert a term into the divisor. The elements
     * in the range <tt>[begin,end)</tt> will be used to construct the \f$ a_{i,j} \f$ of the term, while \p e
     * will be used to construct the exponent (after a call to piranha::safe_cast()).
     * If no term with the same set of \f$ a_{i,j} \f$ exists, then
     * a new term will be inserted; otherwise, \p e will be added to the exponent of the existing term.
     *
     * This method provides the basic exception safety guarantee.
     *
     * @param begin start of the range of \f$ a_{i,j} \f$.
     * @param end end of the range of \f$ a_{i,j} \f$.
     * @param e exponent.
     *
     * @throws std::invalid_argument if the term to be inserted is not in canonical form, or if the insertion
     * leads to an overflow in the value of an exponent.
     * @throws std::overflow_error if the insertion results in the container to be resized over
     * an implementation-defined limit.
     * @throws unspecified any exception thrown by:
     * - piranha::safe_cast(),
     * - manipulations of piranha::small_vector,
     * - the public interface of piranha::hash_set,
     * - arithmetic operations on the exponent.
     */
    template <typename It, typename Exponent, insert_enabler<It, Exponent> = 0>
    void insert(It begin, It end, const Exponent &e)
    {
        p_type term;
        // Assign the exponent.
        term.e = safe_cast<value_type>(e);
        if (unlikely(term.e <= 0)) {
            piranha_throw(std::invalid_argument, "a term of a divisor must have a positive exponent");
        }
        // Build the vector to be inserted.
        for (; begin != end; ++begin) {
            term.v.push_back(safe_cast<value_type>(*begin));
        }
        // Range check.
        if (unlikely(!term_range_check(term))) {
            piranha_throw(std::invalid_argument, "an element in a term of a divisor is outside the allowed range");
        }
        // Check that the term is canonical.
        if (unlikely(!term_is_canonical(term))) {
            piranha_throw(std::invalid_argument, "term not in canonical form");
        }
        // Perform the insertion.
        insertion_impl(std::move(term));
    }
    /// Size.
    /**
     * @return the size of the internal container - that is, the number of terms in the product.
     */
    size_type size() const
    {
        return m_container.size();
    }
    /// Clear.
    /**
     * This method will remove all terms from the divisor.
     */
    void clear()
    {
        m_container.clear();
    }
    /// Equality operator.
    /**
     * Two divisors are considered equal if:
     * - they have the same number of terms, and
     * - for each term in the first divisor there exist an identical term in the
     *   second divisor.
     *
     * @param other comparison argument.
     *
     * @return \p true if \p this is equal to \p other, \p false otherwise.
     */
    bool operator==(const divisor &other) const
    {
        if (size() != other.size()) {
            return false;
        }
        const auto it_f_x = m_container.end(), it_f_y = other.m_container.end();
        for (auto it = m_container.begin(); it != it_f_x; ++it) {
            const auto tmp_it = other.m_container.find(*it);
            if (tmp_it == it_f_y || tmp_it->e != it->e) {
                return false;
            }
        }
        return true;
    }
    /// Inequality operator.
    /**
     * @param other comparison argument.
     *
     * @return the opposite of operator==().
     */
    bool operator!=(const divisor &other) const
    {
        return !((*this) == other);
    }
    /// Hash value.
    /**
     * The hash value is computed by combining the hash values of all terms. An empty divisor
     * has a hash value of 0. Two equal divisors have the same hash value.
     *
     * @return a hash value for the divisor.
     */
    std::size_t hash() const
    {
        // NOTE: here we are using a simple sum to compute the hash of a vector of vectors. Indeed,
        // we need some add/multiply like operation in order to make sure that two divisors
        // with the same elements stored in different order hash to the same thing. This seems
        // to be ok, according to this,
        // http://stackoverflow.com/questions/18021643/hashing-a-set-of-integers-in-an-order-independent-way,
        // as the hash of each subvector should be rather good (it comes from boost hash combine).
        // XOR is also commutative and might be used (see link above).
        // If problem arises, we can consider muliplying the hash of each subvector by a prime
        // (similar to Kronecker substitution).
        std::size_t retval = 0u;
        p_type_hasher hasher;
        const auto it_f = m_container.end();
        for (auto it = m_container.begin(); it != it_f; ++it) {
            retval = static_cast<std::size_t>(retval + hasher(*it));
        }
        return retval;
    }
    /// Compatibility check.
    /**
     * An empty divisor is considered compatible with any set of symbols. Otherwise, a non-empty
     * divisor is compatible if the number of variables in the terms is the same as the number
     * of symbols in \p args.
     *
     * @param args reference symbol set.
     *
     * @return \p true if \p this is compatible with \p args, \p false otherwise.
     */
    bool is_compatible(const symbol_set &args) const noexcept
    {
        return m_container.empty() ? true : (m_container.begin()->v.size() == args.size());
    }
    /// Ignorability check.
    /**
     * A divisor is never considered ignorable (an empty divisor is equal to 1).
     *
     * @return \p false.
     */
    bool is_ignorable(const symbol_set &) const noexcept
    {
        return false;
    }
    /// Check if divisor is unitary.
    /**
     * Only an empty divisor is considered unitary.
     *
     * @param args reference symbol set.
     *
     * @return \p true if \p this is empty, \p false otherwise.
     *
     * @throws std::invalid_argument if the number of variables in the divisor is different
     * from the size of \p args.
     */
    bool is_unitary(const symbol_set &args) const
    {
        if (m_container.empty()) {
            return true;
        }
        if (unlikely(m_container.begin()->v.size() != args.size())) {
            piranha_throw(std::invalid_argument, "invalid arguments set");
        }
        return false;
    }
    /// Merge arguments.
    /**
     * This method will merge the new arguments set \p new_args into \p this, given the current reference arguments set
     * \p orig_args. Arguments in \p new_args not appearing in \p orig_args will be inserted in the terms,
     * with the corresponding \f$ a_{i,j} \f$ values constructed from the integral constant 0.
     *
     * @param orig_args current reference arguments set for \p this.
     * @param new_args new arguments set.
     *
     * @return a divisor resulting from merging \p new_args into \p this.
     *
     * @throws std::invalid_argument in the following cases:
     * - the number of variables in the terms of \p this is different from the size of \p orig_args,
     * - the size of \p new_args is not greater than the size of \p orig_args,
     * - not all elements of \p orig_args are included in \p new_args.
     * @throws unspecified any exception thrown by:
     * - piranha::small_vector::push_back(),
     * - the copy assignment of piranha::divisor::value_type,
     * - piranha::hash_set::insert().
     */
    divisor merge_args(const symbol_set &orig_args, const symbol_set &new_args) const
    {
        divisor retval;
        const auto it_f = m_container.end();
        p_type tmp;
        for (auto it = m_container.begin(); it != it_f; ++it) {
            tmp.v = detail::vector_merge_args(it->v, orig_args, new_args);
            tmp.e = it->e;
            auto ret = retval.m_container.insert(std::move(tmp));
            (void)ret;
            piranha_assert(ret.first != retval.m_container.end());
            piranha_assert(ret.second);
        }
        return retval;
    }
    /// Print to stream.
    /**
     * This method will print to the stream \p os a text representation of \p this.
     *
     * @param os target stream.
     * @param args reference symbol set.
     *
     * @throws std::invalid_argument if the number of variables in the terms of \p this is different from the
     * size of \p args.
     * @throws unspecified any exception thrown by printing to \p os piranha::divisor::value_type, strings or
     * characters.
     */
    void print(std::ostream &os, const symbol_set &args) const
    {
        // Don't print anything if there are no terms.
        if (m_container.empty()) {
            return;
        }
        if (unlikely(m_container.begin()->v.size() != args.size())) {
            piranha_throw(std::invalid_argument, "invalid size of arguments set");
        }
        const auto it_f = m_container.end();
        bool first_term = true;
        os << "1/[";
        for (auto it = m_container.begin(); it != it_f; ++it) {
            // If this is not the first term, print a leading '*' operator.
            if (first_term) {
                first_term = false;
            } else {
                os << '*';
            }
            bool printed_something = false;
            os << '(';
            for (typename v_type::size_type i = 0u; i < it->v.size(); ++i) {
                // If the aij is zero, don't print anything.
                if (math::is_zero(it->v[i])) {
                    continue;
                }
                // A positive aij, in case previous output exists, must be preceded
                // by a "+" sign.
                if (it->v[i] > 0 && printed_something) {
                    os << '+';
                }
                // Print the aij, unless it's "-1": in that case, just print the minus sign.
                if (it->v[i] == -1) {
                    os << '-';
                } else if (it->v[i] != 1) {
                    os << detail::prepare_for_print(it->v[i]) << '*';
                }
                // Finally, print name of variable.
                os << args[i].get_name();
                printed_something = true;
            }
            os << ')';
            // Print the exponent, if different from one.
            if (it->e != 1) {
                os << "**" << detail::prepare_for_print(it->e);
            }
        }
        os << ']';
    }
    /// Print to stream in TeX mode.
    /**
     * This method will print to the stream \p os a TeX representation of \p this.
     *
     * @param os target stream.
     * @param args reference symbol set.
     *
     * @throws std::invalid_argument if the number of variables in the terms of \p this is different from the size
     * of \p args.
     * @throws unspecified any exception thrown by printing to \p os piranha::divisor::value_type, strings or
     * characters.
     */
    void print_tex(std::ostream &os, const symbol_set &args) const
    {
        // Don't print anything if there are no terms.
        if (m_container.empty()) {
            return;
        }
        if (unlikely(m_container.begin()->v.size() != args.size())) {
            piranha_throw(std::invalid_argument, "invalid size of arguments set");
        }
        const auto it_f = m_container.end();
        os << "\\frac{1}{";
        for (auto it = m_container.begin(); it != it_f; ++it) {
            bool printed_something = false;
            os << "\\left(";
            for (typename v_type::size_type i = 0u; i < it->v.size(); ++i) {
                // If the aij is zero, don't print anything.
                if (math::is_zero(it->v[i])) {
                    continue;
                }
                // A positive aij, in case previous output exists, must be preceded
                // by a "+" sign.
                if (it->v[i] > 0 && printed_something) {
                    os << '+';
                }
                // Print the aij, unless it's "-1": in that case, just print the minus sign.
                if (it->v[i] == -1) {
                    os << '-';
                } else if (it->v[i] != 1) {
                    os << detail::prepare_for_print(it->v[i]);
                }
                // Finally, print name of variable.
                os << args[i].get_name();
                printed_something = true;
            }
            os << "\\right)";
            // Print the exponent, if different from one.
            if (it->e != 1) {
                os << "^{" << detail::prepare_for_print(it->e) << "}";
            }
        }
        os << '}';
    }
    /// Evaluation.
    /**
     * \note
     * This method is available only if \p U satisfies the following requirements:
     * - it can be used in piranha::symbol_set::positions_map,
     * - it supports the arithmetic operations necessary to construct the return type.
     *
     * The return value will be built via multiplications of the \f$ a_{i,j} \f$ by the values
     * in the input map, additions, divisions and exponentiations via piranha::math::pow().
     * If the divisor has no terms, 1 will be returned. If \p args is not compatible with \p this and \p pmap,
     * or the positions in \p pmap do not reference only and all the variables in the divisor, an error will be thrown.
     *
     * @param pmap piranha::symbol_set::positions_map that will be used for substitution.
     * @param args reference set of piranha::symbol.
     *
     * @return the result of evaluating \p this with the values provided in \p pmap.
     *
     * @throws std::invalid_argument if there exist an incompatibility between \p this,
     * \p args or \p pmap.
     * @throws unspecified any exception thrown by the construction of the return type.
     */
    template <typename U>
    eval_type<U> evaluate(const symbol_set::positions_map<U> &pmap, const symbol_set &args) const
    {
        // Just return 1 if the container is empty.
        eval_type<U> retval(1);
        if (m_container.empty()) {
            return retval;
        }
        // NOTE: the positions map must have the same number of elements as this, and it must
        // be made of consecutive positions [0,size - 1].
        if (unlikely(pmap.size() != m_container.begin()->v.size()
                     || (pmap.size() && pmap.back().first != pmap.size() - 1u))) {
            piranha_throw(std::invalid_argument, "invalid positions map for evaluation");
        }
        // Args must be compatible with this.
        if (unlikely(args.size() != m_container.begin()->v.size())) {
            piranha_throw(std::invalid_argument, "invalid size of arguments set");
        }
        const auto it_f = m_container.end();
        for (auto it = m_container.begin(); it != it_f; ++it) {
            eval_sum_type<U> tmp(0);
            auto map_it = pmap.begin();
            for (typename v_type::size_type i = 0u; i < it->v.size(); ++i, ++map_it) {
                piranha_assert(map_it != pmap.end() && map_it->first == i);
                // NOTE: might use multadd here eventually for performance.
                tmp += it->v[i] * map_it->second;
            }
            piranha_assert(map_it == pmap.end());
            // NOTE: consider rewriting this in terms of multiplications as a performance
            // improvement - the eval_type deduction should be changed accordingly.
            retval /= math::pow(tmp, it->e);
        }
        return retval;
    }
    /// Multiply terms with a divisor key.
    /**
     * \note
     * This method is enabled only if \p Cf satisfies piranha::is_cf and piranha::has_mul3.
     *
     * Multiply \p t1 by \p t2, storing the result in the only element of \p res.  If \p Cf is an instance of
     * piranha::mp_rational, then
     * only the numerators of the coefficients will be multiplied.
     *
     * This method offers the basic exception safety guarantee.
     *
     * @param res return value.
     * @param t1 first argument.
     * @param t2 second argument.
     * @param args reference set of arguments.
     *
     * @throws std::invalid_argument if the key of \p t1 and/or the key of \p t2 are incompatible with \p args, or if
     * the multiplication
     * of the keys results in an exponent exceeding the allowed range.
     * @throws std::overflow_error if the multiplication of the keys results in the container of the result key to be
     * resized over
     * an implementation-defined limit.
     * @throws unspecified any exception thrown by:
     * - piranha::math::mul3(),
     * - the public interface of piranha::hash_set,
     * - arithmetic operations on the exponent.
     */
    template <typename Cf, multiply_enabler<Cf> = 0>
    static void multiply(std::array<term<Cf, divisor>, multiply_arity> &res, const term<Cf, divisor> &t1,
                         const term<Cf, divisor> &t2, const symbol_set &args)
    {
        term<Cf, divisor> &t = res[0u];
        if (unlikely(!t1.m_key.is_compatible(args) || !t2.m_key.is_compatible(args))) {
            piranha_throw(std::invalid_argument, "invalid size of arguments set");
        }
        // Coefficient.
        detail::cf_mult_impl(t.m_cf, t1.m_cf, t2.m_cf);
        // Now deal with the key.
        // Establish the largest and smallest divisor.
        const divisor &large = (t1.m_key.size() >= t2.m_key.size()) ? t1.m_key : t2.m_key;
        const divisor &small = (t1.m_key.size() < t2.m_key.size()) ? t1.m_key : t2.m_key;
        // Assign the large to the result.
        t.m_key = large;
        // Run the multiplication.
        const auto it_f = small.m_container.end();
        for (auto it = small.m_container.begin(); it != it_f; ++it) {
            t.m_key.insertion_impl(*it);
        }
    }
    /// Identify symbols that can be trimmed.
    /**
     * This method is used in piranha::series::trim(). The input parameter \p candidates
     * contains a set of symbols that are candidates for elimination. The method will remove
     * from \p candidates those symbols whose \f$ a_{i,j} \f$ in \p this are not all zeroes.
     *
     * @param candidates set of candidates for elimination.
     * @param args reference arguments set.
     *
     * @throws std::invalid_argument if \p this is not compatible with \p args.
     * @throws unspecified any exception thrown by piranha::math::is_zero() or piranha::symbol_set::remove().
     */
    void trim_identify(symbol_set &candidates, const symbol_set &args) const
    {
        if (unlikely(!is_compatible(args))) {
            piranha_throw(std::invalid_argument, "invalid arguments set for trim_identify()");
        }
        const auto it_f = m_container.end();
        for (auto it = m_container.begin(); it != it_f; ++it) {
            for (typename v_type::size_type i = 0u; i < it->v.size(); ++i) {
                if (!math::is_zero(it->v[i]) && std::binary_search(candidates.begin(), candidates.end(),
                                                                   args[static_cast<symbol_set::size_type>(i)])) {
                    candidates.remove(args[static_cast<symbol_set::size_type>(i)]);
                }
            }
        }
    }
    /// Trim.
    /**
     * This method will return a copy of \p this with the \f$ a_{i,j} \f$ associated to the symbols
     * in \p trim_args removed.
     *
     * @param trim_args arguments whose \f$ a_{i,j} \f$ will be removed.
     * @param orig_args original arguments set.
     *
     * @return trimmed copy of \p this.
     *
     * @throws std::invalid_argument if \p this is not compatible with \p orig_args.
     * @throws unspecified any exception thrown by piranha::small_vector::push_back()
     * or piranha::divisor::insert().
     */
    divisor trim(const symbol_set &trim_args, const symbol_set &orig_args) const
    {
        if (unlikely(!is_compatible(orig_args))) {
            piranha_throw(std::invalid_argument, "invalid arguments set for trim()");
        }
        divisor retval;
        const auto it_f = m_container.end();
        for (auto it = m_container.begin(); it != it_f; ++it) {
            v_type tmp;
            for (typename v_type::size_type i = 0u; i < it->v.size(); ++i) {
                if (!std::binary_search(trim_args.begin(), trim_args.end(),
                                        orig_args[static_cast<symbol_set::size_type>(i)])) {
                    tmp.push_back(it->v[i]);
                }
            }
            retval.insert(tmp.begin(), tmp.end(), it->e);
        }
        return retval;
    }
    /// Split divisor.
    /**
     * This method will split \p this into two parts: the first one will contain the terms of the divisor
     * whose \f$ a_{i,j} \f$ values for the only symbol in \p p are not zero, the second one the terms whose
     * \f$ a_{i,j} \f$  values for the only symbol in \p p are zero.
     *
     * @param p a piranha::symbol_set::positions containing exactly one element.
     * @param args reference set of piranha::symbol.
     *
     * @return the original divisor split into two parts.
     *
     * @throws std::invalid_argument if \p args is not compatible with \p this or \p p, or \p p
     * does not contain exactly one element.
     * @throws unspecified any exception thrown by:
     * - piranha::math::is_zero(),
     * - piranha::hash_set::insert().
     */
    std::pair<divisor, divisor> split(const symbol_set::positions &p, const symbol_set &args) const
    {
        if (unlikely(!is_compatible(args))) {
            piranha_throw(std::invalid_argument, "invalid size of arguments set");
        }
        if (unlikely(p.size() != 1u || p.back() >= args.size())) {
            piranha_throw(std::invalid_argument, "invalid size of symbol_set::positions");
        }
        using s_type = typename v_type::size_type;
        std::pair<divisor, divisor> retval;
        const auto it_f = m_container.end();
        for (auto it = m_container.begin(); it != it_f; ++it) {
            // NOTE: static cast is safe here, as we checked for compatibility.
            if (math::is_zero(it->v[static_cast<s_type>(p.back())])) {
                retval.second.m_container.insert(*it);
            } else {
                retval.first.m_container.insert(*it);
            }
        }
        return retval;
    }

private:
#if !defined(PIRANHA_DOXYGEN_INVOKED)
    // Make friend with the s11n functions.
    template <typename Archive, typename T1>
    friend void boost::serialization::save(Archive &, const piranha::boost_s11n_key_wrapper<piranha::divisor<T1>> &,
                                           unsigned);
    template <typename Archive, typename T1>
    friend void boost::serialization::load(Archive &, piranha::boost_s11n_key_wrapper<piranha::divisor<T1>> &,
                                           unsigned);
#endif

#if defined(PIRANHA_WITH_MSGPACK)
    template <typename Stream>
    using msgpack_pack_enabler
        = enable_if_t<conjunction<is_msgpack_stream<Stream>, has_msgpack_pack<Stream, container_type>>::value, int>;
    template <typename U>
    using msgpack_convert_enabler = enable_if_t<has_msgpack_convert<typename U::container_type>::value, int>;

public:
    /// Pack in msgpack format.
    /**
     * \note
     * This method is enabled only if \p Stream satisfies piranha::is_msgpack_stream and the internal container type
     * satisfies piranha::has_msgpack_pack.
     *
     * This method will pack \p this in to \p p using the format f.
     *
     * @param p the target <tt>msgpack::packer</tt>.
     * @param f the desired piranha::msgpack_format.
     * @param args reference symbol set.
     *
     * @throws std::invalid_argument if \p args is not compatible with \p this.
     * @throws unspecified any exception thrown by piranha::msgpack_pack().
     */
    template <typename Stream, msgpack_pack_enabler<Stream> = 0>
    void msgpack_pack(msgpack::packer<Stream> &p, msgpack_format f, const symbol_set &args) const
    {
        if (unlikely(!is_compatible(args))) {
            piranha_throw(std::invalid_argument, "an invalid symbol_set was passed as an argument for the "
                                                 "msgpack_pack() method of a divisor");
        }
        piranha::msgpack_pack(p, m_container, f);
    }
    /// Convert from msgpack object.
    /**
     * \note
     * This method is enabled only if the internal container type satisfies piranha::has_msgpack_convert.
     *
     * This method will convert the input msgpack object \p o into \p this, using the format \p f. The method
     * provides the basic exception safety guarantee.
     *
     * @param o the input <tt>msgpack::object</tt>.
     * @param f the desired piranha::msgpack_format.
     * @param args reference symbol set.
     *
     * @throws std::invalid_argument if the deserialized divisor fails internal consistency checks, or if it is not
     * compatible with \p args.
     * @throws unspecified any exception throw by piranha::msgpack_convert().
     */
    template <typename U = divisor, msgpack_convert_enabler<U> = 0>
    void msgpack_convert(const msgpack::object &o, msgpack_format f, const symbol_set &args)
    {
        try {
            piranha::msgpack_convert(m_container, o, f);
            if (unlikely(!destruction_checks())) {
                piranha_throw(std::invalid_argument, "the divisor loaded from a msgpack object failed internal "
                                                     "consistency checks");
            }
            if (unlikely(!is_compatible(args))) {
                piranha_throw(std::invalid_argument, "the divisor loaded from a msgpack object is not compatible "
                                                     "with the supplied symbol set");
            }
        } catch (...) {
            m_container = container_type{};
            throw;
        }
    }
#endif

private:
    container_type m_container;
};

template <typename T>
const std::size_t divisor<T>::multiply_arity;

inline namespace impl
{

template <typename Archive, typename T>
using divisor_boost_save_enabler = enable_if_t<has_boost_save<Archive, typename divisor<T>::container_type>::value>;

template <typename Archive, typename T>
using divisor_boost_load_enabler = enable_if_t<has_boost_load<Archive, typename divisor<T>::container_type>::value>;
}

/// Specialisation of piranha::boost_save() for piranha::divisor.
/**
 * \note
 * This specialisation is enabled only if piranha::divisor::container_type satisfies piranha::has_boost_save.
 *
 * @throws std::invalid_argument if the symbol set is incompatible with the divisor.
 * @throws unspecified any exception thrown by piranha::boost_save().
 */
template <typename Archive, typename T>
struct boost_save_impl<Archive, boost_s11n_key_wrapper<divisor<T>>, divisor_boost_save_enabler<Archive, T>>
    : boost_save_via_boost_api<Archive, boost_s11n_key_wrapper<divisor<T>>> {
};

/// Specialisation of piranha::boost_load() for piranha::divisor.
/**
 * \note
 * This specialisation is enabled only if piranha::divisor::container_type satisfies piranha::has_boost_load.
 *
 * The basic exception safety guarantee is provided.
 *
 * @throws std::invalid_argument if the symbol set is not compatible with the loaded divisor or if the loaded divisor
 * fails internal consistency checks.
 * @throws unspecified any exception thrown by piranha::boost_load().
 */
template <typename Archive, typename T>
struct boost_load_impl<Archive, boost_s11n_key_wrapper<divisor<T>>, divisor_boost_load_enabler<Archive, T>>
    : boost_load_via_boost_api<Archive, boost_s11n_key_wrapper<divisor<T>>> {
};
}

namespace std
{

template <typename T>
struct hash<piranha::divisor<T>> {
    /// Result type.
    typedef size_t result_type;
    /// Argument type.
    typedef piranha::divisor<T> argument_type;
    /// Hash operator.
    /**
     * @param a piranha::divisor whose hash value will be returned.
     *
     * @return piranha::divisor::hash().
     */
    result_type operator()(const argument_type &a) const
    {
        return a.hash();
    }
};
}

#endif

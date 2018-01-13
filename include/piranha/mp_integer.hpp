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

#ifndef PIRANHA_MP_INTEGER_HPP
#define PIRANHA_MP_INTEGER_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <piranha/config.hpp>
#include <piranha/detail/demangle.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/is_key.hpp>
#include <piranha/math.hpp>
#define MPPP_WITH_LONG_DOUBLE
#include <piranha/mppp/mp++.hpp>
#undef MPPP_WITH_LONG_DOUBLE
#include <piranha/s11n.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

/// Alias for mppp::mp_integer.
template <std::size_t SSize>
using mp_integer = mppp::mp_integer<SSize>;

/// Alias for piranha::mp_integer with 1 limb of static storage.
using integer = mp_integer<1>;

inline namespace impl
{

// Detect if T is an instance of piranha::mp_integer.
// This is used in some enablers (e.g., gcd).
template <typename>
struct is_mp_integer : std::false_type {
};

template <std::size_t SSize>
struct is_mp_integer<mp_integer<SSize>> : std::true_type {
};

// Detect if T and U are both mp_integer with same SSize.
template <typename, typename>
struct is_same_mp_integer : std::false_type {
};

template <std::size_t SSize>
struct is_same_mp_integer<mp_integer<SSize>, mp_integer<SSize>> : std::true_type {
};
}

namespace math
{

/// Specialisation of the implementation of piranha::math::multiply_accumulate() for piranha::mp_integer.
template <std::size_t SSize>
struct multiply_accumulate_impl<mp_integer<SSize>> {
    /// Call operator.
    /**
     * This implementation will use piranha::mp_integer::addmul().
     *
     * @param x target value for accumulation.
     * @param y first argument.
     * @param z second argument.
     */
    void operator()(mp_integer<SSize> &x, const mp_integer<SSize> &y, const mp_integer<SSize> &z) const
    {
        addmul(x, y, z);
    }
};

/// Specialisation of the implementation of piranha::math::negate() for piranha::mp_integer.
template <std::size_t SSize>
struct negate_impl<mp_integer<SSize>> {
    /// Call operator.
    /**
     * This implementation will use piranha::mp_integer::neg().
     *
     * @param n piranha::mp_integer to be negated.
     */
    void operator()(mp_integer<SSize> &n) const
    {
        n.neg();
    }
};

/// Specialisation of the implementation of piranha::math::is_zero() for piranha::mp_integer.
template <std::size_t SSize>
struct is_zero_impl<mp_integer<SSize>> {
    /// Call operator.
    /**
     * This implementation will use internally piranha::mp_integer::is_zero().
     *
     * @param n piranha::mp_integer to be tested.
     *
     * @return \p true if \p n is zero, \p false otherwise.
     */
    bool operator()(const mp_integer<SSize> &n) const
    {
        return n.is_zero();
    }
};

/// Specialisation of the implementation of piranha::math::is_unitary() for piranha::mp_integer.
template <std::size_t SSize>
struct is_unitary_impl<mp_integer<SSize>> {
    /// Call operator.
    /**
     * This implementation will use internally piranha::mp_integer::is_one().
     *
     * @param n piranha::mp_integer to be tested.
     *
     * @return \p true if \p n is equal to 1, \p false otherwise.
     */
    bool operator()(const mp_integer<SSize> &n) const
    {
        return n.is_one();
    }
};
}

inline namespace impl
{

// Avoid name clash below.
template <std::size_t SSize>
inline mp_integer<SSize> mp_integer_abs_wrapper(const mp_integer<SSize> &n)
{
    return abs(n);
}
}

namespace math
{
/// Specialisation of the implementation of piranha::math::abs() for piranha::mp_integer.
template <std::size_t SSize>
struct abs_impl<mp_integer<SSize>> {
    /// Call operator.
    /**
     * This implementation will use internally piranha::mp_integer::abs(const mp_integer &).
     *
     * @param n input parameter.
     *
     * @return absolute value of \p n.
     */
    mp_integer<SSize> operator()(const mp_integer<SSize> &n) const
    {
        return mp_integer_abs_wrapper(n);
    }
};

/// Specialisation of the implementation of piranha::math::sin() for piranha::mp_integer.
template <std::size_t SSize>
struct sin_impl<mp_integer<SSize>> {
    /// Call operator.
    /**
     * @param n argument.
     *
     * @return sine of \p n.
     *
     * @throws std::invalid_argument if the argument is not zero.
     */
    mp_integer<SSize> operator()(const mp_integer<SSize> &n) const
    {
        if (is_zero(n)) {
            return mp_integer<SSize>{};
        }
        piranha_throw(std::invalid_argument, "cannot compute the sine of a non-zero integer");
    }
};

/// Specialisation of the implementation of piranha::math::cos() for piranha::mp_integer.
template <std::size_t SSize>
struct cos_impl<mp_integer<SSize>> {
    /// Call operator.
    /**
     * @param n argument.
     *
     * @return cosine of \p n.
     *
     * @throws std::invalid_argument if the argument is not zero.
     */
    mp_integer<SSize> operator()(const mp_integer<SSize> &n) const
    {
        if (is_zero(n)) {
            return mp_integer<SSize>{1};
        }
        piranha_throw(std::invalid_argument, "cannot compute the cosine of a non-zero integer");
    }
};

/// Specialisation of the implementation of piranha::math::partial() for piranha::mp_integer.
template <std::size_t SSize>
struct partial_impl<mp_integer<SSize>> {
    /// Call operator.
    /**
     * @return an instance of piranha::mp_integer constructed from zero.
     */
    mp_integer<SSize> operator()(const mp_integer<SSize> &, const std::string &) const
    {
        return mp_integer<SSize>{};
    }
};

/// Factorial.
/**
 * This function will use internally piranha::mp_integer::fac_ui().
 *
 * @param n factorial argument.
 *
 * @return the factorial of \p n.
 *
 * @throws std::domain_error if \p n is negative.
 * @throws unspecified any exception thrown by piranha::mp_integer::fac_ui() or by the conversion
 * of \p n to <tt>unsigned long</tt>.
 */
template <std::size_t SSize>
inline mp_integer<SSize> factorial(const mp_integer<SSize> &n)
{
    if (unlikely(sgn(n) < 0)) {
        piranha_throw(std::domain_error, "cannot compute the factorial of the negative integer " + n.to_string());
    }
    mp_integer<SSize> retval;
    fac_ui(retval, static_cast<unsigned long>(n));
    return retval;
}

/// Default functor for the implementation of piranha::math::ipow_subs().
/**
 * This functor can be specialised via the \p std::enable_if mechanism. The default implementation does not define
 * the call operator, and will thus generate a compile-time error if used.
 */
template <typename T, typename U, typename = void>
struct ipow_subs_impl {
};
}

inline namespace impl
{

// Enabler for ipow_subs().
template <typename T, typename U>
using math_ipow_subs_t_
    = decltype(math::ipow_subs_impl<T, U>{}(std::declval<const T &>(), std::declval<const std::string &>(),
                                            std::declval<const integer &>(), std::declval<const U &>()));

template <typename T, typename U>
using math_ipow_subs_t = enable_if_t<is_returnable<math_ipow_subs_t_<T, U>>::value, math_ipow_subs_t_<T, U>>;
}

namespace math
{

/// Substitution of integral power.
/**
 * \note
 * This function is enabled only if the expression <tt>ipow_subs_impl<T, U>{}(x, name, n, y)</tt> is valid, returning
 * a type which satisfies piranha::is_returnable.
 *
 * Substitute, in \p x, <tt>name**n</tt> with \p y. The actual implementation of this function is in the
 * piranha::math::ipow_subs_impl functor's call operator. The body of this function is equivalent to:
 * @code
 * return ipow_subs_impl<T, U>{}(x, name, n, y);
 * @endcode
 *
 * @param x quantity that will be subject to substitution.
 * @param name name of the symbolic variable that will be substituted.
 * @param n power of \p name that will be substituted.
 * @param y object that will substitute the variable.
 *
 * @return \p x after the substitution of \p name to the power of \p n with \p y.
 *
 * @throws unspecified any exception thrown by the call operator of piranha::math::subs_impl.
 */
template <typename T, typename U>
inline math_ipow_subs_t<T, U> ipow_subs(const T &x, const std::string &name, const integer &n, const U &y)
{
    return ipow_subs_impl<T, U>{}(x, name, n, y);
}

/// Specialisation of the implementation of piranha::math::add3() for piranha::mp_integer.
template <std::size_t SSize>
struct add3_impl<mp_integer<SSize>> {
    /// Call operator.
    /**
     * This implementation will use internally piranha::mp_integer::add().
     *
     * @param out the output value.
     * @param a the first operand.
     * @param b the second operand.
     */
    void operator()(mp_integer<SSize> &out, const mp_integer<SSize> &a, const mp_integer<SSize> &b) const
    {
        add(out, a, b);
    }
};

/// Specialisation of the implementation of piranha::math::sub3() for piranha::mp_integer.
template <std::size_t SSize>
struct sub3_impl<mp_integer<SSize>> {
    /// Call operator.
    /**
     * This implementation will use internally piranha::mp_integer::sub().
     *
     * @param out the output value.
     * @param a the first operand.
     * @param b the second operand.
     */
    void operator()(mp_integer<SSize> &out, const mp_integer<SSize> &a, const mp_integer<SSize> &b) const
    {
        sub(out, a, b);
    }
};

/// Specialisation of the implementation of piranha::math::mul3() for piranha::mp_integer.
template <std::size_t SSize>
struct mul3_impl<mp_integer<SSize>> {
    /// Call operator.
    /**
     * This implementation will use internally piranha::mp_integer::mul().
     *
     * @param out the output value.
     * @param a the first operand.
     * @param b the second operand.
     */
    void operator()(mp_integer<SSize> &out, const mp_integer<SSize> &a, const mp_integer<SSize> &b) const
    {
        mul(out, a, b);
    }
};

/// Specialisation of the implementation of piranha::math::div3() for piranha::mp_integer.
template <std::size_t SSize>
struct div3_impl<mp_integer<SSize>> {
    /// Call operator.
    /**
     * This implementation will use internally piranha::mp_integer::tdiv_qr().
     *
     * @param out the output value.
     * @param a the dividend.
     * @param b the divisor.
     *
     * @throws unspecified any exception thrown by piranha::mp_integer::tdiv_qr().
     */
    void operator()(mp_integer<SSize> &out, const mp_integer<SSize> &a, const mp_integer<SSize> &b) const
    {
        mp_integer<SSize> r;
        tdiv_qr(out, r, a, b);
    }
};
}

inline namespace impl
{

// Enabler for the GCD specialisation.
template <typename T, typename U>
using math_mp_integer_gcd_enabler
    = enable_if_t<disjunction<conjunction<std::is_integral<T>, is_mp_integer<U>, std::is_constructible<U, const T &>>,
                              conjunction<std::is_integral<U>, is_mp_integer<T>, std::is_constructible<T, const U &>>,
                              is_same_mp_integer<T, U>>::value>;

// Wrapper to avoid clashes with piranha::math::gcd().
template <std::size_t SSize>
inline mp_integer<SSize> mp_integer_gcd_wrapper(const mp_integer<SSize> &a, const mp_integer<SSize> &b)
{
    return gcd(a, b);
}
}

namespace math
{

/// Specialisation of the implementation of piranha::math::gcd() for piranha::mp_integer.
/**
 * This specialisation is enabled when:
 * - \p T and \p U are both instances of piranha::mp_integer with the same static size,
 * - \p T is an instance of piranha::mp_integer and \p U is an integral type from which piranha::mp_integer is
 *   constructible,
 * - \p U is an instance of piranha::mp_integer and \p T is an integral type from which piranha::mp_integer is
 *   constructible.
 *
 * The result will be calculated via piranha::mp_integer::gcd(const mp_integer &, const mp_integer &), after any
 * necessary type conversion.
 */
template <typename T, typename U>
struct gcd_impl<T, U, math_mp_integer_gcd_enabler<T, U>> {
    /// Call operator, piranha::mp_integer - piranha::mp_integer overload.
    /**
     * @param a first argument.
     * @param b second argument.
     *
     * @return the GCD of \p a and \p b.
     */
    template <std::size_t SSize>
    mp_integer<SSize> operator()(const mp_integer<SSize> &a, const mp_integer<SSize> &b) const
    {
        return mp_integer_gcd_wrapper(a, b);
    }
    /// Call operator, piranha::mp_integer - integral overload.
    /**
     * @param a first argument.
     * @param b second argument.
     *
     * @return the GCD of \p a and \p b.
     */
    template <std::size_t SSize, typename T1>
    mp_integer<SSize> operator()(const mp_integer<SSize> &a, const T1 &b) const
    {
        return operator()(a, mp_integer<SSize>(b));
    }
    /// Call operator, integral - piranha::mp_integer overload.
    /**
     * @param a first argument.
     * @param b second argument.
     *
     * @return the GCD of \p a and \p b.
     */
    template <std::size_t SSize, typename T1>
    mp_integer<SSize> operator()(const T1 &a, const mp_integer<SSize> &b) const
    {
        return operator()(b, a);
    }
};

/// Specialisation of the implementation of piranha::math::gcd3() for piranha::mp_integer.
template <std::size_t SSize>
struct gcd3_impl<mp_integer<SSize>> {
    /// Call operator.
    /**
     * This call operator will use internally
     * piranha::mp_integer::gcd(mp_integer &, const mp_integer &, const mp_integer &).
     *
     * @param out return value.
     * @param a first argument.
     * @param b second argument.
     */
    void operator()(mp_integer<SSize> &out, const mp_integer<SSize> &a, const mp_integer<SSize> &b) const
    {
        gcd(out, a, b);
    }
};
}

inline namespace impl
{

// Enabler for the overload below.
// NOTE: is_returnable is already checked by the invocation of the other overload.
template <typename T, typename U, typename Int>
using math_ipow_subs_int_t_
    = decltype(math::ipow_subs(std::declval<const T &>(), std::declval<const std::string &>(),
                               integer{std::declval<const Int &>()}, std::declval<const U &>()));

template <typename T, typename U, typename Int>
using math_ipow_subs_int_t = enable_if_t<std::is_integral<Int>::value, math_ipow_subs_int_t_<T, U, Int>>;
}

namespace math
{

/// Substitution of integral power (convenience overload).
/**
 * \note
 * This function is enabled only if:
 * - \p Int is a C++ integral type from which piranha::integer can be constructed, and
 * - the other overload of piranha::math::ipow_subs() is enabled with template arguments \p T and \p U.
 *
 * This function is a convenience wrapper that will call the other piranha::math::ipow_subs() overload, with \p n
 * converted to a piranha::integer.
 *
 * @param x quantity that will be subject to substitution.
 * @param name name of the symbolic variable that will be substituted.
 * @param n power of \p name that will be substituted.
 * @param y object that will substitute the variable.
 *
 * @return \p x after the substitution of \p name to the power of \p n with \p y.
 *
 * @throws unspecified any exception thrown by the other overload of piranha::math::ipow_subs().
 */
template <typename T, typename U, typename Int>
inline math_ipow_subs_int_t<T, U, Int> ipow_subs(const T &x, const std::string &name, const Int &n, const U &y)
{
    return ipow_subs(x, name, integer{n}, y);
}
}

/// Type trait to detect the availability of the piranha::math::ipow_subs() function.
/**
 * This type trait will be \p true if piranha::math::ipow_subs() can be successfully called
 * with template parameter \p T and \p U, \p false otherwise..
 */
template <typename T, typename U>
class has_ipow_subs
{
    template <typename T1, typename U1>
    using ipow_subs_t = decltype(math::ipow_subs(std::declval<const T1 &>(), std::declval<std::string const &>(),
                                                 std::declval<integer const &>(), std::declval<const U1 &>()));
    static const bool implementation_defined = is_detected<ipow_subs_t, T, U>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T, typename U>
const bool has_ipow_subs<T, U>::value;

/// Type trait to detect the presence of the integral power substitution method in keys.
/**
 * This type trait will be \p true if \p Key provides a const method <tt>ipow_subs()</tt> accepting as
 * const parameters a const reference to a piranha::symbol_idx, an instance of piranha::integer, an instance of \p T and
 * an instance of piranha::symbol_fset. The return value of the method must be an <tt>std::vector</tt> of pairs in which
 * the second type must be \p Key itself (after the removal of cv/reference qualifiers). The <tt>ipow_subs()</tt> method
 * represents the substitution of the integral power of a symbol with an instance of type \p T.
 *
 * \p Key must satisfy piranha::is_key after the removal of cv/reference qualifiers, otherwise
 * a compile-time error will be emitted.
 */
template <typename Key, typename T>
class key_has_ipow_subs
{
    PIRANHA_TT_CHECK(is_key, uncvref_t<Key>);
    template <typename Key1, typename T1>
    using key_ipow_subs_t = decltype(
        std::declval<const Key1 &>().ipow_subs(std::declval<const symbol_idx &>(), std::declval<const integer &>(),
                                               std::declval<const T1 &>(), std::declval<const symbol_fset &>()));
    template <typename T1>
    struct check_result_type : std::false_type {
    };
    template <typename Res>
    struct check_result_type<std::vector<std::pair<Res, uncvref_t<Key>>>> : std::true_type {
    };
    static const bool implementation_defined = check_result_type<detected_t<key_ipow_subs_t, Key, T>>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

// Static init.
template <typename Key, typename T>
const bool key_has_ipow_subs<Key, T>::value;

inline namespace literals
{

/// Literal for arbitrary-precision integers.
/**
 * @param s literal string.
 *
 * @return a piranha::integer constructed from \p s.
 *
 * @throws unspecified any exception thrown by the constructor of
 * piranha::integer from string.
 */
inline integer operator"" _z(const char *s)
{
    return integer(s);
}
}

inline namespace impl
{

// These are a few utils useful for serialization.
using mpz_size_t = mppp::mppp_impl::mpz_size_t;

inline mpz_size_t mp_integer_safe_abs_size(mpz_size_t s)
{
    if (unlikely(s < -detail::safe_abs_sint<mpz_size_t>::value)) {
        piranha_throw(std::overflow_error, "the number of limbs is too large");
    }
    return static_cast<mpz_size_t>(s >= 0 ? s : -s);
}
}
}

namespace boost
{
namespace serialization
{

template <typename Archive, std::size_t SSize,
          piranha::enable_if_t<std::is_same<Archive, boost::archive::binary_oarchive>::value, int> = 0>
inline void save(Archive &ar, const piranha::mp_integer<SSize> &n, unsigned)
{
    const auto &int_u = n._get_union();
    if (n.is_static()) {
        piranha::boost_save(ar, true);
        // NOTE: alloc size is known for static ints.
        const auto size = int_u.g_st()._mp_size;
        piranha::boost_save(ar, size);
        std::for_each(int_u.g_st().m_limbs.data(), int_u.g_st().m_limbs.data() + (size >= 0 ? size : -size),
                      [&ar](const ::mp_limb_t &l) { piranha::boost_save(ar, l); });
    } else {
        piranha::boost_save(ar, false);
        // NOTE: don't record alloc size, we will reserve an adequate size on load.
        piranha::boost_save(ar, int_u.g_dy()._mp_size);
        std::for_each(int_u.g_dy()._mp_d, int_u.g_dy()._mp_d + ::mpz_size(&int_u.g_dy()),
                      [&ar](const ::mp_limb_t &l) { piranha::boost_save(ar, l); });
    }
}

template <typename Archive, std::size_t SSize,
          piranha::enable_if_t<std::is_same<Archive, boost::archive::binary_iarchive>::value, int> = 0>
inline void load(Archive &ar, piranha::mp_integer<SSize> &n, unsigned)
{
    const bool n_s = n.is_static();
    bool s;
    piranha::boost_load(ar, s);
    // If the staticness of n and the serialized object differ, we have
    // to adjust n.
    if (s != n_s) {
        if (n_s) {
            // n is static, serialized is dynamic.
            const bool pstatus = n.promote();
            (void)pstatus;
            assert(pstatus);
        } else {
            // n is dynamic, serialized is static.
            n = piranha::mp_integer<SSize>{};
        }
    }
    auto &int_u = n._get_union();
    if (s) {
        try {
            // NOTE: alloc is already set to the correct value.
            piranha_assert(int_u.is_static());
            piranha::boost_load(ar, int_u.g_st()._mp_size);
            // Check the size from the archive is not bogus
            auto size = int_u.g_st()._mp_size;
            if (unlikely(size > int_u.g_st().s_size || size < -int_u.g_st().s_size)) {
                piranha_throw(std::invalid_argument, "cannot deserialize a static integer with signed limb size "
                                                         + std::to_string(size) + " (the maximum static limb size is "
                                                         + std::to_string(int_u.g_st().s_size) + ")");
            }
            // Make absolute value. This is safe as we now know that size fits in the static size range.
            size = (size >= 0) ? size : -size;
            // Deserialize.
            auto data = int_u.g_st().m_limbs.data();
            std::for_each(data, data + size, [&ar](::mp_limb_t &l) { piranha::boost_load(ar, l); });
            // Zero the limbs that were not loaded from the archive.
            std::fill(data + size, data + int_u.g_st().s_size, 0u);
        } catch (...) {
            // Reset the static before re-throwing.
            int_u.g_st()._mp_size = 0;
            std::fill(int_u.g_st().m_limbs.begin(), int_u.g_st().m_limbs.end(), 0u);
            throw;
        }
    } else {
        piranha::mpz_size_t sz;
        piranha::boost_load(ar, sz);
        const auto size = piranha::mp_integer_safe_abs_size(sz);
        // NOTE: static_cast is safe because of the type checks in mp++.
        ::_mpz_realloc(&int_u.g_dy(), static_cast<::mp_size_t>(size));
        try {
            std::for_each(int_u.g_dy()._mp_d, int_u.g_dy()._mp_d + size,
                          [&ar](::mp_limb_t &l) { piranha::boost_load(ar, l); });
            int_u.g_dy()._mp_size = sz;
        } catch (...) {
            // NOTE: only possible exception here is when boost_load(ar,l) throws. In this case we have successfully
            // reallocated and possibly written some limbs, but we have not set the size yet. Just zero out the mpz.
            ::mpz_set_ui(&int_u.g_dy(), 0u);
            throw;
        }
    }
}

// Portable serialization.
template <class Archive, std::size_t SSize,
          piranha::enable_if_t<!std::is_same<Archive, boost::archive::binary_oarchive>::value, int> = 0>
inline void save(Archive &ar, const piranha::mp_integer<SSize> &n, unsigned)
{
    // NOTE: here we have an unnecessary copy (but at least we are avoiding memory allocations).
    // Maybe we should consider providing an API from mp++ to interact directly with strings
    // rather than vectors of chars.
    PIRANHA_MAYBE_TLS std::vector<char> tmp_v;
    mppp::mppp_impl::mpz_to_str(tmp_v, n.get_mpz_view());
    PIRANHA_MAYBE_TLS std::string tmp_s;
    tmp_s.assign(tmp_v.data());
    piranha::boost_save(ar, tmp_s);
}

template <class Archive, std::size_t SSize,
          piranha::enable_if_t<!std::is_same<Archive, boost::archive::binary_iarchive>::value, int> = 0>
inline void load(Archive &ar, piranha::mp_integer<SSize> &n, unsigned)
{
    PIRANHA_MAYBE_TLS std::string tmp;
    piranha::boost_load(ar, tmp);
    n = piranha::mp_integer<SSize>{tmp};
}

template <class Archive, std::size_t SSize>
inline void serialize(Archive &ar, piranha::mp_integer<SSize> &n, const unsigned int file_version)
{
    split_free(ar, n, file_version);
}
}
}

namespace piranha
{

inline namespace impl
{

template <typename Archive>
using mp_integer_boost_save_enabler
    = enable_if_t<conjunction<has_boost_save<Archive, std::string>, has_boost_save<Archive, mpz_size_t>,
                              has_boost_save<Archive, bool>, has_boost_save<Archive, ::mp_limb_t>>::value>;

template <typename Archive>
using mp_integer_boost_load_enabler
    = enable_if_t<conjunction<has_boost_load<Archive, std::string>, has_boost_load<Archive, mpz_size_t>,
                              has_boost_load<Archive, bool>, has_boost_load<Archive, ::mp_limb_t>>::value>;
}

/// Specialisation of piranha::boost_save() for piranha::mp_integer.
/**
 * \note
 * This specialisation is enabled only if \p std::string and all the integral types in terms of which
 * piranha::mp_integer is implemented satisfy piranha::has_boost_save.
 *
 * If \p Archive is \p boost::archive::binary_oarchive, a platform-dependent non-portable
 * representation of the input integer is saved. Otherwise, a string representation of the input
 * integer is saved.
 *
 * @throws unspecified any exception thrown by
 * - piranha::boost_save(),
 * - the conversion of piranha::mp_integer to string,
 * - memory errors in standard containers.
 */
template <typename Archive, std::size_t SSize>
struct boost_save_impl<Archive, mp_integer<SSize>, mp_integer_boost_save_enabler<Archive>>
    : boost_save_via_boost_api<Archive, mp_integer<SSize>> {
};

/// Specialisation of piranha::boost_load() for piranha::mp_integer.
/**
 * \note
 * This specialisation is enabled only if \p std::string and all the integral types in terms of which
 * piranha::mp_integer is implemented satisfy piranha::has_boost_load.
 *
 * If \p Archive is \p boost::archive::binary_iarchive, this specialisation offers the basic exception guarantee
 * and performs minimal checking of the input data.
 *
 * @throws std::invalid_argument if the serialized integer is static and its number of limbs is greater than \p SSize.
 * @throws std::overflow_error if the number of limbs is larger than an implementation-defined value.
 * @throws unspecified any exception thrown by piranha::boost_load(), or by the constructor of
 * piranha::mp_integer from string.
 */
template <typename Archive, std::size_t SSize>
struct boost_load_impl<Archive, mp_integer<SSize>, mp_integer_boost_load_enabler<Archive>>
    : boost_load_via_boost_api<Archive, mp_integer<SSize>> {
};

#if defined(PIRANHA_WITH_MSGPACK)

inline namespace impl
{

// Enablers for msgpack serialization.
template <typename Stream>
using mp_integer_msgpack_pack_enabler = enable_if_t<
    conjunction<is_msgpack_stream<Stream>, has_msgpack_pack<Stream, bool>, has_msgpack_pack<Stream, ::mp_limb_t>,
                has_safe_cast<std::uint32_t, mpz_size_t>, has_msgpack_pack<Stream, std::string>>::value>;

template <typename T>
using mp_integer_msgpack_convert_enabler
    = enable_if_t<conjunction<is_mp_integer<T>, has_msgpack_convert<bool>, has_msgpack_convert<::mp_limb_t>,
                              has_msgpack_convert<std::string>,
                              has_safe_cast<mpz_size_t, typename std::vector<msgpack::object>::size_type>>::value>;
}

/// Specialisation of piranha::msgpack_pack() for piranha::mp_integer.
/**
 * \note
 * This specialisation is enabled only if:
 * - \p Stream satisfies piranha::is_msgpack_stream,
 * - \p std::string and all the integral types used in the representation of a piranha::mp_integer support
 *   piranha::has_msgpack_pack,
 * - the integral type representing the size of the integer can be safely cast to \p std::uint32_t.
 */
template <typename Stream, std::size_t SSize>
struct msgpack_pack_impl<Stream, mp_integer<SSize>, mp_integer_msgpack_pack_enabler<Stream>> {
    /// Call operator.
    /**
     * If the serialization format \p f is msgpack_format::portable, then
     * a decimal string representation of the integer is packed. Otherwise, an array of 3 elements
     * is packed: the first element is a boolean representing whether the integer is stored in static
     * storage or not, the second element is a boolean representing the sign of the integer (\p true
     * for positive or zero, \p false for negative) and the last element is an array of limbs.
     *
     * @param p target <tt>msgpack::packer</tt>.
     * @param n the input piranha::mp_integer.
     * @param f the desired piranha::msgpack_format.
     *
     * @throws std::overflow_error if the number of limbs is larger than an implementation-defined value.
     * @throws unspecified any exception thrown by:
     * - the public interface of \p msgpack::packer,
     * - memory errors in standard containers,
     * - piranha::msgpack_pack(),
     * - the conversion of \p n to string.
     */
    void operator()(msgpack::packer<Stream> &p, const mp_integer<SSize> &n, msgpack_format f) const
    {
        if (f == msgpack_format::binary) {
            const auto &int_u = n._get_union();
            // Regardless of storage type, we always pack an array of 3 elements:
            // - staticness,
            // - sign of size,
            // - array of limbs.
            p.pack_array(3);
            if (n.is_static()) {
                const auto size = int_u.g_st()._mp_size;
                // No problems with the static casting here, abs(size) is guaranteed to be small.
                const auto usize = static_cast<std::uint32_t>(size >= 0 ? size : -size);
                piranha::msgpack_pack(p, true, f);
                // Store whether the size is positive (true) or negative (false).
                piranha::msgpack_pack(p, size >= 0, f);
                p.pack_array(usize);
                std::for_each(int_u.g_st().m_limbs.data(), int_u.g_st().m_limbs.data() + usize,
                              [&p, f](const ::mp_limb_t &l) { piranha::msgpack_pack(p, l, f); });
            } else {
                const auto size = int_u.g_dy()._mp_size;
                std::uint32_t usize;
                try {
                    usize = safe_cast<std::uint32_t>(mp_integer_safe_abs_size(size));
                } catch (...) {
                    piranha_throw(std::overflow_error, "the number of limbs is too large");
                }
                piranha::msgpack_pack(p, false, f);
                piranha::msgpack_pack(p, size >= 0, f);
                p.pack_array(usize);
                std::for_each(int_u.g_dy()._mp_d, int_u.g_dy()._mp_d + usize,
                              [&p, f](const ::mp_limb_t &l) { piranha::msgpack_pack(p, l, f); });
            }
        } else {
            // NOTE: here we have an unnecessary copy (but at least we are avoiding memory allocations).
            // Maybe we should consider providing an API from mp++ to interact directly with strings
            // rather than vectors of chars.
            PIRANHA_MAYBE_TLS std::vector<char> tmp_v;
            mppp::mppp_impl::mpz_to_str(tmp_v, n.get_mpz_view());
            PIRANHA_MAYBE_TLS std::string tmp_s;
            tmp_s.assign(tmp_v.data());
            piranha::msgpack_pack(p, tmp_s, f);
        }
    }
};

/// Specialisation of piranha::msgpack_convert() for piranha::mp_integer.
/**
 * \note
 * This specialisation is enabled if:
 * - \p T is an instance of piranha::mp_integer,
 * - the integral types used in the internal representation of the integer, \p bool
 *   and \p std::string satisfy piranha::has_msgpack_convert,
 * - the size type of \p std::vector<msgpack::object> can be safely converted to the integral type representing the
 *   size of the integer.
 */
// NOTE: we need to keep the generic T param here, instead of mp_integer<SSize>, in order to trigger
// SFINAE in the enabler.
template <typename T>
struct msgpack_convert_impl<T, mp_integer_msgpack_convert_enabler<T>> {
    /// Call operator.
    /**
     * This method will convert the object \p o into \p n. If \p f is piranha::msgpack_format::binary,
     * this method offers the basic exception safety guarantee and it performs minimal checking on the input data.
     * Calling this method in binary mode will result in undefined behaviour if \p o does not contain an integer
     * serialized via msgpack_pack().
     *
     * @param n the destination piranha::mp_integer.
     * @param o source object.
     * @param f the desired piranha::msgpack_format.
     *
     * @throws std::invalid_argument if, in binary mode, the serialized static integer has a number of limbs
     * greater than its static size.
     * @throws std::overflow_error if the number of limbs is larger than an implementation-defined value.
     * @throws unspecified any exception thrown by:
     * - memory errors in standard containers,
     * - the public interface of <tt>msgpack::object</tt>,
     * - piranha::msgpack_convert(),
     * - the constructor of piranha::mp_integer from string.
     */
    void operator()(T &n, const msgpack::object &o, msgpack_format f) const
    {
        if (f == msgpack_format::binary) {
            PIRANHA_MAYBE_TLS std::array<msgpack::object, 3> vobj;
            o.convert(vobj);
            // Get the staticness of the serialized object.
            bool s;
            piranha::msgpack_convert(s, vobj[0u], f);
            // Bring n into the same staticness as the serialized object.
            const auto n_s = n.is_static();
            if (s != n_s) {
                if (n_s) {
                    const bool pstatus = n.promote();
                    (void)pstatus;
                    assert(pstatus);
                } else {
                    n = T{};
                }
            }
            auto &int_u = n._get_union();
            // Get the size sign.
            bool size_sign;
            piranha::msgpack_convert(size_sign, vobj[1u], f);
            PIRANHA_MAYBE_TLS std::vector<msgpack::object> vlimbs;
            // Get the limbs.
            vobj[2u].convert(vlimbs);
            const auto size = vlimbs.size();
            // We will need to iterate over the vector of limbs in both cases.
            auto it = vlimbs.begin();
            if (s) {
                if (unlikely(size > std::size_t(int_u.g_st().s_size))) {
                    piranha_throw(std::invalid_argument,
                                  "cannot deserialize a static integer with " + std::to_string(vlimbs.size())
                                      + " limbs, the static size is " + std::to_string(int_u.g_st().s_size));
                }
                try {
                    // Fill in the limbs.
                    auto data = int_u.g_st().m_limbs.data();
                    // NOTE: for_each is guaranteed to proceed in order, so we are sure that it and l are consistent.
                    std::for_each(data, data + size, [f, &it](::mp_limb_t &l) {
                        piranha::msgpack_convert(l, *it, f);
                        ++it;
                    });
                    // Zero the limbs that were not loaded from the archive.
                    std::fill(data + size, data + int_u.g_st().s_size, 0u);
                    // Set the size, with sign.
                    int_u.g_st()._mp_size = static_cast<mpz_size_t>(size_sign ? static_cast<mpz_size_t>(size)
                                                                              : -static_cast<mpz_size_t>(size));
                } catch (...) {
                    // Reset the static before re-throwing.
                    int_u.g_st()._mp_size = 0;
                    std::fill(int_u.g_st().m_limbs.begin(), int_u.g_st().m_limbs.end(), 0u);
                    throw;
                }
            } else {
                mpz_size_t sz;
                try {
                    sz = safe_cast<mpz_size_t>(size);
                    // We need to be able to negate n.
                    if (unlikely(sz > detail::safe_abs_sint<mpz_size_t>::value)) {
                        throw std::overflow_error("");
                    }
                } catch (...) {
                    piranha_throw(std::overflow_error, "the number of limbs is too large");
                }
                ::_mpz_realloc(&int_u.g_dy(), sz);
                try {
                    std::for_each(int_u.g_dy()._mp_d, int_u.g_dy()._mp_d + sz, [f, &it](::mp_limb_t &l) {
                        piranha::msgpack_convert(l, *it, f);
                        ++it;
                    });
                    int_u.g_dy()._mp_size = static_cast<mpz_size_t>(size_sign ? sz : -sz);
                } catch (...) {
                    ::mpz_set_ui(&int_u.g_dy(), 0u);
                    throw;
                }
            }
        } else {
            PIRANHA_MAYBE_TLS std::string tmp;
            piranha::msgpack_convert(tmp, o, f);
            n = T{tmp};
        }
    }
};

#endif

inline namespace impl
{

// Enabler for safe_cast specialisation.
template <typename To, typename From>
using mp_integer_safe_cast_enabler = enable_if_t<
    disjunction<conjunction<is_mp_integer<To>, std::is_floating_point<From>, std::is_constructible<To, From>>,
                conjunction<is_mp_integer<To>, std::is_integral<From>, std::is_constructible<To, From>>,
                conjunction<is_mp_integer<From>, std::is_integral<To>, std::is_constructible<To, From>>>::value>;
}

/// Specialisation of piranha::safe_cast() for conversions involving piranha::mp_integer.
/**
 * \note
 * This specialisation is enabled in the following cases:
 * - \p To is an instance of piranha::mp_integer and \p From is a C++ floating-point type from which piranha::mp_integer
 *   can be constructed,
 * - \p To is an instance of piranha::mp_integer and \p From is a C++ integral type from which piranha::mp_integer can
 *   be constructed,
 * - \p From is an instance of piranha::mp_integer and \p To is a C++ integral type constructible from
 *   piranha::mp_integer.
 */
template <typename To, typename From>
struct safe_cast_impl<To, From, mp_integer_safe_cast_enabler<To, From>> {
private:
    template <typename T>
    using float_enabler = enable_if_t<std::is_floating_point<T>::value, int>;
    template <typename T>
    using mp_int_enabler = enable_if_t<is_mp_integer<T>::value, int>;
    template <typename T>
    using int_enabler = enable_if_t<std::is_integral<T>::value, int>;
    template <typename T, float_enabler<T> = 0>
    static To impl(const T &f)
    {
        if (unlikely(!std::isfinite(f))) {
            piranha_throw(safe_cast_failure, "the non-finite floating-point value " + std::to_string(f)
                                                 + " cannot be converted to an arbitrary-precision integer");
        }
        if (unlikely(f != std::trunc(f))) {
            piranha_throw(safe_cast_failure, "the floating-point value with nonzero fractional part "
                                                 + std::to_string(f)
                                                 + " cannot be converted to an arbitrary-precision integer");
        }
        return To{f};
    }
    template <typename T, mp_int_enabler<T> = 0>
    static To impl(const T &n)
    {
        try {
            return To(n);
        } catch (const std::overflow_error &) {
            piranha_throw(safe_cast_failure, "the arbitrary-precision integer " + n.to_string()
                                                 + " cannot be converted to the type '" + detail::demangle<To>()
                                                 + "', as the conversion cannot preserve the original value");
        }
    }
    template <typename T, int_enabler<T> = 0>
    static To impl(const T &n)
    {
        return To{n};
    }

public:
    /// Call operator.
    /**
     * @param x input value to be converted.
     *
     * @return \p x converted to \p To.
     *
     * @throws piranha::safe_cast_failure if:
     * - the input float is not finite or if it has a nonzero fractional part,
     * - the target integral type \p To cannot represent the value of the input piranha::mp_integer.
     * @throws unspecified any exception thrown by the constructor of piranha::mp_integer from a floating-point.
     */
    To operator()(const From &x) const
    {
        return impl(x);
    }
};
}

#endif

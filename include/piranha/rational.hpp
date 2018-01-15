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

#ifndef PIRANHA_RATIONAL_HPP
#define PIRANHA_RATIONAL_HPP

// #include <array>
// #include <boost/functional/hash.hpp>
// #include <boost/lexical_cast.hpp>
// #include <climits>
// #include <cmath>
#include <cstddef>
// #include <cstring>
// #include <functional>
#include <iostream>
// #include <limits>
#include <stdexcept>
#include <string>
// #include <type_traits>
// #include <unordered_map>
#include <utility>

#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#include <piranha/binomial.hpp>
#include <piranha/config.hpp>
// #include <piranha/detail/demangle.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/math.hpp>
#include <piranha/pow.hpp>
#include <piranha/print_tex_coefficient.hpp>
// #include <piranha/s11n.hpp>
// #include <piranha/safe_cast.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

/// Multiprecision rational type.
using rational = mppp::rational<1>;

inline namespace literals
{

/// Literal for arbitrary-precision rationals.
/**
 * @param s a literal string.
 *
 * @return a piranha::rational constructed from \p s.
 *
 * @throws unspecified any exception thrown by the constructor of
 * piranha::rational from string.
 */
inline rational operator"" _q(const char *s)
{
    return rational{s};
}
}

/// Specialisation of the piranha::print_tex_coefficient() functor for mp++'s rationals.
template <std::size_t SSize>
struct print_tex_coefficient_impl<mppp::rational<SSize>> {
    /// Call operator.
    /**
     * @param os the target stream.
     * @param cf the coefficient to be printed.
     *
     * @throws unspecified any exception thrown by streaming mp++'s rationals to \p os.
     */
    void operator()(std::ostream &os, const mppp::rational<SSize> &cf) const
    {
        if (cf.is_zero()) {
            os << "0";
            return;
        }
        if (cf.get_den().is_one()) {
            os << cf.get_num();
            return;
        }
        PIRANHA_MAYBE_TLS mppp::integer<SSize> num;
        num = cf.get_num();
        if (num.sgn() < 0) {
            os << "-";
            num.neg();
        }
        os << "\\frac{" << num << "}{" << cf.get_den() << "}";
    }
};

namespace math
{

/// Specialisation of the implementation of piranha::math::is_zero() for mp++'s rationals.
template <std::size_t SSize>
struct is_zero_impl<mppp::rational<SSize>> {
    /// Call operator.
    /**
     * @param q the rational to be tested.
     *
     * @return \p true if \p q is zero, \p false otherwise.
     */
    bool operator()(const mppp::rational<SSize> &q) const
    {
        return q.is_zero();
    }
};

/// Specialisation of the implementation of piranha::math::is_unitary() for mp++'s rationals.
template <std::size_t SSize>
struct is_unitary_impl<mppp::rational<SSize>> {
    /// Call operator.
    /**
     * @param q the rational to be tested.
     *
     * @return \p true if \p q is equal to one, \p false otherwise.
     */
    bool operator()(const mppp::rational<SSize> &q) const
    {
        return q.is_one();
    }
};

/// Specialisation of the implementation of piranha::math::negate() for mp++'s rationals.
template <std::size_t SSize>
struct negate_impl<mppp::rational<SSize>> {
    /// Call operator.
    /**
     * @param q the rational to be negated.
     */
    void operator()(mppp::rational<SSize> &q) const
    {
        q.negate();
    }
};

inline namespace impl
{

// Enabler for the pow specialisation.
template <typename T, typename U>
using math_rational_pow_enabler = enable_if_t<is_detected<mppp::rational_common_t, T, U>::value>;
}

/// Specialisation of the implementation of piranha::math::pow() for mp++'s rationals.
/**
 * This specialisation is activated when at least one of the arguments is an mp++ rational,
 * and the mp++ exponentiation function can be successfully called on instances of ``T`` and ``U``.
 */
template <typename T, typename U>
struct pow_impl<T, U, math_rational_pow_enabler<T, U>> {
    /// Call operator.
    /**
     * @param b the base.
     * @param e the exponent.
     *
     * @returns <tt>b**e</tt>.
     *
     * @throws unspecified any exception thrown by the mp++ exponentiation function.
     */
    auto operator()(const T &b, const U &e) const -> decltype(mppp::pow(b, e))
    {
        return mppp::pow(b, e);
    }
};

/// Specialisation of the implementation of piranha::math::sin() for mp++'s rationals.
template <std::size_t SSize>
struct sin_impl<mppp::rational<SSize>> {
    /// Call operator.
    /**
     * @param q the argument.
     *
     * @return the sine of \p q.
     *
     * @throws std::invalid_argument if the argument is not zero.
     */
    mppp::rational<SSize> operator()(const mppp::rational<SSize> &q) const
    {
        if (q.is_zero()) {
            return mppp::rational<SSize>{};
        }
        piranha_throw(std::invalid_argument, "cannot compute the sine of a non-zero rational");
    }
};

/// Specialisation of the implementation of piranha::math::cos() for mp++'s rationals.
template <std::size_t SSize>
struct cos_impl<mppp::rational<SSize>> {
    /// Call operator.
    /**
     * @param q the argument.
     *
     * @return the cosine of \p q.
     *
     * @throws std::invalid_argument if the argument is not zero.
     */
    mppp::rational<SSize> operator()(const mppp::rational<SSize> &q) const
    {
        if (q.is_zero()) {
            return mppp::rational<SSize>{1};
        }
        piranha_throw(std::invalid_argument, "cannot compute the cosine of a non-zero rational");
    }
};

/// Specialisation of the implementation of piranha::math::abs() for mp++'s rationals.
template <std::size_t SSize>
struct abs_impl<mppp::rational<SSize>> {
    /// Call operator.
    /**
     * @param q the input parameter.
     *
     * @return the absolute value of \p q.
     */
    mppp::rational<SSize> operator()(const mppp::rational<SSize> &q) const
    {
        return mppp::abs(q);
    }
};

/// Specialisation of the implementation of piranha::math::partial() for mp++'s rationals.
template <std::size_t SSize>
struct partial_impl<mppp::rational<SSize>> {
    /// Call operator.
    /**
     * @return a rational constructed from zero.
     */
    mppp::rational<SSize> operator()(const mppp::rational<SSize> &, const std::string &) const
    {
        return mppp::rational<SSize>{};
    }
};

inline namespace impl
{

// Binomial follows the same rules as pow.
template <typename T, typename U>
using math_rational_binomial_enabler = math_rational_pow_enabler<T, U>;
}

/// Specialisation of the implementation of piranha::math::binomial() for mp++'s rationals.
/**
 * This specialisation is activated when at least one of the arguments is an mp++ rational,
 * and the mp++ exponentiation function can be successfully called on instances of ``T`` and ``U``.
 *
 * The implementation follows these rules:
 * - if the top is rational and the bottom an integral type or piranha::mp_integer, then
 *   piranha::mp_rational::binomial() is used;
 * - if the non-rational argument is a floating-point type, then the rational argument is converted
 *   to that floating-point type and piranha::math::binomial() is used;
 * - if both arguments are rational, they are both converted to \p double and then piranha::math::binomial()
 *   is used;
 * - if the top is an integral type or piranha::mp_integer and the bottom a rational, then both
 *   arguments are converted to \p double and piranha::math::binomial() is used.
 */
template <typename T, typename U>
struct binomial_impl<T, U, math_rational_binomial_enabler<T, U>> {
private:
    template <std::size_t SSize, typename T2>
    static auto impl(const mp_rational<SSize> &x, const T2 &y) -> decltype(x.binomial(y))
    {
        return x.binomial(y);
    }
    template <std::size_t SSize, typename T2, enable_if_t<std::is_floating_point<T2>::value, int> = 0>
    static T2 impl(const mp_rational<SSize> &x, const T2 &y)
    {
        return math::binomial(static_cast<T2>(x), y);
    }
    template <std::size_t SSize, typename T2, enable_if_t<std::is_floating_point<T2>::value, int> = 0>
    static T2 impl(const T2 &x, const mp_rational<SSize> &y)
    {
        return math::binomial(x, static_cast<T2>(y));
    }
    template <std::size_t SSize>
    static double impl(const mp_rational<SSize> &x, const mp_rational<SSize> &y)
    {
        return math::binomial(static_cast<double>(x), static_cast<double>(y));
    }
    template <std::size_t SSize, typename T2,
              enable_if_t<disjunction<std::is_integral<T2>, is_mp_integer<T2>>::value, int> = 0>
    static double impl(const T2 &x, const mp_rational<SSize> &y)
    {
        return math::binomial(static_cast<double>(x), static_cast<double>(y));
    }
    using ret_type = decltype(impl(std::declval<const T &>(), std::declval<const U &>()));

public:
    /// Call operator.
    /**
     * @param x top argument.
     * @param y bottom argument.
     *
     * @returns \f$ x \choose y \f$.
     *
     * @throws unspecified any exception thrown by:
     * - piranha::mp_rational::binomial(),
     * - converting piranha::mp_rational or piranha::mp_integer to a floating-point type.
     */
    ret_type operator()(const T &x, const U &y) const
    {
        return impl(x, y);
    }
};
}

inline namespace impl
{

template <typename To, typename From>
using sc_rat_enabler = enable_if_t<
    disjunction<conjunction<is_mp_rational<To>, disjunction<std::is_arithmetic<From>, is_mp_integer<From>>>,
                conjunction<is_mp_rational<From>, disjunction<std::is_integral<To>, is_mp_integer<To>>>>::value>;
}

/// Specialisation of piranha::safe_cast() for conversions involving piranha::mp_rational.
/**
 * \note
 * This specialisation is enabled in the following cases:
 * - \p To is a rational type and \p From is either an arithmetic type or piranha::mp_integer,
 * - \p To is an integral type or piranha::mp_integer, and \p From is piranha::mp_rational.
 */
template <typename To, typename From>
struct safe_cast_impl<To, From, sc_rat_enabler<To, From>> {
private:
    template <typename T, enable_if_t<disjunction<std::is_arithmetic<T>, is_mp_integer<T>>::value, int> = 0>
    static To impl(const T &x)
    {
        try {
            // NOTE: checks for finiteness of an fp value are in the ctor.
            return To(x);
        } catch (const std::invalid_argument &) {
            piranha_throw(safe_cast_failure, "cannot convert value " + boost::lexical_cast<std::string>(x)
                                                 + " of type '" + detail::demangle<T>()
                                                 + "' to a rational, as the conversion would not preserve the value");
        }
    }
    template <typename T, enable_if_t<is_mp_rational<T>::value, int> = 0>
    static To impl(const T &q)
    {
        if (unlikely(!q.den().is_one())) {
            piranha_throw(safe_cast_failure, "cannot convert the rational value " + boost::lexical_cast<std::string>(q)
                                                 + " to the integral type '" + detail::demangle<To>()
                                                 + "', as the rational value as non-unitary denominator");
        }
        try {
            return static_cast<To>(q);
        } catch (const std::overflow_error &) {
            piranha_throw(safe_cast_failure, "cannot convert the rational value " + boost::lexical_cast<std::string>(q)
                                                 + " to the integral type '" + detail::demangle<To>()
                                                 + "', as the conversion cannot preserve the value");
        }
    }

public:
    /// Call operator.
    /**
     * The conversion is performed via piranha::mp_rational's constructor and conversion operator.
     *
     * @param x input value.
     *
     * @return \p x converted to \p To.
     *
     * @throws piranha::safe_cast_failure if the conversion fails.
     */
    To operator()(const From &x) const
    {
        return impl(x);
    }
};

inline namespace impl
{

template <typename Archive, std::size_t SSize>
using mp_rational_boost_save_enabler
    = enable_if_t<has_boost_save<Archive, typename mp_rational<SSize>::int_type>::value>;

template <typename Archive, std::size_t SSize>
using mp_rational_boost_load_enabler
    = enable_if_t<has_boost_load<Archive, typename mp_rational<SSize>::int_type>::value>;
}

/// Specialisation of piranha::boost_save() for piranha::mp_rational.
/**
 * \note
 * This specialisation is enabled only if the numerator/denominator type of piranha::mp_rational satisfies
 * piranha::has_boost_save.
 *
 * The rational will be serialized as a numerator/denominator pair.
 *
 * @throws unspecified any exception thrown by piranha::boost_save().
 */
template <typename Archive, std::size_t SSize>
struct boost_save_impl<Archive, mp_rational<SSize>, mp_rational_boost_save_enabler<Archive, SSize>>
    : boost_save_via_boost_api<Archive, mp_rational<SSize>> {
};

/// Specialisation of piranha::boost_load() for piranha::mp_rational.
/**
 * \note
 * This specialisation is enabled only if the numerator/denominator type of piranha::mp_rational satisfies
 * piranha::has_boost_load.
 *
 * If \p Archive is boost::archive::binary_iarchive, the serialized numerator/denominator pair is loaded
 * as-is, without canonicality checks. Otherwise, the rational will be canonicalised after deserialization.
 *
 * @throws unspecified any exception thrown by piranha::boost_load() or by the constructor of piranha::mp_rational
 * from numerator and denominator.
 */
template <typename Archive, std::size_t SSize>
struct boost_load_impl<Archive, mp_rational<SSize>, mp_rational_boost_load_enabler<Archive, SSize>>
    : boost_load_via_boost_api<Archive, mp_rational<SSize>> {
};

#if defined(PIRANHA_WITH_MSGPACK)

inline namespace impl
{

template <typename Stream, typename T>
using mp_rational_msgpack_pack_enabler
    = enable_if_t<conjunction<is_mp_rational<T>, is_detected<msgpack_pack_member_t, Stream, T>>::value>;

template <typename T>
using mp_rational_msgpack_convert_enabler
    = enable_if_t<conjunction<is_mp_rational<T>, is_detected<msgpack_convert_member_t, T>>::value>;
}

/// Specialisation of piranha::msgpack_pack() for piranha::mp_rational.
/**
 * \note
 * This specialisation is enabled only if \p T is an instance of piranha::mp_rational supporting the
 * piranha::mp_rational::msgpack_pack() method.
 */
template <typename Stream, typename T>
struct msgpack_pack_impl<Stream, T, mp_rational_msgpack_pack_enabler<Stream, T>> {
    /// Call operator.
    /**
     * The call operator will use the piranha::mp_rational::msgpack_pack() method of \p q.
     *
     * @param p the source <tt>msgpack::packer</tt>.
     * @param q the input rational.
     * @param f the desired piranha::msgpack_format.
     *
     * @throws unspecified any exception thrown by piranha::mp_rational::msgpack_pack().
     */
    void operator()(msgpack::packer<Stream> &p, const T &q, msgpack_format f) const
    {
        q.msgpack_pack(p, f);
    }
};

/// Specialisation of piranha::msgpack_convert() for piranha::mp_rational.
/**
 * \note
 * This specialisation is enabled only if \p T is an instance of piranha::mp_rational supporting the
 * piranha::mp_rational::msgpack_convert() method.
 */
template <typename T>
struct msgpack_convert_impl<T, mp_rational_msgpack_convert_enabler<T>> {
    /// Call operator.
    /**
     * The call operator will use the piranha::mp_rational::msgpack_convert() method of \p q.
     *
     * @param q the target rational.
     * @param o the source <tt>msgpack::object</tt>.
     * @param f the desired piranha::msgpack_format.
     *
     * @throws unspecified any exception thrown by piranha::mp_rational::msgpack_convert().
     */
    void operator()(T &q, const msgpack::object &o, msgpack_format f) const
    {
        q.msgpack_convert(o, f);
    }
};

#endif
}

namespace std
{

/// Specialisation of \p std::hash for piranha::mp_rational.
template <std::size_t SSize>
struct hash<piranha::mp_rational<SSize>> {
    /// Result type.
    typedef size_t result_type;
    /// Argument type.
    typedef piranha::mp_rational<SSize> argument_type;
    /// Hash operator.
    /**
     * @param q piranha::mp_rational whose hash value will be returned.
     *
     * @return <tt>q.hash()</tt>.
     *
     * @see piranha::mp_rational::hash()
     */
    result_type operator()(const argument_type &q) const
    {
        return q.hash();
    }
};
}

#endif

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

#include <cmath>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <mp++/concepts.hpp>
#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#include <piranha/config.hpp>
#include <piranha/detail/demangle.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/math.hpp>
#include <piranha/pow.hpp>
#include <piranha/print_tex_coefficient.hpp>
#include <piranha/s11n.hpp>
#include <piranha/safe_cast.hpp>
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
        q.neg();
    }
};

inline namespace impl
{

// Enabler for the pow specialisation.
template <typename T, typename U>
using math_rational_pow_enabler = enable_if_t<mppp::are_rational_op_types<T, U>::value>;
}

/// Specialisation of the implementation of piranha::math::pow() for mp++'s rationals.
/**
 * This specialisation is activated if the mp++ rational exponentiation function can be successfully called on instances
 * of ``T`` and ``U``.
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
}

inline namespace impl
{

// Conversion to rational is enabled from integral interoperable
// types for rational and C++ floats.
template <typename To, typename From>
struct sc_to_rational : std::false_type {
};

template <std::size_t SSize, typename From>
struct sc_to_rational<mppp::rational<SSize>, From> : disjunction<mppp::is_rational_integral_interoperable<From, SSize>,
                                                                 mppp::is_cpp_floating_point_interoperable<From>> {
};

// Conversion from rational is enabled if the destination is an integral interoperable
// type for rational.
template <typename To, typename From>
struct sc_from_rational : std::false_type {
};

template <typename To, std::size_t SSize>
struct sc_from_rational<To, mppp::rational<SSize>> : mppp::is_rational_integral_interoperable<To, SSize> {
};

template <typename To, typename From>
using sc_rat_enabler = enable_if_t<disjunction<sc_to_rational<To, From>, sc_from_rational<To, From>>::value>;
}

/// Specialisation of piranha::safe_cast() for conversions involving mp++'s rationals.
/**
 * \note
 * This specialisation is enabled in the following cases:
 * - \p To is a rational and \p From is an integral or floating-point interoperable type for \p To,
 * - \p From is a rational and \p To is is an integral interoperable type for \p From.
 */
template <typename To, typename From>
struct safe_cast_impl<To, From, sc_rat_enabler<To, From>> {
private:
    // Small local utility for string conversion.
    template <typename T>
    static std::string to_string(const T &x)
    {
        return std::to_string(x);
    }
    template <std::size_t SSize>
    static std::string to_string(const mppp::integer<SSize> &n)
    {
        return n.to_string();
    }
    template <std::size_t SSize>
    static std::string to_string(const mppp::rational<SSize> &q)
    {
        return q.to_string();
    }
    // Small local utility to check for finiteness.
    template <typename T>
    static bool isfinite(const T &x, const std::true_type &)
    {
        return std::isfinite(x);
    }
    template <typename T>
    static bool isfinite(const T &, const std::false_type &)
    {
        return true;
    }
    // Conversion to rational.
    template <typename T>
    static To impl(const T &x)
    {
        // NOTE: the only way this fails is if x is a non-finite floating-point.
        if (unlikely(!isfinite(x, std::is_floating_point<T>{}))) {
            piranha_throw(safe_cast_failure, "cannot convert the non-finite floating-point value " + to_string(x)
                                                 + " of type '" + demangle<T>() + "' to a rational");
        }
        return To{x};
    }
    // Conversion from rational.
    template <std::size_t SSize>
    static To impl(const mppp::rational<SSize> &q)
    {
        // Only conversion to C++ integral can fail, due to non-unitary denom of overflow error.
        if (unlikely(!q.get_den().is_one())) {
            piranha_throw(safe_cast_failure, "cannot convert the rational value " + to_string(q)
                                                 + " to the integral type '" + demangle<To>()
                                                 + "', as the rational value has a non-unitary denominator");
        }
        To retval;
        const bool status = mppp::get(retval, q);
        if (unlikely(!status)) {
            piranha_throw(safe_cast_failure, "cannot convert the rational value " + to_string(q)
                                                 + " to the integral type '" + demangle<To>()
                                                 + "', as the conversion would result in overflow");
        }
        return retval;
    }

public:
    /// Call operator.
    /**
     * The conversion is performed via the constructors and conversion operators of mp++'s rationals.
     *
     * Conversion from a rational can fail if the rational has a non-unitary denominator or if the
     * target C++ integral type has an insufficient range.
     *
     * Conversion to a rational can fail only if the floating-point argument is not finite.
     *
     * @param x the input value.
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
#if 0
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
#endif // If zero.
}

#endif

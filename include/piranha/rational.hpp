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

#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <mp++/concepts.hpp>
#include <mp++/exceptions.hpp>
#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#include <piranha/config.hpp>
#include <piranha/detail/demangle.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/math.hpp>
#include <piranha/math/binomial.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/print_tex_coefficient.hpp>
#include <piranha/s11n.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

// Main multiprecision rational type.
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

// Specialisation of the implementation of piranha::math::pow() for mp++'s rationals.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename U, mppp::RationalOpTypes<U> T>
class pow_impl<T, U>
#else
template <typename T, typename U>
class pow_impl<T, U, enable_if_t<mppp::are_rational_op_types<T, U>::value>>
#endif
{
public:
    template <typename T1, typename U1>
    auto operator()(T1 &&b, U1 &&e) const -> decltype(mppp::pow(std::forward<T1>(b), std::forward<U1>(e)))
    {
        return mppp::pow(std::forward<T1>(b), std::forward<U1>(e));
    }
};

// Specialisation of the implementation of piranha::math::binomial() for mp++ rational top argument.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <std::size_t SSize, mppp::RationalIntegralInteroperable<SSize> T>
class binomial_impl<mppp::rational<SSize>, T>
#else
template <std::size_t SSize, typename T>
class binomial_impl<mppp::rational<SSize>, T, enable_if_t<mppp::is_rational_integral_interoperable<T, SSize>::value>>
#endif
{
public:
    template <typename T1, typename U1>
    auto operator()(T1 &&x, U1 &&y) const -> decltype(mppp::binomial(std::forward<T1>(x), std::forward<U1>(y)))
    {
        return mppp::binomial(std::forward<T1>(x), std::forward<U1>(y));
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
    // NOTE: this needs to be here in order for the impl()
    // overloads below to compile, but it is never called
    // as conversion from integral to rational can never fail.
    // LCOV_EXCL_START
    template <std::size_t SSize>
    static std::string to_string(const mppp::integer<SSize> &)
    {
        piranha_assert(false);
    }
    // LCOV_EXCL_STOP
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
}

namespace boost
{
namespace serialization
{

template <typename Archive, std::size_t SSize>
inline void save(Archive &ar, const mppp::rational<SSize> &q, unsigned)
{
    // Just store numerator and denominator.
    piranha::boost_save(ar, q.get_num());
    piranha::boost_save(ar, q.get_den());
}

template <typename Archive, std::size_t SSize>
inline void load(Archive &ar, mppp::rational<SSize> &q, unsigned)
{
    try {
        piranha::boost_load(ar, q._get_num());
        piranha::boost_load(ar, q._get_den());
        // Run a zero check on the denominator. This is cheap,
        // so we do it always.
        if (unlikely(q.get_den().is_zero())) {
            piranha_throw(mppp::zero_division_error,
                          "a zero denominator was encountered during the deserialisation of a rational");
        }
        if (!std::is_same<Archive, boost::archive::binary_iarchive>::value) {
            // If the archive is not a binary archive, we want to make sure
            // that the loaded rational is canonical.
            q.canonicalise();
        }
    } catch (...) {
        // In case of any error, make sure we leave q in a sane
        // state before re-throwing.
        q._get_num().set_zero();
        q._get_den().set_one();
        throw;
    }
}

template <typename Archive, std::size_t SSize>
inline void serialize(Archive &ar, mppp::rational<SSize> &q, const unsigned int file_version)
{
    split_free(ar, q, file_version);
}
}
}

namespace piranha
{

inline namespace impl
{

template <typename Archive, std::size_t SSize>
using rational_boost_save_enabler = enable_if_t<has_boost_save<Archive, typename mppp::rational<SSize>::int_t>::value>;

template <typename Archive, std::size_t SSize>
using rational_boost_load_enabler = enable_if_t<has_boost_load<Archive, typename mppp::rational<SSize>::int_t>::value>;
}

/// Specialisation of piranha::boost_save() for mp++'s rationals.
/**
 * \note
 * This specialisation is enabled only if the numerator/denominator type of
 * the mp++ rational type satisfies piranha::has_boost_save.
 *
 * The rational will be serialized as a numerator/denominator pair.
 *
 * @throws unspecified any exception thrown by piranha::boost_save().
 */
template <typename Archive, std::size_t SSize>
struct boost_save_impl<Archive, mppp::rational<SSize>, rational_boost_save_enabler<Archive, SSize>>
    : boost_save_via_boost_api<Archive, mppp::rational<SSize>> {
};

/// Specialisation of piranha::boost_load() for mp++'s rationals.
/**
 * \note
 * This specialisation is enabled only if the numerator/denominator type of
 * the mp++ rational type satisfies piranha::has_boost_load.
 *
 * If \p Archive is ``boost::archive::binary_iarchive``, the serialized numerator/denominator pair is loaded
 * as-is, without canonicality checks. Otherwise, the rational will be canonicalised after deserialization.
 *
 * @throws mppp::zero_division_error if a zero denominator is detected.
 * @throws unspecified any exception thrown by piranha::boost_load().
 */
template <typename Archive, std::size_t SSize>
struct boost_load_impl<Archive, mppp::rational<SSize>, rational_boost_load_enabler<Archive, SSize>>
    : boost_load_via_boost_api<Archive, mppp::rational<SSize>> {
};

#if defined(PIRANHA_WITH_MSGPACK)

inline namespace impl
{

template <typename Stream, std::size_t SSize>
using rational_msgpack_pack_enabler = enable_if_t<
    conjunction<is_msgpack_stream<Stream>, has_msgpack_pack<Stream, typename mppp::rational<SSize>::int_t>>::value>;

template <std::size_t SSize>
using rational_msgpack_convert_enabler = enable_if_t<has_msgpack_convert<typename mppp::rational<SSize>::int_t>::value>;
}

/// Specialisation of piranha::msgpack_pack() for mp++'s rationals.
/**
 * \note
 * This specialisation is enabled only if \p Stream satisfies piranha::is_msgpack_stream and the type representing the
 * numerator and denominator of the rational satisfies piranha::has_msgpack_pack.
 */
template <typename Stream, std::size_t SSize>
struct msgpack_pack_impl<Stream, mppp::rational<SSize>, rational_msgpack_pack_enabler<Stream, SSize>> {
    /// Call operator.
    /**
     * This method will pack \p q into \p p as a numerator-denominator pair.
     *
     * @param p the target <tt>msgpack::packer</tt>.
     * @param q the input rational.
     * @param f the desired piranha::msgpack_format.
     *
     * @throws unspecified any exception thrown by:
     * - the public interface of <tt>msgpack::packer</tt>,
     * - piranha::msgpack_pack().
     */
    void operator()(msgpack::packer<Stream> &p, const mppp::rational<SSize> &q, msgpack_format f) const
    {
        p.pack_array(2u);
        piranha::msgpack_pack(p, q.get_num(), f);
        piranha::msgpack_pack(p, q.get_den(), f);
    }
};

/// Specialisation of piranha::msgpack_convert() for for mp++'s rationals.
/**
 * \note
 * This specialisation is enabled only if the type representing the
 * numerator and denominator of the rational satisfies piranha::has_msgpack_convert.
 */
template <std::size_t SSize>
struct msgpack_convert_impl<mppp::rational<SSize>, rational_msgpack_convert_enabler<SSize>> {
    /// Call operator.
    /**
     * This method will convert \p o into \p q. If \p f is piranha::msgpack_format::portable
     * this method will ensure that the deserialized rational is in canonical form,
     * otherwise no check will be performed.
     *
     * @param o the source <tt>msgpack::object</tt>.
     * @param q the output rational.
     * @param f the desired piranha::msgpack_format.
     *
     * @throws mppp::zero_division_error if a zero denominator is detected.
     * @throws unspecified any exception thrown by:
     * - the public interface of <tt>msgpack::object</tt>,
     * - piranha::msgpack_convert().
     */
    void operator()(mppp::rational<SSize> &q, const msgpack::object &o, msgpack_format f) const
    {
        PIRANHA_MAYBE_TLS std::array<msgpack::object, 2u> v;
        o.convert(v);
        try {
            piranha::msgpack_convert(q._get_num(), v[0], f);
            piranha::msgpack_convert(q._get_den(), v[1], f);
            // Always run the cheap zero den detection.
            if (unlikely(q.get_den().is_zero())) {
                piranha_throw(mppp::zero_division_error,
                              "a zero denominator was encountered during the deserialisation of a rational");
            }
            if (f != msgpack_format::binary) {
                // If the serialisation format is not binary, we want to make sure
                // that the loaded rational is canonical.
                q.canonicalise();
            }
        } catch (...) {
            // In case of any error, make sure we leave q in a sane
            // state before re-throwing.
            q._get_num().set_zero();
            q._get_den().set_one();
            throw;
        }
    }
};

#endif
}

#endif

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
#include <piranha/math/cos.hpp>
#include <piranha/math/is_one.hpp>
#include <piranha/math/is_zero.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/math/sin.hpp>
#include <piranha/print_tex_coefficient.hpp>
#include <piranha/s11n.hpp>
#include <piranha/safe_convert.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

// Main multiprecision rational type.
using rational = mppp::rational<1>;

inline namespace literals
{

// Literal for arbitrary-precision rationals.
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

// Specialisation of the implementation of piranha::is_zero() for mp++'s rationals.
template <std::size_t SSize>
class is_zero_impl<mppp::rational<SSize>>
{
public:
    bool operator()(const mppp::rational<SSize> &q) const
    {
        return q.is_zero();
    }
};

// Specialisation of the implementation of piranha::is_one() for mp++'s rationals.
template <std::size_t SSize>
class is_one_impl<mppp::rational<SSize>>
{
public:
    bool operator()(const mppp::rational<SSize> &q) const
    {
        return q.is_one();
    }
};

namespace math
{

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
}

// Specialisation of the implementation of piranha::pow() for mp++'s rationals.
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

// Specialisation of the implementation of piranha::binomial() for mp++ rational top argument.
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

template <std::size_t SSize>
class sin_impl<mppp::rational<SSize>>
{
public:
    mppp::rational<SSize> operator()(const mppp::rational<SSize> &q) const
    {
        if (unlikely(!q.is_zero())) {
            piranha_throw(std::domain_error, "cannot compute the sine of the non-zero rational " + q.to_string());
        }
        return mppp::rational<SSize>{};
    }
};

template <std::size_t SSize>
class cos_impl<mppp::rational<SSize>>
{
public:
    mppp::rational<SSize> operator()(const mppp::rational<SSize> &q) const
    {
        if (unlikely(!q.is_zero())) {
            piranha_throw(std::domain_error, "cannot compute the cosine of the non-zero rational " + q.to_string());
        }
        return mppp::rational<SSize>{1};
    }
};

namespace math
{

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

// From rational integral interop to rational.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <std::size_t SSize, mppp::RationalIntegralInteroperable<SSize> From>
class safe_convert_impl<mppp::rational<SSize>, From>
#else
template <std::size_t SSize, typename From>
class safe_convert_impl<mppp::rational<SSize>, From,
                        enable_if_t<mppp::is_rational_integral_interoperable<From, SSize>::value>>
#endif
{
public:
    bool operator()(mppp::rational<SSize> &out, const From &n) const
    {
        out = n;
        return true;
    }
};

// From C++ FP to rational.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <std::size_t SSize, mppp::CppFloatingPointInteroperable From>
class safe_convert_impl<mppp::rational<SSize>, From>
#else
template <std::size_t SSize, typename From>
class safe_convert_impl<mppp::rational<SSize>, From,
                        enable_if_t<mppp::is_cpp_floating_point_interoperable<From>::value>>
#endif
{
public:
    bool operator()(mppp::rational<SSize> &out, From x) const
    {
        if (!std::isfinite(x)) {
            return false;
        }
        out = x;
        return true;
    }
};

// From rational to rational integral interop.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <std::size_t SSize, mppp::RationalIntegralInteroperable<SSize> To>
class safe_convert_impl<To, mppp::rational<SSize>>
#else
template <std::size_t SSize, typename To>
class safe_convert_impl<To, mppp::rational<SSize>,
                        enable_if_t<mppp::is_rational_integral_interoperable<To, SSize>::value>>
#endif
{
public:
    bool operator()(To &n, const mppp::rational<SSize> &q) const
    {
        return q.get_den().is_one() ? q.get(n) : false;
    }
};
}

#if defined(PIRANHA_WITH_BOOST_S11N)

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

#endif

namespace piranha
{

#if defined(PIRANHA_WITH_BOOST_S11N)

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

#endif

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

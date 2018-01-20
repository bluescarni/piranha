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

#ifndef PIRANHA_REAL_HPP
#define PIRANHA_REAL_HPP

#include <mp++/config.hpp>

#if defined(MPPP_WITH_MPFR)

#include <string>
#include <type_traits>
#include <utility>

#include <mp++/concepts.hpp>
#include <mp++/detail/mpfr.hpp>
#include <mp++/integer.hpp>
#include <mp++/rational.hpp>
#include <mp++/real.hpp>

#include <piranha/config.hpp>
#include <piranha/detail/demangle.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/math.hpp>
#include <piranha/pow.hpp>
#include <piranha/s11n.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

/// Multiprecision floating-point type.
using real = mppp::real;

namespace math
{

/// Specialisation of piranha::math::negate() for piranha::real.
template <>
struct negate_impl<real> {
    /// Call operator.
    /**
     * @param r the piranha::real to be negated.
     */
    void operator()(real &r) const
    {
        r.neg();
    }
};

/// Specialisation of piranha::math::is_zero() for piranha::real.
template <>
struct is_zero_impl<real> {
    /// Call operator.
    /**
     * @param r the piranha::real to be tested.
     *
     * @return \p true if \p r is zero, \p false otherwise.
     */
    bool operator()(const real &r) const
    {
        return r.zero_p();
    }
};

inline namespace impl
{

// Enabler for real pow.
template <typename T, typename U>
using math_real_pow_enabler = enable_if_t<mppp::are_real_op_types<T, U>::value>;
}

/// Specialisation of piranha::math::pow() for piranha::real.
/**
 * This specialisation is activated when one of the arguments is piranha::real
 * and the other is either piranha::real or an interoperable type for piranha::real.
 * The result is computed via mp++'s exponentiation function for piranha::real.
 */
template <typename T, typename U>
struct pow_impl<T, U, math_real_pow_enabler<T, U>> {
    /// Call operator.
    /**
     * @param base the base.
     * @param exp the exponent.
     *
     * @return ``base**exp``.
     *
     * @throws unspecified any exception thrown by the invoked mp++ exponentiation function.
     */
    template <typename T1, typename U1>
    auto operator()(T1 &&base, U1 &&exp) const -> decltype(mppp::pow(std::forward<T1>(base), std::forward<U1>(exp)))
    {
        return mppp::pow(std::forward<T1>(base), std::forward<U1>(exp));
    }
};

/// Specialisation of piranha::math::sin() for piranha::real.
template <>
struct sin_impl<real> {
    /// Call operator.
    /**
     * @param r the piranha::real argument.
     *
     * @return the sine of \p r.
     */
    template <typename T>
    real operator()(T &&r) const
    {
        return mppp::sin(std::forward<T>(r));
    }
};

/// Specialisation of piranha::math::cos() for piranha::real.
template <>
struct cos_impl<real> {
    /// Call operator.
    /**
     * @param r the piranha::real argument.
     *
     * @return the cosine of \p r.
     */
    template <typename T>
    real operator()(T &&r) const
    {
        return mppp::cos(std::forward<T>(r));
    }
};

/// Specialisation of piranha::math::abs() for piranha::real.
template <>
struct abs_impl<real> {
    /// Call operator.
    /**
     * @param r the piranha::real argument.
     *
     * @return the absolute value of \p r.
     */
    template <typename T>
    real operator()(T &&r) const
    {
        return mppp::abs(std::forward<T>(r));
    }
};

/// Specialisation of piranha::math::partial() for piranha::real.
template <>
struct partial_impl<real> {
    /// Call operator.
    /**
     * @return an instance of piranha::real constructed from zero.
     */
    real operator()(const real &, const std::string &) const
    {
        return real{};
    }
};

/// Specialisation of piranha::math::multiply_accumulate() for piranha::real.
template <>
struct multiply_accumulate_impl<real> {
    /// Call operator.
    /**
     * @param x the target value for the accumulation.
     * @param y the first argument.
     * @param z the second argument.
     */
    void operator()(real &x, const real &y, const real &z) const
    {
    // So the story here is that mpfr's fma() has been reported to be slower than the two separate
    // operations:
    // http://www.loria.fr/~zimmerma/mpfr-mpc-2014.html
    // Benchmarks on fateman1 indicate this is indeed the case. This may be fixed in MPFR 4, but I haven't actually
    // checked yet. For versions earlier than 4, and if we have thread local storage, we avoid the slowdown by using two
    // separate operations rather than the fma().
    // NOTE: it's not clear if here we could benefit from perfect forwarding, as this
    // operation already comes with a destination variable. We'll need to investigate.
#if MPFR_VERSION_MAJOR < 4 && defined(PIRANHA_HAVE_THREAD_LOCAL)
        static thread_local real tmp;
        mppp::mul(tmp, y, z);
        mppp::add(x, x, tmp);
#else
        mppp::fma(x, y, z, x);
#endif
    }
};
}

inline namespace impl
{

template <typename To>
using sc_real_enabler = enable_if_t<
    disjunction<mppp::is_cpp_integral_interoperable<To>, mppp::is_integer<To>, mppp::is_rational<To>>::value>;
}

/// Specialisation of piranha::safe_cast() for conversions involving piranha::real.
/**
 * \note
 * This specialisation is enabled if \p To is an integral type, an mp++ integer or an mp++ rational.
 */
template <typename To>
struct safe_cast_impl<To, real, sc_real_enabler<To>> {
private:
    // The integral conversion overload.
    static To impl(const real &r, const std::true_type &)
    {
        if (unlikely(!r.number_p() || !r.integer_p())) {
            // For conversions to integrals, r must represent a finite and integral value.
            piranha_throw(safe_cast_failure, "cannot convert the real value " + r.to_string()
                                                 + " to the integral type '" + demangle<To>()
                                                 + "', as the real does not represent a finite integral value");
        }
        To retval;
        const bool status = mppp::get(retval, r);
        if (unlikely(!status)) {
            // NOTE: for integral conversions, the only possible failure is if the value
            // overflows the target C++ integral type's range.
            piranha_throw(safe_cast_failure, "cannot convert the real value " + r.to_string()
                                                 + " to the integral type '" + demangle<To>()
                                                 + "', as the conversion would result in overflow");
        }
        return retval;
    }
    // The rational conversion overload.
    static To impl(const real &r, const std::false_type &)
    {
        if (unlikely(!r.number_p())) {
            // For conversions to rational, r must represent a finite value.
            piranha_throw(safe_cast_failure, "cannot convert the non-finite real value " + r.to_string()
                                                 + " to the rational type '" + demangle<To>() + "'");
        }
        To retval;
        const bool status = mppp::get(retval, r);
        if (unlikely(!status)) {
            // NOTE: for rational conversions, the only possible failure is if the manipulation
            // of the exponent of r results in overflow.
            piranha_throw(
                safe_cast_failure,
                "cannot convert the real value " + r.to_string() + " to the rational type '" + demangle<To>()
                    + "', as the conversion triggers an overflow in the manipulation of the input real's exponent");
        }
        return retval;
    }

public:
    /// Call operator.
    /**
     * If ``To`` is an integral type, the conversion of the input argument will fail in the following cases:
     * - ``r`` is not finite or it does not represent an integral value,
     * - ``To`` is a C++ integral type and the value of ``r`` overflows the range of ``To``.
     *
     * If ``To`` is an mp++ rational, the conversion of the input argument will fail in the following cases:
     * - ``r`` is not finite,
     * - the conversion to the target rational type triggers an overflow in the manipulation of the exponent of
     *   ``r`` (this can happen if the absolute value of ``r`` is extremely large or extremely small).
     *
     * @param r the conversion argument.
     *
     * @return \p r converted to ``To``.
     *
     * @throws piranha::safe_cast_failure if the conversion fails.
     */
    To operator()(const real &r) const
    {
        return impl(r, std::integral_constant<
                           bool, disjunction<mppp::is_cpp_integral_interoperable<To>, mppp::is_integer<To>>::value>{});
    }
};

#if 0

inline namespace impl
{

template <typename Archive>
using real_boost_save_enabler
    = enable_if_t<conjunction<has_boost_save<Archive, ::mpfr_prec_t>, has_boost_save<Archive, std::string>,
                              has_boost_save<Archive, decltype(std::declval<const ::mpfr_t &>()->_mpfr_sign)>,
                              has_boost_save<Archive, decltype(std::declval<const ::mpfr_t &>()->_mpfr_exp)>,
                              has_boost_save<Archive, ::mp_limb_t>>::value>;

template <typename Archive>
using real_boost_load_enabler
    = enable_if_t<conjunction<has_boost_load<Archive, ::mpfr_prec_t>, has_boost_load<Archive, std::string>,
                              has_boost_load<Archive, decltype(std::declval<const ::mpfr_t &>()->_mpfr_sign)>,
                              has_boost_load<Archive, decltype(std::declval<const ::mpfr_t &>()->_mpfr_exp)>,
                              has_boost_load<Archive, ::mp_limb_t>>::value>;
}

/// Specialisation of piranha::boost_save() for piranha::real.
/**
 * \note
 * This specialisation is enabled only if \p std::string and all the integral types in terms of which piranha::real
 * is implemented satisfy piranha::has_boost_save.
 *
 * If \p Archive is \p boost::archive::binary_oarchive, a platform dependent binary representation of the input
 * piranha::real will be saved. Otherwise, the piranha::real is serialized in string form.
 *
 * @throws unspecified any exception thrown by piranha::boost_save() or by the conversion of the input piranha::real to
 * string.
 */
template <typename Archive>
struct boost_save_impl<Archive, real, real_boost_save_enabler<Archive>> : boost_save_via_boost_api<Archive, real> {
};

/// Specialisation of piranha::boost_load() for piranha::real.
/**
 * \note
 * This specialisation is enabled only if \p std::string and all the integral types in terms of which piranha::real
 * is implemented satisfy piranha::has_boost_load.
 *
 * If \p Archive is \p boost::archive::binary_iarchive, no checking is performed on the deserialized piranha::real
 * and the implementation offers the basic exception safety guarantee.
 *
 * @throws unspecified any exception thrown by piranha::boost_load(), or by the constructor of
 * piranha::real from string.
 */
template <typename Archive>
struct boost_load_impl<Archive, real, real_boost_load_enabler<Archive>> : boost_load_via_boost_api<Archive, real> {
};

#if defined(PIRANHA_WITH_MSGPACK)

inline namespace impl
{

// Enablers for msgpack serialization.
template <typename Stream>
using real_msgpack_pack_enabler = enable_if_t<is_detected<msgpack_pack_member_t, Stream, real>::value>;

template <typename T>
using real_msgpack_convert_enabler
    = enable_if_t<conjunction<std::is_same<real, T>, is_detected<msgpack_convert_member_t, T>>::value>;
}

/// Specialisation of piranha::msgpack_pack() for piranha::real.
/**
 * \note
 * This specialisation is enabled if \p T is piranha::real and
 * the piranha::real::msgpack_pack() method is supported with a stream of type \p Stream.
 */
template <typename Stream>
struct msgpack_pack_impl<Stream, real, real_msgpack_pack_enabler<Stream>> {
    /// Call operator.
    /**
     * The call operator will use piranha::real::msgpack_pack() internally.
     *
     * @param p target <tt>msgpack::packer</tt>.
     * @param x piranha::real to be serialized.
     * @param f the desired piranha::msgpack_format.
     *
     * @throws unspecified any exception thrown by piranha::real::msgpack_pack().
     */
    void operator()(msgpack::packer<Stream> &p, const real &x, msgpack_format f) const
    {
        x.msgpack_pack(p, f);
    }
};

/// Specialisation of piranha::msgpack_convert() for piranha::real.
/**
 * \note
 * This specialisation is enabled if \p T is piranha::real and
 * the piranha::real::msgpack_convert() method is supported.
 */
template <typename T>
struct msgpack_convert_impl<T, real_msgpack_convert_enabler<T>> {
    /// Call operator.
    /**
     * The call operator will use piranha::real::msgpack_convert() internally.
     *
     * @param x target piranha::real.
     * @param o the <tt>msgpack::object</tt> to be converted into \p n.
     * @param f the desired piranha::msgpack_format.
     *
     * @throws unspecified any exception thrown by piranha::real::msgpack_convert().
     */
    void operator()(T &x, const msgpack::object &o, msgpack_format f) const
    {
        x.msgpack_convert(o, f);
    }
};

#endif

inline namespace impl
{

template <typename T>
using real_zero_is_absorbing_enabler = enable_if_t<std::is_same<uncvref_t<T>, real>::value>;
}

/// Specialisation of piranha::zero_is_absorbing for piranha::real.
/**
 * \note
 * This specialisation is enabled if \p T, after the removal of cv/reference qualifiers, is piranha::real.
 *
 * Due to the presence of NaN, the zero element is not absorbing for piranha::real.
 */
template <typename T>
struct zero_is_absorbing<T, real_zero_is_absorbing_enabler<T>> {
    /// Value of the type trait.
    static const bool value = false;
};

template <typename T>
const bool zero_is_absorbing<T, real_zero_is_absorbing_enabler<T>>::value;

#endif
}

#else

#error The real.hpp header was included but mp++ was not configured with the MPPP_WITH_MPFR option.

#endif

#endif

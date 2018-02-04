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

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <mp++/concepts.hpp>
#include <mp++/detail/gmp.hpp>
#include <mp++/detail/mpfr.hpp>
#include <mp++/integer.hpp>
#include <mp++/rational.hpp>
#include <mp++/real.hpp>

#include <piranha/config.hpp>
#include <piranha/detail/demangle.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/math.hpp>
#include <piranha/math/cos.hpp>
#include <piranha/math/is_zero.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/math/sin.hpp>
#include <piranha/s11n.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

// Multiprecision floating-point type.
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

// Specialisation of piranha::math::is_zero() for piranha::real.
template <>
class is_zero_impl<real>
{
public:
    bool operator()(const real &r) const
    {
        return r.zero_p();
    }
};

/// Specialisation of the implementation of piranha::math::is_unitary() for piranha::real.
template <>
struct is_unitary_impl<real> {
    /// Call operator.
    /**
     * @param r the value to be tested.
     *
     * @return \p true if \p r is exactly one, \p false otherwise.
     */
    bool operator()(const real &r) const
    {
        return r.is_one();
    }
};

// Specialisation of piranha::math::pow() for piranha::real.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename U, mppp::RealOpTypes<U> T>
class pow_impl<T, U>
#else
template <typename T, typename U>
class pow_impl<T, U, enable_if_t<mppp::are_real_op_types<T, U>::value>>
#endif
{
public:
    template <typename T1, typename U1>
    auto operator()(T1 &&base, U1 &&exp) const -> decltype(mppp::pow(std::forward<T1>(base), std::forward<U1>(exp)))
    {
        return mppp::pow(std::forward<T1>(base), std::forward<U1>(exp));
    }
};

template <>
class sin_impl<real>
{
public:
    template <typename T>
    real operator()(T &&r) const
    {
        return mppp::sin(std::forward<T>(r));
    }
};

template <>
class cos_impl<real>
{
public:
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

/// Specialisation of the implementation of piranha::math::add3() for piranha::real.
template <>
struct add3_impl<real> {
    /// Call operator.
    /**
     * @param out the return value.
     * @param x the first operand.
     * @param y the second operand.
     */
    void operator()(real &out, const real &x, const real &y) const
    {
        mppp::add(out, x, y);
    }
};

/// Specialisation of the implementation of piranha::math::sub3() for piranha::real.
template <>
struct sub3_impl<real> {
    /// Call operator.
    /**
     * @param out the return value.
     * @param x the first operand.
     * @param y the second operand.
     */
    void operator()(real &out, const real &x, const real &y) const
    {
        mppp::sub(out, x, y);
    }
};

/// Specialisation of the implementation of piranha::math::mul3() for piranha::real.
template <>
struct mul3_impl<real> {
    /// Call operator.
    /**
     * @param out the return value.
     * @param x the first operand.
     * @param y the second operand.
     */
    void operator()(real &out, const real &x, const real &y) const
    {
        mppp::mul(out, x, y);
    }
};

/// Specialisation of the implementation of piranha::math::div3() for piranha::real.
template <>
struct div3_impl<real> {
    /// Call operator.
    /**
     * @param out the return value.
     * @param x the first operand.
     * @param y the second operand.
     */
    void operator()(real &out, const real &x, const real &y) const
    {
        mppp::div(out, x, y);
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
        // LCOV_EXCL_START
        if (unlikely(!status)) {
            // NOTE: for rational conversions, the only possible failure is if the manipulation
            // of the exponent of r results in overflow.
            piranha_throw(
                safe_cast_failure,
                "cannot convert the real value " + r.to_string() + " to the rational type '" + demangle<To>()
                    + "', as the conversion triggers an overflow in the manipulation of the input real's exponent");
        }
        // LCOV_EXCL_STOP
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

inline namespace impl
{

// Utility function to infer the size (i.e., the number of limbs of the significand) of a real
// from its precision. This is used in the s11n functions.
inline ::mpfr_prec_t real_size_from_prec(const ::mpfr_prec_t &prec)
{
    // NOTE: originally we had ::mp_bits_per_limb here instead of GMP_NUMB_BITS, but I changed it because
    // apparently the global ::mp_bits_per_limb variable is not exported in the conda
    // packages for MPFR and we get a link error. The GMP_NUMB_BITS macro should
    // be equivalent (also considering that MPFR does not support nail builds of GMP),
    // and it should also be faster because it's a value known to the compiler
    // at build time.
    const ::mpfr_prec_t q = prec / GMP_NUMB_BITS, r = prec % GMP_NUMB_BITS;
#if !defined(PIRANHA_COMPILER_IS_CLANG_CL)
    piranha_assert(GMP_NUMB_BITS == ::mp_bits_per_limb);
#endif
    return q + (r != 0);
}
}
}

namespace boost
{
namespace serialization
{

// Binary serialisation.
inline void save(boost::archive::binary_oarchive &ar, const piranha::real &r, unsigned)
{
    piranha::boost_save(ar, r.get_mpfr_t()->_mpfr_prec);
    piranha::boost_save(ar, r.get_mpfr_t()->_mpfr_sign);
    piranha::boost_save(ar, r.get_mpfr_t()->_mpfr_exp);
    const ::mpfr_prec_t s = piranha::real_size_from_prec(r.get_prec());
    // NOTE: no need to save the size, as it can be recovered from the prec.
    for (::mpfr_prec_t i = 0; i < s; ++i) {
        piranha::boost_save(ar, r.get_mpfr_t()->_mpfr_d[i]);
    }
}

inline void load(boost::archive::binary_iarchive &ar, piranha::real &r, unsigned)
{
    // First we recover the non-limb members.
    ::mpfr_prec_t prec;
    decltype(r.get_mpfr_t()->_mpfr_sign) sign;
    decltype(r.get_mpfr_t()->_mpfr_exp) exp;
    piranha::boost_load(ar, prec);
    piranha::boost_load(ar, sign);
    piranha::boost_load(ar, exp);
    // Recover the size in limbs from the prec.
    const ::mpfr_prec_t s = piranha::real_size_from_prec(prec);
    // Set the precision.
    r.set_prec(prec);
    piranha_assert(r.get_prec() == prec);
    // Now let's write sign and exponent.
    r._get_mpfr_t()->_mpfr_sign = sign;
    r._get_mpfr_t()->_mpfr_exp = exp;
    try {
        // NOTE: protect in try/catch as in theory boost_load() could throw and we want
        // to deal with this case.
        for (::mpfr_prec_t i = 0; i < s; ++i) {
            piranha::boost_load(ar, r._get_mpfr_t()->_mpfr_d[i]);
        }
        // LCOV_EXCL_START
    } catch (...) {
        // Set to zero before re-throwing.
        ::mpfr_set_ui(r._get_mpfr_t(), 0u, MPFR_RNDN);
        throw;
        // LCOV_EXCL_STOP
    }
}

// Portable serialization.
template <typename Archive>
inline void save(Archive &ar, const piranha::real &r, unsigned)
{
    // Store precision and base-10 string representation.
    piranha::boost_save(ar, r.get_prec());
    piranha::boost_save(ar, r.to_string());
}

template <typename Archive>
inline void load(Archive &ar, piranha::real &r, unsigned)
{
    // Load the precision and the string rep from the archive.
    ::mpfr_prec_t prec;
    PIRANHA_MAYBE_TLS std::string s;
    piranha::boost_load(ar, prec);
    piranha::boost_load(ar, s);
    // Set the precision and then set the value of r to
    // the loaded string representation.
    r.set_prec(prec);
    r.set(s);
}

template <typename Archive>
inline void serialize(Archive &ar, piranha::real &r, const unsigned int file_version)
{
    split_free(ar, r, file_version);
}
}
}

namespace piranha
{

inline namespace impl
{

// We need to be able to (de)serialise all the integral types inside mpfr_t (for the binary archive)
// and string (for the portable archive).
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
 * If \p Archive is \p boost::archive::binary_oarchive, a platform-dependent binary representation of the input
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
 * @throws unspecified any exception thrown by:
 * - piranha::boost_load(),
 * - the assignment of a string to a piranha::real,
 * - setting the precision of a piranha::real.
 */
template <typename Archive>
struct boost_load_impl<Archive, real, real_boost_load_enabler<Archive>> : boost_load_via_boost_api<Archive, real> {
};

#if defined(PIRANHA_WITH_MSGPACK)

inline namespace impl
{

// Enablers for msgpack serialization.
template <typename Stream>
using real_msgpack_pack_enabler = enable_if_t<conjunction<
    is_msgpack_stream<Stream>, has_msgpack_pack<Stream, ::mpfr_prec_t>, has_msgpack_pack<Stream, std::string>,
    has_msgpack_pack<Stream, decltype(std::declval<const ::mpfr_t &>()->_mpfr_sign)>,
    has_msgpack_pack<Stream, decltype(std::declval<const ::mpfr_t &>()->_mpfr_exp)>,
    has_msgpack_pack<Stream, ::mp_limb_t>, has_safe_cast<std::uint32_t, ::mpfr_prec_t>>::value>;

template <typename T>
using real_msgpack_convert_enabler = enable_if_t<
    conjunction<std::is_same<T, real>, has_msgpack_convert<::mpfr_prec_t>, has_msgpack_convert<std::string>,
                has_msgpack_convert<decltype(std::declval<const ::mpfr_t &>()->_mpfr_sign)>,
                has_msgpack_convert<decltype(std::declval<const ::mpfr_t &>()->_mpfr_exp)>,
                has_msgpack_convert<::mp_limb_t>>::value>;
}

/// Specialisation of piranha::msgpack_pack() for piranha::real.
/**
 * \note
 * This specialisation is enabled if:
 * - \p Stream satisfies piranha::is_msgpack_stream,
 * - all the integral types in terms of which piranha::real is implemented satisfy piranha::has_msgpack_pack,
 * - ``std::string`` satisfies piranha::has_msgpack_pack.
 */
template <typename Stream>
struct msgpack_pack_impl<Stream, real, real_msgpack_pack_enabler<Stream>> {
    /// Call operator.
    /**
     * This method will pack ``x`` into \p p. If \p f is msgpack_format::portable, then
     * the precision of ``x`` and a decimal string representation of ``x`` are packed in an array.
     * Otherwise, an array of 4 elements storing the internal MPFR representation of ``x`` is packed.
     *
     * @param p the target <tt>msgpack::packer</tt>.
     * @param x the piranha::real to be serialized.
     * @param f the desired piranha::msgpack_format.
     *
     * @throws unspecified any exception thrown by:
     * - piranha::safe_cast(),
     * - the public interface of <tt>msgpack::packer</tt>,
     * - piranha::msgpack_pack(),
     * - the conversion of ``x`` to string.
     */
    void operator()(msgpack::packer<Stream> &p, const real &x, msgpack_format f) const
    {
        if (f == msgpack_format::portable) {
            p.pack_array(2);
            piranha::msgpack_pack(p, x.get_prec(), f);
            piranha::msgpack_pack(p, x.to_string(), f);
        } else {
            // NOTE: storing both prec and the number of limbs is a bit redundant: it is possible to
            // infer the number of limbs from prec but not viceversa (only an upper/lower bound). So let's
            // store them both.
            p.pack_array(4);
            piranha::msgpack_pack(p, x.get_prec(), f);
            piranha::msgpack_pack(p, x.get_mpfr_t()->_mpfr_sign, f);
            piranha::msgpack_pack(p, x.get_mpfr_t()->_mpfr_exp, f);
            const auto s = safe_cast<std::uint32_t>(real_size_from_prec(x.get_mpfr_t()->_mpfr_prec));
            p.pack_array(s);
            // NOTE: no need to save the size, as it can be recovered from the prec.
            for (std::uint32_t i = 0; i < s; ++i) {
                piranha::msgpack_pack(p, x.get_mpfr_t()->_mpfr_d[i], f);
            }
        }
    }
};

/// Specialisation of piranha::msgpack_convert() for piranha::real.
/**
 * \note
 * This specialisation is enabled if:
 * - \p T is piranha::real,
 * - all the integral types in terms of which piranha::real is implemented satisfy piranha::has_msgpack_convert,
 * - ``std::string`` satisfies piranha::has_msgpack_convert.
 */
template <typename T>
struct msgpack_convert_impl<T, real_msgpack_convert_enabler<T>> {
    /// Call operator.
    /**
     * This method will convert the object \p o into ``x``. If \p f is piranha::msgpack_format::binary,
     * this method offers the basic exception safety guarantee and it performs minimal checking on the input data.
     * Calling this method in binary mode will result in undefined behaviour if \p o does not contain an integer
     * serialized via msgpack_pack().
     *
     * @param x the destination piranha::real.
     * @param o the source object.
     * @param f the desired piranha::msgpack_format.
     *
     * @throws std::invalid_argument if, in binary mode, the number of serialized limbs is inconsistent with the
     * precision.
     * @throws unspecified any exception thrown by:
     * - piranha::safe_cast(),
     * - setting the precision of a piranha::real,
     * - memory errors in standard containers,
     * - the public interface of <tt>msgpack::object</tt>,
     * - piranha::msgpack_convert(),
     * - the assignment of a string to a piranha::real.
     */
    void operator()(T &x, const msgpack::object &o, msgpack_format f) const
    {
        if (f == msgpack_format::portable) {
            PIRANHA_MAYBE_TLS std::array<msgpack::object, 2> vobj;
            o.convert(vobj);
            ::mpfr_prec_t prec;
            PIRANHA_MAYBE_TLS std::string s;
            piranha::msgpack_convert(prec, vobj[0], f);
            piranha::msgpack_convert(s, vobj[1], f);
            x.set_prec(prec);
            x.set(s);
        } else {
            PIRANHA_MAYBE_TLS std::array<msgpack::object, 4> vobj;
            o.convert(vobj);
            // First let's handle the non-limbs members.
            ::mpfr_prec_t prec;
            decltype(x.get_mpfr_t()->_mpfr_sign) sign;
            decltype(x.get_mpfr_t()->_mpfr_exp) exp;
            piranha::msgpack_convert(prec, vobj[0], f);
            piranha::msgpack_convert(sign, vobj[1], f);
            piranha::msgpack_convert(exp, vobj[2], f);
            x.set_prec(prec);
            piranha_assert(x.get_prec() == prec);
            x._get_mpfr_t()->_mpfr_sign = sign;
            x._get_mpfr_t()->_mpfr_exp = exp;
            // Next the limbs. Protect in try/catch so if anything goes wrong we can fix this in the
            // catch block before re-throwing.
            try {
                PIRANHA_MAYBE_TLS std::vector<msgpack::object> vlimbs;
                vobj[3].convert(vlimbs);
                const auto s = safe_cast<decltype(vlimbs.size())>(real_size_from_prec(prec));
                if (unlikely(s != vlimbs.size())) {
                    piranha_throw(std::invalid_argument,
                                  "error in the msgpack deserialization of a real: the number of serialized limbs ("
                                      + std::to_string(vlimbs.size())
                                      + ") is not consistent with the number of limbs inferred from the precision ("
                                      + std::to_string(s) + ")");
                }
                for (decltype(vlimbs.size()) i = 0; i < s; ++i) {
                    piranha::msgpack_convert(x._get_mpfr_t()->_mpfr_d[i], vlimbs[i], f);
                }
            } catch (...) {
                // Set to zero before re-throwing.
                ::mpfr_set_ui(x._get_mpfr_t(), 0u, MPFR_RNDN);
                throw;
            }
        }
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
    static constexpr bool value = false;
};

#if PIRANHA_CPLUSPLUS < 201703L

template <typename T>
constexpr bool zero_is_absorbing<T, real_zero_is_absorbing_enabler<T>>::value;

#endif
}

#else

#error The real.hpp header was included but mp++ was not configured with the MPPP_WITH_MPFR option.

#endif

#endif

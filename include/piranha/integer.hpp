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

#ifndef PIRANHA_INTEGER_HPP
#define PIRANHA_INTEGER_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <mp++/concepts.hpp>
#include <mp++/integer.hpp>

#include <piranha/config.hpp>
#include <piranha/detail/demangle.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/is_key.hpp>
#include <piranha/math.hpp>
#include <piranha/s11n.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

// Main multiprecision integer type.
using integer = mppp::integer<1>;

namespace math
{

/// Specialisation of the implementation of piranha::math::multiply_accumulate() for mp++'s integers.
template <std::size_t SSize>
struct multiply_accumulate_impl<mppp::integer<SSize>> {
    /// Call operator.
    /**
     * @param x target value for accumulation.
     * @param y first argument.
     * @param z second argument.
     */
    void operator()(mppp::integer<SSize> &x, const mppp::integer<SSize> &y, const mppp::integer<SSize> &z) const
    {
        mppp::addmul(x, y, z);
    }
};

/// Specialisation of the implementation of piranha::math::negate() for mp++'s integers.
template <std::size_t SSize>
struct negate_impl<mppp::integer<SSize>> {
    /// Call operator.
    /**
     * @param n the integer to be negated.
     */
    void operator()(mppp::integer<SSize> &n) const
    {
        n.neg();
    }
};

/// Specialisation of the implementation of piranha::math::is_zero() for mp++'s integers.
template <std::size_t SSize>
struct is_zero_impl<mppp::integer<SSize>> {
    /// Call operator.
    /**
     * @param n the integer to be tested.
     *
     * @return \p true if \p n is zero, \p false otherwise.
     */
    bool operator()(const mppp::integer<SSize> &n) const
    {
        return n.is_zero();
    }
};

/// Specialisation of the implementation of piranha::math::is_unitary() for mp++'s integers.
template <std::size_t SSize>
struct is_unitary_impl<mppp::integer<SSize>> {
    /// Call operator.
    /**
     * @param n the integer to be tested.
     *
     * @return \p true if \p n is equal to 1, \p false otherwise.
     */
    bool operator()(const mppp::integer<SSize> &n) const
    {
        return n.is_one();
    }
};

/// Specialisation of the implementation of piranha::math::abs() for mp++'s integers.
template <std::size_t SSize>
struct abs_impl<mppp::integer<SSize>> {
    /// Call operator.
    /**
     * @param n the input integer.
     *
     * @return the absolute value of \p n.
     */
    mppp::integer<SSize> operator()(const mppp::integer<SSize> &n) const
    {
        return mppp::abs(n);
    }
};

/// Specialisation of the implementation of piranha::math::sin() for mp++'s integers.
template <std::size_t SSize>
struct sin_impl<mppp::integer<SSize>> {
    /// Call operator.
    /**
     * @param n the input integer.
     *
     * @return the sine of \p n.
     *
     * @throws std::invalid_argument if the argument is not zero.
     */
    mppp::integer<SSize> operator()(const mppp::integer<SSize> &n) const
    {
        if (likely(n.is_zero())) {
            return mppp::integer<SSize>{};
        }
        piranha_throw(std::invalid_argument, "cannot compute the sine of a non-zero integer");
    }
};

/// Specialisation of the implementation of piranha::math::cos() for mp++'s integers.
template <std::size_t SSize>
struct cos_impl<mppp::integer<SSize>> {
    /// Call operator.
    /**
     * @param n the input integer.
     *
     * @return the cosine of \p n.
     *
     * @throws std::invalid_argument if the argument is not zero.
     */
    mppp::integer<SSize> operator()(const mppp::integer<SSize> &n) const
    {
        if (likely(n.is_zero())) {
            return mppp::integer<SSize>{1};
        }
        piranha_throw(std::invalid_argument, "cannot compute the cosine of a non-zero integer");
    }
};

/// Specialisation of the implementation of piranha::math::partial() for mp++'s integers.
template <std::size_t SSize>
struct partial_impl<mppp::integer<SSize>> {
    /// Call operator.
    /**
     * @return zero.
     */
    mppp::integer<SSize> operator()(const mppp::integer<SSize> &, const std::string &) const
    {
        return mppp::integer<SSize>{};
    }
};

/// Factorial.
/**
 * @param n the factorial argument.
 *
 * @return the factorial of \p n.
 *
 * @throws std::domain_error if \p n is negative.
 * @throws unspecified any exception thrown by the invoked mp++ function or by the conversion
 * of \p n to <tt>unsigned long</tt>.
 */
template <std::size_t SSize>
inline mppp::integer<SSize> factorial(const mppp::integer<SSize> &n)
{
    if (unlikely(n.sgn() < 0)) {
        piranha_throw(std::domain_error, "cannot compute the factorial of the negative integer " + n.to_string());
    }
    mppp::integer<SSize> retval;
    mppp::fac_ui(retval, static_cast<unsigned long>(n));
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

/// Substitution of integral power.
/**
 * \note
 * This function is enabled only if the expression <tt>ipow_subs_impl<T, U>{}(x, name, n, y)</tt> is valid,
 * returning a type which satisfies piranha::is_returnable.
 *
 * Substitute, in \p x, <tt>name**n</tt> with \p y. The actual implementation of this function is in the
 * piranha::math::ipow_subs_impl functor's call operator. The body of this function is equivalent to:
 * @code
 * return ipow_subs_impl<T, U>{}(x, name, n, y);
 * @endcode
 *
 * @param x the quantity that will be subject to substitution.
 * @param name the name of the symbolic variable that will be substituted.
 * @param n the power of \p name that will be substituted.
 * @param y the object that will substitute the variable.
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

inline namespace impl
{

// Enabler for the overload below.
// NOTE: is_returnable is already checked by the invocation of the other overload.
template <typename T, typename U, typename Int>
using math_ipow_subs_int_t = enable_if_t<mppp::is_cpp_integral_interoperable<Int>::value, math_ipow_subs_t<T, U>>;
}

/// Substitution of integral power (convenience overload).
/**
 * \note
 * This function is enabled only if:
 * - \p Int is a C++ integral type with which mp++'s integers can interoperate, and
 * - the other overload of piranha::math::ipow_subs() is enabled with template arguments \p T and \p U.
 *
 * This function is a convenience wrapper that will call the other piranha::math::ipow_subs() overload, with \p n
 * converted to a piranha::integer.
 *
 * @param x the quantity that will be subject to substitution.
 * @param name the name of the symbolic variable that will be substituted.
 * @param n the power of \p name that will be substituted.
 * @param y the object that will substitute the variable.
 *
 * @return \p x after the substitution of \p name to the power of \p n with \p y.
 *
 * @throws unspecified any exception thrown by the other overload of piranha::math::ipow_subs().
 */
template <typename T, typename U, typename Int>
inline math_ipow_subs_int_t<T, U, Int> ipow_subs(const T &x, const std::string &name, const Int &n, const U &y)
{
    return math::ipow_subs(x, name, integer{n}, y);
}

/// Specialisation of the implementation of piranha::math::add3() for mp++'s integers.
template <std::size_t SSize>
struct add3_impl<mppp::integer<SSize>> {
    /// Call operator.
    /**
     * @param out the output value.
     * @param a the first operand.
     * @param b the second operand.
     */
    void operator()(mppp::integer<SSize> &out, const mppp::integer<SSize> &a, const mppp::integer<SSize> &b) const
    {
        mppp::add(out, a, b);
    }
};

/// Specialisation of the implementation of piranha::math::sub3() for mp++'s integers.
template <std::size_t SSize>
struct sub3_impl<mppp::integer<SSize>> {
    /// Call operator.
    /**
     * @param out the output value.
     * @param a the first operand.
     * @param b the second operand.
     */
    void operator()(mppp::integer<SSize> &out, const mppp::integer<SSize> &a, const mppp::integer<SSize> &b) const
    {
        mppp::sub(out, a, b);
    }
};

/// Specialisation of the implementation of piranha::math::mul3() for mp++'s integers.
template <std::size_t SSize>
struct mul3_impl<mppp::integer<SSize>> {
    /// Call operator.
    /**
     * @param out the output value.
     * @param a the first operand.
     * @param b the second operand.
     */
    void operator()(mppp::integer<SSize> &out, const mppp::integer<SSize> &a, const mppp::integer<SSize> &b) const
    {
        mppp::mul(out, a, b);
    }
};

/// Specialisation of the implementation of piranha::math::div3() for mp++'s integers.
template <std::size_t SSize>
struct div3_impl<mppp::integer<SSize>> {
    /// Call operator.
    /**
     * @param out the output value.
     * @param a the dividend.
     * @param b the divisor.
     *
     * @throws unspecified any exception thrown by the invoked mp++ function.
     */
    void operator()(mppp::integer<SSize> &out, const mppp::integer<SSize> &a, const mppp::integer<SSize> &b) const
    {
        mppp::tdiv_q(out, a, b);
    }
};

/// Specialisation of the implementation of piranha::math::gcd() for mp++'s integers.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename U, mppp::IntegerIntegralOpTypes<U> T>
struct gcd_impl<T, U>
#else
template <typename T, typename U>
struct gcd_impl<T, U, enable_if_t<mppp::are_integer_integral_op_types<T, U>::value>>
#endif
{
    /// Call operator, overload for mp++'s integers.
    /**
     * @param a the first argument.
     * @param b the second argument.
     *
     * @return the GCD of \p a and \p b.
     */
    template <std::size_t SSize>
    mppp::integer<SSize> operator()(const mppp::integer<SSize> &a, const mppp::integer<SSize> &b) const
    {
        return mppp::gcd(a, b);
    }
    /// Call operator, mp++ integer - integral overload.
    /**
     * @param a the first argument.
     * @param b the second argument.
     *
     * @return the GCD of \p a and \p b.
     */
    template <std::size_t SSize, typename T1>
    mppp::integer<SSize> operator()(const mppp::integer<SSize> &a, const T1 &b) const
    {
        return operator()(a, mppp::integer<SSize>{b});
    }
    /// Call operator, integral - mp++ integer overload.
    /**
     * @param a the first argument.
     * @param b the second argument.
     *
     * @return the GCD of \p a and \p b.
     */
    template <std::size_t SSize, typename T1>
    mppp::integer<SSize> operator()(const T1 &a, const mppp::integer<SSize> &b) const
    {
        return operator()(b, a);
    }
};

/// Specialisation of the implementation of piranha::math::gcd3() for mp++'s integers.
template <std::size_t SSize>
struct gcd3_impl<mppp::integer<SSize>> {
    /// Call operator.
    /**
     * @param out return value.
     * @param a first argument.
     * @param b second argument.
     */
    void operator()(mppp::integer<SSize> &out, const mppp::integer<SSize> &a, const mppp::integer<SSize> &b) const
    {
        mppp::gcd(out, a, b);
    }
};
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
    static constexpr bool value = implementation_defined;
};

#if PIRANHA_CPLUSPLUS < 201703L

template <typename T, typename U>
constexpr bool has_ipow_subs<T, U>::value;

#endif

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
    static constexpr bool value = implementation_defined;
};

#if PIRANHA_CPLUSPLUS < 201703L

template <typename Key, typename T>
constexpr bool key_has_ipow_subs<Key, T>::value;

#endif

inline namespace literals
{

/// Literal for arbitrary-precision integers.
/**
 * @param s a literal string.
 *
 * @return a piranha::integer constructed from \p s.
 *
 * @throws unspecified any exception thrown by the constructor of
 * piranha::integer from string.
 */
inline integer operator"" _z(const char *s)
{
    return integer{s};
}
}
}

namespace boost
{
namespace serialization
{

// Binary serialisation.
template <std::size_t SSize>
inline void save(boost::archive::binary_oarchive &ar, const mppp::integer<SSize> &n, unsigned)
{
    PIRANHA_MAYBE_TLS std::vector<char> tmp_v;
    // Make sure we use exactly the amount of storage required
    // by serialisation.
    tmp_v.resize(piranha::safe_cast<decltype(tmp_v.size())>(n.binary_size()));
    // Save into the temp buffer.
    n.binary_save(tmp_v);
    // Write the buffer into the archive.
    // NOTE: here we use directly the boost API (instead of boost_save()), as we know exactly
    // the type and the archive, and this allows us to avoid dealing with vectors in boost_save().
    ar << tmp_v;
}

template <std::size_t SSize>
inline void load(boost::archive::binary_iarchive &ar, mppp::integer<SSize> &n, unsigned)
{
    PIRANHA_MAYBE_TLS std::vector<char> tmp_v;
    // Load the data in the temp buffer from the archive.
    ar >> tmp_v;
    // Deserialise into n.
    n.binary_load(tmp_v);
}

// Portable serialization.
template <typename Archive, std::size_t SSize>
inline void save(Archive &ar, const mppp::integer<SSize> &n, unsigned)
{
    // NOTE: here we have an unnecessary copy (but at least we are avoiding memory allocations).
    // Maybe we should consider providing an API from mp++ to interact directly with strings
    // rather than vectors of chars.
    PIRANHA_MAYBE_TLS std::vector<char> tmp_v;
    mppp::mpz_to_str(tmp_v, n.get_mpz_view());
    PIRANHA_MAYBE_TLS std::string tmp_s;
    tmp_s.assign(tmp_v.data());
    piranha::boost_save(ar, tmp_s);
}

template <typename Archive, std::size_t SSize>
inline void load(Archive &ar, mppp::integer<SSize> &n, unsigned)
{
    PIRANHA_MAYBE_TLS std::string tmp;
    piranha::boost_load(ar, tmp);
    n = mppp::integer<SSize>{tmp};
}

template <typename Archive, std::size_t SSize>
inline void serialize(Archive &ar, mppp::integer<SSize> &n, const unsigned int file_version)
{
    split_free(ar, n, file_version);
}
}
}

namespace piranha
{

inline namespace impl
{

// NOTE: binary serialisation goes through std::vector<char>, which is always supported in
// boost's binary archive (so no need for the enabler). Portable serialisation goes through string.
template <typename Archive>
using integer_boost_save_enabler = enable_if_t<has_boost_save<Archive, std::string>::value>;

template <typename Archive>
using integer_boost_load_enabler = enable_if_t<has_boost_load<Archive, std::string>::value>;
}

/// Specialisation of piranha::boost_save() for mp++'s integers.
/**
 * \note
 * This specialisation is enabled only if \p std::string satisfies piranha::has_boost_save.
 *
 * If \p Archive is \p boost::archive::binary_oarchive, a platform-dependent non-portable
 * representation of the input integer is saved. Otherwise, a string representation of the input
 * integer is saved.
 *
 * @throws unspecified any exception thrown by
 * - piranha::boost_save(),
 * - piranha::safe_cast(),
 * - the public interface of mp++'s integers,
 * - memory errors in standard containers.
 */
template <typename Archive, std::size_t SSize>
struct boost_save_impl<Archive, mppp::integer<SSize>, integer_boost_save_enabler<Archive>>
    : boost_save_via_boost_api<Archive, mppp::integer<SSize>> {
};

/// Specialisation of piranha::boost_load() for mp++'s integers.
/**
 * \note
 * This specialisation is enabled only if \p std::string satisfies piranha::has_boost_load.
 *
 * If \p Archive is \p boost::archive::binary_iarchive, this specialisation offers the basic exception guarantee
 * and performs minimal checking of the input data.
 *
 * @throws unspecified any exception thrown by:
 * - piranha::boost_load(),
 * - the public interface of mp++'s integers.
 */
template <typename Archive, std::size_t SSize>
struct boost_load_impl<Archive, mppp::integer<SSize>, integer_boost_load_enabler<Archive>>
    : boost_load_via_boost_api<Archive, mppp::integer<SSize>> {
};

#if defined(PIRANHA_WITH_MSGPACK)

inline namespace impl
{

// Enablers for msgpack serialization.
template <typename Stream>
using integer_msgpack_pack_enabler
    = enable_if_t<conjunction<is_msgpack_stream<Stream>, has_msgpack_pack<Stream, std::string>>::value>;

template <typename T>
using integer_msgpack_convert_enabler
    = enable_if_t<conjunction<mppp::is_integer<T>, has_msgpack_convert<std::string>>::value>;
}

/// Specialisation of piranha::msgpack_pack() for mp++'s integers.
/**
 * \note
 * This specialisation is enabled only if:
 * - \p Stream satisfies piranha::is_msgpack_stream,
 * - \p std::string supports piranha::has_msgpack_pack.
 */
template <typename Stream, std::size_t SSize>
struct msgpack_pack_impl<Stream, mppp::integer<SSize>, integer_msgpack_pack_enabler<Stream>> {
    /// Call operator.
    /**
     * If the serialization format \p f is msgpack_format::portable, then
     * a decimal string representation of the integer is packed. Otherwise, a raw binary
     * representation of the integer is packed.
     *
     * @param p the target <tt>msgpack::packer</tt>.
     * @param n the input integer.
     * @param f the desired piranha::msgpack_format.
     *
     * @throws unspecified any exception thrown by:
     * - the public interface of \p msgpack::packer,
     * - memory errors in standard containers,
     * - piranha::msgpack_pack(),
     * - the public interface of mp++'s integers,
     * - piranha::safe_cast().
     */
    void operator()(msgpack::packer<Stream> &p, const mppp::integer<SSize> &n, msgpack_format f) const
    {
        if (f == msgpack_format::binary) {
            // Go through a local buffer for serialisation.
            PIRANHA_MAYBE_TLS std::vector<char> tmp_v;
            // Make sure we use exactly the amount of storage required
            // by serialisation.
            tmp_v.resize(safe_cast<decltype(tmp_v.size())>(n.binary_size()));
            // Save into the temp buffer.
            n.binary_save(tmp_v);
            // Do the binary packing into the packer.
            p.pack_bin(safe_cast<std::uint32_t>(tmp_v.size()));
            p.pack_bin_body(tmp_v.data(), safe_cast<std::uint32_t>(tmp_v.size()));
        } else {
            // NOTE: here we have an unnecessary copy (but at least we are avoiding memory allocations).
            // Maybe we should consider providing an API from mp++ to interact directly with strings
            // rather than vectors of chars.
            PIRANHA_MAYBE_TLS std::vector<char> tmp_v;
            mppp::mpz_to_str(tmp_v, n.get_mpz_view());
            PIRANHA_MAYBE_TLS std::string tmp_s;
            tmp_s.assign(tmp_v.data());
            piranha::msgpack_pack(p, tmp_s, f);
        }
    }
};

/// Specialisation of piranha::msgpack_convert() for mp++'s integers.
/**
 * \note
 * This specialisation is enabled if:
 * - \p T is an mp++ integer,
 * - \p std::string satisfies piranha::has_msgpack_convert.
 */
// NOTE: we need to keep the generic T param here, instead of integer<SSize>, in order to trigger
// SFINAE in the enabler.
template <typename T>
struct msgpack_convert_impl<T, integer_msgpack_convert_enabler<T>> {
    /// Call operator.
    /**
     * This method will convert the object \p o into \p n. If \p f is piranha::msgpack_format::binary,
     * this method offers the basic exception safety guarantee and it performs minimal checking on the input data.
     * Calling this method in binary mode will result in undefined behaviour if \p o does not contain an integer
     * serialized via msgpack_pack().
     *
     * @param n the destination mp++ integer.
     * @param o the source object.
     * @param f the desired piranha::msgpack_format.
     *
     * @throws unspecified any exception thrown by:
     * - the public interface of <tt>msgpack::object</tt>,
     * - piranha::msgpack_convert(),
     * - the public interface of mp++'s integers.
     */
    void operator()(T &n, const msgpack::object &o, msgpack_format f) const
    {
        if (f == msgpack_format::binary) {
            PIRANHA_MAYBE_TLS std::vector<char> tmp_v;
            // Try to convert the msgpack object into a char vector.
            o.convert(tmp_v);
            // Deserialise from the char vector.
            n.binary_load(tmp_v);
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
using integer_safe_cast_enabler = enable_if_t<
    // NOTE: let's keep the first disjunction here separated: we don't want to catch all
    // CppInteroperable types here, as safe casting needs to be done on a controlled
    // case-by-case basis.
    disjunction<conjunction<mppp::is_integer<To>, disjunction<mppp::is_cpp_floating_point_interoperable<From>,
                                                              mppp::is_cpp_integral_interoperable<From>>>,
                conjunction<mppp::is_integer<From>, mppp::is_cpp_integral_interoperable<To>>>::value>;
}

/// Specialisation of piranha::safe_cast() for conversions involving mp++'s integers.
/**
 * \note
 * This specialisation is enabled in the following cases:
 * - \p To is an mp++ integer and \p From is a C++ floating-point or integral type with which
 *   mp++ integer can interoperate,
 * - \p From is an mp++ integer and \p To is a C++ integral type with which
 *   mp++ integer can interoperate.
 */
template <typename To, typename From>
struct safe_cast_impl<To, From, integer_safe_cast_enabler<To, From>> {
private:
    // From float to mppp::integer.
    template <typename T, enable_if_t<mppp::is_cpp_floating_point_interoperable<T>::value, int> = 0>
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
    // From C++ integral to mppp::integer.
    template <typename T, enable_if_t<mppp::is_cpp_integral_interoperable<T>::value, int> = 0>
    static To impl(const T &n)
    {
        return To{n};
    }
    // From mppp::integer to C++ integral.
    template <typename T, enable_if_t<mppp::is_integer<T>::value, int> = 0>
    static To impl(const T &n)
    {
        To retval;
        const auto status = mppp::get(retval, n);
        if (unlikely(!status)) {
            piranha_throw(safe_cast_failure, "the arbitrary-precision integer " + n.to_string()
                                                 + " cannot be converted to the type '" + demangle<To>()
                                                 + "', as the conversion would result in overflow");
        }
        return retval;
    }

public:
    /// Call operator.
    /**
     * @param x the input value to be converted.
     *
     * @return \p x converted to \p To.
     *
     * @throws piranha::safe_cast_failure if:
     * - the input float is not finite or if it has a nonzero fractional part,
     * - the target integral type \p To cannot represent the value of the input mp++ integer.
     * @throws unspecified any exception thrown by the constructor of mp++'s integers from a floating-point.
     */
    To operator()(const From &x) const
    {
        return impl(x);
    }
};
}

#endif

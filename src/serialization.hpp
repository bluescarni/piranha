/* Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)

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

#ifndef PIRANHA_SERIALIZATION_HPP
#define PIRANHA_SERIALIZATION_HPP

// Common headers for serialization.
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/config/suffix.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/integral_c_tag.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/level.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>

// Serialization todo:
// - provide alternative overloads of the serialization methods
//   when working with binary archives. This should lead to improved performance
//   at the expense of portability;
// - think about potentially "malicious" archives being loaded. We have some classes that
//   rely on their members to satisfy certain conditions. Examples:
//   - the rational numerator and denominator being coprimes (done),
//   - elements in a symbol_set being ordered (done)
//   - series symbol set inconsistent with contained terms (done),
//   - possibly more...
//   Right now the general policy is to just restore class members whatever their value is.
//   We need to decide ultimately if we want to have a layer of safety in the serialization methods
//   or not. It comes at a price in terms of complexity and performance. A possible middle way would
//   be to enable the safety checks in the text archives but disable them in the binary ones,
//   for performance. The safety layer could be checked by crafting bad text archives and storing
//   them as strings from the tests.
// - Should probably test the exception safety of the serialization routines. Some examples:
//   - serialization of small vector leaves the object in a consistent state if the deserialization
//     of an element fails;
//   - deserialization of a term leaves cf/key in the original state in case of errors;
//   - more...
//   At the moment the routines are coded to provide at least the basic exception safety, but
//   we should test this explicitly eventually.

// Macro for trivial serialization through base class.
#define PIRANHA_SERIALIZE_THROUGH_BASE(base)                                                                           \
    friend class boost::serialization::access;                                                                         \
    template <typename Archive>                                                                                        \
    void serialize(Archive &ar, unsigned int)                                                                          \
    {                                                                                                                  \
        ar &boost::serialization::base_object<base>(*this);                                                            \
    }

// Macro to customize the serialization level for a template class. Adapted from:
// http://www.boost.org/doc/libs/release/libs/serialization/doc/traits.html#tracking
// The exisiting boost macro only covers concrete classes, not generic classes.
#define PIRANHA_TEMPLATE_SERIALIZATION_LEVEL(TClass, L)                                                                \
    namespace boost                                                                                                    \
    {                                                                                                                  \
    namespace serialization                                                                                            \
    {                                                                                                                  \
    template <typename... Args>                                                                                        \
    struct implementation_level_impl<const TClass<Args...>> {                                                          \
        typedef mpl::integral_c_tag tag;                                                                               \
        typedef mpl::int_<L> type;                                                                                     \
        BOOST_STATIC_CONSTANT(int, value = implementation_level_impl::type::value);                                    \
    };                                                                                                                 \
    }                                                                                                                  \
    }

#include "config.hpp"

#if defined(PIRANHA_ENABLE_MSGPACK)

#include <msgpack.hpp>

#if MSGPACK_VERSION_MAJOR < 2

#error Minimum msgpack-c version supported is 2.

#endif

#include <algorithm>
#include <array>
#include <boost/numeric/conversion/cast.hpp>
#include <cmath>
#include <cstddef>
#include <ios>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "detail/sfinae_types.hpp"
#include "detail/symbol_set_fwd.hpp"
#include "exceptions.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

// Scalar types directly supported by msgpack.
template <typename T>
struct is_msgpack_scalar
    : std::integral_constant<bool, std::is_same<char, T>::value || std::is_same<signed char, T>::value
                                       || std::is_same<unsigned char, T>::value || std::is_same<short, T>::value
                                       || std::is_same<unsigned short, T>::value || std::is_same<int, T>::value
                                       || std::is_same<unsigned, T>::value || std::is_same<long, T>::value
                                       || std::is_same<unsigned long, T>::value || std::is_same<long long, T>::value
                                       || std::is_same<unsigned long long, T>::value || std::is_same<float, T>::value
                                       || std::is_same<double, T>::value> {
};

// Wrapper for std stream classes for use in msgpack. The reason for this wrapper is that msgpack expects
// streams with a write(const char *, std::size_t) method, but in std streams the second param is std::streamsize
// (which is a signed int). Hence we wrap the write method to do the safe conversion from size_t to streamsize.
template <typename Stream>
class msgpack_stream_wrapper : public Stream
{
public:
    using Stream::Stream;
    auto write(const typename Stream::char_type *p, std::size_t count)
        -> decltype(std::declval<Stream &>().write(p, boost::numeric_cast<std::streamsize>(count)))
    {
        // NOTE: we need numeric_cast because of circular dep problem if including safe_cast.
        return static_cast<Stream *>(this)->write(p, boost::numeric_cast<std::streamsize>(count));
    }
};
}

/// Detect msgpack stream.
/**
 * This type trait will be \p true if \p T is a type which can be used as template argument to <tt>msgpack::packer</tt>,
 * \p false otherwise. Specifically, in order for this trait to be true \p T must be a non-const, non-reference type
 * with a <tt>write()</tt> method with a signature compatible with
 * @code
 * T::write(const char *, std::size_t);
 * @endcode
 * The return type of the <tt>write()</tt> method is ignored by the type trait.
 */
template <typename T>
class is_msgpack_stream : detail::sfinae_types
{
    template <typename T1>
    static auto test(T1 &t)
        -> decltype(t.write(std::declval<const char *>(), std::declval<std::size_t>()), void(), yes());
    static no test(...);
    static const bool implementation_defined = std::is_same<decltype(test(std::declval<T &>())), yes>::value
                                               && !std::is_reference<T>::value && !std::is_const<T>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T>
const bool is_msgpack_stream<T>::value;

/// Serialization format for msgpack.
/**
 * The serialization of non-primitive objects can often be performed in different ways, with different tradeoffs
 * between performance, storage requirements and portability. The piranha::mp_integer class, for instance, can be
 * serialized either via a string representation (slow and with high storage requirements, but portable across
 * architectures, compilers and operating systems) or as an array of platform-dependent unsigned integrals (fast
 * and compact, but not portable).
 *
 * This enum establishes two strategies for msgpack serialization: a portable format, intended
 * to be usable across different platforms and suitable for the long-term storage of serialized objects, and a binary
 * format, intended for use in high-performance scenarios (e.g., as temporary on-disk storage during long
 * or memory-intensive computations). These formats are used by the serialization functions piranha::msgpack_pack()
 * and piranha::msgpack_convert().
 */
enum class msgpack_format {
    /// Portable.
    portable,
    /// Binary.
    binary
};

template <typename Stream, typename T, typename = void>
struct msgpack_pack_impl {
};

namespace detail
{

template <typename Stream, typename T>
using msgpack_scalar_enabler =
    typename std::enable_if<is_msgpack_stream<Stream>::value && is_msgpack_scalar<T>::value>::type;
}

template <typename Stream, typename T>
struct msgpack_pack_impl<Stream, T, detail::msgpack_scalar_enabler<Stream, T>> {
    void operator()(msgpack::packer<Stream> &packer, const T &x, msgpack_format) const
    {
        packer.pack(x);
    }
};

namespace detail
{

template <typename Stream, typename T>
using msgpack_ld_enabler =
    typename std::enable_if<is_msgpack_stream<Stream>::value && std::is_same<T, long double>::value>::type;
}

template <typename Stream, typename T>
struct msgpack_pack_impl<Stream, T, detail::msgpack_ld_enabler<Stream, T>> {
    void operator()(msgpack::packer<Stream> &packer, const long double &x, msgpack_format f) const
    {
        if (f == msgpack_format::binary) {
            packer.pack_bin(sizeof(long double));
            packer.pack_bin_body(reinterpret_cast<const char *>(&x), sizeof(long double));
        } else {
            if (std::isnan(x)) {
                if (std::signbit(x)) {
                    packer.pack("-nan");
                } else {
                    packer.pack("+nan");
                }
            } else if (std::isinf(x)) {
                if (std::signbit(x)) {
                    packer.pack("-inf");
                } else {
                    packer.pack("+inf");
                }
            } else {
                std::ostringstream oss;
                // Make sure we are using the C locale.
                oss.imbue(std::locale::classic());
                // Use scientific format.
                oss << std::scientific;
                // http://stackoverflow.com/questions/554063/how-do-i-print-a-double-value-with-full-precision-using-cout
                // NOTE: this does not mean that the *exact* value of the long double is printed, just that the
                // value is recovered exactly if reloaded on the same machine. This is a compromise, as the exact
                // printing of the value in string form would take up hundreds of digits. On the other hand, there is a
                // similar situation also for float and double, as there is not guarantee that they conform to IEEE. In
                // the end it seems the only practical approach is to consider all floating-point types as approximate
                // values, subject to various platform/architecture vagaries.
                oss.precision(std::numeric_limits<long double>::max_digits10);
                oss << x;
                packer.pack(oss.str());
            }
        }
    }
};

namespace detail
{

// Enabler for msgpack_pack.
template <typename Stream, typename T>
using msgpack_pack_enabler =
    typename std::enable_if<is_msgpack_stream<Stream>::value
                                && detail::true_tt<decltype(msgpack_pack_impl<Stream, T>{}(
                                       std::declval<msgpack::packer<Stream> &>(), std::declval<const T &>(),
                                       std::declval<msgpack_format>()))>::value,
                            int>::type;
}

template <typename Stream, typename T, detail::msgpack_pack_enabler<Stream, T> = 0>
inline void msgpack_pack(msgpack::packer<Stream> &packer, const T &x, msgpack_format f)
{
    msgpack_pack_impl<Stream, T>{}(packer, x, f);
}

template <typename Stream, typename T, typename = void>
struct msgpack_pack_key_impl {
};

// TODO stream detection, enabler.
template <typename Stream, typename T>
inline void msgpack_pack_key(msgpack::packer<Stream> &packer, const T &x, msgpack_format f, const symbol_set &s)
{
    msgpack_pack_key_impl<Stream, T>{}(packer, x, f, s);
}

template <typename T, typename = void>
struct msgpack_unpack_impl {
};

template <typename T>
struct msgpack_unpack_impl<T, typename std::enable_if<detail::is_msgpack_scalar<T>::value>::type> {
    void operator()(T &x, const msgpack::object &o, msgpack_format) const
    {
        o.convert(x);
    }
};

template <>
struct msgpack_unpack_impl<long double> {
    void operator()(long double &x, const msgpack::object &o, msgpack_format f) const
    {
        using lim = std::numeric_limits<long double>;
        if (f == msgpack_format::binary) {
            std::array<char, sizeof(long double)> tmp;
            o.convert(tmp);
            std::copy(tmp.begin(), tmp.end(), reinterpret_cast<char *>(&x));
        } else {
            std::string tmp;
            o.convert(tmp);
            if (tmp == "+nan") {
                if (lim::has_quiet_NaN) {
                    x = std::copysign(lim::quiet_NaN(), 1.l);
                } else {
                    piranha_throw(std::invalid_argument, "cannot deserialize a NaN if the platform does not support "
                                                         "quiet NaNs");
                }
            } else if (tmp == "-nan") {
                if (lim::has_quiet_NaN) {
                    x = std::copysign(lim::quiet_NaN(), -1.l);
                } else {
                    piranha_throw(std::invalid_argument, "cannot deserialize a NaN if the platform does not support "
                                                         "quiet NaNs");
                }
            } else if (tmp == "+inf") {
                if (lim::has_infinity) {
                    x = lim::infinity();
                } else {
                    piranha_throw(std::invalid_argument, "infinities are not supported by the platform");
                }
            } else if (tmp == "-inf") {
                if (lim::has_infinity) {
                    x = std::copysign(lim::infinity(), -1.l);
                } else {
                    piranha_throw(std::invalid_argument, "infinities are not supported by the platform");
                }
            } else {
                std::istringstream iss;
                iss.imbue(std::locale::classic());
                iss.str(tmp);
                iss >> x;
            }
        }
    }
};

// TODO enabler, T non-const.
template <typename T>
inline void msgpack_unpack(T &x, const msgpack::object &o, msgpack_format f)
{
    msgpack_unpack_impl<T>{}(x, o, f);
}

template <typename T, typename = void>
struct msgpack_unpack_key_impl {
};

// TODO stream detection, enabler.
template <typename T>
inline void msgpack_unpack_key(T &x, const msgpack::object &o, msgpack_format f, const symbol_set &s)
{
    msgpack_unpack_key_impl<T>{}(x, o, f, s);
}

}

#endif // PIRANHA_ENABLE_MSGPACK

#endif

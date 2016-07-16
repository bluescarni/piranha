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

#include <cmath>
#include <ios>
#include <locale>
#include <msgpack.hpp>
#include <sstream>
#include <type_traits>

namespace piranha
{

namespace detail
{

template <typename T>
using msgpack_scalar_enabler = typename std::enable_if<std::is_same<char,T>::value ||
    std::is_same<signed char,T>::value || std::is_same<unsigned char,T>::value ||
    std::is_same<short,T>::value || std::is_same<unsigned short,T>::value ||
    std::is_same<int,T>::value || std::is_same<unsigned,T>::value ||
    std::is_same<long,T>::value || std::is_same<unsigned long,T>::value ||
    std::is_same<long long,T>::value || std::is_same<unsigned long long,T>::value ||
    std::is_same<float,T>::value || std::is_same<double,T>::value
>::type;

}
    
enum class serialization_format {
    portable,
    binary
};

template <typename Stream, typename T, typename = void>
struct msgpack_pack_impl
{};

template <typename Stream, typename T>
struct msgpack_pack_impl<Stream,T,detail::msgpack_scalar_enabler<T>>
{
    void operator()(msgpack::packer<Stream> &packer, const T &x, serialization_format) const
    {
        packer.pack(x);
    }
};

template <typename Stream, typename T>
struct msgpack_pack_impl<Stream,T,typename std::enable_if<std::is_same<T,long double>::value>::type>
{
    void operator()(msgpack::packer<Stream> &packer, const T &x, serialization_format f) const
    {
        if (f == serialization_format::binary) {
            packer.pack_bin(sizeof(T));
            packer.pack_bin_body(reinterpret_cast<const char *>(&x), sizeof(T));
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
                oss.precision(std::numeric_limits<T>::max_digits10);
                oss << x;
                packer.pack(oss.str());
            }
        }
    }
};

// TODO stream detection, enabler.
template <typename Stream, typename T>
inline void msgpack_pack(msgpack::packer<Stream> &packer, const T &x, serialization_format f)
{
    msgpack_pack_impl<Stream,T>{}(packer,x,f);
}

}

#endif

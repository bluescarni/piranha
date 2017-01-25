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

#ifndef PIRANHA_DETAIL_KM_COMMONS_HPP
#define PIRANHA_DETAIL_KM_COMMONS_HPP

#include <algorithm>
#include <set>
#include <stdexcept>
#include <string>

#include <piranha/config.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/math.hpp>
#include <piranha/symbol_set.hpp>
#include <piranha/type_traits.hpp>

// Common routines for use in kronecker monomial classes.

namespace piranha
{
namespace detail
{

template <typename VType, typename KaType, typename T>
inline VType km_unpack(const symbol_set &args, const T &value)
{
    if (unlikely(args.size() > VType::max_size)) {
        piranha_throw(std::invalid_argument, "input set of arguments is too large for unpacking");
    }
    VType retval(static_cast<typename VType::size_type>(args.size()), 0);
    piranha_assert(args.size() == retval.size());
    KaType::decode(retval, value);
    return retval;
}

template <typename VType, typename KaType, typename T>
inline T km_merge_args(const symbol_set &orig_args, const symbol_set &new_args, const T &value)
{
    using size_type = min_int<typename VType::size_type, decltype(new_args.size())>;
    if (unlikely(new_args.size() <= orig_args.size()
                 || !std::includes(new_args.begin(), new_args.end(), orig_args.begin(), orig_args.end()))) {
        piranha_throw(std::invalid_argument, "invalid argument(s) for symbol set merging");
    }
    piranha_assert(std::is_sorted(orig_args.begin(), orig_args.end()));
    piranha_assert(std::is_sorted(new_args.begin(), new_args.end()));
    const auto old_vector = km_unpack<VType, KaType>(orig_args, value);
    VType new_vector;
    auto it_new = new_args.begin();
    for (size_type i = 0u; i < old_vector.size(); ++i, ++it_new) {
        while (*it_new != orig_args[i]) {
            // NOTE: for arbitrary int types, value_type(0) might throw. Update docs
            // if needed.
            new_vector.push_back(T(0));
            piranha_assert(it_new != new_args.end());
            ++it_new;
            piranha_assert(it_new != new_args.end());
        }
        new_vector.push_back(old_vector[i]);
    }
    // Fill up arguments at the tail of new_args but not in orig_args.
    for (; it_new != new_args.end(); ++it_new) {
        new_vector.push_back(T(0));
    }
    piranha_assert(new_vector.size() == new_args.size());
    // Return new encoded value.
    return KaType::encode(new_vector);
}

template <typename VType, typename KaType, typename T>
inline void km_trim_identify(symbol_set &candidates, const symbol_set &args, const T &value)
{
    const VType tmp = km_unpack<VType, KaType>(args, value);
    for (min_int<decltype(tmp.size()), decltype(args.size())> i = 0u; i < tmp.size(); ++i) {
        if (!math::is_zero(tmp[i]) && std::binary_search(candidates.begin(), candidates.end(), args[i])) {
            candidates.remove(args[i]);
        }
    }
}

template <typename VType, typename KaType, typename T>
inline T km_trim(const symbol_set &trim_args, const symbol_set &orig_args, const T &value)
{
    using size_type = min_int<typename VType::size_type, decltype(orig_args.size())>;
    const VType tmp = km_unpack<VType, KaType>(orig_args, value);
    VType new_vector;
    for (size_type i = 0u; i < tmp.size(); ++i) {
        if (!std::binary_search(trim_args.begin(), trim_args.end(), orig_args[i])) {
            new_vector.push_back(tmp[i]);
        }
    }
    return KaType::encode(new_vector);
}
}
}

#endif

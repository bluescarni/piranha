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
#include <vector>

#include <piranha/config.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

// Common routines for use in kronecker monomial classes.

namespace piranha
{
namespace detail
{

template <typename VType, typename KaType, typename T>
inline VType km_unpack(const symbol_fset &args, const T &value)
{
    if (unlikely(args.size() > VType::max_size)) {
        piranha_throw(std::invalid_argument,
                      "the size of the input arguments set (" + std::to_string(args.size())
                          + ") is larger than the maximum allowed size (" + std::to_string(VType::max_size) + ")");
    }
    VType retval(static_cast<typename VType::size_type>(args.size()), 0);
    piranha_assert(args.size() == retval.size());
    KaType::decode(retval, value);
    return retval;
}

template <typename VType, typename KaType, typename T>
inline T km_merge_symbols(const symbol_idx_fmap<symbol_fset> &ins_map, const symbol_fset &args, const T &value)
{
    if (unlikely(!ins_map.size())) {
        // If we have nothing to insert, it means something is wrong: we should never invoke
        // symbol merging if there's nothing to merge.
        piranha_throw(std::invalid_argument,
                      "invalid argument(s) for symbol set merging: the insertion map cannot be empty");
    }
    if (unlikely(ins_map.rbegin()->first > args.size())) {
        // The last element of the insertion map must be at most args.size(), which means that there
        // are symbols to be appended at the end.
        piranha_throw(std::invalid_argument,
                      "invalid argument(s) for symbol set merging: the last index of the insertion map ("
                          + std::to_string(ins_map.rbegin()->first) + ") must not be greater than the key's size ("
                          + std::to_string(args.size()) + ")");
    }
    const auto old_vector = km_unpack<VType, KaType>(args, value);
    VType new_vector;
    auto map_it = ins_map.begin();
    const auto map_end = ins_map.end();
    for (decltype(old_vector.size()) i = 0; i < old_vector.size(); ++i) {
        if (map_it != map_end && map_it->first == i) {
            // NOTE: if we move to TLS vector for unpacking, this can just
            // become a resize (below as well).
            std::fill_n(std::back_inserter(new_vector), map_it->second.size(), T(0));
            ++map_it;
        }
        new_vector.push_back(old_vector[i]);
    }
    // We could still have symbols which need to be appended at the end.
    if (map_it != map_end) {
        std::fill_n(std::back_inserter(new_vector), map_it->second.size(), T(0));
        piranha_assert(map_it + 1 == map_end);
    }
    // Return new encoded value.
    return KaType::encode(new_vector);
}

template <typename VType, typename KaType, typename T>
inline void km_trim_identify(std::vector<char> &candidates, const symbol_fset &args, const T &value)
{
    if (unlikely(candidates.size() != args.size())) {
        piranha_throw(std::invalid_argument,
                      "invalid mask for trim_identify(): the size of the mask (" + std::to_string(candidates.size())
                          + ") differs from the size of the reference symbol set (" + std::to_string(args.size())
                          + ")");
    }
    const VType tmp = km_unpack<VType, KaType>(args, value);
    for (decltype(tmp.size()) i = 0; i < tmp.size(); ++i) {
        if (tmp[i] != T(0)) {
            candidates[static_cast<decltype(candidates.size())>(i)] = 0;
        }
    }
}

template <typename VType, typename KaType, typename T>
inline T km_trim(const std::vector<char> &trim_idx, const symbol_fset &args, const T &value)
{
    if (unlikely(trim_idx.size() != args.size())) {
        piranha_throw(std::invalid_argument,
                      "invalid mask for trim(): the size of the mask (" + std::to_string(trim_idx.size())
                          + ") differs from the size of the reference symbol set (" + std::to_string(args.size())
                          + ")");
    }
    const VType tmp = km_unpack<VType, KaType>(args, value);
    VType new_vector;
    for (decltype(tmp.size()) i = 0; i < tmp.size(); ++i) {
        if (!trim_idx[static_cast<decltype(trim_idx.size())>(i)]) {
            new_vector.push_back(tmp[i]);
        }
    }
    return KaType::encode(new_vector);
}
}
}

#endif

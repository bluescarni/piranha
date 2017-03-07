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

#ifndef PIRANHA_SYMBOL_UTILS_HPP
#define PIRANHA_SYMBOL_UTILS_HPP

#include <algorithm>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <piranha/config.hpp>
#include <piranha/exceptions.hpp>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace piranha
{

/// Flat set of symbols.
/**
 * This data structure represents an ordered set of unique symbols.
 */
using symbol_fset = boost::container::flat_set<std::string>;

/// Symbol index.
/**
 * An unsigned integral type representing a position within a piranha::symbol_fset.
 */
using symbol_idx = symbol_fset::size_type;

/// Flat set of symbol indices.
/**
 * This data structure represents an ordered set of unique indices into a piranha::symbol_fset.
 */
using symbol_idx_fset = boost::container::flat_set<symbol_idx>;

/// Flat map of symbol indices.
/**
 * This sorted data structure maps piranha::symbol_idx instances to the generic type \p T.
 */
template <typename T>
using symbol_idx_fmap = boost::container::flat_map<symbol_idx, T>;

inline namespace impl
{

// This function will do symbol-merging for a vector-like key. Vector must support a minimal std::vector interface,
// and the value type must be constructible from zero.
template <typename Vector>
inline void vector_key_merge_symbols(Vector &retval, const Vector &v, const symbol_idx_fmap<symbol_fset> &ins_map,
                                     const symbol_fset &orig_args)
{
    using value_type = typename Vector::value_type;
    if (unlikely(v.size() != orig_args.size())) {
        // The usual sanity check.
        piranha_throw(std::invalid_argument, "invalid argument(s) for symbol set merging: the size of the original "
                                             "symbol set ("
                                                 + std::to_string(orig_args.size())
                                                 + ") must be equal to the key's size (" + std::to_string(v.size())
                                                 + ")");
    }
    if (unlikely(!ins_map.size())) {
        // If we have nothing to insert, it means something is wrong: we should never invoke
        // symbol merging if there's nothing to merge.
        piranha_throw(std::invalid_argument,
                      "invalid argument(s) for symbol set merging: the insertion map cannot be empty");
    }
    if (unlikely(ins_map.rbegin()->first > v.size())) {
        // The last element of the insertion map must be at most v.size(), which means that there
        // are symbols to be appended at the end.
        piranha_throw(std::invalid_argument,
                      "invalid argument(s) for symbol set merging: the last index of the insertion map ("
                          + std::to_string(ins_map.rbegin()->first) + ") must not be greater than the key's size ("
                          + std::to_string(v.size()) + ")");
    }
    // NOTE: possibility of reserve(), if we ever implement it in static/small vector.
    piranha_assert(retval.size() == 0u);
    auto map_it = ins_map.begin();
    const auto map_end = ins_map.end();
    for (decltype(v.size()) i = 0; i < v.size(); ++i) {
        if (map_it != map_end && map_it->first == i) {
            for (symbol_fset::size_type j = 0; j < map_it->second.size(); ++j) {
                // NOTE: here and below we should really use emplace_back, if we ever implement it
                // in static/small vector.
                retval.push_back(value_type(0));
            }
            ++map_it;
        }
        retval.push_back(v[i]);
    }
    // We could still have symbols which need to be appended at the end.
    if (map_it != map_end) {
        for (symbol_fset::size_type j = 0; j < map_it->second.size(); ++j) {
            retval.push_back(value_type(0));
        }
        piranha_assert(map_it + 1 == map_end);
    }
}
}
}

#endif
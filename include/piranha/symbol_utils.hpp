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

/// Unordered set of symbol indices.
/**
 * This data structure represents an unordered set of unique indices into a piranha::symbol_fset.
 */
using symbol_idx_uset = std::unordered_set<symbol_idx>;

inline namespace impl
{

// This function will do symbol-merging for a vector-like key. Vector must support a minimal std::vector interface,
// and the value type must be constructible from zero. Requires as a precondition that all arguments in
// orig_args are in new_args.
template <typename Vector>
inline Vector vector_key_merge_symbols(const Vector &v, const symbol_fset &orig_args, const symbol_fset &new_args)
{
    // NOTE: check this as a precondition, as it can be rather expensive.
    piranha_check_precondition(std::includes(new_args.begin(), new_args.end(), orig_args.begin(), orig_args.end()));
    using value_type = typename Vector::value_type;
    // NOTE: here and elsewhere (i.e., kronecker keys) the check on new_args.size() <= orig_args.size()
    // is not redundant with the std::includes check; indeed it actually checks that the new args are
    // _more_ than the old args (whereas with just the std::includes check identical orig_args and new_args
    // would be allowed).
    if (unlikely(v.size() != orig_args.size() || new_args.size() <= orig_args.size())) {
        piranha_throw(std::invalid_argument, "invalid argument(s) for symbol set merging: the size of the original "
                                             "symbol set ("
                                                 + std::to_string(orig_args.size())
                                                 + ") must be equal to the key's size(" + std::to_string(v.size())
                                                 + "), and strictly less than "
                                                   "the size of the new symbol set ("
                                                 + std::to_string(new_args.size()) + ")");
    }
    // NOTE: possibility of reserve(), if we ever implement it in static/small vector.
    Vector retval;
    auto it_new = new_args.begin();
    auto it_orig = orig_args.begin();
    for (typename Vector::size_type i = 0; i < v.size(); ++i, ++it_new, ++it_orig) {
        piranha_assert(it_new != new_args.end());
        for (; *it_new != *it_orig; ++it_new) {
            piranha_assert(it_new != new_args.end());
            // NOTE: here and below we should really use emplace_back, if we ever implement it
            // in static/small vector.
            retval.push_back(value_type(0));
        }
        retval.push_back(v[i]);
    }
    // Fill up arguments at the tail of new_args but not in orig_args.
    for (; it_new != new_args.end(); ++it_new) {
        retval.push_back(value_type(0));
    }
    piranha_assert(retval.size() == new_args.size());
    return retval;
}
}
}

#endif
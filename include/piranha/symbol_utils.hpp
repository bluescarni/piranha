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
#include <iterator>
#include <piranha/config.hpp>
#include <piranha/exceptions.hpp>
#include <stdexcept>
#include <string>
#include <tuple>
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
            std::fill_n(std::back_inserter(retval), map_it->second.size(), value_type(0));
            ++map_it;
        }
        retval.push_back(v[i]);
    }
    // We could still have symbols which need to be appended at the end.
    if (map_it != map_end) {
        std::fill_n(std::back_inserter(retval), map_it->second.size(), value_type(0));
        piranha_assert(map_it + 1 == map_end);
    }
}
}

/// Merge symbol sets.
/**
 * This function will merge the input piranha::symbol_fset \p s1 and \p s2, returning a tuple
 * of three elements:
 * - a piranha::symbol_fset representing the union \p u of \p s1 and \p s2,
 * - two <em>insertion maps</em> \p m1 and \p m2 representing the set differences <tt>u\\s1</tt>
 *   and <tt>u\\s2</tt> respectively.
 *
 * The insertion maps contain the indices in \p s1 and \p s2 at which symbols must be added
 * so that \p s1 and \p s2, after the insertion of the symbols in \p m1 and \p m2, become identical to \p u.
 *
 * For example, given the input sets <tt>s1 = ["b", "c", "e"]</tt> and <tt>s2 = ["a", "c", "d", "f", "g"]</tt>,
 * the return values will be:
 * - <tt>u = ["a", "b", "c", "d", "e", "f", "g"]</tt>,
 * - <tt>m1 = [(0, ["a"]), (2, ["d"]), (3, ["f", "g"])]</tt>,
 * - <tt>m2 = [(1, ["b"]), (3, ["e"])]</tt>.
 *
 * @param s1 the first set.
 * @param s2 the second set.
 *
 * @return a triple containing the union \p u of \p s1 and \p s2 and the set differences <tt>u\\s1</tt>
 * and <tt>u\\s2</tt>.
 *
 * @throws unspecified any exception thrown by memory allocation errors.
 */
inline std::tuple<symbol_fset, symbol_idx_fmap<symbol_fset>, symbol_idx_fmap<symbol_fset>>
merge_symbol_fsets(const symbol_fset &s1, const symbol_fset &s2)
{
    symbol_fset u_set;
    // NOTE: the max size of the union is the sum of the two sizes.
    u_set.reserve(static_cast<decltype(u_set.size())>(s1.size() + s2.size()));
    std::set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), std::inserter(u_set, u_set.end()));
    auto compute_map = [&u_set](const symbol_fset &s) {
        symbol_idx_fmap<symbol_fset> retval;
        // NOTE: max size of retval is the original size + 1
        // (there could be an insertion before each and every element of s, plus an
        // insertion at the end).
        retval.reserve(static_cast<decltype(retval.size())>(s.size() + 1u));
        auto u_it = u_set.cbegin();
        for (decltype(s.size()) i = 0; i < s.size(); ++i, ++u_it) {
            piranha_assert(u_it != u_set.end());
            const auto &cur_sym = *s.nth(i);
            if (*u_it < cur_sym) {
                const auto new_it = retval.emplace_hint(retval.end(), i, symbol_fset{*(u_it++)});
                for (; *u_it < cur_sym; ++u_it) {
                    piranha_assert(u_it != u_set.end());
                    new_it->second.emplace_hint(new_it->second.end(), *u_it);
                }
                piranha_assert(*u_it == cur_sym);
            }
        }
        // Insertions at the end.
        if (u_it != u_set.cend()) {
            const auto new_it = retval.emplace_hint(retval.end(), s.size(), symbol_fset{*(u_it++)});
            for (; u_it != u_set.cend(); ++u_it) {
                new_it->second.emplace_hint(new_it->second.end(), *u_it);
            }
        }
        return retval;
    };
    return std::make_tuple(std::move(u_set), compute_map(s1), compute_map(s2));
}

/// Identify the index of a symbol in a set.
/**
 * This function will return the positional index of the symbol ``name``
 * in the set ``s``. If ``name`` is not in ``s``, the size of ``s`` will be returned.
 *
 * @param s the input piranha::symbol_fset.
 * @param name the symbol whose index in ``s`` will be returned.
 *
 * @return the positional index of ``name`` in ``s``.
 */
inline symbol_idx index_of(const symbol_fset &s, const std::string &name)
{
    return s.index_of(s.find(name));
}
}

#endif
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
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/iterator/zip_iterator.hpp>
#include <boost/tuple/tuple.hpp>

#include <piranha/config.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/safe_cast.hpp>

namespace piranha
{

/// Flat set of symbols.
/**
 * This data structure represents an ordered set of symbols.
 */
using symbol_fset = boost::container::flat_set<std::string>;

/// Symbol index.
/**
 * An unsigned integral type representing a position within a \link piranha::symbol_fset symbol_fset\endlink.
 */
using symbol_idx = symbol_fset::size_type;

/// Flat set of symbol indices.
/**
 * This data structure represents an ordered set of indices into a \link piranha::symbol_fset symbol_fset\endlink.
 */
using symbol_idx_fset = boost::container::flat_set<symbol_idx>;

/// Flat map of symbol indices.
/**
 * This sorted data structure maps \link piranha::symbol_idx symbol_idx\endlink instances to the generic type \p T.
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

/// Merge two \link piranha::symbol_fset symbol_fset\endlink.
/**
 * This function will merge the input \link piranha::symbol_fset symbol_fset\endlink \p s1 and \p s2, returning a tuple
 * of three elements:
 * - a \link piranha::symbol_fset symbol_fset\endlink representing the union \p u of \p s1 and \p s2,
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
ss_merge(const symbol_fset &s1, const symbol_fset &s2)
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

/// Identify the index of a symbol in a \link piranha::symbol_fset symbol_fset\endlink.
/**
 * This function will return the positional index of the symbol ``name``
 * in the set ``ref``. If ``name`` is not in ``ref``, the size of ``ref`` will be returned.
 *
 * @param ref the input \link piranha::symbol_fset symbol_fset\endlink.
 * @param name the symbol whose index in ``ref`` will be returned.
 *
 * @return the positional index of ``name`` in ``ref``.
 */
inline symbol_idx ss_index_of(const symbol_fset &ref, const std::string &name)
{
    return ref.index_of(ref.find(name));
}

inline namespace impl
{

// Functor for use in the filter iterator below.
struct mask_ss_filter {
    template <typename Tuple>
    bool operator()(const Tuple &t) const
    {
        // NOTE: the filter iterator skips when this returns false.
        // Since the mask is 1 for elements to be skipped, we need to return
        // true if the mask is zero.
        return t.template get<0>() == char(0);
    }
};

// Functor for use in the transform iterator below.
struct mask_ss_transform {
    template <typename Tuple>
    const std::string &operator()(const Tuple &t) const
    {
        // NOTE: we need to extract the symbol name, which is
        // the second element of the tuple.
        return t.template get<1>();
    }
};
}

/// Trim a \link piranha::symbol_fset symbol_fset\endlink.
/**
 * This function will trim the input set ``s`` according to the values in ``mask``.
 * That is, a copy of ``s`` is returned without the symbols whose corresponding
 * values in ``mask`` are nonzero.
 *
 * For instance, if the input set ``s`` is ``["x", "y", "z"]`` and the input mask is
 * ``[0, 1, 0]``, then the return value of this function is the set ``["x", "z"]`` (i.e., the
 * ``"y"`` symbol was eliminated because its corresponding value in ``mask`` is 1).
 *
 * @param s the input \link piranha::symbol_fset symbol_fset\endlink.
 * @param mask the trimming mask.
 *
 * @return a copy of ``s`` trimmed according to ``mask``.
 *
 * @throws std::invalid_argument if the sizes of ``s`` and ``mask`` differ.
 * @throws unspecified any exception thrown by the construction of the return value.
 */
inline symbol_fset ss_trim(const symbol_fset &s, const std::vector<char> &mask)
{
    if (unlikely(s.size() != mask.size())) {
        // The usual sanity check.
        piranha_throw(std::invalid_argument,
                      "invalid argument(s) for symbol set trimming: the size of the original symbol set ("
                          + std::to_string(s.size()) + ") differs from the size of trimming mask ("
                          + std::to_string(mask.size()) + ")");
    }
    // First we zip together s and mask.
    auto zip_begin = boost::make_zip_iterator(boost::make_tuple(mask.begin(), s.begin()));
    auto zip_end = boost::make_zip_iterator(boost::make_tuple(mask.end(), s.end()));
    // Then we create a filter on the zip: iterate only over those zipped values in which
    // the first element (i.e., the mask) is 0.
    auto filter_begin = boost::make_filter_iterator(mask_ss_filter{}, zip_begin, zip_end);
    auto filter_end = boost::make_filter_iterator(mask_ss_filter{}, zip_end, zip_end);
    // Finally, we need to take the skipping iterator and extract only the second element
    // of each value (the actual symbol name).
    auto trans_begin = boost::make_transform_iterator(filter_begin, mask_ss_transform{});
    auto trans_end = boost::make_transform_iterator(filter_end, mask_ss_transform{});
    // Now we can construct the return value. We know it is ordered because the original
    // symbol set is ordered, and we just skip over some elements.
    return symbol_fset{boost::container::ordered_unique_range_t{}, trans_begin, trans_end};
}

/// Find the indices of the intersection of two \link piranha::symbol_fset symbol_fset\endlink.
/**
 * This function first computes the intersection ``ix`` of the two sets ``s1`` and ``s2``, and then returns
 * a set with the positional indices of ``ix`` in ``s1``.
 *
 * For instance, if ``s1`` is ``["b", "d", "e"]`` and ``s2`` is ``["a", "b", "c", "d", "g"]``,
 * the intersection ``ix`` is ``["b", "d"]`` and the returned set is ``[0, 1]``.
 *
 * @param s1 the first operand.
 * @param s2 the second operand.
 *
 * @return the indices in ``s1`` of the intersection of ``s1`` and ``s2``.
 *
 * @throws std::overflow_error if the size of ``s1`` is larger than an implementation-defined value.
 * @throws unspecified any exception thrown by the public interface of \link piranha::symbol_fset symbol_fset\endlink,
 * or by \link piranha::safe_cast() safe_cast()\endlink.
 */
inline symbol_idx_fset ss_intersect_idx(const symbol_fset &s1, const symbol_fset &s2)
{
    using it_diff_t = decltype(s1.end() - s1.begin());
    using it_udiff_t = std::make_unsigned<it_diff_t>::type;
    // NOTE: let's make sure all the indices in s1 can be represented by the iterator diff type.
    // This makes the computation of s1_it - s1_it_b later safe.
    if (unlikely(s1.size() > static_cast<it_udiff_t>(std::numeric_limits<it_diff_t>::max()))) {
        piranha_throw(
            std::overflow_error,
            "overflow in the determination of the indices the intersection of two symbol_fset: the size of one of the "
            "sets ("
                + std::to_string(s1.size())
                + ") is larger than the maximum value representable by the difference type of symbol_fset's iterators ("
                + std::to_string(std::numeric_limits<it_diff_t>::max()) + ")");
    }
    const auto s1_it_b = s1.begin(), s1_it_f = s1.end();
    auto s1_it = s1_it_b;
    // Use a local vector cache to build the result.
    PIRANHA_MAYBE_TLS std::vector<symbol_idx> vidx;
    // Make sure it's empty before going into the loop.
    vidx.resize(0);
    for (const auto &sym : s2) {
        // Try to locate sym into s1, using s1_it to store the result
        // of the search.
        s1_it = std::lower_bound(s1_it, s1_it_f, sym);
        if (s1_it == s1_it_f) {
            // No symbol >= sym was found in s1, we
            // can break out as no more symbols from s can
            // be found in s1.
            break;
        }
        // Now s1_it points to a symbol which is >= sym.
        if (*s1_it == sym) {
            // We found sym in s1, record its index.
            // NOTE: we know by construction that s1_it - s1_it_b is nonnegative, hence we can
            // cast it safely to the unsigned counterpart. We also know we can compute it safely
            // because we checked earlier. Finally, we need a safe cast in principle as symbol_idx
            // and the unsigned counterpart of it_diff_t might be different (in reality, safe_cast
            // will probably be optimised out).
            vidx.push_back(safe_cast<symbol_idx>(static_cast<it_udiff_t>(s1_it - s1_it_b)));
            // Bump up s1_it: we want to start searching from the next
            // element in the next loop iteration.
            ++s1_it;
        }
    }
    // Build the return value. We know that, by construction, vidx has been built
    // as a sorted vector.
    assert(std::is_sorted(vidx.begin(), vidx.end()));
    return symbol_idx_fset{boost::container::ordered_unique_range_t{}, vidx.begin(), vidx.end()};
}
}

#endif

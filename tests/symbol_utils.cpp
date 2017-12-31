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

#include <piranha/symbol_utils.hpp>

#define BOOST_TEST_MODULE symbol_utils_test
#include <boost/test/included/unit_test.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <initializer_list>
#include <stdexcept>
#include <tuple>
#include <vector>

#include <piranha/init.hpp>

using namespace piranha;

BOOST_AUTO_TEST_CASE(symbol_utils_ss_merge_test)
{
    init();
    // The empty test.
    auto ret = ss_merge(symbol_fset{}, symbol_fset{});
    BOOST_CHECK(std::get<0>(ret).empty());
    BOOST_CHECK(std::get<1>(ret).empty());
    BOOST_CHECK(std::get<2>(ret).empty());
    // Non-empty vs empty.
    ret = ss_merge(symbol_fset{"a", "b", "c"}, symbol_fset{});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c"}));
    BOOST_CHECK(std::get<1>(ret).empty());
    BOOST_CHECK((std::get<2>(ret) == symbol_idx_fmap<symbol_fset>{{0, {"a", "b", "c"}}}));
    // Non-empty vs non-empty.
    ret = ss_merge(symbol_fset{"a", "b", "c"}, symbol_fset{"a", "b", "c"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c"}));
    BOOST_CHECK(std::get<1>(ret).empty());
    BOOST_CHECK((std::get<2>(ret).empty()));
    // Empty vs non-empty.
    ret = ss_merge(symbol_fset{}, symbol_fset{"a", "b", "c"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c"}));
    BOOST_CHECK((std::get<1>(ret) == symbol_idx_fmap<symbol_fset>{{0, {"a", "b", "c"}}}));
    BOOST_CHECK(std::get<2>(ret).empty());
    // Subsets left.
    ret = ss_merge(symbol_fset{"a", "c"}, symbol_fset{"a", "b", "c"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c"}));
    BOOST_CHECK((std::get<1>(ret) == symbol_idx_fmap<symbol_fset>{{1, {"b"}}}));
    BOOST_CHECK(std::get<2>(ret).empty());
    ret = ss_merge(symbol_fset{"a", "b"}, symbol_fset{"a", "b", "c"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c"}));
    BOOST_CHECK((std::get<1>(ret) == symbol_idx_fmap<symbol_fset>{{2, {"c"}}}));
    BOOST_CHECK(std::get<2>(ret).empty());
    ret = ss_merge(symbol_fset{"b", "c"}, symbol_fset{"a", "b", "c"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c"}));
    BOOST_CHECK((std::get<1>(ret) == symbol_idx_fmap<symbol_fset>{{0, {"a"}}}));
    BOOST_CHECK(std::get<2>(ret).empty());
    // Subsets right.
    ret = ss_merge(symbol_fset{"a", "b", "c"}, symbol_fset{"a", "c"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c"}));
    BOOST_CHECK(std::get<1>(ret).empty());
    BOOST_CHECK((std::get<2>(ret) == symbol_idx_fmap<symbol_fset>{{1, {"b"}}}));
    ret = ss_merge(symbol_fset{"a", "b", "c"}, symbol_fset{"a", "b"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c"}));
    BOOST_CHECK(std::get<1>(ret).empty());
    BOOST_CHECK((std::get<2>(ret) == symbol_idx_fmap<symbol_fset>{{2, {"c"}}}));
    ret = ss_merge(symbol_fset{"a", "b", "c"}, symbol_fset{"b", "c"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c"}));
    BOOST_CHECK(std::get<1>(ret).empty());
    BOOST_CHECK((std::get<2>(ret) == symbol_idx_fmap<symbol_fset>{{0, {"a"}}}));
    // Disjoint.
    ret = ss_merge(symbol_fset{"a", "b", "c"}, symbol_fset{"d", "e", "f"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c", "d", "e", "f"}));
    BOOST_CHECK((std::get<1>(ret) == symbol_idx_fmap<symbol_fset>{{3, {"d", "e", "f"}}}));
    BOOST_CHECK((std::get<2>(ret) == symbol_idx_fmap<symbol_fset>{{0, {"a", "b", "c"}}}));
    ret = ss_merge(symbol_fset{"d", "e", "f"}, symbol_fset{"a", "b", "c"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c", "d", "e", "f"}));
    BOOST_CHECK((std::get<1>(ret) == symbol_idx_fmap<symbol_fset>{{0, {"a", "b", "c"}}}));
    BOOST_CHECK((std::get<2>(ret) == symbol_idx_fmap<symbol_fset>{{3, {"d", "e", "f"}}}));
    // Misc.
    ret = ss_merge(symbol_fset{"b", "c", "e"}, symbol_fset{"a", "c", "d", "f", "g"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c", "d", "e", "f", "g"}));
    BOOST_CHECK((std::get<1>(ret) == symbol_idx_fmap<symbol_fset>{{0, {"a"}}, {2, {"d"}}, {3, {"f", "g"}}}));
    BOOST_CHECK((std::get<2>(ret) == symbol_idx_fmap<symbol_fset>{{1, {"b"}}, {3, {"e"}}}));
    ret = ss_merge(symbol_fset{"b", "n", "t", "z"}, symbol_fset{"a", "c", "d", "f", "g", "m", "o", "x"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c", "d", "f", "g", "m", "n", "o", "t", "x", "z"}));
    BOOST_CHECK((std::get<1>(ret)
                 == symbol_idx_fmap<symbol_fset>{{0, {"a"}}, {1, {"c", "d", "f", "g", "m"}}, {2, {"o"}}, {3, {"x"}}}));
    BOOST_CHECK((std::get<2>(ret) == symbol_idx_fmap<symbol_fset>{{1, {"b"}}, {6, {"n"}}, {7, {"t"}}, {8, {"z"}}}));
    ret = ss_merge(symbol_fset{"b", "n", "t"}, symbol_fset{"a", "c", "d", "f", "g", "m", "o", "x"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c", "d", "f", "g", "m", "n", "o", "t", "x"}));
    BOOST_CHECK((std::get<1>(ret)
                 == symbol_idx_fmap<symbol_fset>{{0, {"a"}}, {1, {"c", "d", "f", "g", "m"}}, {2, {"o"}}, {3, {"x"}}}));
    BOOST_CHECK((std::get<2>(ret) == symbol_idx_fmap<symbol_fset>{{1, {"b"}}, {6, {"n"}}, {7, {"t"}}}));
}

BOOST_AUTO_TEST_CASE(symbol_utils_ss_index_of_test)
{
    BOOST_CHECK_EQUAL(ss_index_of(symbol_fset{}, "x"), 0u);
    BOOST_CHECK_EQUAL(ss_index_of(symbol_fset{"x", "y"}, "x"), 0u);
    BOOST_CHECK_EQUAL(ss_index_of(symbol_fset{"x", "y", "z"}, "y"), 1u);
    BOOST_CHECK_EQUAL(ss_index_of(symbol_fset{"x", "y", "z"}, "z"), 2u);
    BOOST_CHECK_EQUAL(ss_index_of(symbol_fset{"x", "y", "z"}, "a"), 3u);
}

BOOST_AUTO_TEST_CASE(symbol_utils_ss_trim_test)
{
    BOOST_CHECK(ss_trim(symbol_fset{}, std::vector<char>{}) == symbol_fset{});
    BOOST_CHECK((ss_trim(symbol_fset{"x", "y", "z"}, {0, 0, 0}) == symbol_fset{"x", "y", "z"}));
    BOOST_CHECK((ss_trim(symbol_fset{"x", "y", "z"}, {0, 1, 0}) == symbol_fset{"x", "z"}));
    BOOST_CHECK((ss_trim(symbol_fset{"x", "y", "z"}, {1, 0, 0}) == symbol_fset{"y", "z"}));
    BOOST_CHECK((ss_trim(symbol_fset{"x", "y", "z"}, {0, 0, 1}) == symbol_fset{"x", "y"}));
    BOOST_CHECK((ss_trim(symbol_fset{"x", "y", "z"}, {1, 0, 1}) == symbol_fset{"y"}));
    BOOST_CHECK((ss_trim(symbol_fset{"x", "y", "z"}, {1, 1, 0}) == symbol_fset{"z"}));
    BOOST_CHECK((ss_trim(symbol_fset{"x", "y", "z"}, {0, 1, 1}) == symbol_fset{"x"}));
    BOOST_CHECK((ss_trim(symbol_fset{"x", "y", "z"}, {1, 1, 1}) == symbol_fset{}));
    BOOST_CHECK_EXCEPTION(ss_trim(symbol_fset{"x", "y", "z"}, {0, 0, 0, 0}), std::invalid_argument,
                          [](const std::invalid_argument &e) {
                              return boost::contains(e.what(), "invalid argument(s) for symbol set trimming: the size "
                                                               "of the original symbol set (3) differs from the size "
                                                               "of trimming mask (4)");
                          });
}

BOOST_AUTO_TEST_CASE(symbol_utils_ss_intersect_idx_test)
{
    BOOST_CHECK(ss_intersect_idx(symbol_fset{}, symbol_fset{}).size() == 0u);
    BOOST_CHECK(ss_intersect_idx(symbol_fset{}, symbol_fset{"a"}).size() == 0u);
    BOOST_CHECK(ss_intersect_idx(symbol_fset{}, symbol_fset{"a", "b", "c"}).size() == 0u);
    BOOST_CHECK(ss_intersect_idx(symbol_fset{"d"}, symbol_fset{"b", "c"}).size() == 0u);
    BOOST_CHECK(ss_intersect_idx(symbol_fset{"a"}, symbol_fset{"b", "c"}).size() == 0u);
    BOOST_CHECK(ss_intersect_idx(symbol_fset{"a"}, symbol_fset{"a", "b", "c"}) == symbol_idx_fset{0});
    BOOST_CHECK(ss_intersect_idx(symbol_fset{"b"}, symbol_fset{"a", "b", "c"}) == symbol_idx_fset{0});
    BOOST_CHECK(ss_intersect_idx(symbol_fset{"c"}, symbol_fset{"a", "b", "c"}) == symbol_idx_fset{0});
    BOOST_CHECK(ss_intersect_idx(symbol_fset{"a", "b", "c"}, symbol_fset{"a"}) == symbol_idx_fset{0});
    BOOST_CHECK(ss_intersect_idx(symbol_fset{"a", "b", "c"}, symbol_fset{"b"}) == symbol_idx_fset{1});
    BOOST_CHECK(ss_intersect_idx(symbol_fset{"a", "b", "c"}, symbol_fset{"c"}) == symbol_idx_fset{2});
    BOOST_CHECK(
        (ss_intersect_idx(symbol_fset{"b", "d", "e"}, symbol_fset{"a", "b", "c", "d", "g"}) == symbol_idx_fset{0, 1}));
    BOOST_CHECK(
        (ss_intersect_idx(symbol_fset{"a", "b", "c", "d", "g"}, symbol_fset{"b", "d", "e"}) == symbol_idx_fset{1, 3}));
    BOOST_CHECK(
        (ss_intersect_idx(symbol_fset{"x", "y", "z"}, symbol_fset{"a", "b", "c", "d", "g"}) == symbol_idx_fset{}));
    BOOST_CHECK(
        (ss_intersect_idx(symbol_fset{"a", "b", "c", "d", "g"}, symbol_fset{"x", "y", "z"}) == symbol_idx_fset{}));
    BOOST_CHECK((ss_intersect_idx(symbol_fset{"a", "b", "e"}, symbol_fset{"c", "d", "g"}) == symbol_idx_fset{}));
    BOOST_CHECK((ss_intersect_idx(symbol_fset{"c", "d", "g"}, symbol_fset{"a", "b", "e"}) == symbol_idx_fset{}));
    BOOST_CHECK((ss_intersect_idx(symbol_fset{"a", "b", "e"}, symbol_fset{"c", "e", "g"}) == symbol_idx_fset{2}));
    BOOST_CHECK((ss_intersect_idx(symbol_fset{"c", "e", "g"}, symbol_fset{"a", "b", "e"}) == symbol_idx_fset{1}));
    BOOST_CHECK((ss_intersect_idx(symbol_fset{"c", "e", "g"}, symbol_fset{"c", "e", "g"}) == symbol_idx_fset{0, 1, 2}));
}

BOOST_AUTO_TEST_CASE(symbol_utils_sm_intersect_idx_test)
{
    using map_t = symbol_fmap<int>;
    BOOST_CHECK(sm_intersect_idx(symbol_fset{}, map_t{}).size() == 0u);
    BOOST_CHECK(sm_intersect_idx(symbol_fset{}, map_t{{"a", 1}}).size() == 0u);
    BOOST_CHECK(sm_intersect_idx(symbol_fset{}, map_t{{"a", 1}, {"b", 2}, {"c", 2}}).size() == 0u);
    BOOST_CHECK(sm_intersect_idx(symbol_fset{"d"}, map_t{{"b", 2}, {"c", 2}}).size() == 0u);
    BOOST_CHECK(sm_intersect_idx(symbol_fset{"a"}, map_t{{"b", 2}, {"c", 2}}).size() == 0u);
    BOOST_CHECK(
        (sm_intersect_idx(symbol_fset{"a"}, map_t{{"a", 1}, {"b", 2}, {"c", 2}}) == symbol_idx_fmap<int>{{0, 1}}));
    BOOST_CHECK(
        (sm_intersect_idx(symbol_fset{"b"}, map_t{{"a", 1}, {"b", 2}, {"c", 2}}) == symbol_idx_fmap<int>{{0, 2}}));
    BOOST_CHECK(
        (sm_intersect_idx(symbol_fset{"c"}, map_t{{"a", 1}, {"b", 2}, {"c", 2}}) == symbol_idx_fmap<int>{{0, 2}}));
    BOOST_CHECK((sm_intersect_idx(symbol_fset{"a", "b", "c"}, map_t{{"a", 1}}) == symbol_idx_fmap<int>{{0, 1}}));
    BOOST_CHECK((sm_intersect_idx(symbol_fset{"a", "b", "c"}, map_t{{"b", 2}}) == symbol_idx_fmap<int>{{1, 2}}));
    BOOST_CHECK((sm_intersect_idx(symbol_fset{"a", "b", "c"}, map_t{{"c", 3}}) == symbol_idx_fmap<int>{{2, 3}}));
    BOOST_CHECK((sm_intersect_idx(symbol_fset{"b", "d", "e"}, map_t{{"a", 1}, {"b", 2}, {"c", 3}, {"d", 4}, {"g", 5}})
                 == symbol_idx_fmap<int>{{0, 2}, {1, 4}}));
    BOOST_CHECK((sm_intersect_idx(symbol_fset{"a", "b", "c", "d", "g"}, map_t{{"b", 1}, {"d", 2}, {"e", 3}})
                 == symbol_idx_fmap<int>{{1, 1}, {3, 2}}));
    BOOST_CHECK((sm_intersect_idx(symbol_fset{"x", "y", "z"}, map_t{{"a", 1}, {"b", 2}, {"c", 3}, {"d", 4}, {"g", 5}})
                 == symbol_idx_fmap<int>{}));
    BOOST_CHECK((sm_intersect_idx(symbol_fset{"a", "b", "c", "d", "g"}, map_t{{"x", 1}, {"y", 2}, {"z", 3}})
                 == symbol_idx_fmap<int>{}));
    BOOST_CHECK(
        (sm_intersect_idx(symbol_fset{"a", "b", "e"}, map_t{{"c", 1}, {"d", 2}, {"g", 3}}) == symbol_idx_fmap<int>{}));
    BOOST_CHECK(
        (sm_intersect_idx(symbol_fset{"c", "d", "g"}, map_t{{"a", 1}, {"b", 2}, {"e", 3}}) == symbol_idx_fmap<int>{}));
    BOOST_CHECK((sm_intersect_idx(symbol_fset{"a", "b", "e"}, map_t{{"c", 1}, {"e", 2}, {"g", 3}})
                 == symbol_idx_fmap<int>{{2, 2}}));
    BOOST_CHECK((sm_intersect_idx(symbol_fset{"c", "e", "g"}, map_t{{"a", 1}, {"b", 2}, {"e", 3}})
                 == symbol_idx_fmap<int>{{1, 3}}));
    BOOST_CHECK((sm_intersect_idx(symbol_fset{"c", "e", "g"}, map_t{{"c", 1}, {"e", 2}, {"g", 3}})
                 == symbol_idx_fmap<int>{{0, 1}, {1, 2}, {2, 3}}));
}

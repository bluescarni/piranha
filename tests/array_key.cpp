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

#include <piranha/array_key.hpp>

#define BOOST_TEST_MODULE array_key_test
#include <boost/test/included/unit_test.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include <piranha/exceptions.hpp>
#include <piranha/init.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

using value_types = std::tuple<char, unsigned, integer>;
using size_types = std::tuple<std::integral_constant<std::size_t, 0u>, std::integral_constant<std::size_t, 1u>,
                              std::integral_constant<std::size_t, 5u>, std::integral_constant<std::size_t, 10u>>;

template <typename T, typename... Args>
class g_key_type : public array_key<T, g_key_type<T, Args...>, Args...>
{
    using base = array_key<T, g_key_type<T, Args...>, Args...>;

public:
    g_key_type() = default;
    g_key_type(const g_key_type &) = default;
    g_key_type(g_key_type &&) = default;
    g_key_type &operator=(const g_key_type &) = default;
    g_key_type &operator=(g_key_type &&) = default;
    template <typename U>
    explicit g_key_type(std::initializer_list<U> list) : base(list)
    {
    }
    template <typename... Args2>
    explicit g_key_type(Args2 &&... params) : base(std::forward<Args2>(params)...)
    {
    }
};

namespace std
{

template <typename T, typename... Args>
struct hash<g_key_type<T, Args...>> : public hash<array_key<T, g_key_type<T, Args...>, Args...>> {
};
}

// Constructors, assignments and element access.
struct constructor_tester {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &) const
        {
            typedef g_key_type<T, U> key_type;
            key_type k0;
            BOOST_CHECK_NO_THROW(key_type tmp = key_type());
            BOOST_CHECK_NO_THROW(key_type tmp = key_type(key_type()));
            BOOST_CHECK_NO_THROW(key_type tmp(k0));
            // From init list.
            key_type k1{T(0), T(1), T(2), T(3)};
            BOOST_CHECK_EQUAL(k1.size(), static_cast<decltype(k1.size())>(4));
            for (typename key_type::size_type i = 0; i < k1.size(); ++i) {
                BOOST_CHECK_EQUAL(k1[i], i);
                BOOST_CHECK_NO_THROW(k1[i] = static_cast<T>(T(i) + T(1)));
                BOOST_CHECK_EQUAL(k1[i], T(i) + T(1));
            }
            key_type k1a{0, 1, 2, 3};
            BOOST_CHECK_EQUAL(k1a.size(), static_cast<decltype(k1a.size())>(4));
            for (typename key_type::size_type i = 0; i < k1a.size(); ++i) {
                BOOST_CHECK_EQUAL(k1a[i], i);
                BOOST_CHECK_NO_THROW(k1a[i] = static_cast<T>(T(i) + T(1)));
                BOOST_CHECK_EQUAL(k1a[i], T(i) + T(1));
            }
            BOOST_CHECK_NO_THROW(k0 = k1);
            BOOST_CHECK_NO_THROW(k0 = std::move(k1));
            // Constructor from vector of symbols.
            symbol_fset vs{"a", "b", "c"};
            key_type k2(vs);
            BOOST_CHECK_EQUAL(k2.size(), vs.size());
            BOOST_CHECK_EQUAL(k2[0], T(0));
            BOOST_CHECK_EQUAL(k2[1], T(0));
            BOOST_CHECK_EQUAL(k2[2], T(0));
            // Generic constructor for use in series.
            BOOST_CHECK_THROW(key_type tmp(k2, symbol_fset{}), std::invalid_argument);
            key_type k3(k2, vs);
            BOOST_CHECK_EQUAL(k3.size(), vs.size());
            BOOST_CHECK_EQUAL(k3[0], T(0));
            BOOST_CHECK_EQUAL(k3[1], T(0));
            BOOST_CHECK_EQUAL(k3[2], T(0));
            key_type k4(key_type(vs), vs);
            BOOST_CHECK_EQUAL(k4.size(), vs.size());
            BOOST_CHECK_EQUAL(k4[0], T(0));
            BOOST_CHECK_EQUAL(k4[1], T(0));
            BOOST_CHECK_EQUAL(k4[2], T(0));
            typedef g_key_type<int, U> key_type2;
            key_type2 k5(vs);
            BOOST_CHECK_THROW(key_type tmp(k5, symbol_fset{}), std::invalid_argument);
            key_type k6(k5, vs);
            BOOST_CHECK_EQUAL(k6.size(), vs.size());
            BOOST_CHECK_EQUAL(k6[0], T(0));
            BOOST_CHECK_EQUAL(k6[1], T(0));
            BOOST_CHECK_EQUAL(k6[2], T(0));
            key_type k7(key_type2(vs), vs);
            BOOST_CHECK_EQUAL(k7.size(), vs.size());
            BOOST_CHECK_EQUAL(k7[0], T(0));
            BOOST_CHECK_EQUAL(k7[1], T(0));
            BOOST_CHECK_EQUAL(k7[2], T(0));
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        tuple_for_each(size_types{}, runner<T>{});
    }
};

BOOST_AUTO_TEST_CASE(array_key_constructor_test)
{
    init();
    tuple_for_each(value_types{}, constructor_tester{});
}

struct hash_tester {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &) const
        {
            typedef g_key_type<T, U> key_type;
            key_type k0;
            BOOST_CHECK_EQUAL(k0.hash(), std::size_t());
            BOOST_CHECK_EQUAL(k0.hash(), std::hash<key_type>()(k0));
            key_type k1{T(0), T(1), T(2), T(3)};
            BOOST_CHECK_EQUAL(k1.hash(), std::hash<key_type>()(k1));
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        tuple_for_each(size_types{}, runner<T>{});
    }
};

BOOST_AUTO_TEST_CASE(array_key_hash_test)
{
    tuple_for_each(value_types{}, hash_tester{});
}

struct push_back_tester {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &) const
        {
            typedef g_key_type<T, U> key_type;
            using size_type = typename key_type::size_type;
            key_type k0;
            for (size_type i = 0u; i < 4u; ++i) {
                k0.push_back(T(i));
                BOOST_CHECK_EQUAL(k0[i], T(i));
            }
            key_type k1;
            for (size_type i = 0u; i < 4u; ++i) {
                T tmp = static_cast<T>(i);
                k1.push_back(tmp);
                BOOST_CHECK_EQUAL(k1[i], tmp);
            }
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        tuple_for_each(size_types{}, runner<T>{});
    }
};

BOOST_AUTO_TEST_CASE(array_key_push_back_test)
{
    tuple_for_each(value_types{}, push_back_tester{});
}

struct equality_tester {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &) const
        {
            typedef g_key_type<T, U> key_type;
            key_type k0;
            BOOST_CHECK(k0 == key_type());
            for (int i = 0; i < 4; ++i) {
                k0.push_back(T(i));
            }
            key_type k1{T(0), T(1), T(2), T(3)};
            BOOST_CHECK(k0 == k1);
            // Inequality.
            k0 = key_type();
            BOOST_CHECK(k0 != k1);
            for (int i = 0; i < 3; ++i) {
                k0.push_back(T(i));
            }
            BOOST_CHECK(k0 != k1);
            k0.push_back(T(3));
            k0.push_back(T());
            BOOST_CHECK(k0 != k1);
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        tuple_for_each(size_types{}, runner<T>{});
    }
};

BOOST_AUTO_TEST_CASE(array_key_equality_test)
{
    tuple_for_each(value_types{}, equality_tester{});
}

struct merge_symbols_tester {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &) const
        {
            using key_type = g_key_type<T, U>;
            key_type k;
            auto out = k.merge_symbols({{0, {"a"}}}, symbol_fset{});
            BOOST_CHECK((std::is_same<decltype(out), key_type>::value));
            BOOST_CHECK_EQUAL(out.size(), 1u);
            BOOST_CHECK_EQUAL(out[0], T(0));
            k.push_back(T(2));
            k.push_back(T(4));
            out = k.merge_symbols({{0, {"a"}}, {1, {"c"}}}, {"b", "d"});
            BOOST_CHECK_EQUAL(out.size(), 4u);
            BOOST_CHECK_EQUAL(out[0], T(0));
            BOOST_CHECK_EQUAL(out[1], T(2));
            BOOST_CHECK_EQUAL(out[2], T(0));
            BOOST_CHECK_EQUAL(out[3], T(4));
            out = k.merge_symbols({{0, {"a"}}, {1, {"c"}}}, {"b", "d"});
            k.push_back(T(5));
            k.push_back(T(7));
            out = k.merge_symbols({{0, {"a"}}, {4, {"h"}}, {3, {"f"}}, {1, {"c"}}}, {"b", "d", "g", "e"});
            BOOST_CHECK_EQUAL(out.size(), 8u);
            BOOST_CHECK_EQUAL(out[0], T(0));
            BOOST_CHECK_EQUAL(out[1], T(2));
            BOOST_CHECK_EQUAL(out[2], T(0));
            BOOST_CHECK_EQUAL(out[3], T(4));
            BOOST_CHECK_EQUAL(out[4], T(5));
            BOOST_CHECK_EQUAL(out[5], T(0));
            BOOST_CHECK_EQUAL(out[6], T(7));
            BOOST_CHECK_EQUAL(out[7], T(0));
            k = key_type{T(2), T(4)};
            out = k.merge_symbols({{0, {"a", "b", "c", "d"}}, {1, {"f"}}, {2, {"h"}}}, {"g", "e"});
            BOOST_CHECK_EQUAL(out.size(), 8u);
            BOOST_CHECK_EQUAL(out[0], T(0));
            BOOST_CHECK_EQUAL(out[1], T(0));
            BOOST_CHECK_EQUAL(out[2], T(0));
            BOOST_CHECK_EQUAL(out[3], T(0));
            BOOST_CHECK_EQUAL(out[4], T(2));
            BOOST_CHECK_EQUAL(out[5], T(0));
            BOOST_CHECK_EQUAL(out[6], T(4));
            BOOST_CHECK_EQUAL(out[7], T(0));
            out = k.merge_symbols({{0, {"a"}}, {1, {"f", "e", "c", "d"}}, {2, {"h"}}}, {"b", "g"});
            BOOST_CHECK_EQUAL(out.size(), 8u);
            BOOST_CHECK_EQUAL(out[0], T(0));
            BOOST_CHECK_EQUAL(out[1], T(2));
            BOOST_CHECK_EQUAL(out[2], T(0));
            BOOST_CHECK_EQUAL(out[3], T(0));
            BOOST_CHECK_EQUAL(out[4], T(0));
            BOOST_CHECK_EQUAL(out[5], T(0));
            BOOST_CHECK_EQUAL(out[6], T(4));
            BOOST_CHECK_EQUAL(out[7], T(0));
            k = key_type{T(2)};
            out = k.merge_symbols({{1, {"f", "g", "h"}}, {0, {"a", "b", "c", "d"}}}, {"e"});
            BOOST_CHECK_EQUAL(out.size(), 8u);
            BOOST_CHECK_EQUAL(out[0], T(0));
            BOOST_CHECK_EQUAL(out[1], T(0));
            BOOST_CHECK_EQUAL(out[2], T(0));
            BOOST_CHECK_EQUAL(out[3], T(0));
            BOOST_CHECK_EQUAL(out[4], T(2));
            BOOST_CHECK_EQUAL(out[5], T(0));
            BOOST_CHECK_EQUAL(out[6], T(0));
            BOOST_CHECK_EQUAL(out[7], T(0));
            k = key_type{};
            out = k.merge_symbols({{0, {"a", "b", "c", "d"}}}, symbol_fset{});
            BOOST_CHECK_EQUAL(out.size(), 4u);
            BOOST_CHECK_EQUAL(out[0], T(0));
            BOOST_CHECK_EQUAL(out[1], T(0));
            BOOST_CHECK_EQUAL(out[2], T(0));
            BOOST_CHECK_EQUAL(out[3], T(0));
            k = key_type{T(2)};
            out = k.merge_symbols({{1, {"c", "d", "e", "f"}}}, {"b"});
            BOOST_CHECK_EQUAL(out.size(), 5u);
            BOOST_CHECK_EQUAL(out[0], T(2));
            BOOST_CHECK_EQUAL(out[1], T(0));
            BOOST_CHECK_EQUAL(out[2], T(0));
            BOOST_CHECK_EQUAL(out[3], T(0));
            BOOST_CHECK_EQUAL(out[4], T(0));
            k = key_type{T(2)};
            out = k.merge_symbols({{0, {"c", "d", "e", "f"}}}, {"g"});
            BOOST_CHECK_EQUAL(out.size(), 5u);
            BOOST_CHECK_EQUAL(out[0], T(0));
            BOOST_CHECK_EQUAL(out[1], T(0));
            BOOST_CHECK_EQUAL(out[2], T(0));
            BOOST_CHECK_EQUAL(out[3], T(0));
            BOOST_CHECK_EQUAL(out[4], T(2));
            // Test from the documentation.
            k = key_type{T(1), T(2), T(3), T(4)};
            out = k.merge_symbols({{0, {"a", "b"}}, {1, {"d"}}, {2, {"f"}}, {4, {"i"}}}, {"c", "e", "g", "h"});
            BOOST_CHECK_EQUAL(out.size(), 9u);
            BOOST_CHECK_EQUAL(out[0], T(0));
            BOOST_CHECK_EQUAL(out[1], T(0));
            BOOST_CHECK_EQUAL(out[2], T(1));
            BOOST_CHECK_EQUAL(out[3], T(0));
            BOOST_CHECK_EQUAL(out[4], T(2));
            BOOST_CHECK_EQUAL(out[5], T(0));
            BOOST_CHECK_EQUAL(out[6], T(3));
            BOOST_CHECK_EQUAL(out[7], T(4));
            BOOST_CHECK_EQUAL(out[8], T(0));
            // Check errors.
            k = key_type{T(2)};
            BOOST_CHECK_EXCEPTION(k.merge_symbols({{0, {"c", "d", "e", "f"}}}, {"g", "h"}), std::invalid_argument,
                                  [](const std::invalid_argument &e) {
                                      return boost::contains(
                                          e.what(),
                                          "invalid argument(s) for symbol set merging: the size of the original "
                                          "symbol set (2) must be equal to the key's size (1)");
                                  });
            BOOST_CHECK_EXCEPTION(
                k.merge_symbols(symbol_idx_fmap<symbol_fset>{}, {"g"}), std::invalid_argument,
                [](const std::invalid_argument &e) {
                    return boost::contains(
                        e.what(), "invalid argument(s) for symbol set merging: the insertion map cannot be empty");
                });
            BOOST_CHECK_EXCEPTION(k.merge_symbols({{2, {"f", "g", "h"}}, {0, {"a", "b", "c", "d"}}}, {"g"}),
                                  std::invalid_argument, [](const std::invalid_argument &e) {
                                      return boost::contains(e.what(), "invalid argument(s) for symbol set merging: "
                                                                       "the last index of the insertion map (2) must "
                                                                       "not be greater than the key's size (1)");
                                  });
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        tuple_for_each(size_types{}, runner<T>{});
    }
};

BOOST_AUTO_TEST_CASE(array_key_merge_symbols_test)
{
    tuple_for_each(value_types{}, merge_symbols_tester{});
}

struct iterators_tester {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &) const
        {
            typedef g_key_type<T, U> key_type;
            key_type k0;
            BOOST_CHECK(k0.begin() == k0.end());
            for (int i = 0; i < 4; ++i) {
                k0.push_back(T(i));
            }
            BOOST_CHECK(k0.begin() + 4 == k0.end());
            BOOST_CHECK(k0.begin() != k0.end());
            const key_type k1 = key_type();
            BOOST_CHECK(k1.begin() == k1.end());
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        tuple_for_each(size_types{}, runner<T>{});
    }
};

BOOST_AUTO_TEST_CASE(array_key_iterators_test)
{
    tuple_for_each(value_types{}, iterators_tester{});
}

struct resize_tester {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &) const
        {
            typedef g_key_type<T, U> key_type;
            key_type k0;
            BOOST_CHECK(k0.size() == 0u);
            k0.resize(1u);
            BOOST_CHECK(k0.size() == 1u);
            k0.resize(10u);
            BOOST_CHECK(k0.size() == 10u);
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        tuple_for_each(size_types{}, runner<T>{});
    }
};

BOOST_AUTO_TEST_CASE(array_key_resize_test)
{
    tuple_for_each(value_types{}, resize_tester{});
}

struct add_tester {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &) const
        {
            typedef g_key_type<T, U> key_type;
            key_type k1, k2, retval;
            k1.vector_add(retval, k2);
            BOOST_CHECK(!retval.size());
            k1.resize(1);
            k2.resize(1);
            k1[0] = 1;
            k2[0] = 2;
            k1.vector_add(retval, k2);
            BOOST_CHECK(retval.size() == 1u);
            BOOST_CHECK(retval[0] == T(3));
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        tuple_for_each(size_types{}, runner<T>{});
    }
};

BOOST_AUTO_TEST_CASE(array_key_add_test)
{
    tuple_for_each(value_types{}, add_tester{});
}

struct sub_tester {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &) const
        {
            typedef g_key_type<T, U> key_type;
            key_type k1, k2, retval;
            k1.vector_sub(retval, k2);
            BOOST_CHECK(!retval.size());
            k1.resize(1);
            k2.resize(1);
            k1[0] = 2;
            k2[0] = 1;
            k1.vector_sub(retval, k2);
            BOOST_CHECK(retval.size() == 1u);
            BOOST_CHECK(retval[0] == T(1));
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        tuple_for_each(size_types{}, runner<T>{});
    }
};

BOOST_AUTO_TEST_CASE(array_key_sub_test)
{
    tuple_for_each(value_types{}, sub_tester{});
}

struct trim_identify_tester {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &) const
        {
            using key_type = g_key_type<T, U>;
            key_type k0;
            symbol_idx_fmap<bool> us;
            k0.resize(1u);
            BOOST_CHECK_EXCEPTION(
                k0.trim_identify(us, symbol_fset{}), std::invalid_argument, [](const std::invalid_argument &e) {
                    return boost::contains(e.what(), "invalid arguments set for trim_identify(): the array "
                                                     "has a size of 1, the reference symbol set has a size of 0");
                });
            BOOST_CHECK_EXCEPTION(
                k0.trim_identify(us, {"a"}), std::invalid_argument, [](const std::invalid_argument &e) {
                    return boost::contains(e.what(), "invalid candidates set for trim_identify(): the size of the "
                                                     "candidates set (0) is different from the size of the reference "
                                                     "symbol set (1)");
                });
            us[2] = true;
            BOOST_CHECK_EXCEPTION(
                k0.trim_identify(us, {"a"}), std::invalid_argument, [](const std::invalid_argument &e) {
                    return boost::contains(e.what(), "invalid candidates set for trim_identify(): the largest index of "
                                                     "the candidates set (2) is greater than the largest index of "
                                                     "the reference symbol set (0)");
                });
            us.clear();
            k0 = key_type{};
            k0.trim_identify(us, symbol_fset{});
            us = {{0, true}, {1, true}, {2, true}};
            k0 = key_type{T(1), T(0), T(2)};
            k0.trim_identify(us, {"a", "b", "c"});
            BOOST_CHECK((us == symbol_idx_fmap<bool>{{0, false}, {1, true}, {2, false}}));
            k0 = key_type{T(1), T(3), T(2)};
            k0.trim_identify(us, {"a", "b", "c"});
            BOOST_CHECK((us == symbol_idx_fmap<bool>{{0, false}, {1, false}, {2, false}}));
            us = {{0, true}, {1, true}, {2, true}};
            k0 = key_type{T(0), T(0), T(0)};
            k0.trim_identify(us, {"a", "b", "c"});
            BOOST_CHECK((us == symbol_idx_fmap<bool>{{0, true}, {1, true}, {2, true}}));
            k0 = key_type{T(0), T(0), T(1)};
            k0.trim_identify(us, {"a", "b", "c"});
            BOOST_CHECK((us == symbol_idx_fmap<bool>{{0, true}, {1, true}, {2, false}}));
            k0 = key_type{T(0), T(0), T(0)};
            k0.trim_identify(us, {"a", "b", "c"});
            BOOST_CHECK((us == symbol_idx_fmap<bool>{{0, true}, {1, true}, {2, false}}));
            k0 = key_type{T(1), T(0), T(0)};
            k0.trim_identify(us, {"a", "b", "c"});
            BOOST_CHECK((us == symbol_idx_fmap<bool>{{0, false}, {1, true}, {2, false}}));
            k0 = key_type{T(0), T(1), T(0)};
            k0.trim_identify(us, {"a", "b", "c"});
            BOOST_CHECK((us == symbol_idx_fmap<bool>{{0, false}, {1, false}, {2, false}}));
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        tuple_for_each(size_types{}, runner<T>{});
    }
};

BOOST_AUTO_TEST_CASE(array_key_trim_identify_test)
{
    tuple_for_each(value_types{}, trim_identify_tester{});
}

struct trim_tester {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &) const
        {
            typedef g_key_type<T, U> key_type;
            key_type k0;
            BOOST_CHECK_THROW(k0.trim(symbol_idx_fset{}, symbol_fset{"x", "y", "z"}), std::invalid_argument);
            k0 = key_type{T(1), T(2), T(3)};
            BOOST_CHECK((k0.trim(symbol_idx_fset{0u}, symbol_fset{"x", "y", "z"}) == key_type{T(2), T(3)}));
            BOOST_CHECK((k0.trim(symbol_idx_fset{0u, 2u, 456u}, symbol_fset{"x", "y", "z"}) == key_type{T(2)}));
            BOOST_CHECK((k0.trim(symbol_idx_fset{0u, 1u, 2u, 456u}, symbol_fset{"x", "y", "z"}) == key_type{}));
            BOOST_CHECK((k0.trim(symbol_idx_fset{1u, 456u}, symbol_fset{"x", "y", "z"}) == key_type{T(1), T(3)}));
            BOOST_CHECK(
                (k0.trim(symbol_idx_fset{1u, 2u, 5u, 67u, 456u}, symbol_fset{"x", "y", "z"}) == key_type{T(1)}));
            BOOST_CHECK((k0.trim(symbol_idx_fset{}, symbol_fset{"x", "y", "z"}) == k0));
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        tuple_for_each(size_types{}, runner<T>{});
    }
};

BOOST_AUTO_TEST_CASE(array_key_trim_test)
{
    tuple_for_each(value_types{}, trim_tester{});
}

struct tt_tester {
    template <typename T>
    struct runner {
        template <typename U>
        void operator()(const U &) const
        {
            typedef g_key_type<T, U> key_type;
            BOOST_CHECK(is_hashable<key_type>::value);
            BOOST_CHECK(is_equality_comparable<key_type>::value);
        }
    };
    template <typename T>
    void operator()(const T &) const
    {
        tuple_for_each(size_types{}, runner<T>{});
    }
};

BOOST_AUTO_TEST_CASE(array_key_type_traits_test)
{

    tuple_for_each(value_types{}, tt_tester{});
}

// Fake value type
struct fvt {
    fvt() = default;
    fvt(int);
    fvt(const fvt &) = default;
    fvt(fvt &&) noexcept;
    fvt &operator=(const fvt &) = default;
    fvt &operator=(fvt &&) noexcept;
    bool operator<(const fvt &) const;
    bool operator==(const fvt &) const;
    bool operator!=(const fvt &) const;
};

// Fake value type with addition operation.
struct fvt2 {
    fvt2() = default;
    fvt2(int);
    fvt2(const fvt2 &) = default;
    fvt2(fvt2 &&) noexcept;
    fvt2 &operator=(const fvt2 &) = default;
    fvt2 &operator=(fvt2 &&) noexcept;
    bool operator<(const fvt2 &) const;
    bool operator==(const fvt2 &) const;
    bool operator!=(const fvt2 &) const;
    fvt2 operator+(const fvt2 &) const;
};

// Fake value type with wrong operation.
struct foo_t {
};

struct fvt3 {
    fvt3() = default;
    fvt3(int);
    fvt3(const fvt3 &) = default;
    fvt3(fvt3 &&) noexcept;
    fvt3 &operator=(const fvt3 &) = default;
    fvt3 &operator=(fvt3 &&) noexcept;
    bool operator<(const fvt3 &) const;
    bool operator==(const fvt3 &) const;
    bool operator!=(const fvt3 &) const;
    foo_t operator+(const fvt3 &) const;
};

namespace std
{

template <>
struct hash<fvt> {
    typedef size_t result_type;
    typedef fvt argument_type;
    result_type operator()(const argument_type &) const;
};

template <>
struct hash<fvt2> {
    typedef size_t result_type;
    typedef fvt2 argument_type;
    result_type operator()(const argument_type &) const;
};

template <>
struct hash<fvt3> {
    typedef size_t result_type;
    typedef fvt3 argument_type;
    result_type operator()(const argument_type &) const;
};
}

template <typename T>
using add_type = decltype(std::declval<T const &>().vector_add(std::declval<T &>(), std::declval<T const &>()));

template <typename T, typename = void>
struct has_add {
    static const bool value = false;
};

template <typename T>
struct has_add<T, typename std::enable_if<std::is_same<add_type<T>, add_type<T>>::value>::type> {
    static const bool value = true;
};

struct ae_tester {
    template <typename T>
    void operator()(const T &) const
    {
        typedef g_key_type<fvt, T> key_type;
        BOOST_CHECK(!has_add<key_type>::value);
        typedef g_key_type<fvt2, T> key_type2;
        BOOST_CHECK(has_add<key_type2>::value);
        typedef g_key_type<fvt3, T> key_type3;
        BOOST_CHECK(!has_add<key_type3>::value);
    }
};

BOOST_AUTO_TEST_CASE(array_key_add_enabler_test)
{

    tuple_for_each(size_types{}, ae_tester{});
}

struct sbe_tester {
    template <typename T>
    void operator()(const T &) const
    {
        typedef g_key_type<int, T> key_type;
        key_type tmp;
        auto sbe1 = tmp.size_begin_end();
        BOOST_CHECK((std::is_same<decltype(sbe1), std::tuple<typename key_type::size_type, int *, int *>>::value));
        BOOST_CHECK_EQUAL(std::get<0>(sbe1), 0u);
        BOOST_CHECK(std::get<1>(sbe1) == std::get<2>(sbe1));
        auto sbe2 = static_cast<const key_type &>(tmp).size_begin_end();
        BOOST_CHECK(
            (std::is_same<decltype(sbe2), std::tuple<typename key_type::size_type, int const *, int const *>>::value));
        BOOST_CHECK_EQUAL(std::get<0>(sbe2), 0u);
        BOOST_CHECK(std::get<1>(sbe2) == std::get<2>(sbe2));
        key_type k0({1, 2, 3, 4, 5});
        auto sbe3 = k0.size_begin_end();
        BOOST_CHECK_EQUAL(std::get<0>(sbe3), 5u);
        BOOST_CHECK(std::get<1>(sbe3) + 5 == std::get<2>(sbe3));
        auto sbe4 = static_cast<const key_type &>(k0).size_begin_end();
        BOOST_CHECK_EQUAL(std::get<0>(sbe4), 5u);
        BOOST_CHECK(std::get<1>(sbe4) + 5 == std::get<2>(sbe4));
    }
};

BOOST_AUTO_TEST_CASE(array_key_sbe_test)
{

    tuple_for_each(size_types{}, sbe_tester{});
}

struct subscript_tester {
    template <typename T>
    void operator()(const T &) const
    {
        typedef g_key_type<int, T> key_type;
        key_type k0;
        k0.push_back(0);
        BOOST_CHECK_EQUAL(k0[0], 0);
        BOOST_CHECK_EQUAL(static_cast<key_type const &>(k0)[0], 0);
    }
};

BOOST_AUTO_TEST_CASE(array_key_subscript_test)
{

    tuple_for_each(size_types{}, subscript_tester{});
}

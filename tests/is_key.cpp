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

#include <piranha/is_key.hpp>

#define BOOST_TEST_MODULE is_key_test
#include <boost/test/included/unit_test.hpp>

#include <cstddef>
#include <functional>
#include <iostream>
#include <piranha/init.hpp>
#include <piranha/symbol_utils.hpp>
#include <vector>

using namespace piranha;

struct key01 {
};

struct key02 {
    key02() = default;
    key02(const key02 &) = default;
    key02(key02 &&) noexcept;
    key02 &operator=(const key02 &) = default;
    key02 &operator=(key02 &&) noexcept;
    key02(const symbol_fset &);
    bool operator==(const key02 &) const;
    bool operator!=(const key02 &) const;
    bool is_compatible(const symbol_fset &) const noexcept;
    bool is_zero(const symbol_fset &) const noexcept;
    key02 merge_symbols(const symbol_idx_fmap<symbol_fset> &, const symbol_fset &) const;
    bool is_unitary(const symbol_fset &) const;
    void print(std::ostream &, const symbol_fset &) const;
    void print_tex(std::ostream &, const symbol_fset &) const;
    void trim_identify(std::vector<char> &, const symbol_fset &) const;
    key02 trim(const std::vector<char> &, const symbol_fset &) const;
};

struct key03 {
    key03() = default;
    key03(const key03 &) = default;
    key03(key03 &&) noexcept;
    key03 &operator=(const key03 &) = default;
    key03 &operator=(key03 &&) noexcept;
    key03(const symbol_fset &);
    bool operator==(const key03 &) const;
    bool operator!=(const key03 &) const;
    bool is_compatible(const symbol_fset &) const noexcept;
    bool is_zero(const symbol_fset &) const noexcept;
    key03 merge_symbols(const symbol_idx_fmap<symbol_fset> &, const symbol_fset &) const;
    bool is_unitary(const symbol_fset &) const;
    void print(std::ostream &, const symbol_fset &) const;
    void print_tex(std::ostream &, const symbol_fset &) const;
};

struct key04 {
    key04() = default;
    key04(const key04 &) = default;
    key04(key04 &&) noexcept(false);
    key04 &operator=(const key04 &) = default;
    key04 &operator=(key04 &&) noexcept;
    key04(const symbol_fset &);
    bool operator==(const key04 &) const;
    bool operator!=(const key04 &) const;
    bool is_compatible(const symbol_fset &) const noexcept;
    bool is_zero(const symbol_fset &) const noexcept;
    key04 merge_symbols(const symbol_idx_fmap<symbol_fset> &, const symbol_fset &) const;
    bool is_unitary(const symbol_fset &) const;
    void print(std::ostream &, const symbol_fset &) const;
    void print_tex(std::ostream &, const symbol_fset &) const;
};

struct key05 {
    key05() = default;
    key05(const key05 &) = default;
    key05(key05 &&) noexcept;
    key05 &operator=(const key05 &) = default;
    key05 &operator=(key05 &&) noexcept;
    key05(const symbol_fset &);
    bool operator==(const key05 &) const;
    bool operator!=(const key05 &) const;
    bool is_compatible(const symbol_fset &) const;
    bool is_zero(const symbol_fset &) const noexcept;
    key05 merge_symbols(const symbol_idx_fmap<symbol_fset> &, const symbol_fset &) const;
    bool is_unitary(const symbol_fset &) const;
    void print(std::ostream &, const symbol_fset &) const;
    void print_tex(std::ostream &, const symbol_fset &) const;
};

struct key06 {
    key06() = default;
    key06(const key06 &) = default;
    key06(key06 &&) noexcept;
    key06 &operator=(const key06 &) = default;
    key06 &operator=(key06 &&) noexcept;
    key06(const symbol_fset &);
    bool operator==(const key06 &) const;
    bool operator!=(const key06 &) const;
    bool is_compatible(const symbol_fset &) const noexcept;
    bool is_zero(const symbol_fset &) const;
    key06 merge_symbols(const symbol_idx_fmap<symbol_fset> &, const symbol_fset &) const;
    bool is_unitary(const symbol_fset &) const;
    void print(std::ostream &, const symbol_fset &) const;
    void print_tex(std::ostream &, const symbol_fset &) const;
};

struct key07 {
    key07() = default;
    key07(const key07 &) = default;
    key07(key07 &&) noexcept;
    key07 &operator=(const key07 &) = default;
    key07 &operator=(key07 &&) noexcept;
    key07(const symbol_fset &);
    bool operator==(const key07 &) const;
    bool operator!=(const key07 &) const;
    bool is_compatible(const symbol_fset &) const noexcept;
    bool is_zero(const symbol_fset &) const noexcept;
    key07 merge_symbols(const symbol_idx_fmap<symbol_fset> &, const symbol_fset &) const;
    void print(std::ostream &, const symbol_fset &) const;
    void print_tex(std::ostream &, const symbol_fset &) const;
};

struct key08 {
    key08() = default;
    key08(const key08 &) = default;
    key08(key08 &&) noexcept;
    key08 &operator=(const key08 &) = default;
    key08 &operator=(key08 &&) noexcept;
    key08(const symbol_fset &);
    bool operator==(const key08 &) const;
    bool operator!=(const key08 &) const;
    bool is_compatible(const symbol_fset &) const noexcept;
    bool is_zero(const symbol_fset &) const noexcept;
    key08 merge_symbols(const symbol_idx_fmap<symbol_fset> &, const symbol_fset &) const;
    bool is_unitary(symbol_fset &) const;
    void print(std::ostream &, const symbol_fset &) const;
    void print_tex(std::ostream &, const symbol_fset &) const;
};

namespace std
{

template <>
struct hash<key02> {
    std::size_t operator()(const key02 &) const;
};

template <>
struct hash<key03> {
};

template <>
struct hash<key04> {
    std::size_t operator()(const key04 &) const;
};

template <>
struct hash<key05> {
    std::size_t operator()(const key05 &) const;
};

template <>
struct hash<key06> {
    std::size_t operator()(const key06 &) const;
};

template <>
struct hash<key07> {
    std::size_t operator()(const key07 &) const;
};

template <>
struct hash<key08> {
    std::size_t operator()(const key08 &) const;
};
}

BOOST_AUTO_TEST_CASE(is_key_test_00)
{
    init();
    BOOST_CHECK(!is_key<void>::value);
    BOOST_CHECK(!is_key<int>::value);
    BOOST_CHECK(!is_key<double>::value);
    BOOST_CHECK(!is_key<long *>::value);
    BOOST_CHECK(!is_key<long &>::value);
    BOOST_CHECK(!is_key<long const &>::value);
    BOOST_CHECK(!is_key<key01>::value);
    BOOST_CHECK(!is_key<const key01 &>::value);
    BOOST_CHECK(is_key<key02>::value);
    BOOST_CHECK(!is_key<key02 &>::value);
    BOOST_CHECK(!is_key<key02 &&>::value);
    BOOST_CHECK(!is_key<const key02>::value);
    BOOST_CHECK(!is_key<const key02 &>::value);
    BOOST_CHECK(!is_key<const key02 &&>::value);
    BOOST_CHECK(!is_key<key03>::value);
    BOOST_CHECK(!is_key<key04>::value);
    BOOST_CHECK(!is_key<key05>::value);
    BOOST_CHECK(!is_key<key06>::value);
    BOOST_CHECK(!is_key<key07>::value);
    BOOST_CHECK(!is_key<key08>::value);
}

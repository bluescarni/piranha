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

#include <piranha/is_cf.hpp>

#define BOOST_TEST_MODULE is_cf_test
#include <boost/test/included/unit_test.hpp>

#include <complex>
#include <iostream>
#include <type_traits>

#include <piranha/config.hpp>

struct cf01 {
};

struct cf02 {
    cf02();
    cf02(const int &);
    cf02(const cf02 &);
    cf02(cf02 &&) noexcept;
    cf02 &operator=(const cf02 &);
    cf02 &operator=(cf02 &&) noexcept;
    friend std::ostream &operator<<(std::ostream &, const cf02 &);
    cf02 operator-() const;
    bool operator==(const cf02 &) const;
    bool operator!=(const cf02 &) const;
    cf02 &operator+=(const cf02 &);
    cf02 &operator-=(const cf02 &);
    cf02 operator+(const cf02 &) const;
    cf02 operator-(const cf02 &) const;
};

struct cf03 {
    cf03();
    cf03(const int &);
    cf03(const cf03 &);
    cf03(cf03 &&) noexcept;
    cf03 &operator=(const cf03 &);
    cf03 &operator=(cf03 &&) noexcept;
    friend std::ostream &operator<<(std::ostream &, const cf03 &);
    bool operator==(const cf03 &) const;
    bool operator!=(const cf03 &) const;
    cf03 &operator+=(const cf03 &);
    cf03 &operator-=(const cf03 &);
    cf03 operator+(const cf03 &) const;
    cf03 operator-(const cf03 &) const;
};

struct cf04 {
    cf04();
    cf04(const int &);
    cf04(const cf04 &);
    cf04(cf04 &&) noexcept;
    cf04 &operator=(const cf04 &);
    cf04 &operator=(cf04 &&) noexcept;
    friend std::ostream &operator<<(std::ostream &, const cf04 &);
    cf04 operator-() const;
    cf04 &operator+=(const cf04 &);
    cf04 &operator-=(const cf04 &);
    cf04 operator+(const cf04 &) const;
    cf04 operator-(const cf04 &) const;
};

struct cf05 {
    cf05();
    cf05(const cf05 &);
    cf05(cf05 &&) noexcept;
    cf05 &operator=(const cf05 &);
    cf05 &operator=(cf05 &&) noexcept;
    friend std::ostream &operator<<(std::ostream &, const cf05 &);
    cf05 operator-() const;
    bool operator==(const cf05 &) const;
    bool operator!=(const cf05 &) const;
    cf05 &operator+=(const cf05 &);
    cf05 &operator-=(const cf05 &);
    cf05 operator+(const cf05 &) const;
    cf05 operator-(const cf05 &) const;
};

struct cf06 {
    cf06();
    cf06(const int &);
    cf06(const cf06 &);
    cf06(cf06 &&) noexcept(false);
    cf06 &operator=(const cf06 &);
    cf06 &operator=(cf06 &&) noexcept;
    friend std::ostream &operator<<(std::ostream &, const cf06 &);
    cf06 operator-() const;
    bool operator==(const cf06 &) const;
    bool operator!=(const cf06 &) const;
    cf06 &operator+=(const cf06 &);
    cf06 &operator-=(const cf06 &);
    cf06 operator+(const cf06 &) const;
    cf06 operator-(const cf06 &) const;
};

struct cf07 {
    cf07();
    cf07(const int &);
    cf07(const cf07 &);
    cf07(cf07 &&) noexcept(false);
    cf07 &operator=(const cf07 &);
    cf07 &operator=(cf07 &&) noexcept;
    friend std::ostream &operator<<(std::ostream &, const cf07 &);
    cf07 operator-() const;
    bool operator==(const cf07 &) const;
    bool operator!=(const cf07 &) const;
    cf07 &operator+=(const cf07 &);
    cf07 &operator-=(const cf07 &);
    cf07 operator+(const cf07 &) const;
    cf07 operator-(const cf07 &) const;
};

namespace piranha
{

template <>
struct enable_noexcept_checks<cf07> : std::false_type {
};
}

using namespace piranha;

BOOST_AUTO_TEST_CASE(is_cf_test_00)
{
    BOOST_CHECK(is_cf<int>::value);
    BOOST_CHECK(is_cf<double>::value);
    BOOST_CHECK(is_cf<long double>::value);
    BOOST_CHECK(is_cf<std::complex<double>>::value);
    // NOTE: the checks on the pointers here produce warnings in clang. The reason is that
    // ptr1 -= ptr2
    // expands to
    // ptr1 = ptr1 - ptr2
    // where ptr1 - ptr2 is some integer value (due to pointer arith.) that then gets assigned back to a pointer. It is
    // not
    // entirely clear if this should be a hard error (GCC) or just a warning (clang) so for now it is better to simply
    // disable
    // the check. Note that the same problem would be in is_subtractable_in_place if we checked for pointers there.
    // BOOST_CHECK(!is_cf<double *>::value);
    // BOOST_CHECK(!is_cf<double const *>::value);
    BOOST_CHECK(!is_cf<int &>::value);
    BOOST_CHECK(!is_cf<int const &>::value);
    BOOST_CHECK(!is_cf<int const &>::value);
    BOOST_CHECK(!is_cf<cf01>::value);
    BOOST_CHECK(is_cf<cf02>::value);
    // BOOST_CHECK(!is_cf<cf02 *>::value);
    BOOST_CHECK(!is_cf<cf02 &&>::value);
    BOOST_CHECK(!is_cf<cf03>::value);
    BOOST_CHECK(!is_cf<cf04>::value);
    BOOST_CHECK(!is_cf<cf05>::value);
// Missing noexcept.
#if !defined(PIRANHA_COMPILER_IS_INTEL)
    BOOST_CHECK(!is_cf<cf06>::value);
#endif
    BOOST_CHECK(is_cf<cf07>::value);
}

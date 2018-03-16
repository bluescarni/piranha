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

#include <piranha/type_traits.hpp>

#define BOOST_TEST_MODULE type_traits_test
#include <boost/test/included/unit_test.hpp>

#include <piranha/config.hpp>

#include <complex>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <ios>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#if PIRANHA_CPLUSPLUS >= 201703L
#include <string_view>
#endif
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

using namespace piranha;

BOOST_AUTO_TEST_CASE(type_traits_is_nonconst_rvalue_ref_test)
{
    BOOST_CHECK_EQUAL(is_nonconst_rvalue_ref<int>::value, false);
    BOOST_CHECK_EQUAL(is_nonconst_rvalue_ref<int &>::value, false);
    BOOST_CHECK_EQUAL(is_nonconst_rvalue_ref<const int>::value, false);
    BOOST_CHECK_EQUAL(is_nonconst_rvalue_ref<const volatile int>::value, false);
    BOOST_CHECK_EQUAL(is_nonconst_rvalue_ref<const volatile int &>::value, false);
    BOOST_CHECK_EQUAL(is_nonconst_rvalue_ref<volatile int>::value, false);
    BOOST_CHECK_EQUAL(is_nonconst_rvalue_ref<volatile int &&>::value, true);
    BOOST_CHECK_EQUAL(is_nonconst_rvalue_ref<volatile int const &&>::value, false);
    BOOST_CHECK_EQUAL(is_nonconst_rvalue_ref<const int &&>::value, false);
    BOOST_CHECK_EQUAL(is_nonconst_rvalue_ref<int &&>::value, true);
}

struct trivial {
};

struct nontrivial_copy {
    nontrivial_copy(nontrivial_copy &&) noexcept(false) {}
    nontrivial_copy &operator=(nontrivial_copy &&) noexcept(false)
    {
        return *this;
    }
    nontrivial_copy(const nontrivial_copy &other) : n(other.n) {}
    int n;
};

struct trivial_copy {
    trivial_copy(trivial_copy &&) = default;
    trivial_copy(const trivial_copy &) = default;
    int n;
};

struct nontrivial_dtor {
    nontrivial_dtor(const nontrivial_dtor &) = default;
    nontrivial_dtor(nontrivial_dtor &&) noexcept(false) {}
    nontrivial_dtor &operator=(nontrivial_dtor &&) noexcept(false)
    {
        return *this;
    }
    ~nontrivial_dtor() noexcept(false)
    {
        n = 0;
    }
    int n;
};

BOOST_AUTO_TEST_CASE(type_traits_is_addable_test)
{
    BOOST_CHECK(is_addable<int>::value);
    BOOST_CHECK(!is_addable<void>::value);
    BOOST_CHECK(is_addable<const int>::value);
    BOOST_CHECK((is_addable<const int, int>::value));
    BOOST_CHECK((is_addable<int, const int>::value));
    BOOST_CHECK((is_addable<const int &, int &>::value));
    BOOST_CHECK((is_addable<int &&, const int &>::value));
    BOOST_CHECK(is_addable<double>::value);
    BOOST_CHECK((!is_addable<double, void>::value));
    BOOST_CHECK((!is_addable<void, double>::value));
    BOOST_CHECK(is_addable<std::complex<double>>::value);
    BOOST_CHECK((is_addable<const std::complex<double>, double>::value));
    BOOST_CHECK((is_addable<std::complex<double>, const double>::value));
    BOOST_CHECK((is_addable<int, int>::value));
    BOOST_CHECK((is_addable<int, double>::value));
    BOOST_CHECK((is_addable<double, int>::value));
    BOOST_CHECK((is_addable<std::complex<double>, double>::value));
    BOOST_CHECK((is_addable<double, std::complex<double>>::value));
    BOOST_CHECK((!is_addable<trivial, std::complex<double>>::value));
// The Intel compiler seems to have some nonstandard extensions to the complex class.
#if !defined(PIRANHA_COMPILER_IS_INTEL)
    BOOST_CHECK((!is_addable<int, std::complex<double>>::value));
    BOOST_CHECK((!is_addable<std::complex<double>, int>::value));
#endif
    BOOST_CHECK((is_addable<std::string, std::string>::value));
    BOOST_CHECK((is_addable<std::string, const char *>::value));
    BOOST_CHECK((is_addable<const char *, std::string>::value));
    BOOST_CHECK((is_addable<int *, std::size_t>::value));
    BOOST_CHECK((is_addable<std::size_t, int *>::value));
    BOOST_CHECK(!is_addable<int *>::value);
    BOOST_CHECK(is_addable<int &>::value);
    BOOST_CHECK((is_addable<int &, double &>::value));
    BOOST_CHECK((is_addable<double &, int &>::value));
    BOOST_CHECK(is_addable<int const &>::value);
    BOOST_CHECK((is_addable<int const &, double &>::value));
    BOOST_CHECK((is_addable<double const &, int &>::value));
    BOOST_CHECK(is_addable<int &&>::value);
    BOOST_CHECK((is_addable<int &&, double &&>::value));
    BOOST_CHECK((is_addable<double &&, int &&>::value));
    BOOST_CHECK((!is_addable<int &&, std::string &>::value));
    BOOST_CHECK((is_addable<int *&, int>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_is_addable_in_place_test)
{
    BOOST_CHECK((!is_addable_in_place<void>::value));
    BOOST_CHECK((!is_addable_in_place<void, int>::value));
    BOOST_CHECK((!is_addable_in_place<int, void>::value));
    BOOST_CHECK((is_addable_in_place<int>::value));
    BOOST_CHECK((is_addable_in_place<int, int>::value));
    BOOST_CHECK((is_addable_in_place<int, double>::value));
    BOOST_CHECK((is_addable_in_place<double, int>::value));
    BOOST_CHECK((is_addable_in_place<std::complex<double>, double>::value));
    BOOST_CHECK((!is_addable_in_place<double, std::complex<double>>::value));
    BOOST_CHECK((!is_addable_in_place<trivial, std::complex<double>>::value));
    BOOST_CHECK((is_addable_in_place<std::string, std::string>::value));
    BOOST_CHECK((is_addable_in_place<int, const int>::value));
    BOOST_CHECK((!is_addable_in_place<const int, int>::value));
    BOOST_CHECK((!is_addable_in_place<const int &, int>::value));
    BOOST_CHECK((is_addable_in_place<int &&, const int &>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_is_subtractable_test)
{
    BOOST_CHECK((!is_subtractable<void>::value));
    BOOST_CHECK((!is_subtractable<void, int>::value));
    BOOST_CHECK((!is_subtractable<int, void>::value));
    BOOST_CHECK(is_subtractable<int>::value);
    BOOST_CHECK(is_subtractable<const int>::value);
    BOOST_CHECK((is_subtractable<const int, int>::value));
    BOOST_CHECK((is_subtractable<int, const int>::value));
    BOOST_CHECK((is_subtractable<const int &, int &>::value));
    BOOST_CHECK((is_subtractable<int &&, const int &>::value));
    BOOST_CHECK(is_subtractable<double>::value);
    BOOST_CHECK(is_subtractable<std::complex<double>>::value);
    BOOST_CHECK((is_subtractable<const std::complex<double>, double>::value));
    BOOST_CHECK((is_subtractable<std::complex<double>, const double>::value));
    BOOST_CHECK((is_subtractable<int, int>::value));
    BOOST_CHECK((is_subtractable<int, double>::value));
    BOOST_CHECK((is_subtractable<double, int>::value));
    BOOST_CHECK((is_subtractable<std::complex<double>, double>::value));
    BOOST_CHECK((is_subtractable<double, std::complex<double>>::value));
    BOOST_CHECK((!is_subtractable<trivial, std::complex<double>>::value));
// Same as above.
#if !defined(PIRANHA_COMPILER_IS_INTEL)
    BOOST_CHECK((!is_subtractable<int, std::complex<double>>::value));
    BOOST_CHECK((!is_subtractable<std::complex<double>, int>::value));
#endif
    BOOST_CHECK((!is_subtractable<std::string, std::string>::value));
    BOOST_CHECK((!is_subtractable<std::string, const char *>::value));
    BOOST_CHECK((!is_subtractable<const char *, std::string>::value));
    BOOST_CHECK((is_subtractable<int *, std::size_t>::value));
    BOOST_CHECK((!is_subtractable<std::size_t, int *>::value));
    BOOST_CHECK(is_subtractable<int *>::value);
    BOOST_CHECK(is_subtractable<int &>::value);
    BOOST_CHECK((is_subtractable<int &, double &>::value));
    BOOST_CHECK((is_subtractable<double &, int &>::value));
    BOOST_CHECK(is_subtractable<int const &>::value);
    BOOST_CHECK((is_subtractable<int const &, double &>::value));
    BOOST_CHECK((is_subtractable<double const &, int &>::value));
    BOOST_CHECK(is_subtractable<int &&>::value);
    BOOST_CHECK((is_subtractable<int &&, double &&>::value));
    BOOST_CHECK((is_subtractable<double &&, int &&>::value));
    BOOST_CHECK((!is_subtractable<int &&, std::string &>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_is_subtractable_in_place_test)
{
    BOOST_CHECK((!is_subtractable_in_place<void>::value));
    BOOST_CHECK((!is_subtractable_in_place<void, int>::value));
    BOOST_CHECK((!is_subtractable_in_place<int, void>::value));
    BOOST_CHECK((is_subtractable_in_place<int>::value));
    BOOST_CHECK((is_subtractable_in_place<int, int>::value));
    BOOST_CHECK((is_subtractable_in_place<int, double>::value));
    BOOST_CHECK((is_subtractable_in_place<double, int>::value));
    BOOST_CHECK((is_subtractable_in_place<std::complex<double>, double>::value));
    BOOST_CHECK((!is_subtractable_in_place<double, std::complex<double>>::value));
    BOOST_CHECK((!is_subtractable_in_place<trivial, std::complex<double>>::value));
    BOOST_CHECK((!is_subtractable_in_place<std::string, std::string>::value));
    BOOST_CHECK((is_subtractable_in_place<int, const int>::value));
    BOOST_CHECK((!is_subtractable_in_place<const int, int>::value));
    BOOST_CHECK((!is_subtractable_in_place<const int &, int>::value));
    BOOST_CHECK((is_subtractable_in_place<int &&, const int &>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_is_multipliable_test)
{
    BOOST_CHECK((!is_multipliable<void>::value));
    BOOST_CHECK((!is_multipliable<void, int>::value));
    BOOST_CHECK((!is_multipliable<int, void>::value));
    BOOST_CHECK(is_multipliable<int>::value);
    BOOST_CHECK(is_multipliable<const int>::value);
    BOOST_CHECK((is_multipliable<const int, int>::value));
    BOOST_CHECK((is_multipliable<int, const int>::value));
    BOOST_CHECK((is_multipliable<const int &, int &>::value));
    BOOST_CHECK((is_multipliable<int &&, const int &>::value));
    BOOST_CHECK(is_multipliable<double>::value);
    BOOST_CHECK(is_multipliable<std::complex<double>>::value);
    BOOST_CHECK((is_multipliable<const std::complex<double>, double>::value));
    BOOST_CHECK((is_multipliable<std::complex<double>, const double>::value));
    BOOST_CHECK((is_multipliable<int, int>::value));
    BOOST_CHECK((is_multipliable<int, double>::value));
    BOOST_CHECK((is_multipliable<double, int>::value));
    BOOST_CHECK((is_multipliable<std::complex<double>, double>::value));
    BOOST_CHECK((is_multipliable<double, std::complex<double>>::value));
    BOOST_CHECK((!is_multipliable<trivial, std::complex<double>>::value));
    BOOST_CHECK((!is_multipliable<int *, std::size_t>::value));
    BOOST_CHECK((!is_multipliable<std::size_t, int *>::value));
    BOOST_CHECK(!is_multipliable<int *>::value);
    BOOST_CHECK(is_multipliable<int &>::value);
    BOOST_CHECK((is_multipliable<int &, double &>::value));
    BOOST_CHECK((is_multipliable<double &, int &>::value));
    BOOST_CHECK(is_multipliable<int const &>::value);
    BOOST_CHECK((is_multipliable<int const &, double &>::value));
    BOOST_CHECK((is_multipliable<double const &, int &>::value));
    BOOST_CHECK(is_multipliable<int &&>::value);
    BOOST_CHECK((is_multipliable<int &&, double &&>::value));
    BOOST_CHECK((is_multipliable<double &&, int &&>::value));
    BOOST_CHECK((!is_multipliable<int *&, int>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_is_multipliable_in_place_test)
{
    BOOST_CHECK((!is_multipliable_in_place<void>::value));
    BOOST_CHECK((!is_multipliable_in_place<void, int>::value));
    BOOST_CHECK((!is_multipliable_in_place<int, void>::value));
    BOOST_CHECK((is_multipliable_in_place<int>::value));
    BOOST_CHECK((is_multipliable_in_place<int, int>::value));
    BOOST_CHECK((is_multipliable_in_place<int, double>::value));
    BOOST_CHECK((is_multipliable_in_place<double, int>::value));
    BOOST_CHECK((is_multipliable_in_place<std::complex<double>, double>::value));
    BOOST_CHECK((!is_multipliable_in_place<double, std::complex<double>>::value));
    BOOST_CHECK((!is_multipliable_in_place<trivial, std::complex<double>>::value));
    BOOST_CHECK((is_multipliable_in_place<int, const int>::value));
    BOOST_CHECK((!is_multipliable_in_place<const int, int>::value));
    BOOST_CHECK((!is_multipliable_in_place<const int &, int>::value));
    BOOST_CHECK((is_multipliable_in_place<int &&, const int &>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_is_divisible_test)
{
    BOOST_CHECK((!is_divisible<void>::value));
    BOOST_CHECK((!is_divisible<void, int>::value));
    BOOST_CHECK((!is_divisible<int, void>::value));
    BOOST_CHECK(is_divisible<int>::value);
    BOOST_CHECK(is_divisible<const int>::value);
    BOOST_CHECK((is_divisible<const int, int>::value));
    BOOST_CHECK((is_divisible<int, const int>::value));
    BOOST_CHECK((is_divisible<const int &, int &>::value));
    BOOST_CHECK((is_divisible<int &&, const int &>::value));
    BOOST_CHECK(is_divisible<double>::value);
    BOOST_CHECK(is_divisible<std::complex<double>>::value);
    BOOST_CHECK((is_divisible<const std::complex<double>, double>::value));
    BOOST_CHECK((is_divisible<std::complex<double>, const double>::value));
    BOOST_CHECK((is_divisible<int, int>::value));
    BOOST_CHECK((is_divisible<int, double>::value));
    BOOST_CHECK((is_divisible<double, int>::value));
    BOOST_CHECK((is_divisible<std::complex<double>, double>::value));
    BOOST_CHECK((is_divisible<double, std::complex<double>>::value));
    BOOST_CHECK((!is_divisible<trivial, std::complex<double>>::value));
    BOOST_CHECK((!is_divisible<int *, std::size_t>::value));
    BOOST_CHECK((!is_divisible<std::size_t, int *>::value));
    BOOST_CHECK(!is_divisible<int *>::value);
    BOOST_CHECK(is_divisible<int &>::value);
    BOOST_CHECK((is_divisible<int &, double &>::value));
    BOOST_CHECK((is_divisible<double &, int &>::value));
    BOOST_CHECK(is_divisible<int const &>::value);
    BOOST_CHECK((is_divisible<int const &, double &>::value));
    BOOST_CHECK((is_divisible<double const &, int &>::value));
    BOOST_CHECK(is_divisible<int &&>::value);
    BOOST_CHECK((is_divisible<int &&, double &&>::value));
    BOOST_CHECK((is_divisible<double &&, int &&>::value));
    BOOST_CHECK((!is_divisible<int *&, int>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_is_divisible_in_place_test)
{
    BOOST_CHECK((!is_divisible_in_place<void>::value));
    BOOST_CHECK((!is_divisible_in_place<void, int>::value));
    BOOST_CHECK((!is_divisible_in_place<int, void>::value));
    BOOST_CHECK((is_divisible_in_place<int>::value));
    BOOST_CHECK((is_divisible_in_place<int, int>::value));
    BOOST_CHECK((is_divisible_in_place<int, double>::value));
    BOOST_CHECK((is_divisible_in_place<double, int>::value));
    BOOST_CHECK((is_divisible_in_place<std::complex<double>, double>::value));
    BOOST_CHECK((!is_divisible_in_place<double, std::complex<double>>::value));
    BOOST_CHECK((!is_divisible_in_place<trivial, std::complex<double>>::value));
    BOOST_CHECK((is_divisible_in_place<int, const int>::value));
    BOOST_CHECK((!is_divisible_in_place<const int, int>::value));
    BOOST_CHECK((!is_divisible_in_place<const int &, int>::value));
    BOOST_CHECK((is_divisible_in_place<int &&, const int &>::value));
}

struct frob {
    bool operator==(const frob &) const;
    bool operator!=(const frob &) const;
    bool operator<(const frob &) const;
    bool operator>(const frob &) const;
};

struct frob_nonconst {
    bool operator==(const frob_nonconst &);
    bool operator!=(const frob_nonconst &);
    bool operator<(const frob_nonconst &);
    bool operator>(const frob_nonconst &);
};

struct frob_nonbool {
    char operator==(const frob_nonbool &) const;
    char operator!=(const frob_nonbool &) const;
    char operator<(const frob_nonbool &) const;
    char operator>(const frob_nonbool &) const;
};

struct frob_void {
    void operator==(const frob_nonbool &) const;
    void operator!=(const frob_nonbool &) const;
    void operator<(const frob_nonbool &) const;
    void operator>(const frob_nonbool &) const;
};

struct frob_copy {
    int operator==(frob_copy) const;
    int operator!=(frob_copy) const;
    int operator<(frob_copy) const;
    int operator>(frob_copy) const;
};

struct frob_mix {
};

short operator==(const frob_mix &, frob_mix);
short operator!=(const frob_mix &, frob_mix);
short operator<(const frob_mix &, frob_mix);
short operator>(const frob_mix &, frob_mix);

struct frob_mix_wrong {
};

short operator==(frob_mix_wrong, frob_mix_wrong &);
short operator!=(frob_mix_wrong, frob_mix_wrong &);
short operator<(frob_mix_wrong, frob_mix_wrong &);
short operator>(frob_mix_wrong, frob_mix_wrong &);

struct frob_mix_not_ineq {
};

bool operator==(const frob_mix_not_ineq &, const frob_mix_not_ineq &);

struct frob_mix_not_eq {
};

bool operator!=(const frob_mix_not_eq &, const frob_mix_not_eq &);

BOOST_AUTO_TEST_CASE(type_traits_is_equality_comparable_test)
{
    BOOST_CHECK((!is_equality_comparable<void>::value));
    BOOST_CHECK((!is_equality_comparable<void, const int &>::value));
    BOOST_CHECK((!is_equality_comparable<int, void>::value));
    BOOST_CHECK(is_equality_comparable<const int &>::value);
    BOOST_CHECK(!is_equality_comparable<const trivial &>::value);
    BOOST_CHECK((is_equality_comparable<const int &, const double &>::value));
    BOOST_CHECK((is_equality_comparable<const double &, const int &>::value));
    BOOST_CHECK((!is_equality_comparable<const double &, const trivial &>::value));
    BOOST_CHECK((!is_equality_comparable<const trivial &, const double &>::value));
    BOOST_CHECK(is_equality_comparable<int &>::value);
    BOOST_CHECK(is_equality_comparable<const int *&>::value);
    BOOST_CHECK((is_equality_comparable<int const *, int *>::value));
    BOOST_CHECK((is_equality_comparable<int &, const double &>::value));
    BOOST_CHECK((is_equality_comparable<int const &, double &&>::value));
    BOOST_CHECK(is_equality_comparable<const frob &>::value);
    BOOST_CHECK(!is_equality_comparable<const frob_nonconst &>::value);
    BOOST_CHECK(is_equality_comparable<frob_nonconst &>::value);
    BOOST_CHECK(is_equality_comparable<const frob_nonbool &>::value);
    BOOST_CHECK(!is_equality_comparable<const frob_void &>::value);
    BOOST_CHECK(is_equality_comparable<const frob_copy &>::value);
    BOOST_CHECK(is_equality_comparable<const frob_mix &>::value);
    BOOST_CHECK(!is_equality_comparable<const frob_mix_wrong &>::value);
    BOOST_CHECK(!is_equality_comparable<const frob_mix_not_ineq &>::value);
    BOOST_CHECK(!is_equality_comparable<const frob_mix_not_eq &>::value);
}

BOOST_AUTO_TEST_CASE(type_traits_is_less_than_comparable_test)
{
    BOOST_CHECK((!is_less_than_comparable<void>::value));
    BOOST_CHECK((!is_less_than_comparable<void, int>::value));
    BOOST_CHECK((!is_less_than_comparable<int, void>::value));
    BOOST_CHECK(is_less_than_comparable<int>::value);
    BOOST_CHECK((is_less_than_comparable<int, double>::value));
    BOOST_CHECK((is_less_than_comparable<double, int>::value));
    BOOST_CHECK(is_less_than_comparable<int &>::value);
    BOOST_CHECK((is_less_than_comparable<const int &, double &&>::value));
    BOOST_CHECK((is_less_than_comparable<double, int &>::value));
    BOOST_CHECK((is_less_than_comparable<int *>::value));
    BOOST_CHECK((is_less_than_comparable<int const *>::value));
    BOOST_CHECK((is_less_than_comparable<int const *, int *>::value));
    BOOST_CHECK((!is_less_than_comparable<int *, double *>::value));
    BOOST_CHECK((!is_less_than_comparable<int *, double *>::value));
    BOOST_CHECK((is_less_than_comparable<frob>::value));
    BOOST_CHECK((!is_less_than_comparable<frob_nonconst>::value));
    BOOST_CHECK((is_less_than_comparable<frob_nonbool>::value));
    BOOST_CHECK((!is_less_than_comparable<frob_void>::value));
    BOOST_CHECK((is_less_than_comparable<frob_copy>::value));
    BOOST_CHECK((is_less_than_comparable<frob_mix>::value));
    BOOST_CHECK((!is_less_than_comparable<frob_mix_wrong>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_is_greater_than_comparable_test)
{
    BOOST_CHECK((!is_greater_than_comparable<void>::value));
    BOOST_CHECK((!is_greater_than_comparable<void, int>::value));
    BOOST_CHECK((!is_greater_than_comparable<int, void>::value));
    BOOST_CHECK(is_greater_than_comparable<int>::value);
    BOOST_CHECK((is_greater_than_comparable<int, double>::value));
    BOOST_CHECK((is_greater_than_comparable<double, int>::value));
    BOOST_CHECK(is_greater_than_comparable<int &>::value);
    BOOST_CHECK((is_greater_than_comparable<const int &, double &&>::value));
    BOOST_CHECK((is_greater_than_comparable<double, int &>::value));
    BOOST_CHECK((is_greater_than_comparable<int *>::value));
    BOOST_CHECK((is_greater_than_comparable<int const *>::value));
    BOOST_CHECK((is_greater_than_comparable<int const *, int *>::value));
    BOOST_CHECK((!is_greater_than_comparable<int *, double *>::value));
    BOOST_CHECK((!is_greater_than_comparable<int *, double *>::value));
    BOOST_CHECK((is_greater_than_comparable<frob>::value));
    BOOST_CHECK((!is_greater_than_comparable<frob_nonconst>::value));
    BOOST_CHECK((is_greater_than_comparable<frob_nonbool>::value));
    BOOST_CHECK((!is_greater_than_comparable<frob_void>::value));
    BOOST_CHECK((is_greater_than_comparable<frob_copy>::value));
    BOOST_CHECK((is_greater_than_comparable<frob_mix>::value));
    BOOST_CHECK((!is_greater_than_comparable<frob_mix_wrong>::value));
}

template <typename T>
struct iio_base {
};

struct stream1 {
};

std::ostream &operator<<(std::ostream &, const stream1 &);

struct stream2 {
};

std::ostream &operator<<(std::ostream &, stream2);

struct stream3 {
};

std::ostream &operator<<(std::ostream &, stream3 &);

struct stream4 {
};

void operator<<(std::ostream &, const stream4 &);

struct stream5 {
};

std::ostream &operator<<(const std::ostream &, const stream5 &);

struct stream6 {
};

const std::ostream &operator<<(std::ostream &, const stream6 &);

BOOST_AUTO_TEST_CASE(type_traits_is_ostreamable_test)
{
    BOOST_CHECK(is_ostreamable<int>::value);
    BOOST_CHECK(is_ostreamable<double>::value);
    BOOST_CHECK(is_ostreamable<int &>::value);
    BOOST_CHECK(is_ostreamable<double &&>::value);
    BOOST_CHECK(is_ostreamable<const int &>::value);
    BOOST_CHECK(!is_ostreamable<iio_base<int>>::value);
    BOOST_CHECK(is_ostreamable<stream1>::value);
    BOOST_CHECK(is_ostreamable<stream2>::value);
    BOOST_CHECK(!is_ostreamable<stream3>::value);
    BOOST_CHECK(!is_ostreamable<stream4>::value);
    BOOST_CHECK(is_ostreamable<stream5>::value);
    BOOST_CHECK(!is_ostreamable<stream6>::value);
    BOOST_CHECK(!is_ostreamable<void>::value);
}

struct c_element {
};

struct nc_element1 {
    nc_element1() = delete;
};

struct nc_element2 {
    nc_element2() = default;
    nc_element2(const nc_element2 &) = default;
    nc_element2(nc_element2 &&) = default;
    nc_element2 &operator=(nc_element2 &&);
};

struct c_element2 {
    c_element2() = default;
    c_element2(const c_element2 &) = default;
    c_element2(c_element2 &&) = default;
    c_element2 &operator=(c_element2 &&) noexcept;
};

BOOST_AUTO_TEST_CASE(type_traits_is_container_element_test)
{
    BOOST_CHECK(!is_container_element<void>::value);
    BOOST_CHECK(is_container_element<int>::value);
    BOOST_CHECK(!is_container_element<int const>::value);
    BOOST_CHECK(is_container_element<double>::value);
    BOOST_CHECK(is_container_element<c_element>::value);
    BOOST_CHECK(!is_container_element<c_element const>::value);
    BOOST_CHECK(!is_container_element<c_element &>::value);
    BOOST_CHECK(!is_container_element<c_element const &>::value);
    BOOST_CHECK(!is_container_element<nc_element1>::value);
// Missing nothrow detection in the Intel compiler.
#if !defined(PIRANHA_COMPILER_IS_INTEL)
    BOOST_CHECK(!is_container_element<nc_element2>::value);
#endif
    BOOST_CHECK(is_container_element<c_element2>::value);
    BOOST_CHECK(!is_container_element<int &>::value);
    BOOST_CHECK(!is_container_element<int &&>::value);
    BOOST_CHECK(!is_container_element<int const &>::value);
}

struct unhashable1 {
};

struct unhashable2 {
};

struct unhashable3 {
};

struct unhashable4 {
};

struct unhashable5 {
};

struct unhashable6 {
};

struct unhashable7 {
};

struct unhashable8 {
};

struct unhashable9 {
};

struct unhashable10 {
};

struct unhashable11 {
};

struct unhashable12 {
};

struct hashable1 {
};

struct hashable2 {
};

struct hashable3 {
};

struct hashable4 {
};

namespace std
{

template <>
struct hash<unhashable2> {
};

template <>
struct hash<unhashable3> {
    int operator()(const unhashable3 &);
};

template <>
struct hash<unhashable4> {
    std::size_t operator()(unhashable4 &);
};

template <>
struct hash<unhashable5> {
    hash(const hash &) = delete;
    hash &operator=(const hash &) = default;
    std::size_t operator()(const unhashable5 &);
};

template <>
struct hash<unhashable6> {
    ~hash() = delete;
    std::size_t operator()(const unhashable6 &);
};

template <>
struct hash<unhashable7> {
    std::size_t operator()(const unhashable7 &);
};

template <>
struct hash<unhashable8> {
    hash();
    std::size_t operator()(const unhashable8 &);
};

template <>
struct hash<unhashable9> {
    std::size_t operator()(const unhashable9 &);
};

template <>
struct hash<unhashable10> {
    hash(const hash &) = default;
    std::size_t operator()(const unhashable10 &) const;
    ~hash() noexcept(false) {}
};

template <>
struct hash<unhashable11> {
    std::size_t operator()(const unhashable11 &) const;
    hash() noexcept;
    hash(const hash &);
    hash(hash &&) noexcept(false);
    hash &operator=(hash &&) noexcept;
    ~hash();
};

template <>
struct hash<unhashable12> {
    std::size_t operator()(const unhashable12 &) const;
    hash() noexcept;
    hash(const hash &);
    hash(hash &&) noexcept;
    hash &operator=(hash &&) noexcept(false);
    ~hash();
};

template <>
struct hash<hashable1> {
    std::size_t operator()(const hashable1 &) const;
};

template <>
struct hash<hashable2> {
    std::size_t operator()(const hashable2 &) const;
};

template <>
struct hash<hashable3> {
    hash() noexcept;
    std::size_t operator()(const hashable3 &) const;
};

template <>
struct hash<hashable4> {
    std::size_t operator()(const hashable4 &) const;
};
}

BOOST_AUTO_TEST_CASE(type_traits_is_hashable_test)
{
    BOOST_CHECK(is_hashable<int>::value);
    BOOST_CHECK(is_hashable<std::string>::value);
    BOOST_CHECK(is_hashable<double>::value);
    BOOST_CHECK(is_hashable<double &>::value);
    BOOST_CHECK(is_hashable<double &&>::value);
    BOOST_CHECK(is_hashable<const double &>::value);
    BOOST_CHECK(is_hashable<const double>::value);
    // This is gonna fail on GCC 4.7.2 at least, depending on whether static_assert() is used
    // in the default implementation of the hasher.
    // http://stackoverflow.com/questions/16302977/static-assertions-and-sfinae
    // BOOST_CHECK(!is_hashable<unhashable1>::value);
    BOOST_CHECK(is_hashable<unhashable1 *>::value);
    BOOST_CHECK(is_hashable<unhashable1 const *>::value);
    BOOST_CHECK(!is_hashable<unhashable2>::value);
    BOOST_CHECK(!is_hashable<unhashable3>::value);
    BOOST_CHECK(!is_hashable<unhashable4>::value);
    BOOST_CHECK(!is_hashable<unhashable5>::value);
    BOOST_CHECK(!is_hashable<unhashable6>::value);
    BOOST_CHECK(!is_hashable<unhashable7>::value);
    BOOST_CHECK(!is_hashable<unhashable8>::value);
    BOOST_CHECK(!is_hashable<unhashable9>::value);
// Missing noexcept detect.
#if !defined(PIRANHA_COMPILER_IS_INTEL)
    BOOST_CHECK(!is_hashable<unhashable10>::value);
    BOOST_CHECK(!is_hashable<unhashable11>::value);
    BOOST_CHECK(!is_hashable<unhashable12>::value);
#endif
    BOOST_CHECK(is_hashable<hashable1>::value);
    BOOST_CHECK(is_hashable<hashable2>::value);
    BOOST_CHECK(is_hashable<hashable2 &>::value);
    BOOST_CHECK(is_hashable<hashable2 &&>::value);
    BOOST_CHECK(is_hashable<hashable2 const &>::value);
    BOOST_CHECK(is_hashable<hashable2 *>::value);
    BOOST_CHECK(is_hashable<hashable2 const *>::value);
    BOOST_CHECK(is_hashable<hashable3>::value);
    BOOST_CHECK(is_hashable<hashable4>::value);
}

struct fo1 {
};

struct fo2 {
    void operator()();
    void operator()() const;
};

struct fo3 {
    void operator()(int);
};

struct fo4 {
    void operator()(int);
    std::string operator()(int, double &);
};

struct fo5 {
    template <typename... Args>
    int operator()(Args &&...);
};

struct fo6 {
    int operator()(int, int = 0);
};

void not_fo();

struct l5 {
    std::string &operator()(int &);
};

struct l6 {
    std::string const &operator()(int &);
};

BOOST_AUTO_TEST_CASE(type_traits_is_function_object_test)
{
    // NOTE: regarding lambdas:
    // http://en.cppreference.com/w/cpp/language/lambda
    // Specifically, they are always function objects and they have defaulted constructors.
    auto l1 = []() {};
    auto l2 = [](const int &) {};
    auto l3 = [](int &) {};
    auto l4 = [](int &) { return std::string{}; };
    BOOST_CHECK((!is_function_object<void, void>::value));
    BOOST_CHECK((!is_function_object<int, void>::value));
    BOOST_CHECK((is_function_object<decltype(l1), void>::value));
    BOOST_CHECK((is_function_object<const decltype(l1), void>::value));
    BOOST_CHECK((!is_function_object<decltype(l1), void, int>::value));
    BOOST_CHECK((!is_function_object<const decltype(l2), void>::value));
    BOOST_CHECK((is_function_object<decltype(l2), void, int>::value));
    BOOST_CHECK((is_function_object<const decltype(l2), void, int &>::value));
    BOOST_CHECK((is_function_object<const decltype(l2), void, const int &>::value));
    BOOST_CHECK((!is_function_object<decltype(l3), void>::value));
    BOOST_CHECK((!is_function_object<const decltype(l3), void, int>::value));
    BOOST_CHECK((is_function_object<decltype(l3), void, int &>::value));
    BOOST_CHECK((!is_function_object<const decltype(l3), void, const int &>::value));
    BOOST_CHECK((!is_function_object<decltype(l3) &, void, int &>::value));
    BOOST_CHECK((!is_function_object<decltype(l3) const &, void, int &>::value));
    BOOST_CHECK((is_function_object<decltype(l4), std::string, int &>::value));
    BOOST_CHECK((!is_function_object<decltype(l4), std::string &, int &>::value));
    BOOST_CHECK((!is_function_object<l5, std::string, int &>::value));
    BOOST_CHECK((is_function_object<l5, std::string &, int &>::value));
    BOOST_CHECK((!is_function_object<l5, std::string const &, int &>::value));
    BOOST_CHECK((!is_function_object<l6, std::string, int &>::value));
    BOOST_CHECK((!is_function_object<l6, std::string &, int &>::value));
    BOOST_CHECK((is_function_object<l6, std::string const &, int &>::value));
    BOOST_CHECK((is_function_object<std::hash<int>, std::size_t, int>::value));
    BOOST_CHECK((is_function_object<const std::hash<int>, std::size_t, int &&>::value));
    BOOST_CHECK((is_function_object<const std::hash<int>, std::size_t, const int &>::value));
    BOOST_CHECK((is_function_object<const std::hash<int>, std::size_t, int &>::value));
    BOOST_CHECK((!is_function_object<const std::hash<int> &, std::size_t, int &>::value));
    BOOST_CHECK((!is_function_object<std::hash<int> &, std::size_t, int &>::value));
    BOOST_CHECK((!is_function_object<const std::hash<int>, int, int &>::value));
    BOOST_CHECK((!is_function_object<const std::hash<int>, std::size_t, int &, int &>::value));
    BOOST_CHECK((!is_function_object<const std::hash<int>, std::size_t>::value));
    BOOST_CHECK((!is_function_object<fo1, void>::value));
    BOOST_CHECK((!is_function_object<fo1, void, int>::value));
    BOOST_CHECK((is_function_object<fo2, void>::value));
    BOOST_CHECK((!is_function_object<fo2 *, void>::value));
    BOOST_CHECK((is_function_object<const fo2, void>::value));
    BOOST_CHECK((is_function_object<fo3, void, int>::value));
    BOOST_CHECK((!is_function_object<const fo3, void, int>::value));
    BOOST_CHECK((!is_function_object<fo3, void, int, int>::value));
    BOOST_CHECK((is_function_object<fo4, void, int>::value));
    BOOST_CHECK((is_function_object<fo4, std::string, int, double &>::value));
    BOOST_CHECK((!is_function_object<fo4, std::string, int, double &, int>::value));
    BOOST_CHECK((!is_function_object<fo4, std::string, int>::value));
    BOOST_CHECK((!is_function_object<fo4, std::string &, int, double &>::value));
    BOOST_CHECK((!is_function_object<fo4, std::string, int, double const &>::value));
    BOOST_CHECK((is_function_object<fo5, int>::value));
    BOOST_CHECK((is_function_object<fo5, int, double>::value));
    BOOST_CHECK((is_function_object<fo5, int, double, const std::string &>::value));
    BOOST_CHECK((!is_function_object<fo5, void, double, const std::string &>::value));
    BOOST_CHECK((is_function_object<fo6, int, int>::value));
    BOOST_CHECK((is_function_object<fo6, int, int, int>::value));
    BOOST_CHECK((!is_function_object<fo6, int, int, int, double>::value));
    BOOST_CHECK((!is_function_object<decltype(not_fo), void>::value));
    BOOST_CHECK((is_function_object<std::function<void(int)>, void, int>::value));
    BOOST_CHECK((!is_function_object<std::function<void(int)>, void>::value));
}

struct hfo1 {
};

struct hfo2 {
    hfo2() noexcept;
    std::size_t operator()(int);
};

struct hfo3 {
    hfo3() noexcept;
    std::size_t operator()(int) const;
};

struct hfo4 {
    hfo4() noexcept;
    hfo4(const hfo4 &) = default;
    std::size_t operator()(int) const;
    ~hfo4() noexcept(false);
};

struct hfo5 {
    hfo5() noexcept;
    std::size_t operator()(int) const;
};

struct hfo6 {
    hfo6() noexcept;
    hfo6(const hfo6 &) = delete;
    std::size_t operator()(int) const;
};

struct hfo7 {
    hfo7() noexcept;
    std::size_t operator()(int) const;
    hfo7(const hfo7 &);
    hfo7(hfo7 &&) noexcept;
    hfo7 &operator=(hfo7 &&) noexcept;
};

struct hfo8 {
    hfo8() noexcept;
    std::size_t operator()(int) const;
    hfo8(const hfo7 &);
    hfo8(hfo8 &&) noexcept(false);
    hfo8 &operator=(hfo8 &&) noexcept;
};

struct hfo9 {
    hfo9() noexcept;
    std::size_t operator()(int) const;
    hfo9(const hfo9 &);
    hfo9(hfo9 &&) noexcept;
    hfo9 &operator=(hfo9 &&) noexcept(false);
};

BOOST_AUTO_TEST_CASE(type_traits_is_hash_function_object_test)
{
    BOOST_CHECK((!is_hash_function_object<void, int>::value));
    BOOST_CHECK((!is_hash_function_object<int, void>::value));
    BOOST_CHECK((!is_hash_function_object<void, void>::value));
    BOOST_CHECK((is_hash_function_object<std::hash<int>, int>::value));
    BOOST_CHECK((is_hash_function_object<std::hash<int const *>, int const *>::value));
    BOOST_CHECK((is_hash_function_object<std::hash<int const *>, int *>::value));
    BOOST_CHECK((!is_hash_function_object<const std::hash<int const *>, int *>::value));
    BOOST_CHECK((!is_hash_function_object<std::hash<int> &, int &>::value));
    BOOST_CHECK((!is_hash_function_object<std::hash<int> const &, int &>::value));
    BOOST_CHECK((!is_hash_function_object<std::hash<int> &, const int &>::value));
    BOOST_CHECK((is_hash_function_object<std::hash<std::string>, std::string>::value));
    BOOST_CHECK((!is_hash_function_object<std::hash<int>, std::string>::value));
    BOOST_CHECK((!is_hash_function_object<int, int>::value));
    BOOST_CHECK((!is_hash_function_object<hfo1, int>::value));
    BOOST_CHECK((!is_hash_function_object<hfo2, int>::value));
    BOOST_CHECK((is_hash_function_object<hfo3, int>::value));
    BOOST_CHECK((is_hash_function_object<hfo3, short>::value));
    BOOST_CHECK((!is_hash_function_object<hfo4, int>::value));
    BOOST_CHECK((is_hash_function_object<hfo5, int>::value));
    BOOST_CHECK((!is_hash_function_object<hfo6, int>::value));
    BOOST_CHECK((is_hash_function_object<hfo7, int>::value));
    BOOST_CHECK((!is_hash_function_object<hfo8, int>::value));
// Missing noexcept.
#if !defined(PIRANHA_COMPILER_IS_INTEL)
    BOOST_CHECK((!is_hash_function_object<hfo9, int>::value));
#endif
}

struct efo1 {
};

struct efo2 {
    bool operator()(int, int) const;
};

struct efo3 {
    bool operator()(int, int);
};

struct efo4 {
    efo4(const efo4 &) = default;
    bool operator()(int, int) const;
    ~efo4() noexcept(false);
};

struct efo5 {
    efo5() = delete;
    bool operator()(int, int) const;
};

struct efo6 {
    template <typename... Args>
    bool operator()(Args &&...) const;
};

struct efo7 {
    efo7();
    efo7(const efo7 &);
    efo7(efo7 &&) noexcept;
    efo7 &operator=(const efo7 &);
    efo7 &operator=(efo7 &&) noexcept;
    bool operator()(int, int) const;
};

struct efo8 {
    efo8();
    efo8(const efo8 &);
    efo8(efo8 &&);
    efo8 &operator=(const efo8 &);
    efo8 &operator=(efo8 &&) noexcept;
    bool operator()(int, int) const;
};

struct efo9 {
    efo9();
    efo9(const efo9 &);
    efo9(efo9 &&) noexcept;
    efo9 &operator=(const efo9 &);
    efo9 &operator=(efo9 &&);
    bool operator()(int, int) const;
};

struct efo10 {
    bool operator()(int) const;
    bool operator()(int, int, int) const;
};

BOOST_AUTO_TEST_CASE(type_traits_is_equality_function_object_test)
{
    BOOST_CHECK((!is_equality_function_object<void, int>::value));
    BOOST_CHECK((!is_equality_function_object<int, void>::value));
    BOOST_CHECK((!is_equality_function_object<void, void>::value));
    BOOST_CHECK((is_equality_function_object<std::equal_to<int>, int>::value));
    BOOST_CHECK((is_equality_function_object<std::equal_to<int>, short>::value));
    BOOST_CHECK((!is_equality_function_object<const std::equal_to<int>, short>::value));
    BOOST_CHECK((!is_equality_function_object<std::equal_to<int> &, short>::value));
    BOOST_CHECK((!is_equality_function_object<std::equal_to<int> &&, short>::value));
    BOOST_CHECK((!is_equality_function_object<std::hash<int>, int>::value));
    BOOST_CHECK((!is_equality_function_object<bool, int>::value));
    BOOST_CHECK((!is_equality_function_object<efo1, int>::value));
    BOOST_CHECK((is_equality_function_object<efo2, int>::value));
    BOOST_CHECK((!is_equality_function_object<efo3, int>::value));
    BOOST_CHECK((!is_equality_function_object<efo4, int>::value));
    BOOST_CHECK((!is_equality_function_object<efo5, int>::value));
    BOOST_CHECK((is_equality_function_object<efo6, int>::value));
    BOOST_CHECK((is_equality_function_object<efo6, std::string>::value));
    BOOST_CHECK((is_equality_function_object<efo7, int>::value));
    BOOST_CHECK((!is_equality_function_object<efo7 const, int>::value));
    BOOST_CHECK((!is_equality_function_object<efo7 const &, int>::value));
    BOOST_CHECK((!is_equality_function_object<efo7 &, int>::value));
// Missing noexcept.
#if !defined(PIRANHA_COMPILER_IS_INTEL)
    BOOST_CHECK((!is_equality_function_object<efo8, int>::value));
    BOOST_CHECK((!is_equality_function_object<efo9, int>::value));
#endif
    BOOST_CHECK((!is_equality_function_object<efo10, int>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_min_max_int_test)
{
    BOOST_CHECK((std::is_same<int, min_int<int>>::value));
    BOOST_CHECK((std::is_same<unsigned, min_int<unsigned>>::value));
    BOOST_CHECK((std::is_same<int, max_int<int>>::value));
    BOOST_CHECK((std::is_same<unsigned, max_int<unsigned>>::value));
    BOOST_CHECK((std::is_same<int, max_int<short, int>>::value));
    BOOST_CHECK((std::is_same<unsigned, max_int<unsigned short, unsigned>>::value));
    if (std::numeric_limits<long long>::max() > std::numeric_limits<int>::max()
        && std::numeric_limits<long long>::min() < std::numeric_limits<int>::min()) {
        BOOST_CHECK((std::is_same<long long, max_int<short, int, signed char, long long>>::value));
        BOOST_CHECK((std::is_same<long long, max_int<long long, int, signed char, short>>::value));
        BOOST_CHECK((std::is_same<long long, max_int<int, long long, signed char, short>>::value));
        BOOST_CHECK((std::is_same<long long, max_int<short, signed char, long long, int>>::value));
    }
    if (std::numeric_limits<unsigned long long>::max() > std::numeric_limits<unsigned>::max()) {
        BOOST_CHECK((std::is_same<unsigned long long,
                                  max_int<unsigned short, unsigned, unsigned char, unsigned long long>>::value));
        BOOST_CHECK((std::is_same<unsigned long long,
                                  max_int<unsigned long long, unsigned, unsigned char, unsigned short>>::value));
        BOOST_CHECK((std::is_same<unsigned long long,
                                  max_int<unsigned, unsigned long long, unsigned char, unsigned short>>::value));
        BOOST_CHECK((std::is_same<unsigned long long,
                                  max_int<unsigned short, unsigned char, unsigned long long, unsigned>>::value));
    }
    if (std::numeric_limits<signed char>::max() < std::numeric_limits<short>::max()
        && std::numeric_limits<signed char>::min() > std::numeric_limits<short>::min()) {
        BOOST_CHECK((std::is_same<signed char, min_int<short, int, signed char, long long>>::value));
        BOOST_CHECK((std::is_same<signed char, min_int<long long, int, signed char, short>>::value));
        BOOST_CHECK((std::is_same<signed char, min_int<int, long long, signed char, short>>::value));
        BOOST_CHECK((std::is_same<signed char, min_int<short, signed char, long long, int>>::value));
    }
    if (std::numeric_limits<unsigned char>::min() < std::numeric_limits<unsigned short>::max()) {
        BOOST_CHECK(
            (std::is_same<unsigned char, min_int<unsigned short, unsigned, unsigned char, unsigned long long>>::value));
        BOOST_CHECK(
            (std::is_same<unsigned char, min_int<unsigned long long, unsigned, unsigned char, unsigned short>>::value));
        BOOST_CHECK(
            (std::is_same<unsigned char, min_int<unsigned, unsigned long long, unsigned char, unsigned short>>::value));
        BOOST_CHECK(
            (std::is_same<unsigned char, min_int<unsigned short, unsigned char, unsigned long long, unsigned>>::value));
    }
}

// Boilerplate to test the arrow op type trait.
struct arrow01 {
    int *operator->();
};

struct arrow02 {
    arrow01 operator->();
    // NOTE: calling -> on a const instance will fail,
    // as it returns a non pointer type which does not
    // provide an operator->() member function.
    void operator->() const;
};

struct arrow03 {
    int operator->();
};

struct arrow03a {
    arrow02 operator->();
};

struct arrow04 {
    arrow03 operator->();
};

// Good forward iterator.
template <typename T>
struct fake_it_traits_forward {
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using iterator_category = std::forward_iterator_tag;
};

// Broken reference type for forward it.
template <typename T>
struct fake_it_traits_forward_broken_ref {
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = void;
    using iterator_category = std::forward_iterator_tag;
};

// Good output iterator.
template <typename T>
struct fake_it_traits_output {
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using iterator_category = std::output_iterator_tag;
};

// Good input iterator.
template <typename T>
struct fake_it_traits_input {
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using iterator_category = std::input_iterator_tag;
};

// Broken trait, incorrect category.
template <typename T>
struct fake_it_traits_wrong_tag {
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using iterator_category = void;
};

// Broken trait, missing typedefs.
template <typename T>
struct fake_it_traits_missing {
    using value_type = void;
    using pointer = void;
    using iterator_category = void;
};

#define PIRANHA_DECL_ITT_SPEC(it_type, trait_class)                                                                    \
    namespace std                                                                                                      \
    {                                                                                                                  \
    template <>                                                                                                        \
    struct iterator_traits<it_type> : trait_class {                                                                    \
    };                                                                                                                 \
    }

// Good input iterator.
struct iter01 {
    int &operator*() const;
    int *operator->() const;
    iter01 &operator++();
    iter01 &operator++(int);
    bool operator==(const iter01 &) const;
    bool operator!=(const iter01 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter01, fake_it_traits_input<int>)

// Good iterator, minimal requirements.
struct iter02 {
    int &operator*();
    iter02 &operator++();
};

PIRANHA_DECL_ITT_SPEC(iter02, fake_it_traits_input<int>)

// Broken iterator, minimal requirements.
struct iter03 {
    // int &operator*();
    iter03 &operator++();
};

PIRANHA_DECL_ITT_SPEC(iter03, fake_it_traits_input<int>)

// Broken iterator, minimal requirements.
struct iter04 {
    iter04 &operator=(const iter04 &) = delete;
    ~iter04() = delete;
    int &operator*();
    iter04 &operator++();
};

PIRANHA_DECL_ITT_SPEC(iter04, fake_it_traits_input<int>)

// Broken iterator, missing itt spec.
struct iter05 {
    int &operator*();
    iter05 &operator++();
};

// PIRANHA_DECL_ITT_SPEC(iter05,fake_it_traits_input<int>)

// Good input iterator: missing arrow, but the value type is not a class.
struct iter06 {
    int &operator*() const;
    // int *operator->();
    iter06 &operator++();
    iter06 &operator++(int);
    bool operator==(const iter06 &) const;
    bool operator!=(const iter06 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter06, fake_it_traits_input<int>)

struct iter06a_v {
};

// Bad input iterator: missing arrow, and the value type is a class.
struct iter06a {
    iter06a_v &operator*() const;
    // int *operator->();
    iter06a &operator++();
    iter06a &operator++(int);
    bool operator==(const iter06a &) const;
    bool operator!=(const iter06a &) const;
};

PIRANHA_DECL_ITT_SPEC(iter06a, fake_it_traits_input<iter06a_v>)

// Broken input iterator: missing equality.
struct iter07 {
    int &operator*() const;
    int *operator->() const;
    iter07 &operator++();
    iter07 &operator++(int);
    // bool operator==(const iter07 &) const;
    bool operator!=(const iter07 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter07, fake_it_traits_input<int>)

// Broken input iterator: missing itt spec.
struct iter08 {
    int &operator*() const;
    int *operator->() const;
    iter08 &operator++();
    iter08 &operator++(int);
    bool operator==(const iter08 &) const;
    bool operator!=(const iter08 &) const;
};

// PIRANHA_DECL_ITT_SPEC(iter08,fake_it_traits_input<int>)

// Good input iterator: broken arrow, but non-class.
struct iter09 {
    int &operator*() const;
    int operator->() const;
    iter09 &operator++();
    iter09 &operator++(int);
    bool operator==(const iter09 &) const;
    bool operator!=(const iter09 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter09, fake_it_traits_input<int>)

struct iter09a_v {
};

// Bad input iterator: broken arrow, and class value type.
struct iter09a {
    iter09a_v &operator*() const;
    iter09a_v operator->() const;
    iter09a &operator++();
    iter09a &operator++(int);
    bool operator==(const iter09a &) const;
    bool operator!=(const iter09a &) const;
};

PIRANHA_DECL_ITT_SPEC(iter09a, fake_it_traits_input<iter09a_v>)

// Good input iterator: multiple arrow.
struct iter10 {
    int &operator*() const;
    arrow03a operator->() const;
    iter10 &operator++();
    iter10 &operator++(int);
    bool operator==(const iter10 &) const;
    bool operator!=(const iter10 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter10, fake_it_traits_input<int>)

// Good input iterator: multiple broken arrow, but non-class.
struct iter11 {
    int &operator*() const;
    arrow04 operator->() const;
    iter11 &operator++();
    iter11 &operator++(int);
    bool operator==(const iter11 &) const;
    bool operator!=(const iter11 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter11, fake_it_traits_input<int>)

// Bad input iterator: inconsistent arrow / star, and class value.
struct foo_it_12 {
};

struct iter12_v {
};

struct iter12 {
    iter12_v &operator*() const;
    foo_it_12 *operator->() const;
    iter12 &operator++();
    iter12 &operator++(int);
    bool operator==(const iter12 &) const;
    bool operator!=(const iter12 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter12, fake_it_traits_input<iter12_v>)

// Good input iterator: different but compatible arrow / star.
struct iter13 {
    int operator*() const;
    int *operator->() const;
    iter13 &operator++();
    iter13 &operator++(int);
    bool operator==(const iter13 &) const;
    bool operator!=(const iter13 &) const;
};

// Specialise the it_traits for iter13 manually, as we need
// a custom reference type.
namespace std
{
template <>
struct iterator_traits<iter13> {
    using difference_type = std::ptrdiff_t;
    using value_type = int;
    using pointer = int *;
    using reference = int;
    using iterator_category = std::input_iterator_tag;
};
}

// Good forward iterator.
struct iter14 {
    int &operator*() const;
    int *operator->() const;
    iter14 &operator++();
    iter14 &operator++(int);
    bool operator==(const iter14 &) const;
    bool operator!=(const iter14 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter14, fake_it_traits_forward<int>)

// Bad forward iterator: missing def ctor.
struct iter15 {
    iter15() = delete;
    int &operator*() const;
    int *operator->() const;
    iter15 &operator++();
    iter15 &operator++(int);
    bool operator==(const iter15 &) const;
    bool operator!=(const iter15 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter15, fake_it_traits_forward<int>)

// Bad forward iterator: not having reference types as reference in traits.
struct iter16 {
    int &operator*() const;
    int *operator->() const;
    iter16 &operator++();
    iter16 &operator++(int);
    bool operator==(const iter16 &) const;
    bool operator!=(const iter16 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter16, fake_it_traits_forward_broken_ref<int>)

// Bad forward iterator: broken tag in traits.
struct iter17 {
    int &operator*() const;
    int *operator->() const;
    iter17 &operator++();
    iter17 &operator++(int);
    bool operator==(const iter17 &) const;
    bool operator!=(const iter17 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter17, fake_it_traits_output<int>)

// Bad forward iterator: broken traits.
struct iter18 {
    int &operator*() const;
    int *operator->() const;
    iter18 &operator++();
    iter18 &operator++(int);
    bool operator==(const iter18 &) const;
    bool operator!=(const iter18 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter18, fake_it_traits_missing<int>)

// Bad forward iterator: broken ++.
struct iter19 {
    int &operator*() const;
    int *operator->() const;
    iter19 &operator++();
    void operator++(int);
    bool operator==(const iter19 &) const;
    bool operator!=(const iter19 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter19, fake_it_traits_forward<int>)

// Bad forward iterator: broken ++.
struct iter20 {
    int &operator*() const;
    int *operator->() const;
    void operator++();
    iter20 &operator++(int);
    bool operator==(const iter20 &) const;
    bool operator!=(const iter20 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter20, fake_it_traits_forward<int>)

struct iter21_v {
};

// Bad forward iterator: arrow returns type with different constness from star operator,
// and class value.
struct iter21 {
    iter21_v &operator*() const;
    const iter21_v *operator->() const;
    iter21 &operator++();
    iter21 &operator++(int);
    bool operator==(const iter21 &) const;
    bool operator!=(const iter21 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter21, fake_it_traits_forward<iter21_v>)

#undef PIRANHA_DECL_ITT_SPEC

BOOST_AUTO_TEST_CASE(type_traits_iterator_test)
{
    // Check the arrow operator type trait.
    BOOST_CHECK((!is_detected<arrow_operator_t, void>::value));
    BOOST_CHECK((std::is_same<int *, detected_t<arrow_operator_t, int *&>>::value));
    BOOST_CHECK((!is_detected<arrow_operator_t, int &>::value));
    BOOST_CHECK((std::is_same<int *, detected_t<arrow_operator_t, arrow01 &>>::value));
    BOOST_CHECK((std::is_same<int *, detected_t<arrow_operator_t, arrow02 &>>::value));
    BOOST_CHECK((!is_detected<arrow_operator_t, const arrow02 &>::value));
    BOOST_CHECK((!is_detected<arrow_operator_t, arrow03 &>::value));
    BOOST_CHECK((std::is_same<int *, detected_t<arrow_operator_t, arrow03a &>>::value));
    // Iterator.
    BOOST_CHECK(has_iterator_traits<int *>::value);
    BOOST_CHECK(has_iterator_traits<const int *>::value);
    BOOST_CHECK(!has_iterator_traits<int>::value);
    BOOST_CHECK(!has_iterator_traits<double>::value);
    BOOST_CHECK(has_iterator_traits<std::vector<int>::iterator>::value);
    BOOST_CHECK(has_iterator_traits<std::vector<int>::const_iterator>::value);
    BOOST_CHECK(!is_iterator<void>::value);
    BOOST_CHECK(is_iterator<int *>::value);
    BOOST_CHECK(is_iterator<const int *>::value);
    BOOST_CHECK(is_iterator<std::vector<int>::iterator>::value);
    BOOST_CHECK(is_iterator<std::vector<int>::const_iterator>::value);
    BOOST_CHECK(!is_iterator<std::vector<int>::iterator &>::value);
    BOOST_CHECK(!is_iterator<int>::value);
    BOOST_CHECK(!is_iterator<std::string>::value);
    BOOST_CHECK(is_iterator<iter01>::value);
    BOOST_CHECK(!is_iterator<iter01 &>::value);
    BOOST_CHECK(!is_iterator<const iter01>::value);
    BOOST_CHECK(is_iterator<iter02>::value);
    BOOST_CHECK(!is_iterator<iter02 &>::value);
    BOOST_CHECK(!is_iterator<const iter02>::value);
    BOOST_CHECK(!is_iterator<iter03>::value);
    BOOST_CHECK(!is_iterator<iter03 &>::value);
    BOOST_CHECK(!is_iterator<const iter03>::value);
    BOOST_CHECK(is_iterator<std::istreambuf_iterator<char>>::value);
    // The Intel compiler has problems with the destructible
    // type-trait.
#if !defined(PIRANHA_COMPILER_IS_INTEL)
    BOOST_CHECK(!is_iterator<iter04>::value);
    BOOST_CHECK(!is_iterator<iter04 &>::value);
    BOOST_CHECK(!is_iterator<const iter04>::value);
#endif
    BOOST_CHECK(!is_iterator<iter05>::value);
    BOOST_CHECK(!is_iterator<iter05 &>::value);
    BOOST_CHECK(!is_iterator<const iter05>::value);
    BOOST_CHECK(is_iterator<std::ostream_iterator<int>>::value);
    BOOST_CHECK(is_iterator<std::insert_iterator<std::list<int>>>::value);
    // Input iterator.
    BOOST_CHECK(!is_input_iterator<void>::value);
    BOOST_CHECK(is_input_iterator<int *>::value);
    BOOST_CHECK(is_input_iterator<const int *>::value);
    BOOST_CHECK(is_input_iterator<std::vector<int>::iterator>::value);
    BOOST_CHECK(is_input_iterator<std::vector<int>::const_iterator>::value);
    BOOST_CHECK(!is_input_iterator<std::vector<int>::iterator &>::value);
    BOOST_CHECK(is_input_iterator<std::istream_iterator<char>>::value);
    BOOST_CHECK(is_input_iterator<std::istreambuf_iterator<char>>::value);
    BOOST_CHECK(is_input_iterator<iter01>::value);
    BOOST_CHECK((is_output_iterator<iter01, int &>::value));
    BOOST_CHECK((!is_output_iterator<iter01, void>::value));
    BOOST_CHECK(!is_input_iterator<iter01 &>::value);
    BOOST_CHECK(!is_input_iterator<const iter01>::value);
    BOOST_CHECK(!is_input_iterator<iter02>::value);
    BOOST_CHECK(!is_input_iterator<iter02 &>::value);
    BOOST_CHECK(!is_input_iterator<const iter02>::value);
    BOOST_CHECK(is_input_iterator<iter06>::value);
    BOOST_CHECK(!is_input_iterator<iter06 &>::value);
    BOOST_CHECK(!is_input_iterator<const iter06>::value);
    BOOST_CHECK(is_iterator<iter06>::value);
    BOOST_CHECK(!is_iterator<iter06 &>::value);
    BOOST_CHECK(!is_iterator<const iter06>::value);
    BOOST_CHECK(!is_input_iterator<iter06a>::value);
    BOOST_CHECK(!is_input_iterator<iter07>::value);
    BOOST_CHECK(!is_input_iterator<iter07 &>::value);
    BOOST_CHECK(!is_input_iterator<const iter07>::value);
    BOOST_CHECK(is_iterator<iter07>::value);
    BOOST_CHECK(!is_iterator<iter07 &>::value);
    BOOST_CHECK(!is_iterator<const iter07>::value);
    BOOST_CHECK(!is_input_iterator<iter08>::value);
    BOOST_CHECK(!is_input_iterator<iter08 &>::value);
    BOOST_CHECK(!is_input_iterator<const iter08>::value);
    BOOST_CHECK(!is_iterator<iter08>::value);
    BOOST_CHECK(!is_iterator<iter08 &>::value);
    BOOST_CHECK(!is_iterator<const iter08>::value);
    BOOST_CHECK(is_input_iterator<iter09>::value);
    BOOST_CHECK(!is_input_iterator<iter09 &>::value);
    BOOST_CHECK(!is_input_iterator<const iter09>::value);
    BOOST_CHECK(!is_input_iterator<iter09a>::value);
    BOOST_CHECK(is_input_iterator<iter10>::value);
    BOOST_CHECK((is_output_iterator<iter10, int &>::value));
    BOOST_CHECK(!is_input_iterator<iter10 &>::value);
    BOOST_CHECK(!is_input_iterator<const iter10>::value);
    BOOST_CHECK(is_input_iterator<iter11>::value);
    BOOST_CHECK(!is_input_iterator<iter11 &>::value);
    BOOST_CHECK(!is_input_iterator<const iter11>::value);
    BOOST_CHECK(is_iterator<iter11>::value);
    BOOST_CHECK(!is_iterator<iter11 &>::value);
    BOOST_CHECK(!is_iterator<const iter11>::value);
    BOOST_CHECK(!is_input_iterator<iter12>::value);
    BOOST_CHECK(!is_input_iterator<iter12 &>::value);
    BOOST_CHECK(!is_input_iterator<const iter12>::value);
    BOOST_CHECK(is_iterator<iter12>::value);
    BOOST_CHECK(!is_iterator<iter12 &>::value);
    BOOST_CHECK(!is_iterator<const iter12>::value);
    BOOST_CHECK(is_input_iterator<iter13>::value);
    // NOTE: cannot use iter13 for writing, as it dereferences
    // to an int rather than int &.
    BOOST_CHECK((!is_output_iterator<iter13, int &>::value));
    BOOST_CHECK(!is_input_iterator<iter13 &>::value);
    BOOST_CHECK(!is_input_iterator<const iter13>::value);
    // Forward iterator.
    BOOST_CHECK(!is_forward_iterator<void>::value);
    BOOST_CHECK(is_forward_iterator<int *>::value);
    BOOST_CHECK((is_output_iterator<int *, int &>::value));
    BOOST_CHECK(is_forward_iterator<const int *>::value);
    BOOST_CHECK(is_forward_iterator<std::vector<int>::iterator>::value);
    BOOST_CHECK((is_output_iterator<std::vector<int>::iterator, int &>::value));
    BOOST_CHECK((is_output_iterator<std::vector<int>::iterator, double &>::value));
    BOOST_CHECK((!is_output_iterator<std::vector<int>::iterator, std::string &>::value));
    BOOST_CHECK(is_forward_iterator<std::vector<int>::const_iterator>::value);
    BOOST_CHECK(!is_forward_iterator<std::vector<int>::iterator &>::value);
    BOOST_CHECK(!is_forward_iterator<std::istream_iterator<char>>::value);
    BOOST_CHECK((is_forward_iterator<std::map<int, int>::iterator>::value));
    BOOST_CHECK(is_forward_iterator<iter14>::value);
    BOOST_CHECK((is_output_iterator<iter14, int>::value));
    BOOST_CHECK(!is_forward_iterator<iter14 &>::value);
    BOOST_CHECK(!is_forward_iterator<const iter14>::value);
    BOOST_CHECK(!is_forward_iterator<iter15>::value);
    BOOST_CHECK(!is_forward_iterator<iter15 &>::value);
    BOOST_CHECK(!is_forward_iterator<const iter15>::value);
    BOOST_CHECK(is_input_iterator<iter15>::value);
    BOOST_CHECK(!is_input_iterator<iter15 &>::value);
    BOOST_CHECK(!is_input_iterator<const iter15>::value);
    BOOST_CHECK(!is_forward_iterator<iter17>::value);
    BOOST_CHECK(!is_forward_iterator<iter17 &>::value);
    BOOST_CHECK(!is_forward_iterator<const iter17>::value);
    BOOST_CHECK(is_iterator<iter17>::value);
    BOOST_CHECK(!is_iterator<iter17 &>::value);
    BOOST_CHECK(!is_iterator<const iter17>::value);
    BOOST_CHECK(!is_forward_iterator<iter18>::value);
    BOOST_CHECK(!is_forward_iterator<iter18 &>::value);
    BOOST_CHECK(!is_forward_iterator<const iter18>::value);
    BOOST_CHECK(!is_iterator<iter18>::value);
    BOOST_CHECK(!is_iterator<iter18 &>::value);
    BOOST_CHECK(!is_iterator<const iter18>::value);
    BOOST_CHECK(!is_forward_iterator<iter19>::value);
    BOOST_CHECK(!is_forward_iterator<iter19 &>::value);
    BOOST_CHECK(!is_forward_iterator<const iter19>::value);
    BOOST_CHECK(!is_input_iterator<iter19>::value);
    BOOST_CHECK(!is_input_iterator<iter19 &>::value);
    BOOST_CHECK(!is_input_iterator<const iter19>::value);
    BOOST_CHECK(!is_forward_iterator<iter20>::value);
    BOOST_CHECK(!is_forward_iterator<iter20 &>::value);
    BOOST_CHECK(!is_forward_iterator<const iter20>::value);
    BOOST_CHECK(!is_input_iterator<iter20>::value);
    BOOST_CHECK(!is_input_iterator<iter20 &>::value);
    BOOST_CHECK(!is_input_iterator<const iter20>::value);
    BOOST_CHECK(!is_forward_iterator<iter21>::value);
    BOOST_CHECK(!is_forward_iterator<iter21 &>::value);
    BOOST_CHECK(!is_forward_iterator<const iter21>::value);
    BOOST_CHECK(!is_input_iterator<iter21>::value);
    BOOST_CHECK(!is_input_iterator<iter21 &>::value);
    BOOST_CHECK(!is_input_iterator<const iter21>::value);
    BOOST_CHECK(is_iterator<iter21>::value);
    BOOST_CHECK(!is_iterator<iter21 &>::value);
    BOOST_CHECK(!is_iterator<const iter21>::value);
}

BOOST_AUTO_TEST_CASE(type_traits_mutable_forward_iterator)
{
    BOOST_CHECK((!is_mutable_forward_iterator<void>::value));
    BOOST_CHECK((is_mutable_forward_iterator<int *>::value));
    BOOST_CHECK((is_mutable_forward_iterator<std::vector<int>::iterator>::value));
    BOOST_CHECK((is_mutable_forward_iterator<std::list<int>::iterator>::value));
    BOOST_CHECK((!is_mutable_forward_iterator<const int *>::value));
    BOOST_CHECK((!is_mutable_forward_iterator<std::vector<int>::const_iterator>::value));
    BOOST_CHECK((!is_mutable_forward_iterator<std::istreambuf_iterator<char>>::value));
    BOOST_CHECK((!is_mutable_forward_iterator<std::list<int>::const_iterator>::value));
    BOOST_CHECK((!is_mutable_forward_iterator<std::set<int>::iterator>::value));
    BOOST_CHECK((is_mutable_forward_iterator<std::map<int, int>::iterator>::value));
    BOOST_CHECK((!is_mutable_forward_iterator<std::map<int, int>::const_iterator>::value));
}

template <typename S>
using sai = safe_abs_sint<S>;

BOOST_AUTO_TEST_CASE(type_traits_safe_abs_sint_test)
{
    BOOST_CHECK(sai<signed char>::value > 1);
    BOOST_CHECK(sai<short>::value > 1);
    BOOST_CHECK(sai<int>::value > 1);
    BOOST_CHECK(sai<long>::value > 1);
    BOOST_CHECK(sai<long long>::value > 1);
}

struct good_begin_end_mut {
    int *begin();
    int *end();
};

struct good_begin_end_const {
    const int *begin() const;
    const int *end() const;
};

// Missing end.
struct bad_begin_end_00 {
    int *begin();
};

// Missing begin.
struct bad_begin_end_01 {
    int *end();
};

// Bad iters.
struct bad_begin_end_02 {
    int begin();
    int end();
};

// Mismatched iters.
struct bad_begin_end_03 {
    int *begin();
    double *end();
};

// Input but not forward.
struct input_only_00 {
    std::istreambuf_iterator<char> begin();
    std::istreambuf_iterator<char> end();
};

// Forward with adl.
struct forward_adl_00 {
};

int *begin(const forward_adl_00 &);
int *end(const forward_adl_00 &);

// Forward with adl, broken (type mismatch).
struct forward_adl_01 {
};

int *begin(const forward_adl_01 &);
double *end(const forward_adl_01 &);

// Forward with adl, broken (missing end).
struct forward_adl_02 {
};

int *begin(const forward_adl_02 &);

BOOST_AUTO_TEST_CASE(type_traits_input_range_test)
{
    BOOST_CHECK((!is_input_range<void>::value));
    BOOST_CHECK((is_input_range<std::vector<int> &>::value));
    BOOST_CHECK((is_input_range<const std::vector<int> &>::value));
    BOOST_CHECK((is_input_range<std::vector<int> &&>::value));
    BOOST_CHECK((is_input_range<std::initializer_list<int> &&>::value));
    BOOST_CHECK(is_input_range<std::list<int> &>::value);
    BOOST_CHECK(is_input_range<const std::list<double> &>::value);
    BOOST_CHECK(is_input_range<std::set<int> &>::value);
    BOOST_CHECK(is_input_range<const std::set<long> &>::value);
    BOOST_CHECK(is_input_range<int(&)[3]>::value);
    BOOST_CHECK(is_input_range<good_begin_end_mut &>::value);
    BOOST_CHECK(!is_input_range<const good_begin_end_mut &>::value);
    BOOST_CHECK(is_input_range<good_begin_end_const &>::value);
    BOOST_CHECK(is_input_range<const good_begin_end_const &>::value);
    BOOST_CHECK(!is_input_range<bad_begin_end_00 &>::value);
    BOOST_CHECK(is_input_range<input_only_00 &>::value);
    BOOST_CHECK(is_input_range<forward_adl_00 &>::value);
    BOOST_CHECK(!is_input_range<forward_adl_01 &>::value);
    BOOST_CHECK(!is_input_range<forward_adl_02 &>::value);
}

BOOST_AUTO_TEST_CASE(type_traits_forward_range_test)
{
    BOOST_CHECK((!is_forward_range<void>::value));
    BOOST_CHECK((is_forward_range<std::vector<int> &>::value));
    BOOST_CHECK((is_forward_range<const std::vector<int> &>::value));
    BOOST_CHECK((is_forward_range<std::vector<int> &&>::value));
    BOOST_CHECK((is_forward_range<std::initializer_list<int> &&>::value));
    BOOST_CHECK(is_forward_range<std::list<int> &>::value);
    BOOST_CHECK(is_forward_range<const std::list<double> &>::value);
    BOOST_CHECK(is_forward_range<std::set<int> &>::value);
    BOOST_CHECK(is_forward_range<const std::set<long> &>::value);
    BOOST_CHECK(is_forward_range<int(&)[3]>::value);
    BOOST_CHECK(is_forward_range<good_begin_end_mut &>::value);
    BOOST_CHECK(!is_forward_range<const good_begin_end_mut &>::value);
    BOOST_CHECK(is_forward_range<good_begin_end_const &>::value);
    BOOST_CHECK(is_forward_range<const good_begin_end_const &>::value);
    BOOST_CHECK(!is_forward_range<bad_begin_end_00 &>::value);
    BOOST_CHECK(!is_forward_range<input_only_00 &>::value);
    BOOST_CHECK(is_forward_range<forward_adl_00 &>::value);
    BOOST_CHECK(!is_forward_range<forward_adl_01 &>::value);
    BOOST_CHECK(!is_forward_range<forward_adl_02 &>::value);
    BOOST_CHECK((is_forward_range<std::map<int, int> &>::value));
    BOOST_CHECK((is_forward_range<const std::map<int, int> &>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_mutable_forward_range_test)
{
    BOOST_CHECK((!is_mutable_forward_range<void>::value));
    BOOST_CHECK((is_mutable_forward_range<std::vector<int> &>::value));
    BOOST_CHECK((!is_mutable_forward_range<const std::vector<int> &>::value));
    BOOST_CHECK((!is_mutable_forward_range<std::vector<int> &&>::value));
    BOOST_CHECK((!is_mutable_forward_range<std::initializer_list<int> &&>::value));
    BOOST_CHECK(is_mutable_forward_range<std::list<int> &>::value);
    BOOST_CHECK(!is_mutable_forward_range<const std::list<double> &>::value);
    BOOST_CHECK(!is_mutable_forward_range<std::set<int> &>::value);
    BOOST_CHECK(!is_mutable_forward_range<const std::set<long> &>::value);
    BOOST_CHECK(is_mutable_forward_range<int(&)[3]>::value);
    BOOST_CHECK(!is_mutable_forward_range<const int(&)[3]>::value);
    BOOST_CHECK(is_mutable_forward_range<good_begin_end_mut &>::value);
    BOOST_CHECK(!is_mutable_forward_range<const good_begin_end_mut &>::value);
    BOOST_CHECK(!is_mutable_forward_range<good_begin_end_const &>::value);
    BOOST_CHECK(!is_mutable_forward_range<const good_begin_end_const &>::value);
    BOOST_CHECK(!is_mutable_forward_range<bad_begin_end_00 &>::value);
    BOOST_CHECK(!is_mutable_forward_range<input_only_00 &>::value);
    BOOST_CHECK(is_mutable_forward_range<forward_adl_00 &>::value);
    BOOST_CHECK(!is_mutable_forward_range<forward_adl_01 &>::value);
    BOOST_CHECK(!is_mutable_forward_range<forward_adl_02 &>::value);
    BOOST_CHECK((is_mutable_forward_range<std::map<int, int> &>::value));
    BOOST_CHECK((!is_mutable_forward_range<const std::map<int, int> &>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_shift_test)
{
    BOOST_CHECK(has_left_shift<int>::value);
    BOOST_CHECK((!has_left_shift<void>::value));
    BOOST_CHECK((!has_left_shift<void, int>::value));
    BOOST_CHECK((!has_left_shift<int, void>::value));
    BOOST_CHECK((has_left_shift<int, long>::value));
    BOOST_CHECK((has_left_shift<int &, char &&>::value));
    BOOST_CHECK((has_left_shift<int &, const short &>::value));
    BOOST_CHECK(!has_left_shift<double>::value);
    BOOST_CHECK((!has_left_shift<double, long>::value));
    BOOST_CHECK((!has_left_shift<long, double>::value));
    BOOST_CHECK((!has_left_shift<long, std::string>::value));
    BOOST_CHECK((!has_left_shift<std::string, long>::value));
    // lshift operator for ostream has incompatible signature, as
    // the first argument is passed as mutable reference.
    BOOST_CHECK((!has_left_shift<std::ostream, long>::value));
    BOOST_CHECK((!has_left_shift_in_place<void>::value));
    BOOST_CHECK((!has_left_shift_in_place<void, int>::value));
    BOOST_CHECK((!has_left_shift_in_place<int, void>::value));
    BOOST_CHECK(has_left_shift_in_place<int>::value);
    BOOST_CHECK((has_left_shift_in_place<int, long>::value));
    BOOST_CHECK((has_left_shift_in_place<int &, long const>::value));
    BOOST_CHECK((!has_left_shift_in_place<const int, long>::value));
    BOOST_CHECK((!has_left_shift_in_place<float, long>::value));
    BOOST_CHECK((!has_left_shift_in_place<long, float>::value));
    BOOST_CHECK((!has_left_shift_in_place<long, std::string>::value));
    // Right shift.
    BOOST_CHECK((!has_right_shift<void>::value));
    BOOST_CHECK((!has_right_shift<void, int>::value));
    BOOST_CHECK((!has_right_shift<int, void>::value));
    BOOST_CHECK(has_right_shift<int>::value);
    BOOST_CHECK((has_right_shift<int, long>::value));
    BOOST_CHECK((has_right_shift<int &, char &&>::value));
    BOOST_CHECK((has_right_shift<int &, const short &>::value));
    BOOST_CHECK(!has_right_shift<double>::value);
    BOOST_CHECK((!has_right_shift<double, long>::value));
    BOOST_CHECK((!has_right_shift<long, double>::value));
    BOOST_CHECK((!has_right_shift<long, std::string>::value));
    BOOST_CHECK((!has_right_shift<std::string, long>::value));
    BOOST_CHECK((!has_right_shift<std::istream, long>::value));
    BOOST_CHECK((!has_right_shift_in_place<void>::value));
    BOOST_CHECK((!has_right_shift_in_place<void, int>::value));
    BOOST_CHECK((!has_right_shift_in_place<int, void>::value));
    BOOST_CHECK(has_right_shift_in_place<int>::value);
    BOOST_CHECK((has_right_shift_in_place<int, long>::value));
    BOOST_CHECK((has_right_shift_in_place<int &, long const>::value));
    BOOST_CHECK((!has_right_shift_in_place<const int, long>::value));
    BOOST_CHECK((!has_right_shift_in_place<float, long>::value));
    BOOST_CHECK((!has_right_shift_in_place<long, float>::value));
    BOOST_CHECK((!has_right_shift_in_place<long, std::string>::value));
}

struct unreturnable_00 {
    unreturnable_00(const unreturnable_00 &) = delete;
    unreturnable_00(unreturnable_00 &&) = delete;
};

struct unreturnable_01 {
    ~unreturnable_01() = delete;
};

BOOST_AUTO_TEST_CASE(type_traits_is_returnable_test)
{
    BOOST_CHECK(is_returnable<void>::value);
    BOOST_CHECK(is_returnable<int>::value);
    BOOST_CHECK(is_returnable<int &>::value);
    BOOST_CHECK(is_returnable<const int &>::value);
    BOOST_CHECK(is_returnable<int &&>::value);
    BOOST_CHECK(is_returnable<int *>::value);
    BOOST_CHECK(is_returnable<std::string>::value);
    BOOST_CHECK(is_returnable<std::thread>::value);
    BOOST_CHECK(is_returnable<std::unique_ptr<int>>::value);
    BOOST_CHECK(is_returnable<std::shared_ptr<int>>::value);
    BOOST_CHECK(!is_returnable<unreturnable_00>::value);
    BOOST_CHECK(is_returnable<unreturnable_00 &>::value);
    BOOST_CHECK(!is_returnable<unreturnable_01>::value);
    BOOST_CHECK(is_returnable<unreturnable_01 &>::value);
}

BOOST_AUTO_TEST_CASE(type_traits_ref_mod_t)
{
    BOOST_CHECK((std::is_same<int, uncvref_t<int>>::value));
    BOOST_CHECK((std::is_same<int, uncvref_t<int &>>::value));
    BOOST_CHECK((std::is_same<int, uncvref_t<const int &>>::value));
    BOOST_CHECK((std::is_same<int, uncvref_t<const int &&>>::value));
    BOOST_CHECK((std::is_same<int, uncvref_t<const int>>::value));
    BOOST_CHECK((std::is_same<int, uncvref_t<volatile int &>>::value));
    BOOST_CHECK((std::is_same<int, unref_t<int>>::value));
    BOOST_CHECK((std::is_same<int, unref_t<int &>>::value));
    BOOST_CHECK((!std::is_same<int, unref_t<int volatile &>>::value));
    BOOST_CHECK((std::is_same<const int, unref_t<int const &>>::value));
    BOOST_CHECK((!std::is_same<int, unref_t<int const &>>::value));
    BOOST_CHECK((std::is_same<int &, addlref_t<int>>::value));
    BOOST_CHECK((std::is_same<int &, addlref_t<int &>>::value));
    BOOST_CHECK((std::is_same<int &, addlref_t<int &&>>::value));
    BOOST_CHECK((std::is_same<void, addlref_t<void>>::value));
    BOOST_CHECK((std::is_same<int, decay_t<int>>::value));
    BOOST_CHECK((std::is_same<int, decay_t<int &>>::value));
    BOOST_CHECK((std::is_same<int, decay_t<const int &>>::value));
    BOOST_CHECK((std::is_same<int, decay_t<int &&>>::value));
    BOOST_CHECK((std::is_same<int *, decay_t<int[2]>>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_is_detected)
{
    BOOST_CHECK((is_detected<add_t, int, int>::value));
    BOOST_CHECK((std::is_same<detected_t<add_t, int, int>, int>::value));
    BOOST_CHECK((is_detected<add_t, double, int>::value));
    BOOST_CHECK((std::is_same<detected_t<add_t, int, double>, double>::value));
    BOOST_CHECK((is_detected<add_t, char, char>::value));
    BOOST_CHECK((std::is_same<detected_t<add_t, char, char>, int>::value));
    BOOST_CHECK((!is_detected<add_t, double, std::string>::value));
    BOOST_CHECK((std::is_same<detected_t<add_t, double, std::string>, nonesuch>::value));
}

template <typename T>
struct tt_0 {
};

BOOST_AUTO_TEST_CASE(type_traits_conj_disj_neg)
{
    BOOST_CHECK((conjunction<std::is_same<int, int>, std::is_convertible<float, int>>::value));
    BOOST_CHECK((!conjunction<std::is_same<float, int>, std::is_convertible<float, int>>::value));
    BOOST_CHECK((!conjunction<std::is_same<float, int>, tt_0<float>>::value));
    BOOST_CHECK((disjunction<std::is_same<float, int>, std::is_convertible<float, int>>::value));
    BOOST_CHECK((!disjunction<std::is_same<float, int>, std::is_convertible<float, tt_0<int>>>::value));
    BOOST_CHECK((disjunction<std::is_same<float, float>, tt_0<float>>::value));
    BOOST_CHECK((conjunction<negation<std::is_same<float, int>>, std::is_convertible<float, int>>::value));
    BOOST_CHECK((disjunction<negation<std::is_same<float, int>>, std::is_convertible<float, tt_0<int>>>::value));
}

struct times_two {
    template <typename T>
    void operator()(T &x) const
    {
        x = 2 * x;
    }
};

struct minus_one {
    template <typename T>
    void operator()(T &x) const
    {
        x -= 1;
    }
};

BOOST_AUTO_TEST_CASE(type_traits_tuple_for_each)
{
    auto t = std::make_tuple(1, 2., 3l, 4ll);
    tuple_for_each(t, times_two{});
    BOOST_CHECK(t == std::make_tuple(2, 4., 6l, 8ll));
    tuple_for_each(t, minus_one{});
    BOOST_CHECK(t == std::make_tuple(1, 3., 5l, 7ll));
}

BOOST_AUTO_TEST_CASE(type_traits_zero_is_absorbing)
{
    BOOST_CHECK((zero_is_absorbing<int>::value));
    BOOST_CHECK((zero_is_absorbing<short>::value));
    BOOST_CHECK((zero_is_absorbing<long long>::value));
    BOOST_CHECK((zero_is_absorbing<unsigned long>::value));
    BOOST_CHECK((zero_is_absorbing<int &>::value));
    BOOST_CHECK((zero_is_absorbing<const short>::value));
    BOOST_CHECK((zero_is_absorbing<long long &&>::value));
    BOOST_CHECK((zero_is_absorbing<const unsigned long &>::value));
    if (std::numeric_limits<double>::has_quiet_NaN || std::numeric_limits<double>::has_signaling_NaN) {
        BOOST_CHECK((!zero_is_absorbing<double>::value));
        BOOST_CHECK((!zero_is_absorbing<double &>::value));
        BOOST_CHECK((!zero_is_absorbing<const double &>::value));
        BOOST_CHECK((!zero_is_absorbing<double &&>::value));
    }
    if (std::numeric_limits<long double>::has_quiet_NaN || std::numeric_limits<long double>::has_signaling_NaN) {
        BOOST_CHECK((!zero_is_absorbing<long double>::value));
        BOOST_CHECK((!zero_is_absorbing<long double &>::value));
        BOOST_CHECK((!zero_is_absorbing<const long double &>::value));
        BOOST_CHECK((!zero_is_absorbing<long double &&>::value));
    }
    if (std::numeric_limits<float>::has_quiet_NaN || std::numeric_limits<float>::has_signaling_NaN) {
        BOOST_CHECK((!zero_is_absorbing<float>::value));
        BOOST_CHECK((!zero_is_absorbing<float &>::value));
        BOOST_CHECK((!zero_is_absorbing<const float &>::value));
        BOOST_CHECK((!zero_is_absorbing<float &&>::value));
    }
}

BOOST_AUTO_TEST_CASE(type_traits_disj_idx)
{
    BOOST_CHECK(disjunction_idx<>::value == 0u);
    BOOST_CHECK((disjunction_idx<std::is_same<int, int>>::value == 0u));
    BOOST_CHECK((disjunction_idx<std::is_same<int, long>>::value == 1u));
    BOOST_CHECK((disjunction_idx<std::is_same<int, long>, std::is_same<int, int>>::value == 1u));
    BOOST_CHECK((disjunction_idx<std::is_same<int, long>, std::is_same<int, double>>::value == 2u));
    BOOST_CHECK((disjunction_idx<std::is_same<int, int>, std::is_same<int, double>>::value == 0u));
    BOOST_CHECK(
        (disjunction_idx<std::is_same<int, int>, std::is_same<int, double>, std::is_same<int, std::string>>::value
         == 0u));
    BOOST_CHECK(
        (disjunction_idx<std::is_same<int, float>, std::is_same<int, int>, std::is_same<int, std::string>>::value
         == 1u));
    BOOST_CHECK(
        (disjunction_idx<std::is_same<int, float>, std::is_same<int, float>, std::is_same<int, int>>::value == 2u));
    BOOST_CHECK(
        (disjunction_idx<std::is_same<int, float>, std::is_same<int, float>, std::is_same<int, std::string>>::value
         == 3u));
}

BOOST_AUTO_TEST_CASE(type_traits_cpp_complex)
{
    BOOST_CHECK(!is_cpp_complex<void>::value);
    BOOST_CHECK(!is_cpp_complex<float>::value);
    BOOST_CHECK(!is_cpp_complex<float &>::value);
    BOOST_CHECK(is_cpp_complex<std::complex<float>>::value);
    BOOST_CHECK(is_cpp_complex<std::complex<double>>::value);
    BOOST_CHECK(is_cpp_complex<std::complex<long double>>::value);
    BOOST_CHECK(is_cpp_complex<const std::complex<float>>::value);
    BOOST_CHECK(is_cpp_complex<const std::complex<double>>::value);
    BOOST_CHECK(is_cpp_complex<const std::complex<long double>>::value);
    BOOST_CHECK(is_cpp_complex<const volatile std::complex<float>>::value);
    BOOST_CHECK(is_cpp_complex<const volatile std::complex<double>>::value);
    BOOST_CHECK(is_cpp_complex<const volatile std::complex<long double>>::value);
    BOOST_CHECK(is_cpp_complex<volatile std::complex<float>>::value);
    BOOST_CHECK(is_cpp_complex<volatile std::complex<double>>::value);
    BOOST_CHECK(is_cpp_complex<volatile std::complex<long double>>::value);
    BOOST_CHECK(!is_cpp_complex<std::complex<float> &>::value);
    BOOST_CHECK(!is_cpp_complex<std::complex<double> &&>::value);
    BOOST_CHECK(!is_cpp_complex<const std::complex<long double> &>::value);
}

BOOST_AUTO_TEST_CASE(type_traits_is_string_type_test)
{
    BOOST_CHECK(is_string_type<char *>::value);
    BOOST_CHECK(is_string_type<const char *>::value);
    BOOST_CHECK(is_string_type<char[10]>::value);
    BOOST_CHECK(is_string_type<char[]>::value);
    BOOST_CHECK(is_string_type<std::string>::value);
#if PIRANHA_CPLUSPLUS >= 201703L
    BOOST_CHECK(is_string_type<std::string_view>::value);
#endif
    BOOST_CHECK(!is_string_type<std::string &>::value);
    BOOST_CHECK(!is_string_type<std::string &&>::value);
    BOOST_CHECK(!is_string_type<const std::string &>::value);
    BOOST_CHECK(is_string_type<const std::string>::value);
    BOOST_CHECK(!is_string_type<void>::value);
    BOOST_CHECK(!is_string_type<int>::value);
}

BOOST_AUTO_TEST_CASE(type_traits_same_test)
{
    BOOST_CHECK((are_same<void>::value));
    BOOST_CHECK((are_same<int>::value));
    BOOST_CHECK((are_same<const int>::value));
    BOOST_CHECK((are_same<int &>::value));
    BOOST_CHECK((are_same<const int &>::value));
    BOOST_CHECK((are_same<int, int>::value));
    BOOST_CHECK((!are_same<const int, int>::value));
    BOOST_CHECK((!are_same<int, int &>::value));
    BOOST_CHECK((!are_same<int, double>::value));
    BOOST_CHECK((are_same<int, int, int>::value));
    BOOST_CHECK((are_same<double, double, double>::value));
    BOOST_CHECK((!are_same<int, int, const int>::value));
    BOOST_CHECK((!are_same<const int, int, int>::value));
    BOOST_CHECK((!are_same<const int, double &, void>::value));
    BOOST_CHECK((!are_same<double, int, const int>::value));
    BOOST_CHECK((!are_same<int, volatile int, const int>::value));
}

BOOST_AUTO_TEST_CASE(type_traits_preinc_test)
{
    BOOST_CHECK(is_preincrementable<int &>::value);
    BOOST_CHECK(!is_preincrementable<int>::value);
    BOOST_CHECK(!is_preincrementable<const int>::value);
    BOOST_CHECK(!is_preincrementable<const int &>::value);
    BOOST_CHECK(is_preincrementable<double &>::value);
    BOOST_CHECK(is_preincrementable<int *&>::value);
    BOOST_CHECK(!is_preincrementable<void>::value);
}

BOOST_AUTO_TEST_CASE(type_traits_postinc_test)
{
    BOOST_CHECK(is_postincrementable<int &>::value);
    BOOST_CHECK(!is_postincrementable<int>::value);
    BOOST_CHECK(!is_postincrementable<const int>::value);
    BOOST_CHECK(!is_postincrementable<const int &>::value);
    BOOST_CHECK(is_postincrementable<double &>::value);
    BOOST_CHECK(is_postincrementable<int *&>::value);
    BOOST_CHECK(!is_postincrementable<void>::value);
}

BOOST_AUTO_TEST_CASE(type_traits_output_it)
{
    BOOST_CHECK((!is_output_iterator<void, void>::value));
    BOOST_CHECK((!is_output_iterator<void, double>::value));
    BOOST_CHECK((!is_output_iterator<double, void>::value));
    BOOST_CHECK((is_output_iterator<std::ostream_iterator<double>, double &>::value));
    BOOST_CHECK((is_output_iterator<std::ostream_iterator<double>, int>::value));
    BOOST_CHECK((!is_output_iterator<std::ostream_iterator<double>, std::string &>::value));
    BOOST_CHECK((!is_input_iterator<std::ostream_iterator<double>>::value));
    BOOST_CHECK((is_output_iterator<int *, int &>::value));
    BOOST_CHECK((is_output_iterator<int *, int &&>::value));
    BOOST_CHECK((is_output_iterator<int *, double &&>::value));
    BOOST_CHECK((!is_output_iterator<int *, std::string &>::value));
    BOOST_CHECK((is_output_iterator<std::list<int>::iterator, int &>::value));
    BOOST_CHECK((!is_output_iterator<std::list<int>::const_iterator, int &>::value));
}

// Swappable via std::swap().
struct swap00 {
};

// Swappable via ADL.
struct swap00a {
};

void swap(swap00a &, swap00a &);

// Swappable only via ADL and mixed with swap00: swapping with self
// fails because there's no ADL overload, and std::swap()
// is blocked by the lack of move ctor.
struct swap01 {
    swap01(swap01 &&) = delete;
};

void swap(swap00 &, swap01 &);
void swap(swap01 &, swap00 &);

// Same as above, but move assignment is missing.
struct swap02 {
    swap02 &operator=(swap02 &&) = delete;
};

void swap(swap01 &, swap02 &);
void swap(swap02 &, swap01 &);

// std::swap() is disabled, ADL enabled with strange arg types.
struct swap03 {
    swap03(swap03 &&) = delete;
};

void swap(const swap03 &, swap03 &);
void swap(const swap03 &, const swap03 &);

// Missing the other way around for ADL swap.
struct swap04 {
    swap04 &operator=(swap04 &&) = delete;
};

void swap(swap03 &, swap04 &);

BOOST_AUTO_TEST_CASE(type_traits_swappable)
{
    BOOST_CHECK((!is_swappable<void>::value));
    BOOST_CHECK((!is_swappable<void, int &>::value));
    BOOST_CHECK((!is_swappable<int &, void>::value));
    BOOST_CHECK((is_swappable<int &>::value));
    BOOST_CHECK((!is_swappable<const int &>::value));
    BOOST_CHECK((!is_swappable<int &&>::value));
    BOOST_CHECK((is_swappable<swap00 &>::value));
    BOOST_CHECK((is_swappable<swap00a &>::value));
    BOOST_CHECK((!is_swappable<swap01 &>::value));
    BOOST_CHECK((is_swappable<swap01 &, swap00 &>::value));
    BOOST_CHECK((is_swappable<swap00 &, swap01 &>::value));
    BOOST_CHECK((!is_swappable<swap02 &>::value));
    BOOST_CHECK((is_swappable<swap02 &, swap01 &>::value));
    BOOST_CHECK((is_swappable<swap01 &, swap02 &>::value));
    BOOST_CHECK((is_swappable<swap03 &>::value));
    BOOST_CHECK((is_swappable<const swap03 &>::value));
    BOOST_CHECK((is_swappable<const swap03 &, swap03 &>::value));
    BOOST_CHECK((!is_swappable<swap03 &, swap04 &>::value));
    BOOST_CHECK((!is_swappable<swap04 &, swap03 &>::value));
    BOOST_CHECK((is_swappable<int(&)[3]>::value));
    BOOST_CHECK((is_swappable<swap00(&)[3]>::value));
    BOOST_CHECK((is_swappable<std::string(&)[3]>::value));
    BOOST_CHECK((!is_swappable<swap01(&)[3]>::value));
    BOOST_CHECK((!is_swappable<swap01(&)[3], swap00(&)[3]>::value));
#if PIRANHA_CPLUSPLUS < 201703L
    BOOST_CHECK((std_swap_viable<swap00 &, swap00 &>::value));
    BOOST_CHECK((using_std_adl_swap::detected<swap00 &, swap00 &>::value));
    BOOST_CHECK((std_swap_viable<swap00a &, swap00a &>::value));
    BOOST_CHECK((!adl_swap::detected<swap00 &, swap00 &>::value));
    BOOST_CHECK((adl_swap::detected<swap00a &, swap00a &>::value));
    BOOST_CHECK((!std_swap_viable<swap01 &, swap01 &>::value));
    BOOST_CHECK((adl_swap::detected<swap01 &, swap00 &>::value));
    BOOST_CHECK((!std_swap_viable<swap02 &, swap02 &>::value));
    BOOST_CHECK((adl_swap::detected<swap01 &, swap02 &>::value));
    BOOST_CHECK((!std_swap_viable<swap03 &, swap03 &>::value));
    BOOST_CHECK((adl_swap::detected<swap03 &, swap03 &>::value));
    BOOST_CHECK((adl_swap::detected<const swap03 &, const swap03 &>::value));
    BOOST_CHECK((adl_swap::detected<const swap03 &, swap03 &>::value));
    BOOST_CHECK((adl_swap::detected<swap03 &, const swap03 &>::value));
#endif
}

template <typename T>
struct foo_empty {
};

template <typename T>
using dcond_tester_0 = conjunction<std::is_floating_point<T>, dcond<foo_empty<T>, foo_empty<T>, foo_empty<T>>>;

template <typename T>
using dcond_tester_1
    = conjunction<std::is_floating_point<T>, dcond<std::is_same<T, float>, std::true_type, foo_empty<T>>>;

template <typename T>
using dcond_tester_2
    = conjunction<std::is_floating_point<T>, dcond<std::is_same<T, float>, foo_empty<T>, std::true_type>>;

BOOST_AUTO_TEST_CASE(type_traits_dcond_test)
{
    // Check that dcond instantiates the condition type only when dcond itself is instantiated.
    BOOST_CHECK(!dcond_tester_0<int>::value);
    // Check that only one of the branches is instantiated.
    BOOST_CHECK(dcond_tester_1<float>::value);
    BOOST_CHECK(dcond_tester_2<double>::value);
}

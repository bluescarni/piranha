/* Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)

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

#include "../src/convert_to.hpp"

#define BOOST_TEST_MODULE convert_to_test
#include <boost/test/unit_test.hpp>

#include <string>
#include <type_traits>
#include <utility>

#include "../src/config.hpp"
#include "../src/init.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/real.hpp"

static void test_func() {}
static auto l1 = []() {};

struct base {};
struct derived : base {};

struct base2 {};

struct conv1;
struct conv2;

struct conv1 {
  operator conv2() const;
};

struct conv2 {};
struct conv3 {};
struct conv4 {};
struct conv5 {};

struct conv_nr_00 {
  conv_nr_00() = default;
  ~conv_nr_00() = delete;
};

namespace piranha {

// Wrong custom converter.
template <> struct convert_to_impl<conv1, conv2, void> {};

// Good custom converter.
template <> struct convert_to_impl<conv1, conv3, void> {
  conv1 operator()(const conv3 &) const;
};

// Bad custom converter.
template <> struct convert_to_impl<conv1, conv4, void> {
  int operator()(const conv4 &) const;
};

// Bad custom converter.
template <> struct convert_to_impl<conv1, conv5, void> {
  conv1 operator()(conv5 &) const;
};

template <> struct convert_to_impl<conv_nr_00, conv5, void> {
  conv_nr_00 operator()(const conv5 &) const;
};
}

using namespace piranha;

BOOST_AUTO_TEST_CASE(convert_to_main_test) {
  init();
  BOOST_CHECK_EQUAL(convert_to<int>(3.5), 3);
  BOOST_CHECK((std::is_same<int, decltype(convert_to<int>(3.5))>::value));
  BOOST_CHECK_EQUAL(convert_to<std::string>("asdasd"), "asdasd");
  BOOST_CHECK_NO_THROW(convert_to<std::function<void()>>(test_func));
  BOOST_CHECK_NO_THROW(convert_to<std::function<void()>>(l1));
  BOOST_CHECK_NO_THROW(convert_to<void (*)()>(l1));
  BOOST_CHECK((!has_convert_to<int, std::string>::value));
  BOOST_CHECK((has_convert_to<std::string, const char *>::value));
  BOOST_CHECK((has_convert_to<std::string, char *>::value));
  BOOST_CHECK((has_convert_to<double, long double>::value));
  BOOST_CHECK((has_convert_to<long double, double>::value));
  BOOST_CHECK((has_convert_to<long double, int>::value));
  BOOST_CHECK((has_convert_to<std::function<void()>, void (*)()>::value));
  BOOST_CHECK((has_convert_to<std::function<void()>, decltype(l1)>::value));
  BOOST_CHECK((has_convert_to<void (*)(), decltype(l1)>::value));
  BOOST_CHECK((has_convert_to<real, rational>::value));
  BOOST_CHECK((has_convert_to<rational, real>::value));
  BOOST_CHECK((has_convert_to<integer, real>::value));
  BOOST_CHECK((has_convert_to<real, real>::value));
  // NOTE: this used to be problematic with libc++ in an earlier implementation
  // of convert_to().
  BOOST_CHECK((has_convert_to<int, integer>::value));
  BOOST_CHECK_EQUAL(convert_to<int>(45_z), 45);
  // Some pointer conversions.
  BOOST_CHECK((has_convert_to<derived *, base *>::value));
  BOOST_CHECK((has_convert_to<base *, derived *>::value));
  BOOST_CHECK((!has_convert_to<derived *, const base *>::value));
  BOOST_CHECK((has_convert_to<derived const *, const base *>::value));
#if !defined(PIRANHA_COMPILER_IS_INTEL)
  // This looks like an Intel compiler bug. ICC returns this as false:
  // std::is_same<decltype(static_cast<derived const *>((base
  // *)nullptr)),derived const *>::value
  // It erases the const from "derived const *" when casting.
  BOOST_CHECK((has_convert_to<derived const *, base *>::value));
#endif
  BOOST_CHECK((!has_convert_to<base *, base2 *>::value));
  BOOST_CHECK((!has_convert_to<base2 *, base *>::value));
  // User-defined conversion operator and specialisations.
  BOOST_CHECK((has_convert_to<conv2, conv1>::value));
  BOOST_CHECK((!has_convert_to<conv1, conv2>::value));
  BOOST_CHECK((has_convert_to<conv1, conv3>::value));
  BOOST_CHECK((!has_convert_to<conv1, conv4>::value));
  BOOST_CHECK((!has_convert_to<conv1, conv5>::value));
  BOOST_CHECK((!has_convert_to<conv_nr_00, conv5>::value));
  // Check with cv qualifiers.
  BOOST_CHECK((has_convert_to<double &, const long double &>::value));
  BOOST_CHECK((has_convert_to<const long double, double &&>::value));
  BOOST_CHECK((has_convert_to<const long double, int &>::value));
}

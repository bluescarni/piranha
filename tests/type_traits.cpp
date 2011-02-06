/***************************************************************************
 *   Copyright (C) 2009-2011 by Francesco Biscani                          *
 *   bluescarni@gmail.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "../src/type_traits.hpp"

#define BOOST_TEST_MODULE type_traits_test
#include <boost/test/unit_test.hpp>

#include <string>
#include <type_traits>

using namespace piranha;

PIRANHA_DECLARE_HAS_TYPEDEF(foo_type);

BOOST_AUTO_TEST_CASE(type_traits_strip_cv_test)
{
	BOOST_CHECK((std::is_same<int,strip_cv_ref<const int>::type>::value));
	BOOST_CHECK((std::is_same<int,strip_cv_ref<volatile int>::type>::value));
	BOOST_CHECK((std::is_same<int,strip_cv_ref<const volatile int>::type>::value));
	BOOST_CHECK((std::is_same<int,strip_cv_ref<const int &>::type>::value));
	BOOST_CHECK((std::is_same<int,strip_cv_ref<volatile int &>::type>::value));
	BOOST_CHECK((std::is_same<int,strip_cv_ref<const volatile int &>::type>::value));
	BOOST_CHECK((std::is_same<int,strip_cv_ref<int &&>::type>::value));
	BOOST_CHECK((std::is_same<int,strip_cv_ref<const int &&>::type>::value));
	BOOST_CHECK((std::is_same<int,strip_cv_ref<volatile int &&>::type>::value));
	BOOST_CHECK((std::is_same<int,strip_cv_ref<const volatile int &&>::type>::value));
	BOOST_CHECK((std::is_same<int *,strip_cv_ref<int * const>::type>::value));
	BOOST_CHECK((std::is_same<int *,strip_cv_ref<int * volatile>::type>::value));
	BOOST_CHECK((std::is_same<int *,strip_cv_ref<int * const volatile>::type>::value));
}

struct foo
{
	typedef int foo_type;
};

struct bar {};

BOOST_AUTO_TEST_CASE(type_traits_has_typedef_test)
{
	BOOST_CHECK(has_typedef_foo_type<foo>::value);
	BOOST_CHECK(!has_typedef_foo_type<bar>::value);
}

BOOST_AUTO_TEST_CASE(type_traits_is_cv_ref)
{
	BOOST_CHECK_EQUAL(is_cv_or_ref<int>::value,false);
	BOOST_CHECK_EQUAL(is_cv_or_ref<int &>::value,true);
	BOOST_CHECK_EQUAL(is_cv_or_ref<const int>::value,true);
	BOOST_CHECK_EQUAL(is_cv_or_ref<const volatile int>::value,true);
	BOOST_CHECK_EQUAL(is_cv_or_ref<const volatile int &>::value,true);
	BOOST_CHECK_EQUAL(is_cv_or_ref<volatile int>::value,true);
	BOOST_CHECK_EQUAL(is_cv_or_ref<int * const>::value,true);
	BOOST_CHECK_EQUAL(is_cv_or_ref<int const *>::value,false);
}

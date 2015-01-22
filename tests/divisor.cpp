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

#include "../src/divisor.hpp"

#define BOOST_TEST_MODULE divisor_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <functional>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "../src/environment.hpp"
#include "../src/mp_integer.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

typedef boost::mpl::vector<signed char,short,int,long,long long,integer> value_types;

struct ctor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using d_type = divisor<T>;
		// Default constructor.
		d_type d0;
		BOOST_CHECK_EQUAL(d0.size(),0u);
		T e(1);
		std::vector<T> tmp{T(1),T(-3)};
		d0.insert(tmp.begin(),tmp.end(),e);
		tmp[0u] = 4;
		tmp[1u] = -5;
		d0.insert(tmp.begin(),tmp.end(),e);
		BOOST_CHECK_EQUAL(d0.size(),2u);
		// Copy constructor.
		d_type d1{d0};
		BOOST_CHECK_EQUAL(d1.size(),2u);
		BOOST_CHECK(d1 == d0);
		// Move constructor.
		d_type d2{std::move(d1)};
		BOOST_CHECK_EQUAL(d2.size(),2u);
		BOOST_CHECK(d2 == d0);
		BOOST_CHECK_EQUAL(d1.size(),0u);
		// Copy assignment.
		d_type d3;
		d3 = d0;
		BOOST_CHECK_EQUAL(d2.size(),2u);
		BOOST_CHECK(d2 == d0);
		// Move assignment.
		d_type d4;
		d4 = std::move(d3);
		BOOST_CHECK_EQUAL(d4.size(),2u);
		BOOST_CHECK(d4 == d0);
		BOOST_CHECK_EQUAL(d3.size(),0u);
		// Test clear().
		d4.clear();
		BOOST_CHECK_EQUAL(d4.size(),0u);
	}
};

BOOST_AUTO_TEST_CASE(divisor_ctor_test)
{
	environment env;
	boost::mpl::for_each<value_types>(ctor_tester());
}

struct insert_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using d_type = divisor<T>;
		d_type d0;
		// Insertion with non-positive exponent must fail.
		std::vector<T> tmp;
		T exponent(0);
		BOOST_CHECK_THROW(d0.insert(tmp.begin(),tmp.end(),exponent),std::invalid_argument);
		BOOST_CHECK_EQUAL(d0.size(),0u);
		exponent = -1;
		BOOST_CHECK_THROW(d0.insert(tmp.begin(),tmp.end(),exponent),std::invalid_argument);
		BOOST_CHECK_EQUAL(d0.size(),0u);
		// Various canonical checks.
		exponent = 1;
		// Empty vector must fail.
		tmp = {};
		BOOST_CHECK_THROW(d0.insert(tmp.begin(),tmp.end(),exponent),std::invalid_argument);
		BOOST_CHECK_EQUAL(d0.size(),0u);
		// Vectors of zeroes must fail.
		tmp = {T(0)};
		BOOST_CHECK_THROW(d0.insert(tmp.begin(),tmp.end(),exponent),std::invalid_argument);
		BOOST_CHECK_EQUAL(d0.size(),0u);
		tmp = {T(0),T(0)};
		BOOST_CHECK_THROW(d0.insert(tmp.begin(),tmp.end(),exponent),std::invalid_argument);
		BOOST_CHECK_EQUAL(d0.size(),0u);
		// First nonzero element negative must fail.
		tmp = {T(-1),T(2)};
		BOOST_CHECK_THROW(d0.insert(tmp.begin(),tmp.end(),exponent),std::invalid_argument);
		BOOST_CHECK_EQUAL(d0.size(),0u);
		tmp = {T(0),T(-1),T(2)};
		BOOST_CHECK_THROW(d0.insert(tmp.begin(),tmp.end(),exponent),std::invalid_argument);
		BOOST_CHECK_EQUAL(d0.size(),0u);
		tmp = {T(0),T(-2),T(0),T(3),T(0)};
		BOOST_CHECK_THROW(d0.insert(tmp.begin(),tmp.end(),exponent),std::invalid_argument);
		BOOST_CHECK_EQUAL(d0.size(),0u);
		tmp = {T(-7),T(0),T(-2),T(0),T(3),T(0)};
		BOOST_CHECK_THROW(d0.insert(tmp.begin(),tmp.end(),exponent),std::invalid_argument);
		BOOST_CHECK_EQUAL(d0.size(),0u);
		// Non-coprimes must fail.
		tmp = {T(8),T(0),T(-2),T(0),T(6),T(0)};
		BOOST_CHECK_THROW(d0.insert(tmp.begin(),tmp.end(),exponent),std::invalid_argument);
		BOOST_CHECK_EQUAL(d0.size(),0u);
		tmp = {T(0),T(8),T(0),T(-2),T(0),T(6),T(0)};
		BOOST_CHECK_THROW(d0.insert(tmp.begin(),tmp.end(),exponent),std::invalid_argument);
		BOOST_CHECK_EQUAL(d0.size(),0u);
		tmp = {T(8),T(-2),T(6)};
		BOOST_CHECK_THROW(d0.insert(tmp.begin(),tmp.end(),exponent),std::invalid_argument);
		BOOST_CHECK_EQUAL(d0.size(),0u);
		// Some successful insertions.
		tmp = {T(8),T(-3),T(6)};
		d0.insert(tmp.begin(),tmp.end(),exponent);
		tmp = {T(8),T(-3),T(7)};
		d0.insert(tmp.begin(),tmp.end(),exponent);
		BOOST_CHECK_EQUAL(d0.size(),2u);
		// Update an exponent.
		d0.insert(tmp.begin(),tmp.end(),exponent);
		BOOST_CHECK_EQUAL(d0.size(),2u);
		// Insert another new term.
		tmp = {T(8),T(-3),T(35)};
		d0.insert(tmp.begin(),tmp.end(),exponent);
		BOOST_CHECK_EQUAL(d0.size(),3u);
		// Various range checks dependent on T being an integral type.
		range_checks<T>();
	}
	template <typename T, typename U = T, typename std::enable_if<std::is_integral<U>::value,int>::type = 0>
	static void range_checks()
	{
		divisor<T> d;
		std::vector<T> tmp;
		T exponent(1);
		if (std::numeric_limits<T>::min() < -detail::safe_abs_sint<T>::value &&
			std::numeric_limits<T>::max() > detail::safe_abs_sint<T>::value)
		{
			tmp = {std::numeric_limits<T>::min()};
			BOOST_CHECK_THROW(d.insert(tmp.begin(),tmp.end(),exponent),std::invalid_argument);
			BOOST_CHECK_EQUAL(d.size(),0u);
			tmp = {std::numeric_limits<T>::max()};
			BOOST_CHECK_THROW(d.insert(tmp.begin(),tmp.end(),exponent),std::invalid_argument);
			BOOST_CHECK_EQUAL(d.size(),0u);
		}
		// Check potential failure in safe_cast for the exponent and the values.
		const long long long_e = std::numeric_limits<long long>::max();
		if (std::numeric_limits<T>::max() < long_e) {
			tmp = {T(1)};
			BOOST_CHECK_THROW(d.insert(tmp.begin(),tmp.end(),long_e),std::invalid_argument);
			BOOST_CHECK_EQUAL(d.size(),0u);
			std::vector<long long> tmp2 = {long_e,long_e};
			BOOST_CHECK_THROW(d.insert(tmp2.begin(),tmp2.end(),exponent),std::invalid_argument);
			BOOST_CHECK_EQUAL(d.size(),0u);
		}
		// Check failure in updating the exponent.
		tmp = {T(1)};
		exponent = std::numeric_limits<T>::max();
		d.insert(tmp.begin(),tmp.end(),exponent);
		exponent = T(1);
		BOOST_CHECK_THROW(d.insert(tmp.begin(),tmp.end(),exponent),std::invalid_argument);
		BOOST_CHECK_EQUAL(d.size(),1u);
	}
	template <typename T, typename U = T, typename std::enable_if<!std::is_integral<U>::value,int>::type = 0>
	static void range_checks()
	{}
};

BOOST_AUTO_TEST_CASE(divisor_insert_test)
{
	boost::mpl::for_each<value_types>(insert_tester());
}

struct equality_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using d_type = divisor<T>;
		std::vector<T> tmp;
		T exponent(1);
		d_type d0;
		BOOST_CHECK(d0 == d0);
		d_type d1;
		BOOST_CHECK(d0 == d1);
		tmp = {T(1),T(2)};
		d0.insert(tmp.begin(),tmp.end(),exponent);
		BOOST_CHECK(!(d0 == d1));
		BOOST_CHECK(d0 != d1);
		d1.insert(tmp.begin(),tmp.end(),exponent);
		BOOST_CHECK(d0 == d1);
		tmp = {T(1),T(-2)};
		d0.insert(tmp.begin(),tmp.end(),exponent);
		BOOST_CHECK(!(d0 == d1));
		BOOST_CHECK(d0 != d1);
		exponent = T(2);
		d1.insert(tmp.begin(),tmp.end(),exponent);
		BOOST_CHECK(!(d0 == d1));
		BOOST_CHECK(d0 != d1);
		exponent = T(1);
		d0.insert(tmp.begin(),tmp.end(),exponent);
		BOOST_CHECK(d0 == d1);
	}
};

BOOST_AUTO_TEST_CASE(divisor_equality_test)
{
	boost::mpl::for_each<value_types>(equality_tester());
}

struct hash_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using d_type = divisor<T>;
		std::vector<T> tmp;
		T exponent(1);
		d_type d0;
		std::hash<d_type> hasher;
		BOOST_CHECK_EQUAL(d0.hash(),0u);
		BOOST_CHECK_EQUAL(d0.hash(),hasher(d0));
	}
};

BOOST_AUTO_TEST_CASE(divisor_hash_test)
{
	boost::mpl::for_each<value_types>(hash_tester());
}

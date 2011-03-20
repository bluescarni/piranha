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

#include "../src/array_key.hpp"

#define BOOST_TEST_MODULE array_key_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <unordered_set>

#include "../src/integer.hpp"

using namespace piranha;

typedef boost::mpl::vector<unsigned,integer> value_types;

// Constructors, assignments and element access.
struct constructor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef array_key<T> key_type;
		key_type k0;
		BOOST_CHECK_NO_THROW(key_type{});
		BOOST_CHECK_NO_THROW(key_type(key_type{}));
		BOOST_CHECK_NO_THROW(key_type(k0));
		// From init list.
		key_type k1{T(0),T(1),T(2),T(3)};
		BOOST_CHECK_EQUAL(k1.size(),static_cast<decltype(k1.size())>(4));
		for (typename key_type::size_type i = 0; i < k1.size(); ++i) {
			BOOST_CHECK_EQUAL(k1[i],i);
			BOOST_CHECK_NO_THROW(k1[i] = i + 1);
			BOOST_CHECK_EQUAL(k1[i],i + 1);
		}
		BOOST_CHECK_NO_THROW(k0 = k1);
		BOOST_CHECK_NO_THROW(k0 = std::move(k1));
	}
};

BOOST_AUTO_TEST_CASE(array_key_constructor_test)
{
	boost::mpl::for_each<value_types>(constructor_tester());
}

struct hash_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef array_key<T> key_type;
		key_type k0;
		BOOST_CHECK_EQUAL(k0.hash(),std::size_t());
		BOOST_CHECK_EQUAL(k0.hash(),std::hash<key_type>()(k0));
		key_type k1{T(0),T(1),T(2),T(3)};
		BOOST_CHECK_EQUAL(k1.hash(),std::hash<key_type>()(k1));
	}
};

BOOST_AUTO_TEST_CASE(array_key_hash_test)
{
	boost::mpl::for_each<value_types>(hash_tester());
}

struct push_back_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef array_key<T> key_type;
		key_type k0;
		for (int i = 0; i < 4; ++i) {
			k0.push_back(T(i));
			BOOST_CHECK_EQUAL(k0[i],T(i));
		}
		key_type k1;
		for (int i = 0; i < 4; ++i) {
			T tmp(i);
			k1.push_back(tmp);
			BOOST_CHECK_EQUAL(k1[i],tmp);
		}
	}
};

BOOST_AUTO_TEST_CASE(array_key_push_back_test)
{
	boost::mpl::for_each<value_types>(push_back_tester());
}

struct equality_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef array_key<T> key_type;
		key_type k0;
		BOOST_CHECK(k0 == key_type{});
		for (int i = 0; i < 4; ++i) {
			k0.push_back(T(i));
		}
		key_type k1{T(0),T(1),T(2),T(3)};
		BOOST_CHECK(k0 == k1);
		// Inequality.
		k0 = key_type{};
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

BOOST_AUTO_TEST_CASE(array_key_equality_test)
{
	boost::mpl::for_each<value_types>(equality_tester());
}

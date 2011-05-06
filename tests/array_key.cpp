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
#include <initializer_list>
#include <unordered_set>
#include <vector>

#include "../src/debug_access.hpp"
#include "../src/integer.hpp"
#include "../src/symbol.hpp"

using namespace piranha;

typedef boost::mpl::vector<unsigned,integer> value_types;

template <typename T>
class g_key_type: public array_key<T,g_key_type<T>>
{
		typedef array_key<T,g_key_type<T>> base;
	public:
		g_key_type() = default;
		g_key_type(const g_key_type &) = default;
		g_key_type(g_key_type &&) = default;
		g_key_type &operator=(const g_key_type &) = default;
		g_key_type &operator=(g_key_type &&other) piranha_noexcept_spec(true)
		{
			base::operator=(std::move(other));
			return *this;
		}
		template <typename U>
		explicit g_key_type(std::initializer_list<U> list):base(list) {}
		template <typename ... Args>
		explicit g_key_type(Args && ... params):base(std::forward<Args>(params)...) {}
};

namespace std
{

template <typename T>
class hash<g_key_type<T>>: public hash<array_key<T,g_key_type<T>>> {};

}

// Constructors, assignments and element access.
struct constructor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef g_key_type<T> key_type;
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
		key_type k1a{0,1,2,3};
		BOOST_CHECK_EQUAL(k1a.size(),static_cast<decltype(k1a.size())>(4));
		for (typename key_type::size_type i = 0; i < k1a.size(); ++i) {
			BOOST_CHECK_EQUAL(k1a[i],i);
			BOOST_CHECK_NO_THROW(k1a[i] = i + 1);
			BOOST_CHECK_EQUAL(k1a[i],i + 1);
		}
		BOOST_CHECK_NO_THROW(k0 = k1);
		BOOST_CHECK_NO_THROW(k0 = std::move(k1));
		// Constructor from vector of symbols.
		std::vector<symbol> vs = {symbol("a"),symbol("b"),symbol("c")};
		key_type k2(vs);
		BOOST_CHECK_EQUAL(k2.size(),vs.size());
		BOOST_CHECK_EQUAL(k2[0],T(0));
		BOOST_CHECK_EQUAL(k2[1],T(0));
		BOOST_CHECK_EQUAL(k2[2],T(0));
		// Generic constructor for use in series.
		BOOST_CHECK_THROW(key_type tmp(k2,std::vector<symbol>{}),std::invalid_argument);
		key_type k3(k2,vs);
		BOOST_CHECK_EQUAL(k3.size(),vs.size());
		BOOST_CHECK_EQUAL(k3[0],T(0));
		BOOST_CHECK_EQUAL(k3[1],T(0));
		BOOST_CHECK_EQUAL(k3[2],T(0));
		key_type k4(key_type(vs),vs);
		BOOST_CHECK_EQUAL(k4.size(),vs.size());
		BOOST_CHECK_EQUAL(k4[0],T(0));
		BOOST_CHECK_EQUAL(k4[1],T(0));
		BOOST_CHECK_EQUAL(k4[2],T(0));
		typedef g_key_type<int> key_type2;
		key_type2 k5(vs);
		BOOST_CHECK_THROW(key_type tmp(k5,std::vector<symbol>{}),std::invalid_argument);
		key_type k6(k5,vs);
		BOOST_CHECK_EQUAL(k6.size(),vs.size());
		BOOST_CHECK_EQUAL(k6[0],T(0));
		BOOST_CHECK_EQUAL(k6[1],T(0));
		BOOST_CHECK_EQUAL(k6[2],T(0));
		key_type k7(key_type2(vs),vs);
		BOOST_CHECK_EQUAL(k7.size(),vs.size());
		BOOST_CHECK_EQUAL(k7[0],T(0));
		BOOST_CHECK_EQUAL(k7[1],T(0));
		BOOST_CHECK_EQUAL(k7[2],T(0));
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
		typedef g_key_type<T> key_type;
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
		typedef g_key_type<T> key_type;
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
		typedef g_key_type<T> key_type;
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

struct merge_args_tag;

namespace piranha
{

template <>
class debug_access<merge_args_tag>
{
	public:
		template <typename T>
		void operator()(const T &)
		{
			typedef g_key_type<T> key_type;
			std::vector<symbol> v1, v2;
			v2.push_back(symbol("a"));
			key_type k;
			auto out = k.base_merge_args(v1,v2);
			BOOST_CHECK_EQUAL(out.size(),unsigned(1));
			BOOST_CHECK_EQUAL(out[0],T(0));
			v2.push_back(symbol("b"));
			v2.push_back(symbol("c"));
			v2.push_back(symbol("d"));
			v1.push_back(symbol("b"));
			v1.push_back(symbol("d"));
			k.push_back(T(2));
			k.push_back(T(4));
			out = k.base_merge_args(v1,v2);
			BOOST_CHECK_EQUAL(out.size(),unsigned(4));
			BOOST_CHECK_EQUAL(out[0],T(0));
			BOOST_CHECK_EQUAL(out[1],T(2));
			BOOST_CHECK_EQUAL(out[2],T(0));
			BOOST_CHECK_EQUAL(out[3],T(4));
			v2.push_back(symbol("e"));
			v2.push_back(symbol("f"));
			v2.push_back(symbol("g"));
			v2.push_back(symbol("h"));
			v1.push_back(symbol("e"));
			v1.push_back(symbol("g"));
			k.push_back(T(5));
			k.push_back(T(7));
			out = k.base_merge_args(v1,v2);
			BOOST_CHECK_EQUAL(out.size(),unsigned(8));
			BOOST_CHECK_EQUAL(out[0],T(0));
			BOOST_CHECK_EQUAL(out[1],T(2));
			BOOST_CHECK_EQUAL(out[2],T(0));
			BOOST_CHECK_EQUAL(out[3],T(4));
			BOOST_CHECK_EQUAL(out[4],T(5));
			BOOST_CHECK_EQUAL(out[5],T(0));
			BOOST_CHECK_EQUAL(out[6],T(7));
			BOOST_CHECK_EQUAL(out[7],T(0));
		}
};

}

typedef piranha::debug_access<merge_args_tag> merge_args_tester;

BOOST_AUTO_TEST_CASE(array_key_merge_args_test)
{
	boost::mpl::for_each<value_types>(merge_args_tester());
}

struct iterators_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef g_key_type<T> key_type;
		key_type k0;
		BOOST_CHECK(k0.begin() == k0.end());
		for (int i = 0; i < 4; ++i) {
			k0.push_back(T(i));
		}
		BOOST_CHECK(k0.begin() + 4 == k0.end());
		BOOST_CHECK(k0.begin() != k0.end());
		const key_type k1{};
		BOOST_CHECK(k1.begin() == k1.end());
	}
};

BOOST_AUTO_TEST_CASE(array_key_iterators_test)
{
	boost::mpl::for_each<value_types>(iterators_tester());
}

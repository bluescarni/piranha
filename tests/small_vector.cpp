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

#include "../src/small_vector.hpp"

#define BOOST_TEST_MODULE small_vector_test
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <cstddef>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <functional>
#include <iterator>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "../src/environment.hpp"
#include "../src/integer.hpp"
#include "../src/rational.hpp"

using namespace piranha;

typedef boost::mpl::vector<signed char,short,int,long,long long,integer,rational> value_types;
typedef boost::mpl::vector<std::integral_constant<std::size_t,0u>,std::integral_constant<std::size_t,1u>,std::integral_constant<std::size_t,5u>,
	std::integral_constant<std::size_t,10u>> size_types;

// Class that throws after a few copies.
struct time_bomb
{
	time_bomb():m_vector(5) {}
	time_bomb(time_bomb &&) = default;
	time_bomb(const time_bomb &other):m_vector(other.m_vector)
	{
		if (s_counter == 2u) {
			throw std::runtime_error("ka-pow!");
		}
		++s_counter;
	}
	time_bomb &operator=(time_bomb &&other) noexcept
	{
		m_vector = std::move(other.m_vector);
		return *this;
	}
	~time_bomb() noexcept {}
	std::vector<int> m_vector;
	static unsigned s_counter;
};

unsigned time_bomb::s_counter = 0u;

struct dynamic_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef detail::dynamic_storage<T> d1;
		BOOST_CHECK(is_container_element<d1>::value);
		d1 ds1;
		BOOST_CHECK(ds1.begin() == ds1.end());
		BOOST_CHECK(static_cast<const d1 &>(ds1).begin() == static_cast<const d1 &>(ds1).end());
		BOOST_CHECK(static_cast<const d1 &>(ds1).begin() == ds1.end());
		BOOST_CHECK(ds1.begin() == static_cast<const d1 &>(ds1).end());
		BOOST_CHECK(ds1.empty());
		BOOST_CHECK(ds1.size() == 0u);
		BOOST_CHECK(ds1.capacity() == 0u);
		d1 ds2(ds1);
		BOOST_CHECK(ds2.size() == 0u);
		BOOST_CHECK(ds2.capacity() == 0u);
		ds1.push_back(T(0));
		BOOST_CHECK(ds1[0u] == T(0));
		BOOST_CHECK(ds1.capacity() == 1u);
		BOOST_CHECK(ds1.size() == 1u);
		BOOST_CHECK(!ds1.empty());
		d1 ds3(ds1);
		BOOST_CHECK(ds3[0u] == T(0));
		BOOST_CHECK(ds3.capacity() == 1u);
		BOOST_CHECK(ds3.size() == 1u);
		d1 ds4(std::move(ds3));
		BOOST_CHECK(ds4[0u] == T(0));
		BOOST_CHECK(ds4.capacity() == 1u);
		BOOST_CHECK(ds4.size() == 1u);
		BOOST_CHECK(ds3.capacity() == 0u);
		BOOST_CHECK(ds3.size() == 0u);
		d1 ds5(ds2);
		BOOST_CHECK(ds5.size() == 0u);
		BOOST_CHECK(ds5.capacity() == 0u);
		T tmp(1);
		ds1.push_back(tmp);
		BOOST_CHECK(ds1[1u] == T(1));
		BOOST_CHECK(ds1.capacity() == 2u);
		BOOST_CHECK(ds1.size() == 2u);
		ds1.reserve(1u);
		BOOST_CHECK(ds1[0u] == T(0));
		BOOST_CHECK(ds1[1u] == T(1));
		BOOST_CHECK(ds1.capacity() == 2u);
		BOOST_CHECK(ds1.size() == 2u);
		d1 ds6(std::move(ds1));
		BOOST_CHECK(ds6[0u] == T(0));
		BOOST_CHECK(ds6[1u] == T(1));
		BOOST_CHECK(ds6.capacity() == 2u);
		BOOST_CHECK(ds6.size() == 2u);
		BOOST_CHECK_THROW(ds6.reserve(d1::max_size + 1u),std::bad_alloc);
		d1 ds7;
		ds7.reserve(10u);
		BOOST_CHECK(ds7.capacity() == 10u);
		BOOST_CHECK(ds7.size() == 0u);
		for (int i = 0; i < 11; ++i) {
			ds7.push_back(T(i));
		}
		BOOST_CHECK(ds7.capacity() == 20u);
		BOOST_CHECK(ds7.size() == 11u);
		std::vector<T> tmp_vec = {T(0),T(1),T(2),T(3),T(4),T(5),T(6),T(7),T(8),T(9),T(10)};
		BOOST_CHECK(std::equal(tmp_vec.begin(),tmp_vec.end(),ds7.begin()));
		d1 ds8;
		std::vector<T> tmp_vec2;
		for (typename d1::size_type i = 0; i < d1::max_size; ++i) {
			ds8.push_back(T(i));
			tmp_vec2.push_back(T(i));
		}
		BOOST_CHECK(std::equal(ds8.begin(),ds8.end(),tmp_vec2.begin()));
		BOOST_CHECK_THROW(ds8.push_back(T(0)),std::bad_alloc);
		d1 ds9;
		ds9.reserve(d1::max_size - 1u);
		for (typename d1::size_type i = 0; i < d1::max_size; ++i) {
			ds9.push_back(T(i));
		}
		BOOST_CHECK(std::equal(ds9.begin(),ds9.end(),ds8.begin()));
		detail::dynamic_storage<time_bomb> ds10;
		ds10.push_back(time_bomb{});
		ds10.push_back(time_bomb{});
		ds10.push_back(time_bomb{});
		ds10.push_back(time_bomb{});
		BOOST_CHECK_THROW((detail::dynamic_storage<time_bomb>{ds10}),std::runtime_error);
		// Assignment.
		d1 ds11, ds12;
		ds11.push_back(T(42));
		auto ptr1 = &ds11[0u];
		ds11 = ds11;
		ds11 = std::move(ds11);
		BOOST_CHECK(ptr1 == &ds11[0u]);
		BOOST_CHECK(ds11.size() == 1u);
		BOOST_CHECK(ds11.capacity() == 1u);
		BOOST_CHECK(ds11[0u] == T(42));
		ds12 = std::move(ds11);
		BOOST_CHECK(ds12.size() == 1u);
		BOOST_CHECK(ds12.capacity() == 1u);
		BOOST_CHECK(ds12[0u] == T(42));
		BOOST_CHECK(ds11.size() == 0u);
		BOOST_CHECK(ds11.capacity() == 0u);
		// Revive with assignment.
		ds11 = ds12;
		BOOST_CHECK(ds11.size() == 1u);
		BOOST_CHECK(ds11.capacity() == 1u);
		BOOST_CHECK(ds11[0u] == T(42));
		auto ds13 = std::move(ds11);
		ds11 = std::move(ds13);
		BOOST_CHECK(ds11.size() == 1u);
		BOOST_CHECK(ds11.capacity() == 1u);
		BOOST_CHECK(ds11[0u] == T(42));
		ds11.push_back(T(43));
		ds11.push_back(T(44));
		ds11.push_back(T(45));
		BOOST_CHECK(ds11.size() == 4u);
		BOOST_CHECK(ds11.capacity() == 4u);
		// Iterators tests.
		auto it1 = ds11.begin();
		std::advance(it1,4);
		BOOST_CHECK(it1 == ds11.end());
		auto it2 = static_cast<d1 const &>(ds11).begin();
		std::advance(it2,4);
		BOOST_CHECK(it2 == static_cast<d1 const &>(ds11).end());
		BOOST_CHECK(ds11.begin() == &ds11[0u]);
		// Some STL algos.
		d1 ds14;
		std::copy(tmp_vec.rbegin(),tmp_vec.rend(),std::back_inserter(ds14));
		std::random_shuffle(ds14.begin(),ds14.end());
		std::sort(ds14.begin(),ds14.end());
		BOOST_CHECK(*std::max_element(ds14.begin(),ds14.end()) == T(10));
		BOOST_CHECK(*std::min_element(ds14.begin(),ds14.end()) == T(0));
		BOOST_CHECK(std::equal(ds14.begin(),ds14.end(),tmp_vec.begin()));
		// Capacity tests.
		const auto orig_cap = ds14.capacity();
		const auto orig_ptr = ds14[0u];
		ds14.reserve(0u);
		BOOST_CHECK(ds14.capacity() == orig_cap);
		BOOST_CHECK(orig_ptr == ds14[0u]);
		ds14.reserve(orig_cap);
		BOOST_CHECK(ds14.capacity() == orig_cap);
		BOOST_CHECK(orig_ptr == ds14[0u]);
		// Hash.
		d1 ds15;
		BOOST_CHECK(ds15.hash() == 0u);
		ds15.push_back(T(1));
		BOOST_CHECK(ds15.hash() == std::hash<T>()(T(1)));
	}
};

BOOST_AUTO_TEST_CASE(small_vector_dynamic_test)
{
	environment env;
	boost::mpl::for_each<value_types>(dynamic_tester());
}

struct constructor_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			using v_type = small_vector<T,U::value>;
			v_type v1;
			BOOST_CHECK(v1.size() == 0u);
			BOOST_CHECK(v1.begin() == v1.end());
			BOOST_CHECK(static_cast<v_type const &>(v1).begin() == static_cast<v_type const &>(v1).end());
			BOOST_CHECK(v1.begin() == static_cast<v_type const &>(v1).end());
			BOOST_CHECK(static_cast<v_type const &>(v1).begin() == v1.end());
			BOOST_CHECK(v1.is_static());
			int n = 0;
			std::generate_n(std::back_inserter(v1),integer(v_type::max_static_size) * 8 + 3,[&n](){return T(n++);});
			BOOST_CHECK(!v1.is_static());
			v_type v2(v1);
			BOOST_CHECK(!v2.is_static());
			BOOST_CHECK(std::equal(v1.begin(),v1.end(),v2.begin()));
			v_type v3(std::move(v2));
			BOOST_CHECK(std::equal(v1.begin(),v1.end(),v3.begin()));
			n = 0;
			v_type v4;
			std::generate_n(std::back_inserter(v4),integer(v_type::max_static_size),[&n](){return T(n++);});
			BOOST_CHECK(v4.is_static());
			v_type v5(v4);
			BOOST_CHECK(v5.is_static());
			BOOST_CHECK(std::equal(v4.begin(),v4.end(),v5.begin()));
			v_type v6(std::move(v5));
			BOOST_CHECK(std::equal(v4.begin(),v4.end(),v6.begin()));
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(small_vector_constructor_test)
{
	boost::mpl::for_each<value_types>(constructor_tester());
}

struct assignment_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			using v_type = small_vector<T,U::value>;
			v_type v1;
			v1.push_back(T(0));
			auto *ptr = std::addressof(v1[0]);
			// Verify that self assignment does not do anything funky.
			v1 = v1;
			v1 = std::move(v1);
			BOOST_CHECK(ptr == std::addressof(v1[0]));
			v_type v2;
			// This will be static vs static (there is always enough static storage for at least 1 element).
			v2 = v1;
			BOOST_CHECK(v2.size() == 1u);
			BOOST_CHECK(v2[0] == v1[0]);
			// Push enough into v1 to make it dynamic.
			int n = 0;
			std::generate_n(std::back_inserter(v1),integer(v_type::max_static_size),[&n](){return T(n++);});
			BOOST_CHECK(!v1.is_static());
			BOOST_CHECK(v2.is_static());
			// Static vs dynamic.
			v2 = v1;
			BOOST_CHECK(!v2.is_static());
			BOOST_CHECK(std::equal(v2.begin(),v2.end(),v1.begin()));
			v_type v3;
			// Dynamic vs static.
			v1 = v3;
			BOOST_CHECK(v1.is_static());
			BOOST_CHECK(v1.size() == 0u);
			// Dynamic vs dynamic.
			v_type v4(v2), v5(v2);
			std::transform(v5.begin(),v5.end(),v5.begin(),[](const T &x) {return x / 2;});
			v4 = v5;
			BOOST_CHECK(std::equal(v4.begin(),v4.end(),v5.begin()));
			v4 = std::move(v5);
			BOOST_CHECK(v5.size() == 0u);
			BOOST_CHECK(!v5.is_static());
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(small_vector_assignment_test)
{
	boost::mpl::for_each<value_types>(assignment_tester());
}

struct push_back_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			using v_type = small_vector<T,U::value>;
			v_type v1;
			std::vector<T> check;
			BOOST_CHECK(v1.size() == 0u);
			for (typename std::decay<decltype(v_type::max_static_size)>::type i = 0u; i < v_type::max_static_size; ++i) {
				v1.push_back(T(i));
				check.push_back(T(i));
			}
			v1.push_back(T(5));
			check.push_back(T(5));
			v1.push_back(T(6));
			check.push_back(T(6));
			v1.push_back(T(7));
			check.push_back(T(7));
			BOOST_CHECK(v1.size() == integer(v_type::max_static_size) + 3);
			BOOST_CHECK(std::equal(check.begin(),check.end(),v1.begin()));
			check.resize(0u);
			v_type v2;
			BOOST_CHECK(v2.size() == 0u);
			T tmp;
			for (typename std::decay<decltype(v_type::max_static_size)>::type i = 0u; i < v_type::max_static_size; ++i) {
				tmp = T(i);
				check.push_back(tmp);
				v2.push_back(tmp);
			}
			tmp = T(5);
			v2.push_back(tmp);
			check.push_back(tmp);
			tmp = T(6);
			v2.push_back(tmp);
			check.push_back(tmp);
			tmp = T(7);
			v2.push_back(tmp);
			check.push_back(tmp);
			BOOST_CHECK(v2.size() == integer(v_type::max_static_size) + 3);
			BOOST_CHECK(std::equal(check.begin(),check.end(),v2.begin()));
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(small_vector_push_back_test)
{
	boost::mpl::for_each<value_types>(push_back_tester());
}

struct equality_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			using v_type = small_vector<T,U::value>;
			v_type v1;
			BOOST_CHECK(v1 == v1);
			BOOST_CHECK(!(v1 != v1));
			v_type v2 = v1;
			v1.push_back(T(0));
			BOOST_CHECK(v2 != v1);
			BOOST_CHECK(!(v2 == v1));
			BOOST_CHECK(v1 != v2);
			BOOST_CHECK(!(v1 == v2));
			v2.push_back(T(0));
			BOOST_CHECK(v2 == v1);
			BOOST_CHECK(!(v2 != v1));
			BOOST_CHECK(v1 == v2);
			BOOST_CHECK(!(v1 != v2));
			// Push enough into v1 to make it dynamic.
			int n = 0;
			std::generate_n(std::back_inserter(v1),integer(v_type::max_static_size),[&n](){return T(n++);});
			BOOST_CHECK(v2 != v1);
			BOOST_CHECK(!(v2 == v1));
			BOOST_CHECK(v1 != v2);
			BOOST_CHECK(!(v1 == v2));
			v2 = v1;
			BOOST_CHECK(v2 == v1);
			BOOST_CHECK(!(v2 != v1));
			BOOST_CHECK(v1 == v2);
			BOOST_CHECK(!(v1 != v2));
			v2.push_back(T(5));
			BOOST_CHECK(v2 != v1);
			BOOST_CHECK(!(v2 == v1));
			BOOST_CHECK(v1 != v2);
			BOOST_CHECK(!(v1 == v2));
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(small_vector_equality_test)
{
	boost::mpl::for_each<value_types>(equality_tester());
}

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
		BOOST_CHECK(std::is_nothrow_destructible<d1>::value);
		d1 ds1;
		BOOST_CHECK(ds1.size() == 0u);
		BOOST_CHECK(ds1.capacity() == 0u);
		d1 ds2(ds1);
		BOOST_CHECK(ds2.size() == 0u);
		BOOST_CHECK(ds2.capacity() == 0u);
		ds1.push_back(T(0));
		BOOST_CHECK(ds1[0u] == T(0));
		BOOST_CHECK(ds1.capacity() == 1u);
		BOOST_CHECK(ds1.size() == 1u);
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
	}
};

BOOST_AUTO_TEST_CASE(small_vector_dynamic_test)
{
	environment env;
	boost::mpl::for_each<value_types>(dynamic_tester());
}

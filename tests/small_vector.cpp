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

#include <cstddef>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <memory>
#include <type_traits>

#include "../src/environment.hpp"
#include "../src/integer.hpp"
#include "../src/rational.hpp"

using namespace piranha;

typedef boost::mpl::vector<signed char,short,int,long,long long,integer,rational> value_types;
typedef boost::mpl::vector<std::integral_constant<std::size_t,1u>,std::integral_constant<std::size_t,5u>,
	std::integral_constant<std::size_t,10u>> size_types;

struct dynamic_constructor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef detail::dynamic_storage<T,std::allocator<T>> d1;
		typedef detail::dynamic_storage<T,std::allocator<long>> d2;
		BOOST_CHECK((std::is_same<typename d1::allocator_type,typename d2::allocator_type>::value));
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
	}
};

BOOST_AUTO_TEST_CASE(small_vector_dynamic_constructor_test)
{
	environment env;
	boost::mpl::for_each<value_types>(dynamic_constructor_tester());
}

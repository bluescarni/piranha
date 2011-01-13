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

#include "../src/hop_table.hpp"

#define BOOST_TEST_MODULE hop_table_test
#include <boost/test/unit_test.hpp>

#include <cstddef>

#include "../src/config.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(hop_table_constructors_test)
{
	// Def ctor.
	hop_table<std::string> ht;
	BOOST_CHECK(ht.begin() == ht.end());
	// Ctor from number of buckets.
	hop_table<std::string> ht0(0);
	BOOST_CHECK(ht0.n_buckets() == 0);
	BOOST_CHECK(ht0.begin() == ht0.end());
	hop_table<std::string> ht1(1);
	BOOST_CHECK(ht1.n_buckets() >= 1);
	BOOST_CHECK(ht1.begin() == ht1.end());
	hop_table<std::string> ht2(2);
	BOOST_CHECK(ht2.n_buckets() >= 2);
	BOOST_CHECK(ht2.begin() == ht2.end());
	hop_table<std::string> ht3(3);
	BOOST_CHECK(ht3.n_buckets() >= 3);
	BOOST_CHECK(ht3.begin() == ht3.end());
	hop_table<std::string> ht4(4);
	BOOST_CHECK(ht4.n_buckets() >= 4);
	BOOST_CHECK(ht4.begin() == ht4.end());
	hop_table<std::string> ht5(456);
	BOOST_CHECK(ht5.n_buckets() >= 456);
	BOOST_CHECK(ht5.begin() == ht5.end());
	hop_table<std::string> ht6(100001);
	BOOST_CHECK(ht6.n_buckets() >= 100001);
	BOOST_CHECK(ht6.begin() == ht6.end());


// 	// Copy ctor.
// 	hop_table<std::string> ht7(ht6);
// 	auto it7 = ht7.begin();
// 	for (auto it6 = ht6.begin(); it6 != ht6.end(); ++it6, ++it7) {
// 		BOOST_CHECK_EQUAL(*it6,*it7);
// 	}
// 	BOOST_CHECK(it7 == ht7.end());
// 	// Move ctor.
// 	auto ht8(std::move(ht7));
// 	BOOST_CHECK_EQUAL(ht7.size(),unsigned(0));
// 	BOOST_CHECK_EQUAL(ht7.n_buckets(),unsigned(0));
// 	auto it8 = ht8.begin();
// 	for (auto it6 = ht6.begin(); it6 != ht6.end(); ++it6, ++it8) {
// 		BOOST_CHECK_EQUAL(*it6,*it8);
// 	}
// 	BOOST_CHECK(it8 == ht8.end());
}

BOOST_AUTO_TEST_CASE(hop_table_insert_test)
{
	// Check insert when the resize operation fails on the first try.
	const std::size_t critical_size =
#if defined(PIRANHA_64BIT_MODE)
		193;
#else
		97;
#endif
	auto custom_hash = [](std::size_t i) -> std::size_t {return i;};
	hop_table<std::size_t,decltype(custom_hash)> ht(custom_hash);
	for (std::size_t i = 0; i < critical_size; ++i) {
		BOOST_CHECK_EQUAL(ht.insert(i * critical_size).second,true);
	}
	// Verify insertion of all items.
	for (std::size_t i = 0; i < critical_size; ++i) {
		BOOST_CHECK(ht.find(i * critical_size) != ht.end());
	}
	BOOST_CHECK(ht.size() == critical_size);
}

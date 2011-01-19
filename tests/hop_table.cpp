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

#include <algorithm>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <string>

#include "../src/config.hpp"
#include "../src/exceptions.hpp"
#include "../src/integer.hpp"

using namespace piranha;

typedef boost::mpl::vector<int,integer,std::string> key_types;

const int N = 10000;

template <typename T>
static inline hop_table<T> make_hop_table()
{
	struct lc_func_type
	{
		typedef T result_type;
		T operator()(int n) const
		{
			return boost::lexical_cast<T>(n);
		}
	};
	lc_func_type lc_func;
	typedef boost::transform_iterator<decltype(lc_func),boost::counting_iterator<int>> lc_iterator;
	return hop_table<T>(boost::make_transform_iterator(boost::counting_iterator<int>(0),lc_func),
		boost::make_transform_iterator(boost::counting_iterator<int>(N),lc_func)
	);
}

struct range_ctor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		BOOST_CHECK_EQUAL(make_hop_table<T>().size(),unsigned(N));
	}
};

struct copy_ctor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		hop_table<T> h(make_hop_table<T>()), h_copy(h);
		BOOST_CHECK_EQUAL(h_copy.size(),unsigned(N));
		auto it1 = h.begin();
		for (auto it2 = h_copy.begin(); it2 != h_copy.end(); ++it1, ++it2) {
			BOOST_CHECK_EQUAL(*it1,*it2);
		}
		BOOST_CHECK(it1 == h.end());
	}
};

struct move_ctor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		hop_table<T> h(make_hop_table<T>()), h_copy(h), h_move(std::move(h));
		BOOST_CHECK_EQUAL(h_copy.size(),unsigned(N));
		BOOST_CHECK_EQUAL(h_move.size(),unsigned(N));
		BOOST_CHECK_EQUAL(h.size(),unsigned(0));
		auto it1 = h_move.begin();
		for (auto it2 = h_copy.begin(); it2 != h_copy.end(); ++it1, ++it2) {
			BOOST_CHECK_EQUAL(*it1,*it2);
		}
		BOOST_CHECK(it1 == h_move.end());
	}
};

struct copy_assignment_tester
{
	template <typename T>
	void operator()(const T &)
	{
		hop_table<T> h(make_hop_table<T>()), h_copy;
		h_copy = h;
		BOOST_CHECK_EQUAL(h_copy.size(),unsigned(N));
		auto it1 = h.begin();
		for (auto it2 = h_copy.begin(); it2 != h_copy.end(); ++it1, ++it2) {
			BOOST_CHECK_EQUAL(*it1,*it2);
		}
		BOOST_CHECK(it1 == h.end());
	}
};

struct move_assignment_tester
{
	template <typename T>
	void operator()(const T &)
	{
		hop_table<T> h(make_hop_table<T>()), h_copy(h), h_move;
		h_move = std::move(h);
		BOOST_CHECK_EQUAL(h_copy.size(),unsigned(N));
		BOOST_CHECK_EQUAL(h_move.size(),unsigned(N));
		BOOST_CHECK_EQUAL(h.size(),unsigned(0));
		auto it1 = h_move.begin();
		for (auto it2 = h_copy.begin(); it2 != h_copy.end(); ++it1, ++it2) {
			BOOST_CHECK_EQUAL(*it1,*it2);
		}
		BOOST_CHECK(it1 == h_move.end());
	}
};

BOOST_AUTO_TEST_CASE(hop_table_constructors_test)
{
	// Def ctor.
	hop_table<std::string> ht;
	BOOST_CHECK(ht.begin() == ht.end());
	BOOST_CHECK(ht.empty());
	BOOST_CHECK_EQUAL(ht.size(),unsigned(0));
	BOOST_CHECK_EQUAL(ht.n_buckets(),unsigned(0));
	BOOST_CHECK_THROW(ht.bucket("hello"),zero_division_error);
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
	// Range constructor.
	boost::mpl::for_each<key_types>(range_ctor_tester());
	// Copy ctor.
	boost::mpl::for_each<key_types>(copy_ctor_tester());
	// Move ctor.
	boost::mpl::for_each<key_types>(move_ctor_tester());
	// Copy assignment.
	boost::mpl::for_each<key_types>(copy_assignment_tester());
	// Move assignment.
	boost::mpl::for_each<key_types>(move_assignment_tester());
}

struct iterator_tester
{
	template <typename T>
	void operator()(const T &)
	{
		hop_table<T> h(make_hop_table<T>());
		unsigned count = 0;
		for (auto it = h.begin();  it != h.end(); ++it, ++count) {}
		BOOST_CHECK_EQUAL(h.size(),count);
	}
};

BOOST_AUTO_TEST_CASE(hop_table_iterator_test)
{
	boost::mpl::for_each<key_types>(iterator_tester());
}

struct find_tester
{
	template <typename T>
	void operator()(const T &)
	{
		hop_table<T> h(make_hop_table<T>()), h_empty;
		BOOST_CHECK(h_empty.find(boost::lexical_cast<T>(0)) == h_empty.end());
		for (int i = 0; i < N; ++i) {
			auto it = h.find(boost::lexical_cast<T>(i));
			BOOST_CHECK(it != h.end());
		}
		BOOST_CHECK(h.find(boost::lexical_cast<T>(N + 1)) == h.end());
	}
};

BOOST_AUTO_TEST_CASE(hop_table_find_test)
{
	boost::mpl::for_each<key_types>(find_tester());
}

struct insert_tester
{
	template <typename T>
	void operator()(const T &)
	{
		hop_table<T> h;
		for (int i = 0; i < N; ++i) {
			auto r1 = h.insert(boost::lexical_cast<T>(i));
			BOOST_CHECK_EQUAL(r1.second,true);
			auto r2 = h.insert(boost::lexical_cast<T>(i));
			BOOST_CHECK_EQUAL(r2.second,false);
			BOOST_CHECK(r1.first == r2.first);
		}
		BOOST_CHECK_EQUAL(h.size(),unsigned(N));
	}
};

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
	boost::mpl::for_each<key_types>(insert_tester());
}

struct erase_tester
{
	template <typename T>
	void operator()(const T &)
	{
		hop_table<T> h(make_hop_table<T>());
		for (int i = 0; i < N; ++i) {
			auto r = h.find(boost::lexical_cast<T>(i));
			BOOST_CHECK(r != h.end());
			h.erase(r);
		}
		BOOST_CHECK_EQUAL(h.size(),unsigned(0));
	}
};

BOOST_AUTO_TEST_CASE(hop_table_erase_test)
{
	boost::mpl::for_each<key_types>(erase_tester());
}

struct clear_tester
{
	template <typename T>
	void operator()(const T &)
	{
		hop_table<T> h(make_hop_table<T>());
		h.clear();
		BOOST_CHECK_EQUAL(h.size(),unsigned(0));
		BOOST_CHECK_EQUAL(h.n_buckets(),unsigned(0));
	}
};

BOOST_AUTO_TEST_CASE(hop_table_clear_test)
{
	boost::mpl::for_each<key_types>(clear_tester());
}

struct swap_tester
{
	template <typename T>
	void operator()(const T &)
	{
		hop_table<T> h1(make_hop_table<T>()), h2(h1);
		const auto nb1 = h1.n_buckets(), s1 = h1.size();
		for (int i = 0; i < N / 2; ++i) {
			h2.erase(h2.find(boost::lexical_cast<T>(i)));
		}
		const auto nb2 = h2.n_buckets(), s2 = h2.size();
		h1.swap(h2);
		BOOST_CHECK_EQUAL(h1.n_buckets(),nb2);
		BOOST_CHECK_EQUAL(h2.n_buckets(),nb1);
		BOOST_CHECK_EQUAL(h1.size(),s2);
		BOOST_CHECK_EQUAL(h2.size(),s1);
		for (int i = 0; i < N / 2; ++i) {
			BOOST_CHECK(h1.find(boost::lexical_cast<T>(i)) == h1.end());
		}
	}
};

BOOST_AUTO_TEST_CASE(hop_table_swap_test)
{
	boost::mpl::for_each<key_types>(swap_tester());
}

struct load_factor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		hop_table<T> h;
		BOOST_CHECK_THROW(h.load_factor(),piranha::zero_division_error);
		hop_table<T> i(10);
		BOOST_CHECK_EQUAL(i.load_factor(),0);
	}
};

BOOST_AUTO_TEST_CASE(hop_table_load_factor_test)
{
	boost::mpl::for_each<key_types>(load_factor_tester());
}

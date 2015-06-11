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

#include "../src/array_hash_set.hpp"

#define BOOST_TEST_MODULE array_hash_set_test
#include <boost/test/unit_test.hpp>

#include <boost/iterator/counting_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <functional>
#include <limits>
#include <new>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>

#include "../src/environment.hpp"
#include "../src/exceptions.hpp"
#include "../src/mp_integer.hpp"

using namespace piranha;

// NOTE: here we define a custom string class base on std::string that respects nothrow requirements in array_hash_set:
// in the current GCC (4.6) the destructor of std::string does not have nothrow, so we cannot use it.
class custom_string: public std::string
{
	public:
		custom_string() = default;
		custom_string(const custom_string &) = default;
		custom_string(custom_string &&other) noexcept : std::string(std::move(other)) {}
		template <typename... Args>
		custom_string(Args && ... params) : std::string(std::forward<Args>(params)...) {}
		custom_string &operator=(const custom_string &) = default;
		custom_string &operator=(custom_string &&other) noexcept
		{
			std::string::operator=(std::move(other));
			return *this;
		}
		~custom_string() noexcept {}
};

namespace std
{
template <>
struct hash<custom_string>
{
	typedef size_t result_type;
	typedef custom_string argument_type;
	result_type operator()(const argument_type &s) const
	{
		return hash<std::string>{}(s);
	}
};
}

using key_types = boost::mpl::vector<int,integer,custom_string>;

// Number of set items to be created by make_hash_set().
const int N = 10000;

template <typename T>
static inline array_hash_set<T> make_hash_set()
{
	struct lc_func_type
	{
		T operator()(int n) const
		{
			return boost::lexical_cast<T>(n);
		}
	};
	lc_func_type lc_func;
	return array_hash_set<T>(boost::make_transform_iterator(boost::counting_iterator<int>(0),lc_func),
		boost::make_transform_iterator(boost::counting_iterator<int>(N),lc_func)
	);
}

struct range_ctor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		BOOST_CHECK_EQUAL(make_hash_set<T>().size(),unsigned(N));
	}
};

struct copy_ctor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		array_hash_set<T> h(make_hash_set<T>()), h_copy(h);
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
		array_hash_set<T> h(make_hash_set<T>()), h_copy(h), h_move(std::move(h));
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
		array_hash_set<T> h(make_hash_set<T>()), h_copy;
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
		array_hash_set<T> h(make_hash_set<T>()), h_copy(h), h_move;
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

struct initializer_list_tester
{
	template <typename T>
	void operator()(const T &)
	{
		array_hash_set<T> h({boost::lexical_cast<T>("1"),boost::lexical_cast<T>("2"),boost::lexical_cast<T>("3"),boost::lexical_cast<T>("4"),boost::lexical_cast<T>("4")});
		BOOST_CHECK_EQUAL(h.size(),unsigned(4));
		for (int i = 1; i <= 4; ++i) {
			BOOST_CHECK(h.find(boost::lexical_cast<T>(i)) != h.end());
		}
	}
};

std::mt19937 rng;
std::uniform_int_distribution<int> dist(0,9);

// Struct that randomly fails on copy.
struct random_failure
{
	random_failure()
	{
		throw;
	}
	random_failure(int n):m_str(boost::lexical_cast<std::string>(n)) {}
	random_failure(const random_failure &rf):m_str(rf.m_str)
	{
		if (!dist(rng)) {
			throw std::runtime_error("fail!");
		}
	}
	random_failure(random_failure &&rf) noexcept :m_str(std::move(rf.m_str)) {}
	~random_failure() noexcept {}
	std::size_t hash() const
	{
		return static_cast<std::size_t>(boost::lexical_cast<int>(m_str));
	}
	bool operator==(const random_failure &rf) const
	{
		return m_str == rf.m_str;
	}
	random_failure &operator=(random_failure &&other) noexcept
	{
		m_str = std::move(other.m_str);
		return *this;
	}
	std::string m_str;
};

namespace std
{
template <>
struct hash<random_failure>
{
	typedef size_t result_type;
	typedef random_failure argument_type;
	result_type operator()(const random_failure &rf) const
	{
		return rf.hash();
	}
};
}

BOOST_AUTO_TEST_CASE(array_hash_set_constructors_test)
{
	environment env;
	// Def ctor.
	array_hash_set<custom_string> ht;
	BOOST_CHECK(ht.begin() == ht.end());
	BOOST_CHECK(ht.empty());
	BOOST_CHECK_EQUAL(ht.size(),unsigned(0));
	BOOST_CHECK_EQUAL(ht.bucket_count(),unsigned(0));
	BOOST_CHECK_THROW(ht.bucket("hello"),zero_division_error);
	// Ctor from number of buckets.
	array_hash_set<custom_string> ht0(0);
	BOOST_CHECK(ht0.bucket_count() == 0);
	BOOST_CHECK(ht0.begin() == ht0.end());
	array_hash_set<custom_string> ht1(1);
	BOOST_CHECK(ht1.bucket_count() >= 1);
	BOOST_CHECK(ht1.begin() == ht1.end());
	array_hash_set<custom_string> ht2(2);
	BOOST_CHECK(ht2.bucket_count() >= 2);
	BOOST_CHECK(ht2.begin() == ht2.end());
	array_hash_set<custom_string> ht3(3);
	BOOST_CHECK(ht3.bucket_count() >= 3);
	BOOST_CHECK(ht3.begin() == ht3.end());
	array_hash_set<custom_string> ht4(4);
	BOOST_CHECK(ht4.bucket_count() >= 4);
	BOOST_CHECK(ht4.begin() == ht4.end());
	array_hash_set<custom_string> ht5(456);
	BOOST_CHECK(ht5.bucket_count() >= 456);
	BOOST_CHECK(ht5.begin() == ht5.end());
	array_hash_set<custom_string> ht6(100001);
	BOOST_CHECK(ht6.bucket_count() >= 100001);
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
	// Initializer list.
	boost::mpl::for_each<key_types>(initializer_list_tester());
	// Check that requesting too many buckets throws.
	BOOST_CHECK_THROW(ht6 = array_hash_set<custom_string>(std::numeric_limits<std::size_t>::max()),std::bad_alloc);
	// Check unwind on throw.
	// NOTE: prepare table with large number of buckets, so we are sure the first copy of random_failure will be performed
	// in the assignment below.
	array_hash_set<random_failure> ht7(10000);
	for (int i = 0; i < 1000; ++i) {
		ht7.insert(random_failure(i));
	}
	array_hash_set<random_failure> ht8;
	BOOST_CHECK_THROW(ht8 = ht7,std::runtime_error);
}

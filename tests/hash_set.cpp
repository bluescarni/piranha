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

#include "../src/hash_set.hpp"

#define BOOST_TEST_MODULE hash_set_test
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <boost/integer_traits.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <map>
#include <new>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <tuple>

#include "../src/environment.hpp"
#include "../src/exceptions.hpp"
#include "../src/mp_integer.hpp"
#include "../src/thread_pool.hpp"
#include "../src/type_traits.hpp"

static const int ntries = 1000;

using namespace piranha;

// NOTE: here we define a custom string class base on std::string that respects nothrow requirements in hash_set:
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
	result_type operator()(const argument_type &s) const noexcept
	{
		return hash<std::string>{}(s);
	}
};
}

typedef boost::mpl::vector<int,integer,custom_string> key_types;

const int N = 10000;

template <typename T>
static inline hash_set<T> make_hash_set()
{
	struct lc_func_type
	{
		typedef T result_type;
		result_type operator()(int n) const
		{
			return boost::lexical_cast<T>(n);
		}
	};
	lc_func_type lc_func;
	return hash_set<T>(boost::make_transform_iterator(boost::counting_iterator<int>(0),lc_func),
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
		hash_set<T> h(make_hash_set<T>()), h_copy(h);
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
		hash_set<T> h(make_hash_set<T>()), h_copy(h), h_move(std::move(h));
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
		hash_set<T> h(make_hash_set<T>()), h_copy;
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
		hash_set<T> h(make_hash_set<T>()), h_copy(h), h_move;
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
		hash_set<T> h({boost::lexical_cast<T>("1"),boost::lexical_cast<T>("2"),boost::lexical_cast<T>("3"),boost::lexical_cast<T>("4"),boost::lexical_cast<T>("4")});
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
	result_type operator()(const random_failure &rf) const noexcept
	{
		return rf.hash();
	}
};
}

BOOST_AUTO_TEST_CASE(hash_set_constructors_test)
{
	environment env;
	// Def ctor.
	hash_set<custom_string> ht;
	BOOST_CHECK(ht.begin() == ht.end());
	BOOST_CHECK(ht.empty());
	BOOST_CHECK_EQUAL(ht.size(),unsigned(0));
	BOOST_CHECK_EQUAL(ht.bucket_count(),unsigned(0));
	BOOST_CHECK_THROW(ht.bucket("hello"),zero_division_error);
	// Ctor from number of buckets.
	hash_set<custom_string> ht0(0);
	BOOST_CHECK(ht0.bucket_count() == 0);
	BOOST_CHECK(ht0.begin() == ht0.end());
	hash_set<custom_string> ht1(1);
	BOOST_CHECK(ht1.bucket_count() >= 1);
	BOOST_CHECK(ht1.begin() == ht1.end());
	hash_set<custom_string> ht2(2);
	BOOST_CHECK(ht2.bucket_count() >= 2);
	BOOST_CHECK(ht2.begin() == ht2.end());
	hash_set<custom_string> ht3(3);
	BOOST_CHECK(ht3.bucket_count() >= 3);
	BOOST_CHECK(ht3.begin() == ht3.end());
	hash_set<custom_string> ht4(4);
	BOOST_CHECK(ht4.bucket_count() >= 4);
	BOOST_CHECK(ht4.begin() == ht4.end());
	hash_set<custom_string> ht5(456);
	BOOST_CHECK(ht5.bucket_count() >= 456);
	BOOST_CHECK(ht5.begin() == ht5.end());
	hash_set<custom_string> ht6(100001);
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
	BOOST_CHECK_THROW(ht6 = hash_set<custom_string>(boost::integer_traits<std::size_t>::const_max),std::bad_alloc);
	// Check unwind on throw.
	// NOTE: prepare table with large number of buckets, so we are sure the first copy of random_failure will be performed
	// in the assignment below.
	hash_set<random_failure> ht7(10000);
	for (int i = 0; i < 1000; ++i) {
		ht7.insert(random_failure(i));
	}
	hash_set<random_failure> ht8;
	BOOST_CHECK_THROW(ht8 = ht7,std::runtime_error);
}

struct iterator_tester
{
	template <typename T>
	void operator()(const T &)
	{
		hash_set<T> h(make_hash_set<T>());
		unsigned count = 0;
		for (auto it = h.begin();  it != h.end(); ++it, ++count) {}
		BOOST_CHECK_EQUAL(h.size(),count);
	}
};

BOOST_AUTO_TEST_CASE(hash_set_iterator_test)
{
	boost::mpl::for_each<key_types>(iterator_tester());
}

struct find_tester
{
	template <typename T>
	void operator()(const T &)
	{
		hash_set<T> h(make_hash_set<T>()), h_empty;
		BOOST_CHECK(h_empty.find(boost::lexical_cast<T>(0)) == h_empty.end());
		for (int i = 0; i < N; ++i) {
			auto it = h.find(boost::lexical_cast<T>(i));
			BOOST_CHECK(it != h.end());
		}
		BOOST_CHECK(h.find(boost::lexical_cast<T>(N + 1)) == h.end());
	}
};

BOOST_AUTO_TEST_CASE(hash_set_find_test)
{
	boost::mpl::for_each<key_types>(find_tester());
}

struct insert_tester
{
	template <typename T>
	void operator()(const T &)
	{
		hash_set<T> h;
		for (int i = 0; i < N; ++i) {
			auto r1 = h.insert(boost::lexical_cast<T>(i));
			BOOST_CHECK_EQUAL(r1.second,true);
			auto r2 = h.insert(boost::lexical_cast<T>(i));
			BOOST_CHECK_EQUAL(r2.second,false);
			BOOST_CHECK(r2.first == h.find(boost::lexical_cast<T>(i)));
		}
		BOOST_CHECK_EQUAL(h.size(),unsigned(N));
	}
};

// NOTE: this test had a meaning in a previous implementation of hash_set, now it is jut a simple
// insertion test.
BOOST_AUTO_TEST_CASE(hash_set_insert_test)
{
	// Check insert when the resize operation fails on the first try.
	const std::size_t critical_size = 193;
	struct custom_hash
	{
		std::size_t operator()(std::size_t i) const noexcept
		{
			return i;
		}
	};
	custom_hash ch;
	hash_set<std::size_t,custom_hash> ht(ch);
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
		hash_set<T> h(make_hash_set<T>());
		for (int i = 0; i < N; ++i) {
			auto r = h.find(boost::lexical_cast<T>(i));
			BOOST_CHECK(r != h.end());
			h.erase(r);
		}
		BOOST_CHECK_EQUAL(h.size(),unsigned(0));
		h = make_hash_set<T>();
		for (auto it = h.begin(); it != h.end();) {
			it = h.erase(it);
		}
		BOOST_CHECK_EQUAL(h.size(),unsigned(0));
	}
};

BOOST_AUTO_TEST_CASE(hash_set_erase_test)
{
	boost::mpl::for_each<key_types>(erase_tester());
}

struct clear_tester
{
	template <typename T>
	void operator()(const T &)
	{
		hash_set<T> h(make_hash_set<T>());
		h.clear();
		BOOST_CHECK_EQUAL(h.size(),unsigned(0));
		BOOST_CHECK_EQUAL(h.bucket_count(),unsigned(0));
	}
};

BOOST_AUTO_TEST_CASE(hash_set_clear_test)
{
	boost::mpl::for_each<key_types>(clear_tester());
}

struct swap_tester
{
	template <typename T>
	void operator()(const T &)
	{
		hash_set<T> h1(make_hash_set<T>()), h2(h1);
		const auto nb1 = h1.bucket_count(), s1 = h1.size();
		for (int i = 0; i < N / 2; ++i) {
			h2.erase(h2.find(boost::lexical_cast<T>(i)));
		}
		const auto nb2 = h2.bucket_count(), s2 = h2.size();
		h1.swap(h2);
		BOOST_CHECK_EQUAL(h1.bucket_count(),nb2);
		BOOST_CHECK_EQUAL(h2.bucket_count(),nb1);
		BOOST_CHECK_EQUAL(h1.size(),s2);
		BOOST_CHECK_EQUAL(h2.size(),s1);
		for (int i = 0; i < N / 2; ++i) {
			BOOST_CHECK(h1.find(boost::lexical_cast<T>(i)) == h1.end());
		}
	}
};

BOOST_AUTO_TEST_CASE(hash_set_swap_test)
{
	boost::mpl::for_each<key_types>(swap_tester());
}

struct load_factor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		hash_set<T> h;
		BOOST_CHECK(h.load_factor() == 0.);
		hash_set<T> i(10);
		BOOST_CHECK_EQUAL(i.load_factor(),0);
		hash_set<T> j(make_hash_set<T>());
		BOOST_CHECK(j.load_factor() > 0);
		BOOST_CHECK(j.load_factor() <= 1);
		BOOST_CHECK(h.max_load_factor() > 0);
	}
};

BOOST_AUTO_TEST_CASE(hash_set_load_factor_test)
{
	boost::mpl::for_each<key_types>(load_factor_tester());
}

struct m_iterators_tester
{
	template <typename T>
	void operator()(const T &)
	{
		hash_set<T> h;
		BOOST_CHECK(h._m_begin() == h._m_end());
		h.insert(T());
		BOOST_CHECK(h._m_begin() != h._m_end());
		*h._m_begin() = boost::lexical_cast<T>("42");
		BOOST_CHECK(*h._m_begin() == boost::lexical_cast<T>("42"));
		// Check we can clear and destroy without bad consequences.
		h.clear();
	}
};

BOOST_AUTO_TEST_CASE(hash_set_m_iterators_test)
{
	boost::mpl::for_each<key_types>(m_iterators_tester());
}

struct rehash_tester
{
	template <typename T>
	void operator()(const T &)
	{
		hash_set<T> h;
		BOOST_CHECK(h.bucket_count() == 0u);
		h.rehash(100u);
		BOOST_CHECK(h.bucket_count() >= 100u);
		h.rehash(10u);
		BOOST_CHECK(h.bucket_count() >= 10u);
		h.rehash(1000u);
		BOOST_CHECK(h.bucket_count() >= 1000u);
		h.rehash(0u);
		BOOST_CHECK(h.bucket_count() == 0u);
		h = make_hash_set<T>();
		auto old = h.bucket_count();
		h.rehash(old * 2u);
		BOOST_CHECK(h.bucket_count() >= old * 2u);
		h.rehash(old);
		BOOST_CHECK(h.bucket_count() >= old);
		h = make_hash_set<T>();
		old = h.bucket_count();
		h.rehash(0u);
		BOOST_CHECK(old == h.bucket_count());
		h = hash_set<T>(100u);
		h.rehash(0u);
		BOOST_CHECK(h.bucket_count() == 0u);
		h = make_hash_set<T>();
		old = h.bucket_count();
		h.rehash(1000u);
		BOOST_CHECK(h.bucket_count() == old);
	}
};

BOOST_AUTO_TEST_CASE(hash_set_rehash_test)
{
	boost::mpl::for_each<key_types>(rehash_tester());
}

struct evaluate_sparsity_tester
{
	template <typename T>
	void operator()(const T &)
	{
		hash_set<T> h;
		using size_type = typename hash_set<T>::size_type;
		BOOST_CHECK((h.evaluate_sparsity() == std::map<size_type,size_type>{}));
		T tmp = T();
		h.insert(tmp);
		BOOST_CHECK((h.evaluate_sparsity() == std::map<size_type,size_type>{{1u,1u}}));
	}
};

BOOST_AUTO_TEST_CASE(hash_set_evaluate_sparsity_test)
{
	boost::mpl::for_each<key_types>(evaluate_sparsity_tester());
}

struct type_traits_tester
{
	template <typename T>
	void operator()(const T &)
	{
		BOOST_CHECK(is_container_element<hash_set<T>>::value);
		BOOST_CHECK((is_instance_of<hash_set<T>,hash_set>::value));
		BOOST_CHECK(!is_equality_comparable<hash_set<T>>::value);
		BOOST_CHECK(!is_addable<hash_set<T>>::value);
		BOOST_CHECK(!is_ostreamable<hash_set<T>>::value);
	}
};

BOOST_AUTO_TEST_CASE(hash_set_type_traits_test)
{
	boost::mpl::for_each<key_types>(type_traits_tester());
}

BOOST_AUTO_TEST_CASE(hash_set_mt_test)
{
	thread_pool::resize(4u);
	BOOST_CHECK_THROW(hash_set<int>(10000,std::hash<int>(),std::equal_to<int>(),0u),std::invalid_argument);
	hash_set<int> h1(100000,std::hash<int>(),std::equal_to<int>(),1u);
	hash_set<int> h2(100000,std::hash<int>(),std::equal_to<int>(),2u);
	hash_set<int> h3(100000,std::hash<int>(),std::equal_to<int>(),3u);
	hash_set<int> h4(100000,std::hash<int>(),std::equal_to<int>(),4u);
	// Try with few buckets.
	hash_set<int> h5(1,std::hash<int>(),std::equal_to<int>(),4u);
	hash_set<int> h6(2,std::hash<int>(),std::equal_to<int>(),4u);
	hash_set<int> h7(3,std::hash<int>(),std::equal_to<int>(),4u);
	hash_set<int> h8(4,std::hash<int>(),std::equal_to<int>(),4u);
	// Random testing.
	using size_type = hash_set<int>::size_type;
	std::uniform_int_distribution<size_type> size_dist(0u,100000u);
	std::uniform_int_distribution<unsigned> thread_dist(1u,4u);
	for (int i = 0; i < ntries; ++i) {
		auto bcount = size_dist(rng);
		hash_set<int> h(bcount,std::hash<int>(),std::equal_to<int>(),thread_dist(rng));
		BOOST_CHECK(h.bucket_count() >= bcount);
		bcount = size_dist(rng);
		h.rehash(bcount,thread_dist(rng));
		BOOST_CHECK(h.bucket_count() >= bcount);
	}
}

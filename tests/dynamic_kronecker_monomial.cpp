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

#include "../src/dynamic_kronecker_monomial.hpp"

#define BOOST_TEST_MODULE dynamic_kronecker_monomial_test
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <cstddef>
#include <functional>
#include <iterator>
#include <limits>
#include <random>
#include <tuple>
#include <type_traits>
#include <vector>

#include "../src/environment.hpp"
#include "../src/hash_set.hpp"
#include "../src/kronecker_array.hpp"
#include "../src/symbol_set.hpp"

#include <iostream>

using namespace piranha;

using int_types = boost::mpl::vector<signed char,short,int,long,long long>;
using size_types = boost::mpl::vector<std::integral_constant<int,8>,std::integral_constant<int,12>,
	std::integral_constant<int,16>,std::integral_constant<int,24>>;

static std::mt19937 rng;

// Constructors, assignments, getters, setters, etc.
struct constructor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		using k_type = dynamic_kronecker_monomial<T>;
		std::cout << sizeof(k_type) << '\n';
		k_type k{1,2,3};
		std::cout << k << '\n';
		symbol_set ss;
		ss.add("x");
		ss.add("y");
		ss.add("z");
		std::cout << k.unpack(ss) << '\n';
		std::cout << k.hash() << '\n';
		k = k_type{1,2,3,4,5,6,7,8,9,10,11,12,13,14};
		std::cout << k.hash() << '\n';
	}
};

BOOST_AUTO_TEST_CASE(dynamic_kronecker_monomial_constructor_test)
{
	environment env;
	boost::mpl::for_each<int_types>(constructor_tester());
}

// Generate a vector of size total_size, in which groups of ksize members
// are selected randomly from the limits in kronecker_array for size ksize.
template <std::size_t ksize, typename T, std::size_t total_size>
void generate_random_vector(std::array<T,total_size> &out)
{
	static_assert(total_size >= ksize && !(total_size % ksize),"Invalid size values.");
	using ka = kronecker_array<T>;
	using limits_size = decltype(ka::get_limits().size());
	using v_size = decltype(std::vector<T>{}.size());
	// Create a vector of random distributions with the appropriate limits.
	std::array<std::uniform_int_distribution<T>,ksize> dist_array;
	for (std::size_t i = 0u; i < ksize; ++i) {
		dist_array[i] = std::uniform_int_distribution<T>(T(-std::get<0>(ka::get_limits()[boost::numeric_cast<limits_size>(ksize)])[boost::numeric_cast<v_size>(i)]),
			std::get<0>(ka::get_limits()[boost::numeric_cast<limits_size>(ksize)])[boost::numeric_cast<v_size>(i)]);
	}
	for (std::size_t i = 0u; i < total_size; ++i) {
		out[i] = dist_array[static_cast<std::size_t>(i % ksize)](rng);
	}
}

// Evaluate the sparsity of a set. In this context, it means the average number of elements
// in non-empty buckets.
template <typename Set>
double compute_sparsity(const Set &s)
{
	auto sp = s.evaluate_sparsity();
	auto retval = 0., counter = 0.;
	for (const auto &p: sp) {
		if (p.first != 0u) {
			retval += double(p.first) * double(p.second);
			counter += double(p.second);
		}
	}
	return retval/counter;
}

struct hash_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void sparsity_testing() const
		{
			std::cout << "Sparsity testing with NBits = " << U::value << " and type: '" << typeid(T).name() << "'\n";
			const unsigned nitems = 500;
			using k_type = dynamic_kronecker_monomial<T,U::value>;
			using h_set = hash_set<k_type>;
			{
			std::array<T,k_type::ksize> tmp;
			h_set h;
			for (unsigned i = 0; i < nitems; ++i) {
				generate_random_vector<k_type::ksize>(tmp);
				h.insert(k_type(tmp.begin(),tmp.end()));
			}
			std::cout << "1 packed integral : " << compute_sparsity(h) << '\n';
			}
			{
			if (k_type::ksize > std::numeric_limits<std::size_t>::max() / 2u) {
				return;
			}
			std::array<T,k_type::ksize * 2u> tmp;
			h_set h;
			for (unsigned i = 0; i < nitems; ++i) {
				generate_random_vector<k_type::ksize>(tmp);
				h.insert(k_type(tmp.begin(),tmp.end()));
			}
			std::cout << "2 packed integrals: " << compute_sparsity(h) << '\n';
			}
			{
			if (k_type::ksize > std::numeric_limits<std::size_t>::max() / 3u) {
				return;
			}
			std::array<T,k_type::ksize * 3u> tmp;
			h_set h;
			for (unsigned i = 0; i < nitems; ++i) {
				generate_random_vector<k_type::ksize>(tmp);
				h.insert(k_type(tmp.begin(),tmp.end()));
			}
			std::cout << "3 packed integrals: " << compute_sparsity(h) << '\n';
			}
			{
			if (k_type::ksize > std::numeric_limits<std::size_t>::max() / 4u) {
				return;
			}
			std::array<T,k_type::ksize * 4u> tmp;
			h_set h;
			for (unsigned i = 0; i < nitems; ++i) {
				generate_random_vector<k_type::ksize>(tmp);
				h.insert(k_type(tmp.begin(),tmp.end()));
			}
			std::cout << "4 packed integrals: " << compute_sparsity(h) << '\n';
			}
			{
			if (k_type::ksize > std::numeric_limits<std::size_t>::max() / 5u) {
				return;
			}
			std::array<T,k_type::ksize * 5u> tmp;
			h_set h;
			for (unsigned i = 0; i < nitems; ++i) {
				generate_random_vector<k_type::ksize>(tmp);
				h.insert(k_type(tmp.begin(),tmp.end()));
			}
			std::cout << "5 packed integrals: " << compute_sparsity(h) << '\n';
			}
		}
		template <typename U>
		void hash_equality() const
		{
			// Some tests of consistency between equality and hash.
			using k_type = dynamic_kronecker_monomial<T,U::value>;
			const unsigned nitems = 500;
			{
			std::array<T,k_type::ksize> tmp;
			std::vector<T> tmpv;
			for (unsigned i = 0; i < nitems; ++i) {
				// The test does not make sense if the ksize is only 1.
				if (k_type::ksize == 1u) {
					continue;
				}
				tmpv.clear();
				generate_random_vector<k_type::ksize>(tmp);
				std::copy(tmp.begin(),tmp.end(),std::back_inserter(tmpv));
				// Erase the last element.
				tmpv.back() = 0;
				k_type k1(tmpv.begin(),tmpv.end() - 1);
				k_type k2(tmpv.begin(),tmpv.end());
				BOOST_CHECK(k1 == k2);
				BOOST_CHECK(!(k1 != k2));
				BOOST_CHECK(std::hash<k_type>()(k1) == std::hash<k_type>()(k2));
				// Restore the last element.
				tmpv.back() = 1;
				k_type k3(tmpv.begin(),tmpv.end());
				BOOST_CHECK(k2 != k3);
			}
			}
			{
			if (k_type::ksize > std::numeric_limits<std::size_t>::max() / 2u) {
				return;
			}
			std::array<T,k_type::ksize * 2u> tmp;
			std::vector<T> tmpv;
			for (unsigned i = 0; i < nitems; ++i) {
				if (k_type::ksize == 1u) {
					continue;
				}
				tmpv.clear();
				generate_random_vector<k_type::ksize>(tmp);
				std::copy(tmp.begin(),tmp.end(),std::back_inserter(tmpv));
				generate_random_vector<k_type::ksize>(tmp);
				std::copy(tmp.begin(),tmp.end(),std::back_inserter(tmpv));
				tmpv.back() = 0;
				k_type k1(tmpv.begin(),tmpv.end() - 1);
				k_type k2(tmpv.begin(),tmpv.end());
				BOOST_CHECK(k1 == k2);
				BOOST_CHECK(!(k1 != k2));
				BOOST_CHECK(std::hash<k_type>()(k1) == std::hash<k_type>()(k2));
				tmpv.back() = 1;
				k_type k3(tmpv.begin(),tmpv.end());
				BOOST_CHECK(k2 != k3);
			}
			}
			{
			if (k_type::ksize > std::numeric_limits<std::size_t>::max() / 3u) {
				return;
			}
			std::array<T,k_type::ksize * 3u> tmp;
			std::vector<T> tmpv;
			for (unsigned i = 0; i < nitems; ++i) {
				if (k_type::ksize == 1u) {
					continue;
				}
				tmpv.clear();
				generate_random_vector<k_type::ksize>(tmp);
				std::copy(tmp.begin(),tmp.end(),std::back_inserter(tmpv));
				generate_random_vector<k_type::ksize>(tmp);
				std::copy(tmp.begin(),tmp.end(),std::back_inserter(tmpv));
				tmpv.back() = 0;
				k_type k1(tmpv.begin(),tmpv.end() - 1);
				k_type k2(tmpv.begin(),tmpv.end());
				BOOST_CHECK(k1 == k2);
				BOOST_CHECK(!(k1 != k2));
				BOOST_CHECK(std::hash<k_type>()(k1) == std::hash<k_type>()(k2));
				tmpv.back() = 1;
				k_type k3(tmpv.begin(),tmpv.end());
				BOOST_CHECK(k2 != k3);
			}
			}
		}
		template <typename U>
		void hash_homomorphic() const
		{
			// Some tests of consistency between equality and hash.
			using k_type = dynamic_kronecker_monomial<T,U::value>;
			const unsigned nitems = 500;
			{
			std::array<T,k_type::ksize> tmp1, tmp2, tmp3;
			for (unsigned i = 0; i < nitems; ++i) {
				generate_random_vector<k_type::ksize>(tmp1);
				generate_random_vector<k_type::ksize>(tmp2);
				// Divide everything by two to avoid going outside the kronecker bounds.
				std::for_each(tmp1.begin(),tmp1.end(),[](T &value) {value = static_cast<T>(value / 2);});
				std::for_each(tmp2.begin(),tmp2.end(),[](T &value) {value = static_cast<T>(value / 2);});
				// Add.
				std::transform(tmp1.begin(),tmp1.end(),tmp2.begin(),tmp3.begin(),
					[](const T &n, const T &m) {return static_cast<T>(n+m);});
				k_type k1(tmp1.begin(),tmp1.end()), k2(tmp2.begin(),tmp2.end()), k3(tmp3.begin(),tmp3.end());
				BOOST_CHECK_EQUAL(static_cast<std::size_t>(k1.hash() + k2.hash()),k3.hash());
			}
			}
			{
			if (k_type::ksize > std::numeric_limits<std::size_t>::max() / 2u) {
				return;
			}
			std::array<T,k_type::ksize * 2u> tmp1, tmp2, tmp3;
			for (unsigned i = 0; i < nitems; ++i) {
				generate_random_vector<k_type::ksize>(tmp1);
				generate_random_vector<k_type::ksize>(tmp2);
				std::for_each(tmp1.begin(),tmp1.end(),[](T &value) {value = static_cast<T>(value / 2);});
				std::for_each(tmp2.begin(),tmp2.end(),[](T &value) {value = static_cast<T>(value / 2);});
				std::transform(tmp1.begin(),tmp1.end(),tmp2.begin(),tmp3.begin(),
					[](const T &n, const T &m) {return static_cast<T>(n+m);});
				k_type k1(tmp1.begin(),tmp1.end()), k2(tmp2.begin(),tmp2.end()), k3(tmp3.begin(),tmp3.end());
				BOOST_CHECK_EQUAL(static_cast<std::size_t>(k1.hash() + k2.hash()),k3.hash());
			}
			}
			{
			if (k_type::ksize > std::numeric_limits<std::size_t>::max() / 3u) {
				return;
			}
			std::array<T,k_type::ksize * 3u> tmp1, tmp2, tmp3;
			for (unsigned i = 0; i < nitems; ++i) {
				generate_random_vector<k_type::ksize>(tmp1);
				generate_random_vector<k_type::ksize>(tmp2);
				std::for_each(tmp1.begin(),tmp1.end(),[](T &value) {value = static_cast<T>(value / 2);});
				std::for_each(tmp2.begin(),tmp2.end(),[](T &value) {value = static_cast<T>(value / 2);});
				std::transform(tmp1.begin(),tmp1.end(),tmp2.begin(),tmp3.begin(),
					[](const T &n, const T &m) {return static_cast<T>(n+m);});
				k_type k1(tmp1.begin(),tmp1.end()), k2(tmp2.begin(),tmp2.end()), k3(tmp3.begin(),tmp3.end());
				BOOST_CHECK_EQUAL(static_cast<std::size_t>(k1.hash() + k2.hash()),k3.hash());
			}
			}
			{
			if (k_type::ksize > std::numeric_limits<std::size_t>::max() / 4u) {
				return;
			}
			std::array<T,k_type::ksize * 4u> tmp1, tmp2, tmp3;
			for (unsigned i = 0; i < nitems; ++i) {
				generate_random_vector<k_type::ksize>(tmp1);
				generate_random_vector<k_type::ksize>(tmp2);
				std::for_each(tmp1.begin(),tmp1.end(),[](T &value) {value = static_cast<T>(value / 2);});
				std::for_each(tmp2.begin(),tmp2.end(),[](T &value) {value = static_cast<T>(value / 2);});
				std::transform(tmp1.begin(),tmp1.end(),tmp2.begin(),tmp3.begin(),
					[](const T &n, const T &m) {return static_cast<T>(n+m);});
				k_type k1(tmp1.begin(),tmp1.end()), k2(tmp2.begin(),tmp2.end()), k3(tmp3.begin(),tmp3.end());
				BOOST_CHECK_EQUAL(static_cast<std::size_t>(k1.hash() + k2.hash()),k3.hash());
			}
			}
			{
			if (k_type::ksize > std::numeric_limits<std::size_t>::max() / 16u) {
				return;
			}
			std::array<T,k_type::ksize * 16u> tmp1, tmp2, tmp3;
			for (unsigned i = 0; i < nitems; ++i) {
				generate_random_vector<k_type::ksize>(tmp1);
				generate_random_vector<k_type::ksize>(tmp2);
				std::for_each(tmp1.begin(),tmp1.end(),[](T &value) {value = static_cast<T>(value / 2);});
				std::for_each(tmp2.begin(),tmp2.end(),[](T &value) {value = static_cast<T>(value / 2);});
				std::transform(tmp1.begin(),tmp1.end(),tmp2.begin(),tmp3.begin(),
					[](const T &n, const T &m) {return static_cast<T>(n+m);});
				k_type k1(tmp1.begin(),tmp1.end()), k2(tmp2.begin(),tmp2.end()), k3(tmp3.begin(),tmp3.end());
				BOOST_CHECK_EQUAL(static_cast<std::size_t>(k1.hash() + k2.hash()),k3.hash());
			}
			}
		}
		template <typename U, typename std::enable_if<(U::value <= std::numeric_limits<T>::digits + 1),int>::type = 0>
		void operator()(const U &) const
		{
			// Just a check with zero size / elements.
			using k_type = dynamic_kronecker_monomial<T,U::value>;
			k_type k;
			BOOST_CHECK_EQUAL(std::hash<k_type>()(k),0u);
			k = k_type{0,0,0,0,0,0,0};
			BOOST_CHECK_EQUAL(std::hash<k_type>()(k),0u);
			sparsity_testing<U>();
			hash_equality<U>();
			hash_homomorphic<U>();
		}
		template <typename U, typename std::enable_if<(U::value > std::numeric_limits<T>::digits + 1),int>::type = 0>
		void operator()(const U &) const {}
	};
	template <typename T>
	void operator()(const T &) const
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

// This includes also equality testing.
BOOST_AUTO_TEST_CASE(dynamic_kronecker_monomial_hash_test)
{
	boost::mpl::for_each<int_types>(hash_tester());
}

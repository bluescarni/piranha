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

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <random>
#include <tuple>
#include <type_traits>

#include "../src/environment.hpp"
#include "../src/hash_set.hpp"
#include "../src/kronecker_array.hpp"
#include "../src/symbol_set.hpp"

#include <iostream>

using namespace piranha;

using int_types = boost::mpl::vector<signed char,int,long,long long>;
using size_types = boost::mpl::vector<std::integral_constant<int,8>,std::integral_constant<int,12>,std::integral_constant<int,16>>;

std::mt19937 rng;

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

template <std::size_t ksize, typename T, std::size_t total_size>
void generate_random_vector(std::array<T,total_size> &out)
{
	static_assert(total_size >= ksize && !(total_size % ksize),"Invalid size values.");
	using ka = kronecker_array<T>;
	std::array<std::uniform_int_distribution<T>,ksize> dist_array;
	for (std::size_t i = 0u; i < ksize; ++i) {
		dist_array[i] = std::uniform_int_distribution<T>(T(-std::get<0>(ka::get_limits()[ksize])[i]),
			std::get<0>(ka::get_limits()[ksize])[i]);
	}
	for (std::size_t i = 0u; i < total_size; ++i) {
		out[i] = dist_array[i % ksize](rng);
	}
}

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
	void operator()(const T &)
	{
		const unsigned nitems = 500;
		using k_type = dynamic_kronecker_monomial<T>;
		using h_set = hash_set<k_type>;
		{
		std::array<T,k_type::ksize> tmp;
		h_set h;
		for (unsigned i = 0; i < nitems; ++i) {
			generate_random_vector<k_type::ksize>(tmp);
			h.insert(k_type(tmp.begin(),tmp.end()));
		}
		std::cout << "Sparsity for type '" << typeid(T).name() << "', 1 packed integral: " << compute_sparsity(h) << '\n';
		}
		{
		std::array<T,k_type::ksize * 2u> tmp;
		h_set h;
		for (unsigned i = 0; i < nitems; ++i) {
			generate_random_vector<k_type::ksize>(tmp);
			h.insert(k_type(tmp.begin(),tmp.end()));
		}
		std::cout << "Sparsity for type '" << typeid(T).name() << "', 2 packed integrals: " << compute_sparsity(h) << '\n';
		}
		{
		std::array<T,k_type::ksize * 3u> tmp;
		h_set h;
		for (unsigned i = 0; i < nitems; ++i) {
			generate_random_vector<k_type::ksize>(tmp);
			h.insert(k_type(tmp.begin(),tmp.end()));
		}
		std::cout << "Sparsity for type '" << typeid(T).name() << "', 3 packed integrals: " << compute_sparsity(h) << '\n';
		}
	}
};

BOOST_AUTO_TEST_CASE(dynamic_kronecker_monomial_hash_test)
{
	boost::mpl::for_each<int_types>(hash_tester());
}

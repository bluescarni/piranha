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

#include "../src/kronecker_array.hpp"

#define BOOST_TEST_MODULE kronecker_array_test
#include <boost/test/unit_test.hpp>

#include <boost/integer_traits.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <vector>

#include "../src/environment.hpp"

using namespace piranha;

typedef boost::mpl::vector<std::int_least8_t,std::int_least16_t,std::int_least32_t,std::make_signed<std::size_t>::type> int_types;

// Limits.
struct limits_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_array<T> ka_type;
		auto l = ka_type::get_limits();
		typedef decltype(l) l_type;
		typedef typename l_type::value_type v_type;
		typedef typename l_type::size_type size_type;
		BOOST_CHECK(l.size() > 1u);
		BOOST_CHECK(l[0u] == v_type());
		BOOST_CHECK(std::get<0u>(l[1u])[0u] == -std::get<1u>(l[1u]));
		BOOST_CHECK(std::get<0u>(l[1u])[0u] == std::get<2u>(l[1u]));
		for (size_type i = 1u; i < l.size(); ++i) {
			for (size_type j = 0u; j < std::get<0u>(l[i]).size(); ++j) {
				BOOST_CHECK(std::get<0u>(l[i])[j] > 0);
			}
			BOOST_CHECK(std::get<1u>(l[i]) < 0);
			BOOST_CHECK(std::get<2u>(l[i]) > 0);
			BOOST_CHECK(std::get<3u>(l[i]) > 0);
			if (std::is_same<T,std::make_signed<std::size_t>::type>::value) {
				std::cout << '[';
				for (size_type j = 0u; j < std::get<0u>(l[i]).size(); ++j) {
					std::cout << std::get<0u>(l[i])[j] << ',';
				}
				std::cout << "] " << std::get<1u>(l[i]) << ',' << std::get<2u>(l[i]) << ',' << std::get<3u>(l[i]) << '\n';
			}
		}
	}
};

BOOST_AUTO_TEST_CASE(kronecker_array_limits_test)
{
	environment env;
	boost::mpl::for_each<int_types>(limits_tester());
}

// Coding/decoding.
struct coding_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_array<T> ka_type;
		auto &l = ka_type::get_limits();
		BOOST_CHECK(ka_type::encode(std::vector<std::int16_t>{}) == 0);
		BOOST_CHECK(ka_type::encode(std::vector<std::int16_t>{0}) == 0);
		BOOST_CHECK(ka_type::encode(std::vector<std::int16_t>{1}) == 1);
		BOOST_CHECK(ka_type::encode(std::vector<std::int16_t>{-1}) == -1);
		BOOST_CHECK(ka_type::encode(std::vector<std::int16_t>{-10}) == -10);
		BOOST_CHECK(ka_type::encode(std::vector<std::int16_t>{10}) == 10);
		// NOTE: static_cast for use when T is a char.
		const T emax1 = std::get<0u>(l[1u])[0u], emin1 = static_cast<T>(-emax1);
		BOOST_CHECK(ka_type::encode(std::vector<T>{emin1}) == emin1);
		BOOST_CHECK(ka_type::encode(std::vector<T>{emax1}) == emax1);
		std::mt19937 rng;
		// Test with max/min vectors in various sizes.
		for (std::uint_least8_t i = 1u; i < l.size(); ++i) {
			auto M = std::get<0u>(l[i]);
			auto m = M;
			for (auto it = m.begin(); it != m.end(); ++it) {
				*it = static_cast<T>(-(*it));
			}
			auto tmp(m);
			auto c = ka_type::encode(m);
			ka_type::decode(tmp,c);
			BOOST_CHECK(m == tmp);
			tmp = M;
			c = ka_type::encode(M);
			ka_type::decode(tmp,c);
			BOOST_CHECK(M == tmp);
			auto v1 = std::vector<T>(i,0);
			auto v2 = v1;
			c = ka_type::encode(v1);
			ka_type::decode(v1,c);
			BOOST_CHECK(v2 == v1);
			v1 = std::vector<T>(i,-1);
			v2 = v1;
			c = ka_type::encode(v1);
			ka_type::decode(v1,c);
			BOOST_CHECK(v2 == v1);
			// Test with random values within the bounds.
			for (auto j = 0; j < 10000; ++j) {
				for (decltype(v1.size()) i = 0u; i < v1.size(); ++i) {
					std::uniform_int_distribution<T> dist(m[i],M[i]);
					v1[i] = dist(rng);
				}
				v2 = v1;
				c = ka_type::encode(v1);
				ka_type::decode(v1,c);
				BOOST_CHECK(v2 == v1);
			}
		}
		// Exceptions tests.
		BOOST_CHECK_THROW(ka_type::encode(std::vector<T>(l.size())),std::invalid_argument);
		BOOST_CHECK_THROW(ka_type::encode(std::vector<T>{T(0),boost::integer_traits<T>::const_min}),std::invalid_argument);
		BOOST_CHECK_THROW(ka_type::encode(std::vector<T>{T(0),boost::integer_traits<T>::const_max}),std::invalid_argument);
		std::vector<T> v1(l.size());
		BOOST_CHECK_THROW(ka_type::decode(v1,0),std::invalid_argument);
		v1.resize(0);
		BOOST_CHECK_THROW(ka_type::decode(v1,1),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(kronecker_array_coding_test)
{
	boost::mpl::for_each<int_types>(coding_tester());
}

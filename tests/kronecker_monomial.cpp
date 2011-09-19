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

#include "../src/kronecker_monomial.hpp"

#define BOOST_TEST_MODULE kronecker_monomial_test
#include <boost/test/unit_test.hpp>

#include <boost/integer_traits.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "../src/kronecker_array.hpp"
#include "../src/symbol.hpp"
#include "../src/symbol_set.hpp"

using namespace piranha;

typedef boost::mpl::vector<int,long,long long> int_types;

// Constructors, assignments, getters, setters, etc.
struct constructor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		typedef kronecker_array<T> ka;
		k_type k1;
		BOOST_CHECK_EQUAL(k1.get_int(),0);
		k_type k2({-1,-1});
		std::vector<T> v2(2);
		ka::decode(v2,k2.get_int());
		BOOST_CHECK_EQUAL(v2[0],-1);
		BOOST_CHECK_EQUAL(v2[1],-1);
		k_type k3({});
		BOOST_CHECK_EQUAL(k3.get_int(),0);
		k_type k4({10});
		BOOST_CHECK_EQUAL(k4.get_int(),10);
		k_type k5(symbol_set({}));
		BOOST_CHECK_EQUAL(k5.get_int(),0);
		k_type k6(symbol_set({symbol("a")}));
		BOOST_CHECK_EQUAL(k6.get_int(),0);
		k_type k7(symbol_set({symbol("a"),symbol("b")}));
		BOOST_CHECK_EQUAL(k7.get_int(),0);
		k_type k8(0);
		BOOST_CHECK_EQUAL(k8.get_int(),0);
		k_type k9(1);
		BOOST_CHECK_EQUAL(k9.get_int(),1);
		k_type k10;
		k10.set_int(10);
		BOOST_CHECK_EQUAL(k10.get_int(),10);
		k_type k11;
		k11 = k10;
		BOOST_CHECK_EQUAL(k11.get_int(),10);
		k11 = std::move(k9);
		BOOST_CHECK_EQUAL(k9.get_int(),1);
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_constructor_test)
{
	boost::mpl::for_each<int_types>(constructor_tester());
}

struct compatibility_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		typedef kronecker_array<T> ka;
		const auto &limits = ka::get_limits();
		k_type k1;
		BOOST_CHECK(k1.is_compatible(symbol_set({})));
		k1.set_int(1);
		BOOST_CHECK(!k1.is_compatible(symbol_set({})));
		if (limits.size() < 255u) {
			symbol_set v2;
			for (auto i = 0u; i < 255; ++i) {
				v2.add(std::string(1u,(char)i));
			}
			BOOST_CHECK(!k1.is_compatible(v2));
		}
		k1.set_int(boost::integer_traits<T>::const_max);
		BOOST_CHECK(!k1.is_compatible(symbol_set({symbol("a"),symbol("b")})));
		k1.set_int(-1);
		BOOST_CHECK(k1.is_compatible(symbol_set({symbol("a"),symbol("b")})));
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_compatibility_test)
{
	boost::mpl::for_each<int_types>(compatibility_tester());
}

struct merge_args_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		typedef kronecker_array<T> ka;
		k_type k1;
		symbol_set vs1({symbol("a")}), empty;
		BOOST_CHECK(k1.merge_args(empty,vs1).get_int() == 0);
		std::vector<T> v1(1);
		ka::decode(v1,k1.merge_args(empty,vs1).get_int());
		BOOST_CHECK(v1[0] == 0);
		auto vs2 = vs1;
		vs2.add(symbol("b"));
		k_type k2({-1});
		BOOST_CHECK(k2.merge_args(vs1,vs2).get_int() == ka::encode(std::vector<int>({-1,0})));
		vs1.add(symbol("c"));
		vs2.add(symbol("c"));
		vs2.add(symbol("d"));
		k_type k3({-1,-1});
		BOOST_CHECK(k3.merge_args(vs1,vs2).get_int() == ka::encode(std::vector<int>({-1,0,-1,0})));
		vs1 = symbol_set({symbol("c")});
		k_type k4({-1});
		BOOST_CHECK(k4.merge_args(vs1,vs2).get_int() == ka::encode(std::vector<int>({0,0,-1,0})));
		vs1 = symbol_set({});
		k_type k5({});
		BOOST_CHECK(k5.merge_args(vs1,vs2).get_int() == ka::encode(std::vector<int>({0,0,0,0})));
		vs1.add(symbol("e"));
		BOOST_CHECK_THROW(k5.merge_args(vs1,vs2),std::invalid_argument);
		BOOST_CHECK_THROW(k5.merge_args(vs2,vs1),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_merge_args_test)
{
	boost::mpl::for_each<int_types>(merge_args_tester());
}

struct is_unitary_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		k_type k1;
		symbol_set vs1;
		BOOST_CHECK(k1.is_unitary(vs1));
		k_type k2({-1});
		vs1.add(symbol("a"));
		BOOST_CHECK(!k2.is_unitary(vs1));
		k_type k3({0});
		BOOST_CHECK(k3.is_unitary(vs1));
		vs1.add(symbol("b"));
		k_type k4({0,0});
		BOOST_CHECK(k4.is_unitary(vs1));
		k_type k5({0,1});
		BOOST_CHECK(!k5.is_unitary(vs1));
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_is_unitary_test)
{
	boost::mpl::for_each<int_types>(is_unitary_tester());
}

struct degree_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		k_type k1;
		symbol_set vs1;
		BOOST_CHECK(k1.degree(vs1) == 0);
		k_type k2({0});
		vs1.add(symbol("a"));
		BOOST_CHECK(k2.degree(vs1) == 0);
		k_type k3({-1});
		BOOST_CHECK(k3.degree(vs1) == -1);
		vs1.add(symbol("b"));
		k_type k4({0,0});
		BOOST_CHECK(k4.degree(vs1) == 0);
		k_type k5({-1,-1});
		BOOST_CHECK(k5.degree(vs1) == -2);
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_degree_test)
{
	boost::mpl::for_each<int_types>(degree_tester());
}

struct multiply_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		typedef kronecker_array<T> ka;
		k_type k1, k2, result;
		symbol_set vs1;
		k1.multiply(result,k2,vs1);
		BOOST_CHECK(result.get_int() == 0);
		k1 = k_type({0});
		k2 = k_type({0});
		vs1.add(symbol("a"));
		k1.multiply(result,k2,vs1);
		BOOST_CHECK(result.get_int() == 0);
		k1 = k_type({1});
		k2 = k_type({2});
		k1.multiply(result,k2,vs1);
		BOOST_CHECK(result.get_int() == 3);
		k1 = k_type({1,-1});
		k2 = k_type({2,0});
		vs1.add(symbol("b"));
		k1.multiply(result,k2,vs1);
		std::vector<int> tmp(2);
		ka::decode(tmp,result.get_int());
		BOOST_CHECK(tmp[0] == 3);
		BOOST_CHECK(tmp[1] == -1);
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_multiply_test)
{
	boost::mpl::for_each<int_types>(multiply_tester());
}

struct equality_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		k_type k1, k2;
		BOOST_CHECK(k1 == k2);
		BOOST_CHECK(!(k1 != k2));
		k1 = k_type({0});
		k2 = k_type({0});
		BOOST_CHECK(k1 == k2);
		BOOST_CHECK(!(k1 != k2));
		k2 = k_type({1});
		BOOST_CHECK(!(k1 == k2));
		BOOST_CHECK(k1 != k2);
		k1 = k_type({0,0});
		k2 = k_type({0,0});
		BOOST_CHECK(k1 == k2);
		BOOST_CHECK(!(k1 != k2));
		k1 = k_type({1,0});
		k2 = k_type({1,0});
		BOOST_CHECK(k1 == k2);
		BOOST_CHECK(!(k1 != k2));
		k1 = k_type({1,0});
		k2 = k_type({0,1});
		BOOST_CHECK(!(k1 == k2));
		BOOST_CHECK(k1 != k2);
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_equality_test)
{
	boost::mpl::for_each<int_types>(equality_tester());
}

struct stream_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		std::ostringstream oss;
		oss << k_type({});
		BOOST_CHECK(!oss.str().empty());
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_stream_test)
{
	boost::mpl::for_each<int_types>(stream_tester());
}

struct hash_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		k_type k1;
		BOOST_CHECK(k1.hash() == std::hash<k_type>()(k1));
		k1 = k_type({0});
		BOOST_CHECK(k1.hash() == std::hash<k_type>()(k1));
		k1 = k_type({0,1});
		BOOST_CHECK(k1.hash() == std::hash<k_type>()(k1));
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_hash_test)
{
	boost::mpl::for_each<int_types>(hash_tester());
}

struct get_element_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		symbol_set vs1;
		vs1.add(symbol("a"));
		k_type k1({0});
		BOOST_CHECK(k1.get_element(0,vs1) == 0);
		k_type k2({-1});
		BOOST_CHECK(k2.get_element(0,vs1) == -1);
		k_type k3({-1,0});
		vs1.add(symbol("b"));
		BOOST_CHECK(k3.get_element(0,vs1) == -1);
		BOOST_CHECK(k3.get_element(1,vs1) == 0);
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_get_element_test)
{
	boost::mpl::for_each<int_types>(get_element_tester());
}

struct unpack_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		symbol_set vs1;
		k_type k1({0});
		auto t1 = k1.unpack(vs1);
		BOOST_CHECK(!t1.size());
		vs1.add(symbol("a"));
		k1.set_int(-1);
		auto t2 = k1.unpack(vs1);
		BOOST_CHECK(t2.size());
		BOOST_CHECK(t2[0u] == -1);
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_unpack_test)
{
	boost::mpl::for_each<int_types>(unpack_tester());
}

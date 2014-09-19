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
#include <cstddef>
#include <initializer_list>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../src/environment.hpp"
#include "../src/exceptions.hpp"
#include "../src/kronecker_array.hpp"
#include "../src/math.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/real.hpp"
#include "../src/symbol.hpp"
#include "../src/symbol_set.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

typedef boost::mpl::vector<signed char,int,long,long long> int_types;

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
		// Constructor from iterators.
		v2 = {};
		k_type k12(v2.begin(),v2.end());
		BOOST_CHECK_EQUAL(k12.get_int(),0);
		v2 = {21};
		k_type k13(v2.begin(),v2.end());
		BOOST_CHECK_EQUAL(k13.get_int(),21);
		v2 = {-21};
		k_type k14(v2.begin(),v2.end());
		BOOST_CHECK_EQUAL(k14.get_int(),-21);
		v2 = {1,-2};
		k_type k15(v2.begin(),v2.end());
		auto v = k15.unpack(symbol_set({symbol("a"),symbol("b")}));
		BOOST_CHECK(v.size() == 2u);
		BOOST_CHECK(v[0u] == 1);
		BOOST_CHECK(v[1u] == -2);
		BOOST_CHECK((std::is_constructible<k_type,T *, T *>::value));
		// Iterators have to be of homogeneous type.
		BOOST_CHECK((!std::is_constructible<k_type,T *, T const *>::value));
		BOOST_CHECK((std::is_constructible<k_type,typename std::vector<T>::iterator,typename std::vector<T>::iterator>::value));
		BOOST_CHECK((!std::is_constructible<k_type,typename std::vector<T>::const_iterator,typename std::vector<T>::iterator>::value));
		BOOST_CHECK((!std::is_constructible<k_type,typename std::vector<T>::iterator,int>::value));
		// Converting constructor.
		k_type k16, k17(k16,symbol_set{});
		BOOST_CHECK(k16 == k17);
		k16.set_int(10);
		k_type k18(k16,symbol_set({symbol("a")}));
		BOOST_CHECK(k16 == k18);
		BOOST_CHECK_THROW((k_type(k16,symbol_set({}))),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_constructor_test)
{
	environment env;
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
		typedef kronecker_array<T> ka;
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
		BOOST_CHECK_THROW(k5.is_unitary(symbol_set{}),std::invalid_argument);
		symbol_set vs2;
		const auto &l = ka::get_limits();
		typedef decltype(l.size()) size_type;
		for (size_type i = 0u; i <= l.size(); ++i) {
			vs2.add(boost::lexical_cast<std::string>(i));
		}
		BOOST_CHECK_THROW(k5.is_unitary(vs2),std::invalid_argument);
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
		BOOST_CHECK(k1.ldegree(vs1) == 0);
		k_type k2({0});
		vs1.add(symbol("a"));
		BOOST_CHECK(k2.degree(vs1) == 0);
		BOOST_CHECK(k2.ldegree(vs1) == 0);
		k_type k3({-1});
		BOOST_CHECK(k3.degree(vs1) == -1);
		BOOST_CHECK(k3.ldegree(vs1) == -1);
		vs1.add(symbol("b"));
		k_type k4({0,0});
		BOOST_CHECK(k4.degree(vs1) == 0);
		BOOST_CHECK(k4.ldegree(vs1) == 0);
		k_type k5({-1,-1});
		BOOST_CHECK(k5.degree(vs1) == -2);
		BOOST_CHECK(k5.degree({"a"},vs1) == -1);
		// NOTE: here it seems the compilation error arising when only {} is used (as opposed
		// to std::set<std::string>{}) is a bug in libc++. See:
		// http://clang-developers.42468.n3.nabble.com/C-11-error-about-initializing-explicit-constructor-with-td4029849.html
		BOOST_CHECK(k5.degree(std::set<std::string>{},vs1) == 0);
		BOOST_CHECK(k5.degree({"f"},vs1) == 0);
		BOOST_CHECK(k5.degree({"a","b"},vs1) == -2);
		BOOST_CHECK(k5.degree({"a","c"},vs1) == -1);
		BOOST_CHECK(k5.degree({"d","c"},vs1) == 0);
		BOOST_CHECK(k5.degree({"d","b"},vs1) == -1);
		BOOST_CHECK(k5.degree({"A","a"},vs1) == -1);
		BOOST_CHECK(k5.ldegree(vs1) == -2);
		BOOST_CHECK(k5.ldegree({"a"},vs1) == -1);
		BOOST_CHECK(k5.ldegree(std::set<std::string>{},vs1) == 0);
		BOOST_CHECK(k5.ldegree({"f"},vs1) == 0);
		BOOST_CHECK(k5.ldegree({"a","b"},vs1) == -2);
		BOOST_CHECK(k5.ldegree({"a","c"},vs1) == -1);
		BOOST_CHECK(k5.ldegree({"d","c"},vs1) == 0);
		BOOST_CHECK(k5.ldegree({"d","b"},vs1) == -1);
		BOOST_CHECK(k5.ldegree({"A","a"},vs1) == -1);
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
		std::vector<int> tmp(2u);
		ka::decode(tmp,result.get_int());
		BOOST_CHECK(tmp[0u] == 3);
		BOOST_CHECK(tmp[1u] == -1);
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

struct hash_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		k_type k1;
		BOOST_CHECK(k1.hash() == (std::size_t)(k1.get_int()));
		k1 = k_type({0});
		BOOST_CHECK(k1.hash() == (std::size_t)(k1.get_int()));
		k1 = k_type({0,1});
		BOOST_CHECK(k1.hash() == (std::size_t)(k1.get_int()));
		k1 = k_type({0,1,-1});
		BOOST_CHECK(k1.hash() == (std::size_t)(k1.get_int()));
		BOOST_CHECK(std::hash<k_type>()(k1) == (std::size_t)(k1.get_int()));
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_hash_test)
{
	boost::mpl::for_each<int_types>(hash_tester());
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
		typedef decltype(t1) s_vector_type;
		BOOST_CHECK(!t1.size());
		vs1.add(symbol("a"));
		k1.set_int(-1);
		auto t2 = k1.unpack(vs1);
		BOOST_CHECK(t2.size());
		BOOST_CHECK(t2[0u] == -1);
		// Check for overflow condition.
		std::string tmp = "";
		for (integer i(0u); i < integer(s_vector_type::max_size) + 1; ++i) {
			tmp += "b";
			vs1.add(symbol(tmp));
		}
		BOOST_CHECK_THROW(k1.unpack(vs1),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_unpack_test)
{
	boost::mpl::for_each<int_types>(unpack_tester());
}

struct print_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		symbol_set vs;
		k_type k1;
		std::ostringstream oss;
		k1.print(oss,vs);
		BOOST_CHECK(oss.str().empty());
		vs.add("x");
		k_type k2(vs);
		k2.print(oss,vs);
		BOOST_CHECK(oss.str().empty());
		k_type k3({T(-1)});
		k3.print(oss,vs);
		BOOST_CHECK(oss.str() == "x**-1");
		k_type k4({T(1)});
		oss.str("");
		k4.print(oss,vs);
		BOOST_CHECK(oss.str() == "x");
		k_type k5({T(-1),T(1)});
		vs.add("y");
		oss.str("");
		k5.print(oss,vs);
		BOOST_CHECK(oss.str() == "x**-1*y");
		k_type k6({T(-1),T(-2)});
		oss.str("");
		k6.print(oss,vs);
		BOOST_CHECK(oss.str() == "x**-1*y**-2");
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_print_test)
{
	boost::mpl::for_each<int_types>(print_tester());
}

struct linear_argument_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		symbol_set vs;
		BOOST_CHECK_THROW(k_type().linear_argument(vs),std::invalid_argument);
		vs.add("x");
		BOOST_CHECK_THROW(k_type().linear_argument(vs),std::invalid_argument);
		k_type k({T(1)});
		BOOST_CHECK_EQUAL(k.linear_argument(vs),"x");
		k = k_type({T(0),T(1)});
		vs.add("y");
		BOOST_CHECK_EQUAL(k.linear_argument(vs),"y");
		k = k_type({T(0),T(2)});
		BOOST_CHECK_THROW(k.linear_argument(vs),std::invalid_argument);
		k = k_type({T(2),T(0)});
		BOOST_CHECK_THROW(k.linear_argument(vs),std::invalid_argument);
		k = k_type({T(1),T(1)});
		BOOST_CHECK_THROW(k.linear_argument(vs),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_linear_argument_test)
{
	boost::mpl::for_each<int_types>(linear_argument_tester());
}

struct pow_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		typedef kronecker_array<T> ka;
		const auto &limits = ka::get_limits();
		k_type k1;
		k1.set_int(1);
		symbol_set vs;
		BOOST_CHECK_THROW(k1.pow(42,vs),std::invalid_argument);
		vs.add("x");
		BOOST_CHECK_THROW(k1.pow(42.5,vs),std::invalid_argument);
		k1 = k_type{2};
		k_type k2({4});
		BOOST_CHECK(k1.pow(2,vs) == k2);
		BOOST_CHECK_THROW(k1.pow(boost::integer_traits<T>::const_max,vs),std::overflow_error);
		k1 = k_type{1};
		if (std::get<0u>(limits[1u])[0u] < boost::integer_traits<T>::const_max) {
			BOOST_CHECK_THROW(k1.pow(std::get<0u>(limits[1u])[0u] + T(1),vs),std::invalid_argument);
		}
		k1 = k_type{2};
		BOOST_CHECK(k1.pow(0,vs) == k_type{});
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_pow_test)
{
	boost::mpl::for_each<int_types>(pow_tester());
}

struct partial_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		symbol_set vs;
		k_type k1;
		k1.set_int(T(1));
		BOOST_CHECK_THROW(k1.partial(symbol("x"),vs),std::invalid_argument);
		vs.add("x");
		k1 = k_type({T(2)});
		auto ret = k1.partial(symbol("x"),vs);
		BOOST_CHECK_EQUAL(ret.first,2);
		BOOST_CHECK(ret.second == k_type({T(1)}));
		ret = k1.partial(symbol("y"),vs);
		BOOST_CHECK_EQUAL(ret.first,0);
		k1 = k_type({T(0)});
		ret = k1.partial(symbol("x"),vs);
		BOOST_CHECK_EQUAL(ret.first,0);
		vs.add("y");
		k1 = k_type({T(-1),T(0)});
		ret = k1.partial(symbol("y"),vs);
		BOOST_CHECK_EQUAL(ret.first,0);
		ret = k1.partial(symbol("x"),vs);
		BOOST_CHECK_EQUAL(ret.first,-1);
		BOOST_CHECK(ret.second == k_type({T(-2),T(0)}));
		// Check limits violation.
		typedef kronecker_array<T> ka;
		const auto &limits = ka::get_limits();
		k1 = k_type{-std::get<0u>(limits[2u])[0u],-std::get<0u>(limits[2u])[0u]};
		BOOST_CHECK_THROW(ret = k1.partial(symbol("x"),vs),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_partial_test)
{
	boost::mpl::for_each<int_types>(partial_tester());
}

struct evaluate_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		typedef std::unordered_map<symbol,integer> dict_type;
		BOOST_CHECK((key_is_evaluable<k_type,integer>::value));
		symbol_set vs;
		k_type k1;
		BOOST_CHECK_EQUAL(k1.evaluate(dict_type{},vs),integer(1));
		vs.add("x");
		BOOST_CHECK_THROW(k1.evaluate(dict_type{},vs),std::invalid_argument);
		k1 = k_type({T(1)});
		BOOST_CHECK_THROW(k1.evaluate(dict_type{},vs),std::invalid_argument);
		BOOST_CHECK_EQUAL(k1.evaluate(dict_type{{symbol("x"),integer(1)}},vs),1);
		k1 = k_type({T(2)});
		BOOST_CHECK_EQUAL(k1.evaluate(dict_type{{symbol("x"),integer(3)}},vs),9);
		BOOST_CHECK_EQUAL(k1.evaluate(dict_type{{symbol("x"),integer(3)},{symbol("y"),integer(4)}},vs),9);
		k1 = k_type({T(2),T(3)});
		vs.add("y");
		BOOST_CHECK_EQUAL(k1.evaluate(dict_type{{symbol("x"),integer(3)},{symbol("y"),integer(4)}},vs),576);
		BOOST_CHECK_EQUAL(k1.evaluate(dict_type{{symbol("y"),integer(4)},{symbol("x"),integer(3)}},vs),576);
		typedef std::unordered_map<symbol,double> dict_type2;
		BOOST_CHECK_EQUAL(k1.evaluate(dict_type2{{symbol("y"),-4.3},{symbol("x"),3.2}},vs),math::pow(3.2,2) * math::pow(-4.3,3));
		typedef std::unordered_map<symbol,rational> dict_type3;
		BOOST_CHECK_EQUAL(k1.evaluate(dict_type3{{symbol("y"),rational(1,2)},{symbol("x"),rational(-4,3)}},vs),
			math::pow(rational(4,-3),2) * math::pow(rational(-1,-2),3));
		k1 = k_type({T(-2),T(-3)});
		BOOST_CHECK_EQUAL(k1.evaluate(dict_type3{{symbol("y"),rational(1,2)},{symbol("x"),rational(-4,3)}},vs),
			math::pow(rational(4,-3),-2) * math::pow(rational(-1,-2),-3));
		typedef std::unordered_map<symbol,real> dict_type4;
		BOOST_CHECK_EQUAL(k1.evaluate(dict_type4{{symbol("y"),real(1.234)},{symbol("x"),real(5.678)}},vs),
			math::pow(real(5.678),-2) * math::pow(real(1.234),-3));
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_evaluate_test)
{
	boost::mpl::for_each<int_types>(evaluate_tester());
	BOOST_CHECK((!key_is_evaluable<kronecker_monomial<>,std::vector<int>>::value));
	BOOST_CHECK((!key_is_evaluable<kronecker_monomial<>,char *>::value));
	BOOST_CHECK((!key_is_evaluable<kronecker_monomial<>,std::string>::value));
	BOOST_CHECK((!key_is_evaluable<kronecker_monomial<>,void *>::value));
}

struct subs_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		symbol_set vs;
		k_type k1;
		auto ret = k1.subs(symbol("x"),integer(4),vs);
		BOOST_CHECK_EQUAL(ret.first,1);
		BOOST_CHECK((std::is_same<integer,decltype(ret.first)>::value));
		BOOST_CHECK(ret.second == k1);
		k1 = k_type({T(1)});
		BOOST_CHECK_THROW(k1.subs(symbol("x"),integer(4),vs),std::invalid_argument);
		vs.add("x");
		k1 = k_type({T(2)});
		ret = k1.subs(symbol("y"),integer(4),vs);
		BOOST_CHECK_EQUAL(ret.first,1);
		BOOST_CHECK(ret.second == k1);
		ret = k1.subs(symbol("x"),integer(4),vs);
		BOOST_CHECK_EQUAL(ret.first,math::pow(integer(4),T(2)));
		BOOST_CHECK(ret.second == k_type{});
		k1 = k_type({T(2),T(3)});
		vs.add("y");
		ret = k1.subs(symbol("y"),integer(-2),vs);
		BOOST_CHECK_EQUAL(ret.first,math::pow(integer(-2),T(3)));
		BOOST_CHECK(ret.second == k_type{T(2)});
		auto ret2 = k1.subs(symbol("x"),real(-2.345),vs);
		BOOST_CHECK_EQUAL(ret2.first,math::pow(real(-2.345),T(2)));
		BOOST_CHECK(ret2.second == k_type{T(3)});
		BOOST_CHECK((std::is_same<real,decltype(ret2.first)>::value));
		auto ret3 = k1.subs(symbol("x"),rational(-1,2),vs);
		BOOST_CHECK_EQUAL(ret3.first,rational(1,4));
		BOOST_CHECK(ret3.second == k_type{T(3)});
		BOOST_CHECK((std::is_same<rational,decltype(ret3.first)>::value));
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_subs_test)
{
	boost::mpl::for_each<int_types>(subs_tester());
}

struct print_tex_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		symbol_set vs;
		k_type k1;
		std::ostringstream oss;
		k1.print_tex(oss,vs);
		BOOST_CHECK(oss.str().empty());
		k1 = k_type({T(1)});
		BOOST_CHECK_THROW(k1.print_tex(oss,vs),std::invalid_argument);
		k1 = k_type({T(0)});
		vs.add("x");
		k1.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"");
		k1 = k_type({T(1)});
		k1.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"{x}");
		oss.str("");
		k1 = k_type({T(-1)});
		k1.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"\\frac{1}{{x}}");
		oss.str("");
		k1 = k_type({T(2)});
		k1.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"{x}^{2}");
		oss.str("");
		k1 = k_type({T(-2)});
		k1.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"\\frac{1}{{x}^{2}}");
		vs.add("y");
		oss.str("");
		k1 = k_type({T(-2),T(1)});
		k1.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"\\frac{{y}}{{x}^{2}}");
		oss.str("");
		k1 = k_type({T(-2),T(3)});
		k1.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"\\frac{{y}^{3}}{{x}^{2}}");
		oss.str("");
		k1 = k_type({T(-2),T(-3)});
		k1.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"\\frac{1}{{x}^{2}{y}^{3}}");
		oss.str("");
		k1 = k_type({T(2),T(3)});
		k1.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"{x}^{2}{y}^{3}");
		oss.str("");
		k1 = k_type({T(1),T(3)});
		k1.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"{x}{y}^{3}");
		oss.str("");
		k1 = k_type({T(0),T(3)});
		k1.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"{y}^{3}");
		oss.str("");
		k1 = k_type({T(0),T(0)});
		k1.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"");
		oss.str("");
		k1 = k_type({T(0),T(1)});
		k1.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"{y}");
		oss.str("");
		k1 = k_type({T(0),T(-1)});
		k1.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"\\frac{1}{{y}}");
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_print_tex_test)
{
	boost::mpl::for_each<int_types>(print_tex_tester());
}

struct integrate_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		symbol_set vs;
		k_type k1;
		auto ret = k1.integrate(symbol("a"),vs);
		BOOST_CHECK_EQUAL(ret.first,T(1));
		BOOST_CHECK(ret.second == k_type({T(1)}));
		k1 = k_type{T(1)};
		BOOST_CHECK_THROW(k1.integrate(symbol("b"),vs),std::invalid_argument);
		vs.add("b");
		ret = k1.integrate(symbol("b"),vs);
		BOOST_CHECK_EQUAL(ret.first,T(2));
		BOOST_CHECK(ret.second == k_type({T(2)}));
		k1 = k_type{T(2)};
		ret = k1.integrate(symbol("c"),vs);
		BOOST_CHECK_EQUAL(ret.first,T(1));
		BOOST_CHECK(ret.second == k_type({T(2),T(1)}));
		ret = k1.integrate(symbol("a"),vs);
		BOOST_CHECK_EQUAL(ret.first,T(1));
		BOOST_CHECK(ret.second == k_type({T(1),T(2)}));
		k1 = k_type{T(0),T(1)};
		vs.add("d");
		ret = k1.integrate(symbol("a"),vs);
		BOOST_CHECK_EQUAL(ret.first,T(1));
		BOOST_CHECK(ret.second == k_type({T(1),T(0),T(1)}));
		ret = k1.integrate(symbol("b"),vs);
		BOOST_CHECK_EQUAL(ret.first,T(1));
		BOOST_CHECK(ret.second == k_type({T(1),T(1)}));
		ret = k1.integrate(symbol("c"),vs);
		BOOST_CHECK_EQUAL(ret.first,T(1));
		BOOST_CHECK(ret.second == k_type({T(0),T(1),T(1)}));
		ret = k1.integrate(symbol("d"),vs);
		BOOST_CHECK_EQUAL(ret.first,T(2));
		BOOST_CHECK(ret.second == k_type({T(0),T(2)}));
		ret = k1.integrate(symbol("e"),vs);
		BOOST_CHECK_EQUAL(ret.first,T(1));
		BOOST_CHECK(ret.second == k_type({T(0),T(1),T(1)}));
		k1 = k_type{T(-1),T(0)};
		BOOST_CHECK_THROW(k1.integrate(symbol("b"),vs),std::invalid_argument);
		k1 = k_type{T(0),T(-1)};
		BOOST_CHECK_THROW(k1.integrate(symbol("d"),vs),std::invalid_argument);
		// Check limits violation.
		typedef kronecker_array<T> ka;
		const auto &limits = ka::get_limits();
		k1 = k_type{std::get<0u>(limits[2u])[0u],std::get<0u>(limits[2u])[0u]};
		BOOST_CHECK_THROW(ret = k1.integrate(symbol("b"),vs),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_integrate_test)
{
	boost::mpl::for_each<int_types>(integrate_tester());
}

struct trim_identify_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		k_type k0;
		symbol_set v1, v2;
		k0.set_int(1);
		BOOST_CHECK_THROW(k0.trim_identify(v2,v2),std::invalid_argument);
		v1.add("x");
		v2.add("y");
		v2.add("x");
		k0 = k_type({T(1),T(2)});
		k0.trim_identify(v1,v2);
		BOOST_CHECK(v1 == symbol_set());
		k0 = k_type({T(0),T(2)});
		v1.add("x");
		v1.add("y");
		k0.trim_identify(v1,v2);
		BOOST_CHECK(v1 == symbol_set({symbol("x")}));
		k0 = k_type({T(0),T(0)});
		v1.add("y");
		k0.trim_identify(v1,v2);
		BOOST_CHECK(v1 == symbol_set({symbol("x"),symbol("y")}));
		k0 = k_type({T(1),T(0)});
		k0.trim_identify(v1,v2);
		BOOST_CHECK(v1 == symbol_set({symbol("y")}));
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_trim_identify_test)
{
	boost::mpl::for_each<int_types>(trim_identify_tester());
}

struct trim_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		k_type k0;
		symbol_set v1, v2;
		k0.set_int(1);
		BOOST_CHECK_THROW(k0.trim(v1,v2),std::invalid_argument);
		v1.add("x");
		v1.add("y");
		v1.add("z");
		k0 = k_type{T(1),T(0),T(-1)};
		v2.add("x");
		BOOST_CHECK((k0.trim(v2,v1) == k_type{T(0),T(-1)}));
		v2.add("z");
		v2.add("a");
		BOOST_CHECK((k0.trim(v2,v1) == k_type{T(0)}));
		v2.add("y");
		BOOST_CHECK((k0.trim(v2,v1) == k_type{}));
		v2 = symbol_set();
		BOOST_CHECK((k0.trim(v2,v1) == k0));
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_trim_test)
{
	boost::mpl::for_each<int_types>(trim_tester());
}

struct ipow_subs_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		symbol_set vs;
		k_type k1;
		auto ret = k1.ipow_subs(symbol("x"),integer(45),integer(4),vs);
		BOOST_CHECK_EQUAL(ret.first,1);
		BOOST_CHECK((std::is_same<integer,decltype(ret.first)>::value));
		BOOST_CHECK(ret.second == k1);
		k1.set_int(1);
		BOOST_CHECK_THROW(k1.ipow_subs(symbol("x"),integer(35),integer(4),vs),std::invalid_argument);
		vs.add("x");
		k1 = k_type({T(2)});
		ret = k1.ipow_subs(symbol("y"),integer(2),integer(4),vs);
		BOOST_CHECK_EQUAL(ret.first,1);
		BOOST_CHECK(ret.second == k1);
		ret = k1.ipow_subs(symbol("x"),integer(1),integer(4),vs);
		BOOST_CHECK_EQUAL(ret.first,math::pow(integer(4),T(2)));
		BOOST_CHECK(ret.second == k_type({T(0)}));
		ret = k1.ipow_subs(symbol("x"),integer(2),integer(4),vs);
		BOOST_CHECK_EQUAL(ret.first,math::pow(integer(4),T(1)));
		BOOST_CHECK(ret.second == k_type({T(0)}));
		ret = k1.ipow_subs(symbol("x"),integer(-1),integer(4),vs);
		BOOST_CHECK_EQUAL(ret.first,1);
		BOOST_CHECK(ret.second == k_type({T(2)}));
		ret = k1.ipow_subs(symbol("x"),integer(4),integer(4),vs);
		BOOST_CHECK_EQUAL(ret.first,1);
		BOOST_CHECK(ret.second == k_type({T(2)}));
		if (std::is_same<T,signed char>::value) {
			// Values below are too large for signed char.
			return;
		}
		k1 = k_type({T(7),T(2)});
		vs.add("y");
		ret = k1.ipow_subs(symbol("x"),integer(3),integer(2),vs);
		BOOST_CHECK_EQUAL(ret.first,math::pow(integer(2),T(2)));
		BOOST_CHECK((ret.second == k_type{T(1),T(2)}));
		ret = k1.ipow_subs(symbol("x"),integer(4),integer(2),vs);
		BOOST_CHECK_EQUAL(ret.first,math::pow(integer(2),T(1)));
		BOOST_CHECK((ret.second == k_type{T(3),T(2)}));
		ret = k1.ipow_subs(symbol("x"),integer(-4),integer(2),vs);
		BOOST_CHECK_EQUAL(ret.first,1);
		BOOST_CHECK((ret.second == k_type{T(7),T(2)}));
		k1 = k_type({T(-7),T(2)});
		ret = k1.ipow_subs(symbol("x"),integer(4),integer(2),vs);
		BOOST_CHECK_EQUAL(ret.first,1);
		BOOST_CHECK((ret.second == k_type{T(-7),T(2)}));
		ret = k1.ipow_subs(symbol("x"),integer(-4),integer(2),vs);
		BOOST_CHECK_EQUAL(ret.first,math::pow(integer(2),T(1)));
		BOOST_CHECK((ret.second == k_type{T(-3),T(2)}));
		ret = k1.ipow_subs(symbol("x"),integer(-3),integer(2),vs);
		BOOST_CHECK_EQUAL(ret.first,math::pow(integer(2),T(2)));
		BOOST_CHECK((ret.second == k_type{T(-1),T(2)}));
		k1 = k_type({T(2),T(-7)});
		ret = k1.ipow_subs(symbol("y"),integer(-3),integer(2),vs);
		BOOST_CHECK_EQUAL(ret.first,math::pow(integer(2),T(2)));
		BOOST_CHECK((ret.second == k_type{T(2),T(-1)}));
		BOOST_CHECK_THROW(k1.ipow_subs(symbol("y"),integer(0),integer(2),vs),zero_division_error);
		k1 = k_type({T(-7),T(2)});
		auto ret2 = k1.ipow_subs(symbol("x"),integer(-4),real(-2.345),vs);
		BOOST_CHECK_EQUAL(ret2.first,math::pow(real(-2.345),T(1)));
		BOOST_CHECK((ret2.second == k_type{T(-3),T(2)}));
		BOOST_CHECK((std::is_same<real,decltype(ret2.first)>::value));
		auto ret3 = k1.ipow_subs(symbol("x"),integer(-3),rational(-1,2),vs);
		BOOST_CHECK_EQUAL(ret3.first,math::pow(rational(-1,2),T(2)));
		BOOST_CHECK((ret3.second == k_type{T(-1),T(2)}));
		BOOST_CHECK((std::is_same<rational,decltype(ret3.first)>::value));
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_ipow_subs_test)
{
	boost::mpl::for_each<int_types>(ipow_subs_tester());
}

struct tt_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef kronecker_monomial<T> k_type;
		BOOST_CHECK((!key_has_t_subs<k_type,int,int>::value));
		BOOST_CHECK((!key_has_t_subs<k_type &,int,int>::value));
		BOOST_CHECK((!key_has_t_subs<k_type const &,int,int>::value));
		BOOST_CHECK(is_hashable<k_type>::value);
		BOOST_CHECK(key_has_degree<k_type>::value);
		BOOST_CHECK(key_has_ldegree<k_type>::value);
		BOOST_CHECK(!key_has_t_degree<k_type>::value);
		BOOST_CHECK(!key_has_t_ldegree<k_type>::value);
		BOOST_CHECK(!key_has_t_order<k_type>::value);
		BOOST_CHECK(!key_has_t_lorder<k_type>::value);
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_type_traits_test)
{
	boost::mpl::for_each<int_types>(tt_tester());
}

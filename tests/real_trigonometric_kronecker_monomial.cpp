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

#include "../src/real_trigonometric_kronecker_monomial.hpp"

#define BOOST_TEST_MODULE real_trigonometric_kronecker_monomial_test
#include <boost/test/unit_test.hpp>

#include <boost/integer_traits.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "../src/environment.hpp"
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
		typedef real_trigonometric_kronecker_monomial<T> k_type;
		typedef kronecker_array<T> ka;
		k_type k1;
		BOOST_CHECK_EQUAL(k1.get_int(),0);
		BOOST_CHECK_EQUAL(k1.get_flavour(),true);
		k_type k2({-1,-1});
		std::vector<T> v2(2);
		ka::decode(v2,k2.get_int());
		BOOST_CHECK_EQUAL(v2[0],-1);
		BOOST_CHECK_EQUAL(v2[1],-1);
		BOOST_CHECK_EQUAL(k2.get_flavour(),true);
		k_type k3;
		BOOST_CHECK_EQUAL(k3.get_int(),0);
		BOOST_CHECK_EQUAL(k3.get_flavour(),true);
		k_type k4({10});
		BOOST_CHECK_EQUAL(k4.get_int(),10);
		BOOST_CHECK(k4.get_flavour());
		k_type k5(symbol_set({}));
		BOOST_CHECK_EQUAL(k5.get_int(),0);
		BOOST_CHECK(k5.get_flavour());
		k_type k6(symbol_set({symbol("a")}));
		BOOST_CHECK_EQUAL(k6.get_int(),0);
		BOOST_CHECK_EQUAL(k6.get_flavour(),true);
		k_type k7(symbol_set({symbol("a"),symbol("b")}));
		BOOST_CHECK_EQUAL(k7.get_int(),0);
		BOOST_CHECK(k7.get_flavour());
		k_type k8(0,true);
		BOOST_CHECK_EQUAL(k8.get_int(),0);
		BOOST_CHECK(k8.get_flavour());
		k_type k9(1,true);
		BOOST_CHECK_EQUAL(k9.get_int(),1);
		BOOST_CHECK(k9.get_flavour());
		BOOST_CHECK_EQUAL((k_type(1,false).get_int()),1);
		BOOST_CHECK(!(k_type(1,false).get_flavour()));
		k_type k10;
		k10.set_int(10);
		BOOST_CHECK_EQUAL(k10.get_int(),10);
		BOOST_CHECK(k10.get_flavour());
		k10.set_flavour(false);
		BOOST_CHECK(!k10.get_flavour());
		k_type k11;
		BOOST_CHECK(k11.get_flavour());
		k11 = k10;
		BOOST_CHECK_EQUAL(k11.get_int(),10);
		BOOST_CHECK(!k11.get_flavour());
		k11 = std::move(k9);
		BOOST_CHECK_EQUAL(k9.get_int(),1);
		BOOST_CHECK(k11.get_flavour());
		// Constructor from iterators.
		v2 = {};
		k_type k12(v2.begin(),v2.end());
		BOOST_CHECK_EQUAL(k12.get_int(),0);
		BOOST_CHECK_EQUAL(k12.get_flavour(),true);
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
		BOOST_CHECK(k17.get_flavour());
		BOOST_CHECK(!(k_type(k_type(0,false),symbol_set{}).get_flavour()));
		//BOOST_CHECK(k16 == k17);
		k16.set_int(10);
		k_type k18(k16,symbol_set({symbol("a")}));
		//BOOST_CHECK(k16 == k18);
		BOOST_CHECK_THROW((k_type(k16,symbol_set({}))),std::invalid_argument);
		// First element negative.
		k16 = k_type{-1,0};
		symbol_set tmp_ss{symbol("a"),symbol("b")};
		BOOST_CHECK_THROW((k_type(k16,tmp_ss)),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(rtkm_constructor_test)
{
	environment env;
	boost::mpl::for_each<int_types>(constructor_tester());
}

struct compatibility_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
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
		k1.set_int(1);
		BOOST_CHECK(k1.is_compatible(symbol_set({symbol("a"),symbol("b")})));
		// Negative first element.
		k1 = k_type{-1,0};
		BOOST_CHECK(!k1.is_compatible(symbol_set({symbol("a"),symbol("b")})));
		// Negative first nonzero element.
		k1 = k_type{0,-1};
		BOOST_CHECK(!k1.is_compatible(symbol_set({symbol("a"),symbol("b")})));
		k1 = k_type{1,0};
		BOOST_CHECK(k1.is_compatible(symbol_set({symbol("a"),symbol("b")})));
		k1 = k_type{0,1};
		BOOST_CHECK(k1.is_compatible(symbol_set({symbol("a"),symbol("b")})));
		k1 = k_type{1,-1};
		BOOST_CHECK(k1.is_compatible(symbol_set({symbol("a"),symbol("b")})));
		k1 = k_type{0,0};
		BOOST_CHECK(k1.is_compatible(symbol_set({symbol("a"),symbol("b")})));
	}
};

BOOST_AUTO_TEST_CASE(rtkm_compatibility_test)
{
	boost::mpl::for_each<int_types>(compatibility_tester());
}

struct ignorability_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
		BOOST_CHECK(!k_type().is_ignorable(symbol_set{}));
		BOOST_CHECK(!k_type(symbol_set{symbol("a")}).is_ignorable(symbol_set{}));
		BOOST_CHECK(!(k_type{0,0}.is_ignorable(symbol_set{})));
		BOOST_CHECK(!(k_type(1,false).is_ignorable(symbol_set{symbol("a")})));
		BOOST_CHECK((k_type(0,false).is_ignorable(symbol_set{symbol("a")})));
		k_type k{0,-1};
		k.set_flavour(false);
		BOOST_CHECK(!k.is_ignorable(symbol_set{}));
	}
};

BOOST_AUTO_TEST_CASE(rtkm_ignorability_test)
{
	boost::mpl::for_each<int_types>(ignorability_tester());
}

struct merge_args_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
		typedef kronecker_array<T> ka;
		k_type k1;
		symbol_set vs1({symbol("a")}), empty;
		BOOST_CHECK(k1.merge_args(empty,vs1).get_int() == 0);
		BOOST_CHECK(k1.merge_args(empty,vs1).get_flavour());
		std::vector<T> v1(1);
		ka::decode(v1,k1.merge_args(empty,vs1).get_int());
		BOOST_CHECK(v1[0] == 0);
		auto vs2 = vs1;
		vs2.add(symbol("b"));
		k_type k2({-1});
		BOOST_CHECK(k2.merge_args(vs1,vs2).get_int() == ka::encode(std::vector<int>({-1,0})));
		BOOST_CHECK(k2.merge_args(vs1,vs2).get_flavour());
		vs1.add(symbol("c"));
		vs2.add(symbol("c"));
		vs2.add(symbol("d"));
		k_type k3({-1,-1});
		k3.set_flavour(false);
		BOOST_CHECK(k3.merge_args(vs1,vs2).get_int() == ka::encode(std::vector<int>({-1,0,-1,0})));
		BOOST_CHECK(!(k3.merge_args(vs1,vs2).get_flavour()));
		vs1 = symbol_set({symbol("c")});
		k_type k4({-1});
		BOOST_CHECK(k4.merge_args(vs1,vs2).get_int() == ka::encode(std::vector<int>({0,0,-1,0})));
		vs1 = symbol_set({});
		k_type k5;
		k5.set_flavour(false);
		BOOST_CHECK(k5.merge_args(vs1,vs2).get_int() == ka::encode(std::vector<int>({0,0,0,0})));
		BOOST_CHECK(!(k5.merge_args(vs1,vs2).get_flavour()));
		vs1.add(symbol("e"));
		BOOST_CHECK_THROW(k5.merge_args(vs1,vs2),std::invalid_argument);
		BOOST_CHECK_THROW(k5.merge_args(vs2,vs1),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(rtkm_merge_args_test)
{
	boost::mpl::for_each<int_types>(merge_args_tester());
}

struct is_unitary_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
		typedef kronecker_array<T> ka;
		k_type k1;
		symbol_set vs1;
		BOOST_CHECK(k1.is_unitary(vs1));
		k_type k2({1});
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
		k2 = k_type{-1};
		vs2 = symbol_set({symbol("a")});
		BOOST_CHECK_THROW(k2.is_unitary(vs2),std::invalid_argument);
		k2 = k_type{0};
		k2.set_flavour(false);
		BOOST_CHECK(!k2.is_unitary(vs2));
		k2.set_flavour(true);
		BOOST_CHECK(k2.is_unitary(vs2));
		k2 = k_type{1,1};
		BOOST_CHECK(!k2.is_unitary(vs2));
	}
};

BOOST_AUTO_TEST_CASE(rtkm_is_unitary_test)
{
	boost::mpl::for_each<int_types>(is_unitary_tester());
}

struct t_degree_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
		k_type k1;
		symbol_set vs1;
		BOOST_CHECK(k1.t_degree(vs1) == 0);
		BOOST_CHECK(k1.t_ldegree(vs1) == 0);
		k_type k2({0});
		vs1.add(symbol("a"));
		BOOST_CHECK(k2.t_degree(vs1) == 0);
		BOOST_CHECK(k2.t_ldegree(vs1) == 0);
		k_type k3({-1});
		BOOST_CHECK(k3.t_degree(vs1) == -1);
		BOOST_CHECK(k3.t_ldegree(vs1) == -1);
		vs1.add(symbol("b"));
		k_type k4({0,0});
		BOOST_CHECK(k4.t_degree(vs1) == 0);
		BOOST_CHECK(k4.t_ldegree(vs1) == 0);
		k_type k5({-1,-1});
		BOOST_CHECK(k5.t_degree(vs1) == -2);
		BOOST_CHECK(k5.t_degree({"a"},vs1) == -1);
		BOOST_CHECK(k5.t_degree(std::set<std::string>{},vs1) == 0);
		BOOST_CHECK(k5.t_degree({"f"},vs1) == 0);
		BOOST_CHECK(k5.t_degree({"a","b"},vs1) == -2);
		BOOST_CHECK(k5.t_degree({"a","c"},vs1) == -1);
		BOOST_CHECK(k5.t_degree({"d","c"},vs1) == 0);
		BOOST_CHECK(k5.t_degree({"d","b"},vs1) == -1);
		BOOST_CHECK(k5.t_degree({"A","a"},vs1) == -1);
		BOOST_CHECK(k5.t_ldegree(vs1) == -2);
		BOOST_CHECK(k5.t_ldegree({"a"},vs1) == -1);
		BOOST_CHECK(k5.t_ldegree(std::set<std::string>{},vs1) == 0);
		BOOST_CHECK(k5.t_ldegree({"f"},vs1) == 0);
		BOOST_CHECK(k5.t_ldegree({"a","b"},vs1) == -2);
		BOOST_CHECK(k5.t_ldegree({"a","c"},vs1) == -1);
		BOOST_CHECK(k5.t_ldegree({"d","c"},vs1) == 0);
		BOOST_CHECK(k5.t_ldegree({"d","b"},vs1) == -1);
		BOOST_CHECK(k5.t_ldegree({"A","a"},vs1) == -1);
	}
};

BOOST_AUTO_TEST_CASE(rtkm_t_degree_test)
{
	boost::mpl::for_each<int_types>(t_degree_tester());
}

struct t_order_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
		k_type k1;
		symbol_set vs1;
		BOOST_CHECK(k1.t_order(vs1) == 0);
		BOOST_CHECK(k1.t_lorder(vs1) == 0);
		k_type k2({0});
		vs1.add(symbol("a"));
		BOOST_CHECK(k2.t_order(vs1) == 0);
		BOOST_CHECK(k2.t_lorder(vs1) == 0);
		k_type k3({-1});
		BOOST_CHECK(k3.t_order(vs1) == 1);
		BOOST_CHECK(k3.t_lorder(vs1) == 1);
		vs1.add(symbol("b"));
		k_type k4({0,0});
		BOOST_CHECK(k4.t_order(vs1) == 0);
		BOOST_CHECK(k4.t_lorder(vs1) == 0);
		k_type k5({-1,-1});
		BOOST_CHECK(k5.t_order(vs1) == 2);
		BOOST_CHECK(k5.t_order({"a"},vs1) == 1);
		BOOST_CHECK(k5.t_order(std::set<std::string>{},vs1) == 0);
		BOOST_CHECK(k5.t_order({"f"},vs1) == 0);
		BOOST_CHECK(k5.t_order({"a","b"},vs1) == 2);
		BOOST_CHECK(k5.t_order({"a","c"},vs1) == 1);
		BOOST_CHECK(k5.t_order({"d","c"},vs1) == 0);
		BOOST_CHECK(k5.t_order({"d","b"},vs1) == 1);
		BOOST_CHECK(k5.t_order({"A","a"},vs1) == 1);
		BOOST_CHECK(k5.t_lorder(vs1) == 2);
		BOOST_CHECK(k5.t_lorder({"a"},vs1) == 1);
		BOOST_CHECK(k5.t_lorder(std::set<std::string>{},vs1) == 0);
		BOOST_CHECK(k5.t_lorder({"f"},vs1) == 0);
		BOOST_CHECK(k5.t_lorder({"a","b"},vs1) == 2);
		BOOST_CHECK(k5.t_lorder({"a","c"},vs1) == 1);
		BOOST_CHECK(k5.t_lorder({"d","c"},vs1) == 0);
		BOOST_CHECK(k5.t_lorder({"d","b"},vs1) == 1);
		BOOST_CHECK(k5.t_lorder({"A","a"},vs1) == 1);
		k5 = k_type({-1,1});
		BOOST_CHECK(k5.t_order(vs1) == 2);
		BOOST_CHECK(k5.t_order({"a"},vs1) == 1);
		BOOST_CHECK(k5.t_order(std::set<std::string>{},vs1) == 0);
		BOOST_CHECK(k5.t_order({"f"},vs1) == 0);
		BOOST_CHECK(k5.t_order({"a","b"},vs1) == 2);
		BOOST_CHECK(k5.t_order({"a","c"},vs1) == 1);
		BOOST_CHECK(k5.t_order({"d","c"},vs1) == 0);
		BOOST_CHECK(k5.t_order({"d","b"},vs1) == 1);
		BOOST_CHECK(k5.t_order({"A","a"},vs1) == 1);
		BOOST_CHECK(k5.t_lorder(vs1) == 2);
		BOOST_CHECK(k5.t_lorder({"a"},vs1) == 1);
		BOOST_CHECK(k5.t_lorder(std::set<std::string>{},vs1) == 0);
		BOOST_CHECK(k5.t_lorder({"f"},vs1) == 0);
		BOOST_CHECK(k5.t_lorder({"a","b"},vs1) == 2);
		BOOST_CHECK(k5.t_lorder({"a","c"},vs1) == 1);
		BOOST_CHECK(k5.t_lorder({"d","c"},vs1) == 0);
		BOOST_CHECK(k5.t_lorder({"d","b"},vs1) == 1);
		BOOST_CHECK(k5.t_lorder({"A","a"},vs1) == 1);
		k5 = k_type({1,-1});
		BOOST_CHECK(k5.t_order(vs1) == 2);
		BOOST_CHECK(k5.t_order({"a"},vs1) == 1);
		BOOST_CHECK(k5.t_order(std::set<std::string>{},vs1) == 0);
		BOOST_CHECK(k5.t_order({"f"},vs1) == 0);
		BOOST_CHECK(k5.t_order({"a","b"},vs1) == 2);
		BOOST_CHECK(k5.t_order({"a","c"},vs1) == 1);
		BOOST_CHECK(k5.t_order({"d","c"},vs1) == 0);
		BOOST_CHECK(k5.t_order({"d","b"},vs1) == 1);
		BOOST_CHECK(k5.t_order({"A","a"},vs1) == 1);
		BOOST_CHECK(k5.t_lorder(vs1) == 2);
		BOOST_CHECK(k5.t_lorder({"a"},vs1) == 1);
		BOOST_CHECK(k5.t_lorder(std::set<std::string>{},vs1) == 0);
		BOOST_CHECK(k5.t_lorder({"f"},vs1) == 0);
		BOOST_CHECK(k5.t_lorder({"a","b"},vs1) == 2);
		BOOST_CHECK(k5.t_lorder({"a","c"},vs1) == 1);
		BOOST_CHECK(k5.t_lorder({"d","c"},vs1) == 0);
		BOOST_CHECK(k5.t_lorder({"d","b"},vs1) == 1);
		BOOST_CHECK(k5.t_lorder({"A","a"},vs1) == 1);
	}
};

BOOST_AUTO_TEST_CASE(rtkm_t_order_test)
{
	boost::mpl::for_each<int_types>(t_order_tester());
}

struct multiply_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
		typedef kronecker_array<T> ka;
		k_type k1, k2, result_plus, result_minus;
		symbol_set vs1;
		bool sign_plus = true, sign_minus = true;
		k1.multiply(result_plus,result_minus,k2,sign_plus,sign_minus,vs1);
		BOOST_CHECK(result_plus.get_int() == 0);
		BOOST_CHECK(result_minus.get_int() == 0);
		BOOST_CHECK(result_plus.get_flavour() == true);
		BOOST_CHECK(result_minus.get_flavour() == true);
		BOOST_CHECK(!sign_plus && !sign_minus);
		k1 = k_type({0});
		k2 = k_type({0});
		vs1.add(symbol("a"));
		k1.multiply(result_plus,result_minus,k2,sign_plus,sign_minus,vs1);
		BOOST_CHECK(result_plus.get_int() == 0);
		BOOST_CHECK(result_minus.get_int() == 0);
		BOOST_CHECK(result_plus.get_flavour() == true);
		BOOST_CHECK(result_minus.get_flavour() == true);
		BOOST_CHECK(!sign_plus && !sign_minus);
		k1 = k_type({1});
		k2 = k_type({2});
		k1.multiply(result_plus,result_minus,k2,sign_plus,sign_minus,vs1);
		BOOST_CHECK(result_plus.get_int() == 3);
		BOOST_CHECK(result_minus.get_int() == 1);
		BOOST_CHECK(result_plus.get_flavour() == true);
		BOOST_CHECK(result_minus.get_flavour() == true);
		BOOST_CHECK(!sign_plus && sign_minus);
		k1 = k_type({1,-1});
		k2 = k_type({2,0});
		vs1.add(symbol("b"));
		k1.multiply(result_plus,result_minus,k2,sign_plus,sign_minus,vs1);
		BOOST_CHECK(result_plus.get_flavour() == true);
		BOOST_CHECK(result_minus.get_flavour() == true);
		std::vector<int> tmp(2u);
		ka::decode(tmp,result_plus.get_int());
		BOOST_CHECK(tmp[0u] == 3);
		BOOST_CHECK(tmp[1u] == -1);
		ka::decode(tmp,result_minus.get_int());
		BOOST_CHECK(tmp[0u] == 1);
		BOOST_CHECK(tmp[1u] == 1);
		BOOST_CHECK(!sign_plus && sign_minus);
		k1.set_flavour(false);
		k1.multiply(result_plus,result_minus,k2,sign_plus,sign_minus,vs1);
		BOOST_CHECK(result_plus.get_flavour() == false);
		BOOST_CHECK(result_minus.get_flavour() == false);
		ka::decode(tmp,result_plus.get_int());
		BOOST_CHECK(tmp[0u] == 3);
		BOOST_CHECK(tmp[1u] == -1);
		ka::decode(tmp,result_minus.get_int());
		BOOST_CHECK(tmp[0u] == 1);
		BOOST_CHECK(tmp[1u] == 1);
		BOOST_CHECK(!sign_plus && sign_minus);
		k1.set_flavour(true);
		k2.set_flavour(false);
		k1.multiply(result_plus,result_minus,k2,sign_plus,sign_minus,vs1);
		BOOST_CHECK(result_plus.get_flavour() == false);
		BOOST_CHECK(result_minus.get_flavour() == false);
		ka::decode(tmp,result_plus.get_int());
		BOOST_CHECK(tmp[0u] == 3);
		BOOST_CHECK(tmp[1u] == -1);
		ka::decode(tmp,result_minus.get_int());
		BOOST_CHECK(tmp[0u] == 1);
		BOOST_CHECK(tmp[1u] == 1);
		BOOST_CHECK(!sign_plus && sign_minus);
		k1.set_flavour(false);
		k1.multiply(result_plus,result_minus,k2,sign_plus,sign_minus,vs1);
		BOOST_CHECK(result_plus.get_flavour() == true);
		BOOST_CHECK(result_minus.get_flavour() == true);
		ka::decode(tmp,result_plus.get_int());
		BOOST_CHECK(tmp[0u] == 3);
		BOOST_CHECK(tmp[1u] == -1);
		ka::decode(tmp,result_minus.get_int());
		BOOST_CHECK(tmp[0u] == 1);
		BOOST_CHECK(tmp[1u] == 1);
		BOOST_CHECK(!sign_plus && sign_minus);
		k1 = k_type({1,-1});
		k2 = k_type({-2,-2});
		k1.multiply(result_plus,result_minus,k2,sign_plus,sign_minus,vs1);
		BOOST_CHECK(result_plus.get_flavour() == true);
		BOOST_CHECK(result_minus.get_flavour() == true);
		ka::decode(tmp,result_plus.get_int());
		BOOST_CHECK(tmp[0u] == 1);
		BOOST_CHECK(tmp[1u] == 3);
		ka::decode(tmp,result_minus.get_int());
		BOOST_CHECK(tmp[0u] == 3);
		BOOST_CHECK(tmp[1u] == 1);
		BOOST_CHECK(sign_plus && !sign_minus);
		// Multiplication that produces first multiplier zero, second negative, in the plus.
		k1 = k_type({1,-1});
		k2 = k_type({-1,-2});
		k1.multiply(result_plus,result_minus,k2,sign_plus,sign_minus,vs1);
		BOOST_CHECK(sign_plus && !sign_minus);
		BOOST_CHECK(result_plus.get_flavour() == true);
		BOOST_CHECK(result_minus.get_flavour() == true);
		ka::decode(tmp,result_plus.get_int());
		BOOST_CHECK(tmp[0u] == 0);
		BOOST_CHECK_EQUAL(tmp[1u],3);
		ka::decode(tmp,result_minus.get_int());
		BOOST_CHECK(tmp[0u] == 2);
		BOOST_CHECK_EQUAL(tmp[1u],1);
		// Multiplication that produces first multiplier zero, second negative, in the minus.
		k1 = k_type({1,-2});
		k2 = k_type({1,-1});
		k1.multiply(result_plus,result_minus,k2,sign_plus,sign_minus,vs1);
		BOOST_CHECK(!sign_plus && sign_minus);
		BOOST_CHECK(result_plus.get_flavour() == true);
		BOOST_CHECK(result_minus.get_flavour() == true);
		ka::decode(tmp,result_plus.get_int());
		BOOST_CHECK(tmp[0u] == 2);
		BOOST_CHECK_EQUAL(tmp[1u],-3);
		ka::decode(tmp,result_minus.get_int());
		BOOST_CHECK(tmp[0u] == 0);
		BOOST_CHECK_EQUAL(tmp[1u],1);
	}
};

BOOST_AUTO_TEST_CASE(rtkm_multiply_test)
{
	boost::mpl::for_each<int_types>(multiply_tester());
}

struct equality_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
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
		k1 = k_type{1,2};
		k2 = k_type{1,2};
		k2.set_flavour(false);
		BOOST_CHECK(k1 != k2);
		BOOST_CHECK(!(k1 == k2));
		k1.set_flavour(false);
		BOOST_CHECK(k1 == k2);
		BOOST_CHECK(!(k1 != k2));
	}
};

BOOST_AUTO_TEST_CASE(rtkm_equality_test)
{
	boost::mpl::for_each<int_types>(equality_tester());
}

struct hash_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
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

BOOST_AUTO_TEST_CASE(rtkm_hash_test)
{
	boost::mpl::for_each<int_types>(hash_tester());
}

struct unpack_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
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

BOOST_AUTO_TEST_CASE(rtkm_unpack_test)
{
	boost::mpl::for_each<int_types>(unpack_tester());
}

struct print_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
		symbol_set vs;
		k_type k1;
		std::ostringstream oss;
		k1.print(oss,vs);
		BOOST_CHECK(oss.str().empty());
		vs.add("x");
		k_type k2(vs);
		k2.print(oss,vs);
		BOOST_CHECK(oss.str().empty());
		k_type k3({T(1)});
		k3.print(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"cos(x)");
		k3.set_flavour(false);
		oss.str("");
		k3.print(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"sin(x)");
		k_type k5({T(1),T(-1)});
		vs.add("y");
		oss.str("");
		k5.print(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"cos(x-y)");
		oss.str("");
		k5 = k_type{T(1),T(1)};
		k5.print(oss,vs);
		BOOST_CHECK(oss.str() == "cos(x+y)");
		oss.str("");
		k5 = k_type{T(1),T(2)};
		k5.set_flavour(false);
		k5.print(oss,vs);
		BOOST_CHECK(oss.str() == "sin(x+2*y)");
		oss.str("");
		k5 = k_type{T(1),T(-2)};
		k5.print(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"cos(x-2*y)");
		oss.str("");
		k5 = k_type{T(-1),T(-2)};
		k5.print(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"cos(-x-2*y)");
		oss.str("");
		k5 = k_type{T(-2),T(1)};
		k5.print(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"cos(-2*x+y)");
		oss.str("");
		// Representation bug: would display cos(+y).
		k5 = k_type{T(0),T(1)};
		k5.print(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"cos(y)");
		oss.str("");
		k5 = k_type{T(0),T(-1)};
		k5.print(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"cos(-y)");
	}
};

BOOST_AUTO_TEST_CASE(rtkm_print_test)
{
	boost::mpl::for_each<int_types>(print_tester());
}

struct partial_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
		typedef kronecker_array<T> ka;
		const auto &limits = ka::get_limits();
		symbol_set vs;
		k_type k1{T(1)};
		BOOST_CHECK_THROW(k1.partial(symbol("x"),vs),std::invalid_argument);
		if (std::get<0u>(limits[1u])[0u] < boost::integer_traits<T>::const_max) {
			k1.set_int(boost::integer_traits<T>::const_max);
			BOOST_CHECK_THROW(k1.partial(symbol("x"),vs),std::invalid_argument);
		}
		vs.add("x");
		vs.add("y");
		k1 = k_type{T(1),T(2)};
		auto ret = k1.partial(symbol("x"),vs);
		BOOST_CHECK_EQUAL(ret.first,-1);
		BOOST_CHECK_EQUAL(ret.second.get_flavour(),false);
		BOOST_CHECK_EQUAL(ret.second.get_int(),k1.get_int());
		k1.set_flavour(false);
		ret = k1.partial(symbol("y"),vs);
		BOOST_CHECK_EQUAL(ret.first,2);
		BOOST_CHECK_EQUAL(ret.second.get_flavour(),true);
		BOOST_CHECK_EQUAL(ret.second.get_int(),k1.get_int());
		k1 = k_type{T(0),T(2)};
		ret = k1.partial(symbol("x"),vs);
		BOOST_CHECK_EQUAL(ret.first,0);
		BOOST_CHECK_EQUAL(ret.second.get_flavour(),true);
		BOOST_CHECK_EQUAL(ret.second.get_int(),0);
		k1 = k_type{T(1),T(2)};
		ret = k1.partial(symbol("z"),vs);
		BOOST_CHECK_EQUAL(ret.first,0);
		BOOST_CHECK_EQUAL(ret.second.get_flavour(),true);
		BOOST_CHECK_EQUAL(ret.second.get_int(),0);
		k1 = k_type{T(1),T(2)};
		ret = k1.partial(symbol("y"),vs);
		BOOST_CHECK_EQUAL(ret.first,-2);
		BOOST_CHECK_EQUAL(ret.second.get_flavour(),false);
		BOOST_CHECK_EQUAL(ret.second.get_int(),k1.get_int());
	}
};

BOOST_AUTO_TEST_CASE(rtkm_partial_test)
{
	boost::mpl::for_each<int_types>(partial_tester());
}

struct evaluate_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
		typedef std::unordered_map<symbol,integer> dict_type;
		symbol_set vs;
		k_type k1;
		BOOST_CHECK_EQUAL(k1.evaluate(dict_type{},vs),integer(1));
		k1.set_flavour(false);
		BOOST_CHECK_EQUAL(k1.evaluate(dict_type{},vs),integer(0));
		k1.set_flavour(true);
		vs.add("x");
		BOOST_CHECK_THROW(k1.evaluate(dict_type{},vs),std::invalid_argument);
		k1 = k_type({T(1)});
		BOOST_CHECK_THROW(k1.evaluate(dict_type{},vs),std::invalid_argument);
		BOOST_CHECK_EQUAL(k1.evaluate(dict_type{{symbol("x"),integer(0)}},vs),1);
		BOOST_CHECK_EQUAL(k1.evaluate(dict_type{{symbol("x"),integer(1)}},vs),std::cos(1));
		BOOST_CHECK((std::is_same<double,decltype(k1.evaluate(dict_type{{symbol("x"),integer(1)}},vs))>::value));
		BOOST_CHECK((std::is_same<real,decltype(k1.evaluate(std::unordered_map<symbol,real>{{symbol("x"),real(1)}},vs))>::value));
		BOOST_CHECK((std::is_same<double,decltype(k1.evaluate(std::unordered_map<symbol,rational>{{symbol("x"),rational(1)}},vs))>::value));
		k1.set_flavour(false);
		BOOST_CHECK_EQUAL(k1.evaluate(dict_type{{symbol("x"),integer(0)}},vs),0);
		k1 = k_type({T(2),T(-3)});
		vs.add("y");
		typedef std::unordered_map<symbol,real> dict_type2;
		BOOST_CHECK_EQUAL(k1.evaluate(dict_type2{{symbol("y"),real(-4.3)},{symbol("x"),real(3.2)}},vs),math::cos((0. + (real(3.2) * 2)) + (real(-4.3) * -3)));
		k1.set_flavour(false);
		BOOST_CHECK_EQUAL(k1.evaluate(dict_type2{{symbol("y"),real(-4.3)},{symbol("x"),real(3.2)}},vs),math::sin((0. + (real(3.2) * 2)) + (real(-4.3) * -3)));
		k1 = k_type({T(-2),T(-3)});
		BOOST_CHECK_EQUAL(k1.evaluate(dict_type2{{symbol("y"),real(1.234)},{symbol("x"),real(5.678)}},vs),
			math::cos((real() + (real(5.678) * -2)) + (real(1.234) * -3)));
		k1.set_flavour(false);
		BOOST_CHECK_EQUAL(k1.evaluate(dict_type2{{symbol("y"),real(1.234)},{symbol("x"),real(5.678)}},vs),
			math::sin((real() + (real(5.678) * -2)) + (real(1.234) * -3)));
		typedef std::unordered_map<symbol,rational> dict_type3;
		k1 = k_type({T(3),T(-2)});
		BOOST_CHECK_EQUAL(k1.evaluate(dict_type3{{symbol("y"),rational(2,2)},{symbol("x"),rational(2,3)}},vs),1);
		k1.set_flavour(false);
		BOOST_CHECK_EQUAL(k1.evaluate(dict_type3{{symbol("y"),rational(2,2)},{symbol("x"),rational(2,3)}},vs),0);
	}
};

BOOST_AUTO_TEST_CASE(rtkm_evaluate_test)
{
	boost::mpl::for_each<int_types>(evaluate_tester());
}

struct subs_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
		symbol_set vs;
		k_type k1;
		auto ret = k1.subs(symbol("x"),integer(5),vs);
		BOOST_CHECK_EQUAL(ret.first.first,1);
		BOOST_CHECK(ret.first.second == k1);
		BOOST_CHECK_EQUAL(ret.second.first,0);
		BOOST_CHECK((ret.second.second == k_type(T(0),false)));
		k1.set_flavour(false);
		ret = k1.subs(symbol("x"),integer(5),vs);
		BOOST_CHECK_EQUAL(ret.first.first,0);
		BOOST_CHECK((ret.first.second == k_type(T(0),true)));
		BOOST_CHECK_EQUAL(ret.second.first,1);
		BOOST_CHECK((ret.second.second == k1));
		k1 = k_type{T(1)};
		BOOST_CHECK_THROW(k1.subs(symbol("x"),integer(5),vs),std::invalid_argument);
		k1 = k_type(T(1),false);
		BOOST_CHECK_THROW(k1.subs(symbol("x"),integer(5),vs),std::invalid_argument);
		// Subs with no sign changes.
		vs.add("x");
		vs.add("y");
		k1 = k_type({T(2),T(3)});
		auto ret2 = k1.subs(symbol("x"),real(5),vs);
		BOOST_CHECK_EQUAL(ret2.first.first,math::cos(real(5) * T(2)));
		BOOST_CHECK_EQUAL(ret2.second.first,-math::sin(real(5) * T(2)));
		BOOST_CHECK((ret2.first.second == k_type({T(3)})));
		BOOST_CHECK((ret2.second.second == k_type(T(3),false)));
		k1.set_flavour(false);
		ret2 = k1.subs(symbol("x"),real(5),vs);
		BOOST_CHECK_EQUAL(ret2.first.first,math::sin(real(5) * T(2)));
		BOOST_CHECK_EQUAL(ret2.second.first,math::cos(real(5) * T(2)));
		BOOST_CHECK((ret2.first.second == k_type({T(3)})));
		BOOST_CHECK((ret2.second.second == k_type(T(3),false)));
		// Subs with no actual sub.
		k1.set_flavour(true);
		ret2 = k1.subs(symbol("z"),real(5),vs);
		BOOST_CHECK_EQUAL(ret2.first.first,real(1));
		BOOST_CHECK_EQUAL(ret2.second.first,real(0));
		BOOST_CHECK((ret2.first.second == k1));
		k1.set_flavour(false);
		BOOST_CHECK((ret2.second.second == k1));
		ret2 = k1.subs(symbol("z"),real(5),vs);
		BOOST_CHECK_EQUAL(ret2.first.first,real(0));
		BOOST_CHECK_EQUAL(ret2.second.first,real(1));
		k1.set_flavour(true);
		BOOST_CHECK((ret2.first.second == k1));
		k1.set_flavour(false);
		BOOST_CHECK((ret2.second.second == k1));
		// Subs with sign change.
		k1 = k_type({T(2),T(-3)});
		ret2 = k1.subs(symbol("x"),real(6),vs);
		BOOST_CHECK_EQUAL(ret2.first.first,math::cos(real(6) * T(2)));
		BOOST_CHECK_EQUAL(ret2.second.first,math::sin(real(6) * T(2)));
		BOOST_CHECK((ret2.first.second == k_type({T(3)})));
		BOOST_CHECK((ret2.second.second == k_type(T(3),false)));
		k1.set_flavour(false);
		ret2 = k1.subs(symbol("x"),real(6),vs);
		BOOST_CHECK_EQUAL(ret2.first.first,math::sin(real(6) * T(2)));
		BOOST_CHECK_EQUAL(ret2.second.first,-math::cos(real(6) * T(2)));
		BOOST_CHECK((ret2.first.second == k_type({T(3)})));
		BOOST_CHECK((ret2.second.second == k_type(T(3),false)));
		if (std::is_same<signed char,T>::value) {
			return;
		}
		// Another with sign change.
		k1 = k_type({T(2),T(-2),T(1)});
		vs.add("z");
		ret2 = k1.subs(symbol("x"),real(7),vs);
		BOOST_CHECK_EQUAL(ret2.first.first,math::cos(real(7) * T(2)));
		BOOST_CHECK_EQUAL(ret2.second.first,math::sin(real(7) * T(2)));
		k_type tmp({T(2),T(-1)});
		BOOST_CHECK((ret2.first.second == tmp));
		tmp.set_flavour(false);
		BOOST_CHECK((ret2.second.second == tmp));
		k1.set_flavour(false);
		ret2 = k1.subs(symbol("x"),real(7),vs);
		BOOST_CHECK_EQUAL(ret2.first.first,math::sin(real(7) * T(2)));
		BOOST_CHECK_EQUAL(ret2.second.first,-math::cos(real(7) * T(2)));
		BOOST_CHECK((ret2.second.second == tmp));
		tmp.set_flavour(true);
		BOOST_CHECK((ret2.first.second == tmp));
		// Sign change with leading zero multiplier after substitution.
		k1 = k_type({T(2),T(0),T(-1)});
		ret2 = k1.subs(symbol("x"),real(7),vs);
		BOOST_CHECK_EQUAL(ret2.first.first,math::cos(real(7) * T(2)));
		BOOST_CHECK_EQUAL(ret2.second.first,math::sin(real(7) * T(2)));
		tmp = k_type({T(0),T(1)});
		BOOST_CHECK((ret2.first.second == tmp));
		tmp.set_flavour(false);
		BOOST_CHECK((ret2.second.second == tmp));
		k1.set_flavour(false);
		ret2 = k1.subs(symbol("x"),real(7),vs);
		BOOST_CHECK_EQUAL(ret2.first.first,math::sin(real(7) * T(2)));
		BOOST_CHECK_EQUAL(ret2.second.first,-math::cos(real(7) * T(2)));
		BOOST_CHECK((ret2.second.second == tmp));
		tmp.set_flavour(true);
		BOOST_CHECK((ret2.first.second == tmp));
		// Leading zero and subsequent canonicalisation.
		k1 = k_type({T(0),T(-1),T(1)});
		ret2 = k1.subs(symbol("x"),real(7),vs);
		BOOST_CHECK_EQUAL(ret2.first.first,math::cos(real(7) * T(0)));
		BOOST_CHECK_EQUAL(ret2.second.first,math::sin(real(7) * T(0)));
		tmp = k_type({T(1),T(-1)});
		BOOST_CHECK((ret2.first.second == tmp));
		tmp.set_flavour(false);
		BOOST_CHECK((ret2.second.second == tmp));
		k1.set_flavour(false);
		ret2 = k1.subs(symbol("x"),real(7),vs);
		BOOST_CHECK_EQUAL(ret2.first.first,math::sin(real(7) * T(0)));
		BOOST_CHECK_EQUAL(ret2.second.first,-math::cos(real(7) * T(0)));
		BOOST_CHECK((ret2.second.second == tmp));
		tmp.set_flavour(true);
		BOOST_CHECK((ret2.first.second == tmp));
	}
};

BOOST_AUTO_TEST_CASE(rtkm_subs_test)
{
	boost::mpl::for_each<int_types>(subs_tester());
}

struct print_tex_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
		symbol_set vs;
		k_type k1;
		std::ostringstream oss;
		k1.print_tex(oss,vs);
		BOOST_CHECK(oss.str().empty());
		vs.add("x");
		k_type k2(vs);
		k2.print_tex(oss,vs);
		BOOST_CHECK(oss.str().empty());
		k_type k3({T(1)});
		k3.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"\\cos{\\left({x}\\right)}");
		k3.set_flavour(false);
		oss.str("");
		k3.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"\\sin{\\left({x}\\right)}");
		k_type k5({T(1),T(-1)});
		vs.add("y");
		oss.str("");
		k5.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"\\cos{\\left({x}-{y}\\right)}");
		oss.str("");
		k5 = k_type{T(1),T(1)};
		k5.print_tex(oss,vs);
		BOOST_CHECK(oss.str() == "\\cos{\\left({x}+{y}\\right)}");
		oss.str("");
		k5 = k_type{T(1),T(2)};
		k5.set_flavour(false);
		k5.print_tex(oss,vs);
		BOOST_CHECK(oss.str() == "\\sin{\\left({x}+2{y}\\right)}");
		oss.str("");
		k5 = k_type{T(1),T(-2)};
		k5.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"\\cos{\\left({x}-2{y}\\right)}");
		oss.str("");
		k5 = k_type{T(-1),T(-2)};
		k5.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"\\cos{\\left(-{x}-2{y}\\right)}");
		oss.str("");
		k5 = k_type{T(-2),T(1)};
		k5.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"\\cos{\\left(-2{x}+{y}\\right)}");
		// Representation bug: would display cos(+y).
		oss.str("");
		k5 = k_type{T(0),T(1)};
		k5.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"\\cos{\\left({y}\\right)}");
		oss.str("");
		k5 = k_type{T(0),T(-1)};
		k5.print_tex(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"\\cos{\\left(-{y}\\right)}");
	}
};

BOOST_AUTO_TEST_CASE(rtkm_print_tex_test)
{
	boost::mpl::for_each<int_types>(print_tex_tester());
}

struct integrate_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
		typedef kronecker_array<T> ka;
		const auto &limits = ka::get_limits();
		symbol_set vs;
		k_type k1{T(1)};
		BOOST_CHECK_THROW(k1.integrate(symbol("x"),vs),std::invalid_argument);
		if (std::get<0u>(limits[1u])[0u] < boost::integer_traits<T>::const_max) {
			k1.set_int(boost::integer_traits<T>::const_max);
			BOOST_CHECK_THROW(k1.integrate(symbol("x"),vs),std::invalid_argument);
		}
		vs.add("x");
		vs.add("y");
		k1 = k_type{T(1),T(2)};
		auto ret = k1.integrate(symbol("x"),vs);
		BOOST_CHECK_EQUAL(ret.first,1);
		BOOST_CHECK_EQUAL(ret.second.get_flavour(),false);
		BOOST_CHECK_EQUAL(ret.second.get_int(),k1.get_int());
		k1.set_flavour(false);
		ret = k1.integrate(symbol("y"),vs);
		BOOST_CHECK_EQUAL(ret.first,-2);
		BOOST_CHECK_EQUAL(ret.second.get_flavour(),true);
		BOOST_CHECK_EQUAL(ret.second.get_int(),k1.get_int());
		k1 = k_type{T(0),T(2)};
		ret = k1.integrate(symbol("x"),vs);
		BOOST_CHECK_EQUAL(ret.first,0);
		BOOST_CHECK_EQUAL(ret.second.get_flavour(),true);
		BOOST_CHECK_EQUAL(ret.second.get_int(),0);
		k1 = k_type{T(1),T(2)};
		ret = k1.integrate(symbol("z"),vs);
		BOOST_CHECK_EQUAL(ret.first,0);
		BOOST_CHECK_EQUAL(ret.second.get_flavour(),true);
		BOOST_CHECK_EQUAL(ret.second.get_int(),0);
		k1 = k_type{T(1),T(2)};
		ret = k1.integrate(symbol("y"),vs);
		BOOST_CHECK_EQUAL(ret.first,2);
		BOOST_CHECK_EQUAL(ret.second.get_flavour(),false);
		BOOST_CHECK_EQUAL(ret.second.get_int(),k1.get_int());
	}
};

BOOST_AUTO_TEST_CASE(rtkm_integrate_test)
{
	boost::mpl::for_each<int_types>(integrate_tester());
}

struct canonicalise_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
		symbol_set vs;
		k_type k1;
		BOOST_CHECK(!k1.canonicalise(vs));
		k1 = k_type{T(1)};
		BOOST_CHECK_THROW(k1.canonicalise(vs),std::invalid_argument);
		vs.add("x");
		k1 = k_type{T(0)};
		BOOST_CHECK(!k1.canonicalise(vs));
		k1 = k_type{T(1)};
		BOOST_CHECK(!k1.canonicalise(vs));
		k1 = k_type{T(-1)};
		BOOST_CHECK(k1.canonicalise(vs));
		BOOST_CHECK(k1 == k_type{T(1)});
		vs.add("y");
		k1 = k_type{T(0),T(0)};
		BOOST_CHECK(!k1.canonicalise(vs));
		BOOST_CHECK((k1 == k_type{T(0),T(0)}));
		k1 = k_type{T(1),T(0)};
		BOOST_CHECK(!k1.canonicalise(vs));
		BOOST_CHECK((k1 == k_type{T(1),T(0)}));
		k1 = k_type{T(-1),T(0)};
		BOOST_CHECK(k1.canonicalise(vs));
		BOOST_CHECK((k1 == k_type{T(1),T(0)}));
		k1 = k_type{T(1),T(-1)};
		BOOST_CHECK(!k1.canonicalise(vs));
		BOOST_CHECK((k1 == k_type{T(1),T(-1)}));
		k1 = k_type{T(0),T(-1)};
		BOOST_CHECK(k1.canonicalise(vs));
		BOOST_CHECK((k1 == k_type{T(0),T(1)}));
		k1 = k_type{T(0),T(1)};
		BOOST_CHECK(!k1.canonicalise(vs));
		BOOST_CHECK((k1 == k_type{T(0),T(1)}));
		vs.add("z");
		k1 = k_type{T(0),T(1),T(-1)};
		BOOST_CHECK(!k1.canonicalise(vs));
		BOOST_CHECK((k1 == k_type{T(0),T(1),T(-1)}));
		k1 = k_type{T(0),T(-1),T(-1)};
		BOOST_CHECK(k1.canonicalise(vs));
		BOOST_CHECK((k1 == k_type{T(0),T(1),T(1)}));
		k1 = k_type{T(0),T(0),T(-1)};
		BOOST_CHECK(k1.canonicalise(vs));
		BOOST_CHECK((k1 == k_type{T(0),T(0),T(1)}));
		k1 = k_type{T(1),T(-1),T(-1)};
		BOOST_CHECK(!k1.canonicalise(vs));
		BOOST_CHECK((k1 == k_type{T(1),T(-1),T(-1)}));
	}
};

BOOST_AUTO_TEST_CASE(rtkm_canonicalise_test)
{
	boost::mpl::for_each<int_types>(canonicalise_tester());
}

struct trim_identify_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
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

BOOST_AUTO_TEST_CASE(rtkm_trim_identify_test)
{
	boost::mpl::for_each<int_types>(trim_identify_tester());
}

struct trim_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
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
		BOOST_CHECK((k0.trim(v2,v1) == k_type()));
		v2 = symbol_set();
		BOOST_CHECK((k0.trim(v2,v1) == k0));
		k0.set_flavour(false);
		v2.add("x");
		v2.add("z");
		v2.add("a");
		BOOST_CHECK((k0.trim(v2,v1) == k_type(T(0),false)));
	}
};

BOOST_AUTO_TEST_CASE(rtkm_trim_test)
{
	boost::mpl::for_each<int_types>(trim_tester());
}

struct tt_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
		BOOST_CHECK(key_has_t_degree<k_type>::value);
		BOOST_CHECK(key_has_t_ldegree<k_type>::value);
		BOOST_CHECK(key_has_t_order<k_type>::value);
		BOOST_CHECK(key_has_t_lorder<k_type>::value);
		BOOST_CHECK(is_hashable<k_type>::value);
	}
};

BOOST_AUTO_TEST_CASE(rtkm_tt_test)
{
	boost::mpl::for_each<int_types>(tt_tester());
}

struct fake_int_01
{
	fake_int_01();
	explicit fake_int_01(int);
	fake_int_01(const fake_int_01 &);
	fake_int_01(fake_int_01 &&) noexcept(true);
	fake_int_01 &operator=(const fake_int_01 &);
	fake_int_01 &operator=(fake_int_01 &&) noexcept(true);
	~fake_int_01() noexcept(true);
	fake_int_01 operator+(const fake_int_01 &) const;
	fake_int_01 operator*(const fake_int_01 &) const;
	fake_int_01 &operator+=(const fake_int_01 &);
	fake_int_01 &operator+=(const integer &);
	bool operator==(const fake_int_01 &) const;
	bool operator!=(const fake_int_01 &) const;
};

integer &operator+=(integer &, const fake_int_01 &);
integer operator*(const integer &, const fake_int_01 &);

// Missing math operators.
struct fake_int_02
{
	fake_int_02();
	explicit fake_int_02(int);
	fake_int_02(const fake_int_02 &);
	fake_int_02(fake_int_02 &&) noexcept(true);
	fake_int_02 &operator=(const fake_int_02 &);
	fake_int_02 &operator=(fake_int_02 &&) noexcept(true);
	~fake_int_02() noexcept(true);
	fake_int_02 operator+(const fake_int_02 &) const;
	fake_int_02 operator*(const fake_int_02 &) const;
	fake_int_02 &operator+=(const fake_int_02 &);
	fake_int_02 &operator+=(const integer &);
	bool operator==(const fake_int_02 &) const;
	bool operator!=(const fake_int_02 &) const;
};

BOOST_AUTO_TEST_CASE(rtkm_key_has_t_subs_test)
{
	BOOST_CHECK((key_has_t_subs<real_trigonometric_kronecker_monomial<int>,int>::value));
	BOOST_CHECK((key_has_t_subs<real_trigonometric_kronecker_monomial<int>,int,int>::value));
	BOOST_CHECK((key_has_t_subs<real_trigonometric_kronecker_monomial<int>,fake_int_01,fake_int_01>::value));
	BOOST_CHECK((!key_has_t_subs<real_trigonometric_kronecker_monomial<int>,fake_int_02,fake_int_02>::value));
	// This fails because the cos and sin replacements must be the same type.
	BOOST_CHECK((!key_has_t_subs<real_trigonometric_kronecker_monomial<short>,int,long>::value));
	BOOST_CHECK((key_has_t_subs<real_trigonometric_kronecker_monomial<short>,long,long>::value));
	BOOST_CHECK((key_has_t_subs<real_trigonometric_kronecker_monomial<long> &,long,const long &>::value));
	BOOST_CHECK((key_has_t_subs<const real_trigonometric_kronecker_monomial<short> &,char,const char &>::value));
	BOOST_CHECK((!key_has_t_subs<const real_trigonometric_kronecker_monomial<long long> &,char,int>::value));
	BOOST_CHECK(!key_has_degree<real_trigonometric_kronecker_monomial<int>>::value);
	BOOST_CHECK(!key_has_ldegree<real_trigonometric_kronecker_monomial<int>>::value);
	BOOST_CHECK(key_has_t_degree<real_trigonometric_kronecker_monomial<int>>::value);
	BOOST_CHECK(key_has_t_ldegree<real_trigonometric_kronecker_monomial<int>>::value);
	BOOST_CHECK(key_has_t_order<real_trigonometric_kronecker_monomial<int>>::value);
	BOOST_CHECK(key_has_t_lorder<real_trigonometric_kronecker_monomial<int>>::value);
}

struct t_subs_tester
{
	template <typename T>
	void operator()(const T &, typename std::enable_if<!std::is_same<T,signed char>::value>::type * = nullptr)
	{
		// Test with no substitution.
		typedef real_trigonometric_kronecker_monomial<T> k_type;
		symbol_set v;
		k_type k;
		auto res = k.t_subs("x",real(.5),real(.0),v);
		typedef decltype(res) res_type1;
		BOOST_CHECK((std::is_same<typename res_type1::value_type::first_type,real>::value));
		BOOST_CHECK_EQUAL(res.size(),2u);
		BOOST_CHECK_EQUAL(res[0u].first,real(1));
		BOOST_CHECK_EQUAL(res[1u].first,real(0));
		k.set_flavour(false);
		res = k.t_subs("x",real(.5),real(.0),v);
		BOOST_CHECK_EQUAL(res.size(),2u);
		BOOST_CHECK_EQUAL(res[0u].first,real(0));
		BOOST_CHECK_EQUAL(res[1u].first,real(1));
		k = k_type{T(3)};
		k.set_flavour(true);
		v.add("x");
		res = k.t_subs("y",real(.5),real(.0),v);
		BOOST_CHECK_EQUAL(res.size(),2u);
		BOOST_CHECK_EQUAL(res[0u].first,real(1));
		BOOST_CHECK_EQUAL(res[1u].first,real(0));
		BOOST_CHECK(res[0u].second == k);
		k.set_flavour(false);
		BOOST_CHECK(res[1u].second == k);
		res = k.t_subs("y",real(.5),real(.0),v);
		BOOST_CHECK_EQUAL(res.size(),2u);
		BOOST_CHECK_EQUAL(res[0u].first,real(0));
		BOOST_CHECK_EQUAL(res[1u].first,real(1));
		BOOST_CHECK(res[1u].second == k);
		k.set_flavour(true);
		BOOST_CHECK(res[0u].second == k);
		// Test substitution with no canonicalisation.
		v.add("y");
		k = k_type{T(2),T(3)};
		auto c = rational(1,2), s = rational(4,5);
		auto res2 = k.t_subs("y",c,s,v);
		typedef decltype(res2) res_type2;
		BOOST_CHECK((std::is_same<typename res_type2::value_type::first_type,rational>::value));
		BOOST_CHECK_EQUAL(res2.size(),2u);
		BOOST_CHECK_EQUAL(res2[0u].first,c*c*c-3*s*s*c);
		BOOST_CHECK_EQUAL(res2[1u].first,-3*c*c*s+s*s*s);
		k = k_type{T(2),T(0)};
		BOOST_CHECK(res2[0u].second == k);
		k.set_flavour(false);
		BOOST_CHECK(res2[1u].second == k);
		k = k_type{T(2),T(3)};
		k.set_flavour(false);
		res2 = k.t_subs("y",c,s,v);
		BOOST_CHECK_EQUAL(res2.size(),2u);
		BOOST_CHECK_EQUAL(res2[0u].first,3*c*c*s-s*s*s);
		BOOST_CHECK_EQUAL(res2[1u].first,c*c*c-3*s*s*c);
		k = k_type{T(2),T(0)};
		BOOST_CHECK(res2[0u].second == k);
		k.set_flavour(false);
		BOOST_CHECK(res2[1u].second == k);
		// Negative multiplier
		k = k_type{T(-3),T(3)};
		res2 = k.t_subs("x",c,s,v);
		BOOST_CHECK_EQUAL(res2.size(),2u);
		BOOST_CHECK_EQUAL(res2[0u].first,c*c*c-3*s*s*c);
		BOOST_CHECK_EQUAL(res2[1u].first,3*c*c*s-s*s*s);
		k = k_type{T(0),T(3)};
		BOOST_CHECK(res2[0u].second == k);
		k.set_flavour(false);
		BOOST_CHECK(res2[1u].second == k);
		k = k_type{T(-3),T(3)};
		k.set_flavour(false);
		res2 = k.t_subs("x",c,s,v);
		BOOST_CHECK_EQUAL(res2.size(),2u);
		BOOST_CHECK_EQUAL(res2[0u].first,-3*c*c*s+s*s*s);
		BOOST_CHECK_EQUAL(res2[1u].first,c*c*c-3*s*s*c);
		k = k_type{T(0),T(3)};
		BOOST_CHECK(res2[0u].second == k);
		k.set_flavour(false);
		BOOST_CHECK(res2[1u].second == k);
		// Test substitution with canonicalisation.
		k = k_type{T(-2),T(3)};
		res2 = k.t_subs("y",c,s,v);
		BOOST_CHECK_EQUAL(res2.size(),2u);
		BOOST_CHECK_EQUAL(res2[0u].first,c*c*c-3*s*s*c);
		BOOST_CHECK_EQUAL(res2[1u].first,3*c*c*s-s*s*s);
		k = k_type{T(2),T(0)};
		BOOST_CHECK(res2[0u].second == k);
		k.set_flavour(false);
		BOOST_CHECK(res2[1u].second == k);
		k = k_type{T(-2),T(3)};
		k.set_flavour(false);
		res2 = k.t_subs("y",c,s,v);
		BOOST_CHECK_EQUAL(res2.size(),2u);
		BOOST_CHECK_EQUAL(res2[0u].first,3*c*c*s-s*s*s);
		BOOST_CHECK_EQUAL(res2[1u].first,-c*c*c+3*s*s*c);
		k = k_type{T(2),T(0)};
		BOOST_CHECK(res2[0u].second == k);
		k.set_flavour(false);
		BOOST_CHECK(res2[1u].second == k);
		// Negative multiplier
		k = k_type{T(-3),T(-3)};
		res2 = k.t_subs("x",c,s,v);
		BOOST_CHECK_EQUAL(res2.size(),2u);
		BOOST_CHECK_EQUAL(res2[0u].first,c*c*c-3*s*s*c);
		BOOST_CHECK_EQUAL(res2[1u].first,-3*c*c*s+s*s*s);
		k = k_type{T(0),T(3)};
		BOOST_CHECK(res2[0u].second == k);
		k.set_flavour(false);
		BOOST_CHECK(res2[1u].second == k);
		k = k_type{T(-3),T(-3)};
		k.set_flavour(false);
		res2 = k.t_subs("x",c,s,v);
		BOOST_CHECK_EQUAL(res2.size(),2u);
		BOOST_CHECK_EQUAL(res2[0u].first,-3*c*c*s+s*s*s);
		BOOST_CHECK_EQUAL(res2[1u].first,-c*c*c+3*s*s*c);
		k = k_type{T(0),T(3)};
		BOOST_CHECK(res2[0u].second == k);
		k.set_flavour(false);
		BOOST_CHECK(res2[1u].second == k);
	}
	template <typename T>
	void operator()(const T &, typename std::enable_if<std::is_same<T,signed char>::value>::type * = nullptr)
	{}
};

BOOST_AUTO_TEST_CASE(rtkm_t_subs_test)
{
	boost::mpl::for_each<int_types>(t_subs_tester());
}

struct is_evaluable_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
		BOOST_CHECK((key_is_evaluable<k_type,float>::value));
		BOOST_CHECK((key_is_evaluable<k_type,double>::value));
		BOOST_CHECK((key_is_evaluable<k_type,real>::value));
		BOOST_CHECK((key_is_evaluable<k_type,integer>::value));
		BOOST_CHECK((key_is_evaluable<k_type,rational>::value));
		BOOST_CHECK((!key_is_evaluable<k_type,int>::value));
		BOOST_CHECK((!key_is_evaluable<k_type,long>::value));
		BOOST_CHECK((!key_is_evaluable<k_type,long long>::value));
		BOOST_CHECK((!key_is_evaluable<k_type,std::string>::value));
		BOOST_CHECK((!key_is_evaluable<k_type,void *>::value));
	}
};

BOOST_AUTO_TEST_CASE(rtkm_key_is_evaluable_test)
{
	boost::mpl::for_each<int_types>(is_evaluable_tester());
}

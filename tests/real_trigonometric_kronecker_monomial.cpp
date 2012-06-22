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
#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "../src/integer.hpp"
#include "../src/kronecker_array.hpp"
#include "../src/symbol.hpp"
#include "../src/symbol_set.hpp"

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
		k_type k3{};
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
		BOOST_CHECK_EQUAL((k_type{1,false}.get_int()),1);
		BOOST_CHECK(!(k_type{1,false}.get_flavour()));
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
		// Converting constructor.
		k_type k16, k17(k16,symbol_set{});
		BOOST_CHECK(k17.get_flavour());
		BOOST_CHECK(!(k_type(k_type{0,false},symbol_set{}).get_flavour()));
		//BOOST_CHECK(k16 == k17);
		k16.set_int(10);
		k_type k18(k16,symbol_set({symbol("a")}));
		//BOOST_CHECK(k16 == k18);
		BOOST_CHECK_THROW((k_type(k16,symbol_set({}))),std::invalid_argument);
		// First element negative.
		k16 = k_type{-1,0};
		BOOST_CHECK_THROW((k_type(k16,symbol_set({symbol{"a"},symbol{"b"}}))),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(rtkm_constructor_test)
{
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
		BOOST_CHECK(!k_type{}.is_ignorable(symbol_set{}));
		BOOST_CHECK(!k_type(symbol_set{symbol("a")}).is_ignorable(symbol_set{}));
		BOOST_CHECK(!(k_type{0,0}.is_ignorable(symbol_set{})));
		BOOST_CHECK(!(k_type{1,false}.is_ignorable(symbol_set{symbol{"a"}})));
		BOOST_CHECK((k_type{0,false}.is_ignorable(symbol_set{symbol{"a"}})));
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
		k_type k5{};
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
		vs2 = symbol_set{symbol{"a"}};
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

struct h_degree_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef real_trigonometric_kronecker_monomial<T> k_type;
		k_type k1;
		symbol_set vs1;
		BOOST_CHECK(k1.h_degree(vs1) == 0);
		BOOST_CHECK(k1.h_ldegree(vs1) == 0);
		k_type k2({0});
		vs1.add(symbol("a"));
		BOOST_CHECK(k2.h_degree(vs1) == 0);
		BOOST_CHECK(k2.h_ldegree(vs1) == 0);
		k_type k3({-1});
		BOOST_CHECK(k3.h_degree(vs1) == -1);
		BOOST_CHECK(k3.h_ldegree(vs1) == -1);
		vs1.add(symbol("b"));
		k_type k4({0,0});
		BOOST_CHECK(k4.h_degree(vs1) == 0);
		BOOST_CHECK(k4.h_ldegree(vs1) == 0);
		k_type k5({-1,-1});
		BOOST_CHECK(k5.h_degree(vs1) == -2);
		BOOST_CHECK(k5.h_degree({"a"},vs1) == -1);
		BOOST_CHECK(k5.h_degree({},vs1) == 0);
		BOOST_CHECK(k5.h_degree({"f"},vs1) == 0);
		BOOST_CHECK(k5.h_degree({"a","b"},vs1) == -2);
		BOOST_CHECK(k5.h_degree({"a","c"},vs1) == -1);
		BOOST_CHECK(k5.h_degree({"d","c"},vs1) == 0);
		BOOST_CHECK(k5.h_degree({"d","b"},vs1) == -1);
		BOOST_CHECK(k5.h_degree({"A","a"},vs1) == -1);
		BOOST_CHECK(k5.h_ldegree(vs1) == -2);
		BOOST_CHECK(k5.h_ldegree({"a"},vs1) == -1);
		BOOST_CHECK(k5.h_ldegree({},vs1) == 0);
		BOOST_CHECK(k5.h_ldegree({"f"},vs1) == 0);
		BOOST_CHECK(k5.h_ldegree({"a","b"},vs1) == -2);
		BOOST_CHECK(k5.h_ldegree({"a","c"},vs1) == -1);
		BOOST_CHECK(k5.h_ldegree({"d","c"},vs1) == 0);
		BOOST_CHECK(k5.h_ldegree({"d","b"},vs1) == -1);
		BOOST_CHECK(k5.h_ldegree({"A","a"},vs1) == -1);
	}
};

BOOST_AUTO_TEST_CASE(rtkm_h_degree_test)
{
	boost::mpl::for_each<int_types>(h_degree_tester());
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
		k1.multiply(result_plus,result_minus,k2,vs1);
		BOOST_CHECK(result_plus.get_int() == 0);
		BOOST_CHECK(result_minus.get_int() == 0);
		BOOST_CHECK(result_plus.get_flavour() == true);
		BOOST_CHECK(result_minus.get_flavour() == true);
		k1 = k_type({0});
		k2 = k_type({0});
		vs1.add(symbol("a"));
		k1.multiply(result_plus,result_minus,k2,vs1);
		BOOST_CHECK(result_plus.get_int() == 0);
		BOOST_CHECK(result_minus.get_int() == 0);
		BOOST_CHECK(result_plus.get_flavour() == true);
		BOOST_CHECK(result_minus.get_flavour() == true);
		k1 = k_type({1});
		k2 = k_type({2});
		k1.multiply(result_plus,result_minus,k2,vs1);
		BOOST_CHECK(result_plus.get_int() == 3);
		BOOST_CHECK(result_minus.get_int() == -1);
		BOOST_CHECK(result_plus.get_flavour() == true);
		BOOST_CHECK(result_minus.get_flavour() == true);
		k1 = k_type({1,-1});
		k2 = k_type({2,0});
		vs1.add(symbol("b"));
		k1.multiply(result_plus,result_minus,k2,vs1);
		BOOST_CHECK(result_plus.get_flavour() == true);
		BOOST_CHECK(result_minus.get_flavour() == true);
		std::vector<int> tmp(2u);
		ka::decode(tmp,result_plus.get_int());
		BOOST_CHECK(tmp[0u] == 3);
		BOOST_CHECK(tmp[1u] == -1);
		ka::decode(tmp,result_minus.get_int());
		BOOST_CHECK(tmp[0u] == -1);
		BOOST_CHECK(tmp[1u] == -1);
		k1.set_flavour(false);
		k1.multiply(result_plus,result_minus,k2,vs1);
		BOOST_CHECK(result_plus.get_flavour() == false);
		BOOST_CHECK(result_minus.get_flavour() == false);
		ka::decode(tmp,result_plus.get_int());
		BOOST_CHECK(tmp[0u] == 3);
		BOOST_CHECK(tmp[1u] == -1);
		ka::decode(tmp,result_minus.get_int());
		BOOST_CHECK(tmp[0u] == -1);
		BOOST_CHECK(tmp[1u] == -1);
		k1.set_flavour(true);
		k2.set_flavour(false);
		k1.multiply(result_plus,result_minus,k2,vs1);
		BOOST_CHECK(result_plus.get_flavour() == false);
		BOOST_CHECK(result_minus.get_flavour() == false);
		ka::decode(tmp,result_plus.get_int());
		BOOST_CHECK(tmp[0u] == 3);
		BOOST_CHECK(tmp[1u] == -1);
		ka::decode(tmp,result_minus.get_int());
		BOOST_CHECK(tmp[0u] == -1);
		BOOST_CHECK(tmp[1u] == -1);
		k1.set_flavour(false);
		k1.multiply(result_plus,result_minus,k2,vs1);
		BOOST_CHECK(result_plus.get_flavour() == true);
		BOOST_CHECK(result_minus.get_flavour() == true);
		ka::decode(tmp,result_plus.get_int());
		BOOST_CHECK(tmp[0u] == 3);
		BOOST_CHECK(tmp[1u] == -1);
		ka::decode(tmp,result_minus.get_int());
		BOOST_CHECK(tmp[0u] == -1);
		BOOST_CHECK(tmp[1u] == -1);
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
		// Avoid print test with signed char, garbage will come out.
		if (std::is_same<T,signed char>::value) {
			return;
		}
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
		BOOST_CHECK(oss.str() == "sin(x+2y)");
		oss.str("");
		k5 = k_type{T(1),T(-2)};
		k5.print(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"cos(x-2y)");
		oss.str("");
		k5 = k_type{T(-1),T(-2)};
		k5.print(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"cos(-x-2y)");
		oss.str("");
		k5 = k_type{T(-2),T(1)};
		k5.print(oss,vs);
		BOOST_CHECK_EQUAL(oss.str(),"cos(-2x+y)");
	}
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_print_test)
{
	boost::mpl::for_each<int_types>(print_tester());
}

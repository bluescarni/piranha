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

#include "../src/univariate_monomial.hpp"

#define BOOST_TEST_MODULE univariate_monomial_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <initializer_list>
#include <set>
#include <string>
#include <stdexcept>
#include <unordered_set>

#include "../src/environment.hpp"
#include "../src/math.hpp"
#include "../src/mp_integer.hpp"
#include "../src/symbol.hpp"
#include "../src/symbol_set.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

typedef boost::mpl::vector<int,integer> expo_types;

// Constructors, assignments and element access.
struct constructor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef univariate_monomial<T> um_type;
		um_type u0;
		BOOST_CHECK(u0.get_exponent() == T(0));
		u0.set_exponent(3);
		um_type u1(u0);
		BOOST_CHECK(u1.get_exponent() == T(3));
		um_type u2(std::move(u0));
		BOOST_CHECK(u2.get_exponent() == T(3));
		um_type u3 = um_type(symbol_set{});
		BOOST_CHECK(u3.get_exponent() == T(0));
		um_type u4 = um_type(symbol_set({symbol("x")}));
		BOOST_CHECK(u4.get_exponent() == T(0));
		BOOST_CHECK_THROW(u4 = um_type(symbol_set({symbol("x"),symbol("y")})),std::invalid_argument);
		BOOST_CHECK(um_type{2}.get_exponent() == T(2));
		BOOST_CHECK_THROW((um_type{2,3}),std::invalid_argument);
		u0 = u2;
		BOOST_CHECK(u0.get_exponent() == T(3));
		u0 = um_type{2};
		BOOST_CHECK(u0.get_exponent() == T(2));
		// Converting constructor.
		um_type u5, u6(u5,symbol_set({}));
		BOOST_CHECK(u5 == u6);
		u5.set_exponent(T(10));
		um_type u7(u5,symbol_set({symbol("a")}));
		BOOST_CHECK(u7 == u5);
		BOOST_CHECK_THROW((um_type(u7,symbol_set({symbol("a"),symbol("b")}))),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(univariate_monomial_constructor_test)
{
	environment env;
	boost::mpl::for_each<expo_types>(constructor_tester());
}

struct hash_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef univariate_monomial<T> monomial_type;
		monomial_type m0;
		BOOST_CHECK_EQUAL(m0.hash(),std::size_t());
		BOOST_CHECK_EQUAL(m0.hash(),std::hash<monomial_type>()(m0));
		monomial_type m1{1};
		BOOST_CHECK_EQUAL(m1.hash(),std::hash<monomial_type>()(m1));
	}
};

BOOST_AUTO_TEST_CASE(univariate_monomial_hash_test)
{
	boost::mpl::for_each<expo_types>(hash_tester());
}

struct eq_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef univariate_monomial<T> monomial_type;
		monomial_type m0, m0a;
		BOOST_CHECK(m0 == m0a);
		BOOST_CHECK(m0 == monomial_type{0});
		monomial_type m1{1};
		BOOST_CHECK(m0 != m1);
	}
};

BOOST_AUTO_TEST_CASE(univariate_monomial_equality_test)
{
	boost::mpl::for_each<expo_types>(eq_tester());
}

struct compatibility_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef univariate_monomial<T> monomial_type;
		monomial_type m0;
		BOOST_CHECK(m0.is_compatible(symbol_set{}));
		monomial_type m1{T(0)};
		BOOST_CHECK(m1.is_compatible(symbol_set{}));
		monomial_type m2{T(1)};
		BOOST_CHECK(!m2.is_compatible(symbol_set{}));
		symbol_set ss({symbol("x")});
		BOOST_CHECK(m2.is_compatible(ss));
		ss.add(symbol("y"));
		BOOST_CHECK((!m2.is_compatible(ss)));
	}
};

BOOST_AUTO_TEST_CASE(univariate_monomial_compatibility_test)
{
	boost::mpl::for_each<expo_types>(compatibility_tester());
}

struct ignorability_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef univariate_monomial<T> monomial_type;
		monomial_type m0;
		BOOST_CHECK(!m0.is_ignorable(symbol_set{}));
		monomial_type m1{T(0)};
		BOOST_CHECK(!m1.is_ignorable(symbol_set{symbol("foobarize")}));
	}
};

BOOST_AUTO_TEST_CASE(univariate_monomial_ignorability_test)
{
	boost::mpl::for_each<expo_types>(ignorability_tester());
}

struct merge_args_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef univariate_monomial<T> key_type;
		symbol_set v1, v2;
		v2.add(symbol("a"));
		key_type k;
		auto out = k.merge_args(v1,v2);
		BOOST_CHECK(out.get_exponent() == T(0));
		v2.add(symbol("b"));
		BOOST_CHECK_THROW(out = k.merge_args(v1,v2),std::invalid_argument);
		BOOST_CHECK_THROW(out = k.merge_args(v1,v1),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(univariate_monomial_merge_args_test)
{
	boost::mpl::for_each<expo_types>(merge_args_tester());
}

struct is_unitary_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef univariate_monomial<T> key_type;
		symbol_set v1, v2;
		v2.add(symbol("a"));
		key_type k(v1);
		BOOST_CHECK(k.is_unitary(v1));
		key_type k2(v2);
		BOOST_CHECK(k2.is_unitary(v2));
		k2.set_exponent(1);
		BOOST_CHECK(!k2.is_unitary(v2));
		k2.set_exponent(0);
		BOOST_CHECK(k2.is_unitary(v2));
		k2.set_exponent(1);
		BOOST_CHECK_THROW(k2.is_unitary(v1),std::invalid_argument);
		v2.add(symbol("b"));
		BOOST_CHECK_THROW(k2.is_unitary(v2),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(univariate_monomial_is_unitary_test)
{
	boost::mpl::for_each<expo_types>(is_unitary_tester());
}

struct multiply_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef univariate_monomial<T> key_type;
		key_type k0, k1, k2;
		symbol_set v;
		k1.multiply(k0,k2,v);
		BOOST_CHECK(k0.get_exponent() == T(0));
		k1.set_exponent(1);
		k2.set_exponent(2);
		v.add(symbol("a"));
		k1.multiply(k0,k2,v);
		BOOST_CHECK(k0.get_exponent() == T(3));
		BOOST_CHECK_THROW(k1.multiply(k0,k2,symbol_set{}),std::invalid_argument);
		k2.set_exponent(0);
		BOOST_CHECK_THROW(k1.multiply(k0,k2,symbol_set{}),std::invalid_argument);
		k2.set_exponent(2);
		k1.set_exponent(0);
		BOOST_CHECK_THROW(k1.multiply(k0,k2,symbol_set{}),std::invalid_argument);
		k1.set_exponent(1);
		BOOST_CHECK_THROW(k1.multiply(k0,k2,symbol_set{symbol("a"),symbol("b")}),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(univariate_monomial_multiply_test)
{
	boost::mpl::for_each<expo_types>(multiply_tester());
}

struct exponent_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef univariate_monomial<T> key_type;
		key_type k0;
		BOOST_CHECK(k0.get_exponent() == T(0));
		k0.set_exponent(4);
		BOOST_CHECK(k0.get_exponent() == T(4));
	}
};

BOOST_AUTO_TEST_CASE(univariate_monomial_exponent_test)
{
	boost::mpl::for_each<expo_types>(exponent_tester());
}

struct degree_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef univariate_monomial<T> key_type;
		key_type k0;
		symbol_set v;
		BOOST_CHECK(k0.degree(v) == T(0));
		BOOST_CHECK(k0.ldegree(v) == T(0));
		k0.set_exponent(4);
		v.add(symbol("a"));
		BOOST_CHECK(k0.degree(v) == T(4));
		BOOST_CHECK(k0.ldegree(v) == T(4));
		symbol_set v2;
		BOOST_CHECK_THROW(k0.degree(v2),std::invalid_argument);
		v.add(symbol("b"));
		BOOST_CHECK_THROW(k0.degree(v),std::invalid_argument);
		// Partial degree.
		BOOST_CHECK_THROW(k0.degree(std::set<std::string>{},v),std::invalid_argument);
		BOOST_CHECK_THROW(k0.ldegree(std::set<std::string>{},v),std::invalid_argument);
		BOOST_CHECK_THROW(k0.degree({"a","b"},v2),std::invalid_argument);
		BOOST_CHECK_THROW(k0.ldegree({"a","b"},v2),std::invalid_argument);
		k0.set_exponent(0);
		BOOST_CHECK(k0.degree({"a","b"},v2) == T(0));
		k0.set_exponent(7);
		BOOST_CHECK(k0.degree({"y"},symbol_set{symbol("x")}) == T(0));
		BOOST_CHECK(k0.degree({"y","a"},symbol_set{symbol("x")}) == T(0));
		BOOST_CHECK(k0.degree({"x","a"},symbol_set{symbol("x")}) == T(7));
		BOOST_CHECK(k0.degree({"b","x"},symbol_set{symbol("x")}) == T(7));
		BOOST_CHECK(k0.ldegree({"y"},symbol_set{symbol("x")}) == T(0));
		BOOST_CHECK(k0.ldegree({"y","a"},symbol_set{symbol("x")}) == T(0));
		BOOST_CHECK(k0.ldegree({"x","a"},symbol_set{symbol("x")}) == T(7));
		BOOST_CHECK(k0.ldegree({"b","x"},symbol_set{symbol("x")}) == T(7));
	}
};

BOOST_AUTO_TEST_CASE(univariate_monomial_degree_test)
{
	boost::mpl::for_each<expo_types>(degree_tester());
}

struct print_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef univariate_monomial<T> k_type;
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
		k_type k5;
		vs.add("y");
		BOOST_CHECK_THROW(k5.print(oss,vs),std::invalid_argument);
		vs = symbol_set();
		k_type k6;
		k6.set_exponent(T(1));
		BOOST_CHECK_THROW(k6.print(oss,vs),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(univariate_monomial_print_test)
{
	boost::mpl::for_each<expo_types>(print_tester());
}

struct print_tex_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef univariate_monomial<T> k_type;
		symbol_set vs;
		k_type k1;
		std::ostringstream oss;
		k1.print_tex(oss,vs);
		BOOST_CHECK(oss.str().empty());
		vs.add("x");
		k_type k2(vs);
		k2.print_tex(oss,vs);
		BOOST_CHECK(oss.str().empty());
		k_type k3({T(-1)});
		k3.print_tex(oss,vs);
		BOOST_CHECK(oss.str() == "\\frac{1}{{x}}");
		oss.str("");
		k3 = k_type({T(-2)});
		k3.print_tex(oss,vs);
		BOOST_CHECK(oss.str() == "\\frac{1}{{x}^{2}}");
		k_type k4({T(1)});
		oss.str("");
		k4.print_tex(oss,vs);
		BOOST_CHECK(oss.str() == "{x}");
		oss.str("");
		k3 = k_type({T(5)});
		k3.print_tex(oss,vs);
		BOOST_CHECK(oss.str() == "{x}^{5}");
		k_type k5;
		vs.add("y");
		BOOST_CHECK_THROW(k5.print_tex(oss,vs),std::invalid_argument);
		vs = symbol_set();
		k_type k6;
		k6.set_exponent(T(1));
		BOOST_CHECK_THROW(k6.print_tex(oss,vs),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(univariate_monomial_print_tex_test)
{
	boost::mpl::for_each<expo_types>(print_tex_tester());
}

struct tt_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef univariate_monomial<T> k_type;
		BOOST_CHECK((!key_has_t_subs<k_type,int,int>::value));
		BOOST_CHECK((!key_has_t_subs<k_type &,int,int>::value));
		BOOST_CHECK((!key_has_t_subs<k_type const &,int,int>::value));
		BOOST_CHECK(is_hashable<k_type>::value);
		BOOST_CHECK(is_equality_comparable<k_type>::value);
		BOOST_CHECK(key_has_degree<k_type>::value);
		BOOST_CHECK(key_has_ldegree<k_type>::value);
		BOOST_CHECK(!key_has_t_degree<k_type>::value);
		BOOST_CHECK(!key_has_t_ldegree<k_type>::value);
		BOOST_CHECK(!key_has_t_order<k_type>::value);
		BOOST_CHECK(!key_has_t_lorder<k_type>::value);
	}
};

BOOST_AUTO_TEST_CASE(univariate_monomial_type_traits_test)
{
	boost::mpl::for_each<expo_types>(tt_tester());
}

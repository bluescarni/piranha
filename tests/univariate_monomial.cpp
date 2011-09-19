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
#include <stdexcept>
#include <unordered_set>

#include "../src/integer.hpp"
#include "../src/symbol.hpp"
#include "../src/symbol_set.hpp"

using namespace piranha;

typedef boost::mpl::vector<unsigned,integer> expo_types;

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
	}
};

BOOST_AUTO_TEST_CASE(univariate_monomial_constructor_test)
{
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
		k0.set_exponent(4);
		v.add(symbol("a"));
		BOOST_CHECK(k0.degree(v) == T(4));
	}
};

BOOST_AUTO_TEST_CASE(univariate_monomial_degree_test)
{
	boost::mpl::for_each<expo_types>(degree_tester());
}

struct get_element_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef univariate_monomial<T> k_type;
		symbol_set vs1;
		vs1.add(symbol("a"));
		k_type k1({0});
		BOOST_CHECK(k1.get_element(0,vs1) == 0);
		k_type k2({1});
		BOOST_CHECK(k2.get_element(0,vs1) == 1);
	}
};

BOOST_AUTO_TEST_CASE(univariate_monomial_get_element_test)
{
	boost::mpl::for_each<expo_types>(get_element_tester());
}

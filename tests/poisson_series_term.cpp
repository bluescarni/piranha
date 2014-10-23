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

#include "../src/poisson_series_term.hpp"

#define BOOST_TEST_MODULE poisson_series_term_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <string>
#include <tuple>
#include <type_traits>

#include "../src/base_term.hpp"
#include "../src/environment.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/polynomial.hpp"
#include "../src/real.hpp"
#include "../src/symbol_set.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

typedef boost::mpl::vector<double,integer,real,rational,polynomial<real,short>> cf_types;

struct constructor_tester
{
	template <typename Cf>
	void operator()(const Cf &)
	{
		typedef poisson_series_term<Cf> term_type;
		BOOST_CHECK(is_term<term_type>::value);
		typedef typename term_type::key_type key_type;
		typedef typename key_type::value_type expo_type;
		symbol_set ed;
		ed.add("x");
		// Default constructor.
		BOOST_CHECK_EQUAL(term_type().m_cf,Cf());
		BOOST_CHECK(term_type().m_key == key_type());
		// Copy constructor.
		term_type t;
		t.m_cf = Cf(1);
		t.m_key = key_type{expo_type(2)};
		BOOST_CHECK_EQUAL(term_type(t).m_cf,Cf(1));
		BOOST_CHECK(term_type(t).m_key == key_type{expo_type(2)});
		// Move constructor.
		term_type t_copy1(t), t_copy2 = t;
		BOOST_CHECK_EQUAL(term_type(std::move(t_copy1)).m_cf,Cf(1));
		BOOST_CHECK(term_type(std::move(t_copy2)).m_key == key_type{expo_type(2)});
		// Copy assignment.
		t_copy1 = t;
		BOOST_CHECK_EQUAL(t_copy1.m_cf,Cf(1));
		BOOST_CHECK(t_copy1.m_key == key_type{expo_type(2)});
		// Move assignment.
		t = std::move(t_copy1);
		BOOST_CHECK_EQUAL(t.m_cf,Cf(1));
		BOOST_CHECK(t.m_key == key_type{expo_type(2)});
		// Generic constructor.
		typedef poisson_series_term<float> other_term_type;
		symbol_set other_ed;
		other_ed.add("x");
		other_term_type ot{float(7),key_type{expo_type(2)}};
		term_type t_from_ot(Cf(ot.m_cf),key_type(ot.m_key,ed));
		BOOST_CHECK_EQUAL(t_from_ot.m_cf,Cf(float(7)));
		BOOST_CHECK(t_from_ot.m_key == key_type{expo_type(2)});
		// Type trait check.
		BOOST_CHECK((std::is_constructible<term_type,Cf,key_type>::value));
		BOOST_CHECK((!std::is_constructible<term_type,Cf,std::string>::value));
		BOOST_CHECK((!std::is_constructible<term_type,std::string,key_type,int>::value));
	}
};

BOOST_AUTO_TEST_CASE(poisson_series_term_constructor_test)
{
	environment env;
	boost::mpl::for_each<cf_types>(constructor_tester());
}

struct multiplication_tester
{
	template <typename Cf>
	void operator()(const Cf &)
	{
		typedef poisson_series_term<Cf> term_type;
		BOOST_CHECK(term_is_multipliable<term_type>::value);
		typedef typename term_type::key_type key_type;
		typedef typename key_type::value_type expo_type;
		symbol_set ed;
		ed.add("x");
		term_type t1, t2;
		t1.m_cf = Cf(2);
		t1.m_key = key_type{expo_type(2)};
		t2.m_cf = Cf(3);
		t2.m_key = key_type{expo_type(3)};
		std::tuple<term_type,term_type> retval;
		t1.multiply(retval,t2,ed);
		BOOST_CHECK_EQUAL(std::get<0u>(retval).m_cf,(t1.m_cf * t2.m_cf) / 2);
		BOOST_CHECK_EQUAL(std::get<1u>(retval).m_cf,(t1.m_cf * t2.m_cf) / 2);
		BOOST_CHECK_EQUAL(std::get<0u>(retval).m_key.get_int(),expo_type(5));
		BOOST_CHECK_EQUAL(std::get<1u>(retval).m_key.get_int(),expo_type(1));
		BOOST_CHECK(std::get<0u>(retval).m_key.get_flavour());
		BOOST_CHECK(std::get<1u>(retval).m_key.get_flavour());
		t1.m_key.set_flavour(false);
		t1.multiply(retval,t2,ed);
		BOOST_CHECK_EQUAL(std::get<0u>(retval).m_cf,(t1.m_cf * t2.m_cf) / 2);
		BOOST_CHECK_EQUAL(std::get<1u>(retval).m_cf,-((t1.m_cf * t2.m_cf) / 2));
		BOOST_CHECK_EQUAL(std::get<0u>(retval).m_key.get_int(),expo_type(5));
		BOOST_CHECK_EQUAL(std::get<1u>(retval).m_key.get_int(),expo_type(1));
		BOOST_CHECK(!std::get<0u>(retval).m_key.get_flavour());
		BOOST_CHECK(!std::get<1u>(retval).m_key.get_flavour());
		t2.m_key.set_flavour(false);
		t1.multiply(retval,t2,ed);
		BOOST_CHECK_EQUAL(std::get<0u>(retval).m_cf,-((t1.m_cf * t2.m_cf) / 2));
		BOOST_CHECK_EQUAL(std::get<1u>(retval).m_cf,(t1.m_cf * t2.m_cf) / 2);
		BOOST_CHECK_EQUAL(std::get<0u>(retval).m_key.get_int(),expo_type(5));
		BOOST_CHECK_EQUAL(std::get<1u>(retval).m_key.get_int(),expo_type(1));
		BOOST_CHECK(std::get<0u>(retval).m_key.get_flavour());
		BOOST_CHECK(std::get<1u>(retval).m_key.get_flavour());
		t1.m_key.set_flavour(true);
		t1.multiply(retval,t2,ed);
		BOOST_CHECK_EQUAL(std::get<0u>(retval).m_cf,(t1.m_cf * t2.m_cf) / 2);
		BOOST_CHECK_EQUAL(std::get<1u>(retval).m_cf,(t1.m_cf * t2.m_cf) / 2);
		BOOST_CHECK_EQUAL(std::get<0u>(retval).m_key.get_int(),expo_type(5));
		BOOST_CHECK_EQUAL(std::get<1u>(retval).m_key.get_int(),expo_type(1));
		BOOST_CHECK(!std::get<0u>(retval).m_key.get_flavour());
		BOOST_CHECK(!std::get<1u>(retval).m_key.get_flavour());
		typedef poisson_series_term<polynomial<real,short>> other_term_type;
		std::tuple<other_term_type,other_term_type> other_retval;
		symbol_set other_ed;
		other_ed.add("x");
		other_term_type t4;
		t4.m_cf = polynomial<real,short>(2);
		t4.m_key = key_type{expo_type(2)};
		t4.multiply(other_retval,t2,other_ed);
		BOOST_CHECK_EQUAL(std::get<0u>(other_retval).m_cf,(t4.m_cf * t2.m_cf) / 2);
		BOOST_CHECK_EQUAL(std::get<1u>(other_retval).m_cf,(t4.m_cf * t2.m_cf) / 2);
		BOOST_CHECK_EQUAL(std::get<0u>(other_retval).m_key.get_int(),expo_type(5));
		BOOST_CHECK_EQUAL(std::get<1u>(other_retval).m_key.get_int(),expo_type(1));
		BOOST_CHECK(!std::get<0u>(other_retval).m_key.get_flavour());
		BOOST_CHECK(!std::get<1u>(other_retval).m_key.get_flavour());
	}
};

BOOST_AUTO_TEST_CASE(poisson_series_term_multiplication_test)
{
	boost::mpl::for_each<cf_types>(multiplication_tester());
}

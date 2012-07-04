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

#include "../src/polynomial_term.hpp"

#define BOOST_TEST_MODULE polynomial_term_test
#include <boost/test/unit_test.hpp>

#include <boost/concept/assert.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <stdexcept>
#include <type_traits>

#include "../src/concepts/multipliable_term.hpp"
#include "../src/concepts/term.hpp"
#include "../src/config.hpp"
#include "../src/detail/series_fwd.hpp"
#include "../src/environment.hpp"
#include "../src/integer.hpp"
#include "../src/polynomial.hpp"
#include "../src/real.hpp"
#include "../src/symbol_set.hpp"
#include "../src/symbol.hpp"

using namespace piranha;

typedef boost::mpl::vector<double,integer,real,polynomial<integer>> cf_types;
typedef boost::mpl::vector<unsigned,integer> expo_types;

typedef polynomial<real> other_cf_type;

struct constructor_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef polynomial_term<Cf,Expo> term_type;
			BOOST_CONCEPT_ASSERT((concept::Term<term_type>));
			typedef typename term_type::key_type key_type;
			symbol_set ed;
			ed.add("x");
			// Default constructor.
			BOOST_CHECK_EQUAL(term_type().m_cf,Cf());
			BOOST_CHECK(term_type().m_key == key_type());
			// Copy constructor.
			term_type t;
			t.m_cf = Cf(1);
			t.m_key = key_type{Expo(2)};
			BOOST_CHECK_EQUAL(term_type(t).m_cf,Cf(1));
			BOOST_CHECK(term_type(t).m_key == key_type{Expo(2)});
			// Move constructor.
			term_type t_copy1(t), t_copy2 = t;
			BOOST_CHECK_EQUAL(term_type(std::move(t_copy1)).m_cf,Cf(1));
			BOOST_CHECK(term_type(std::move(t_copy2)).m_key == key_type{Expo(2)});
			// Copy assignment.
			t_copy1 = t;
			BOOST_CHECK_EQUAL(t_copy1.m_cf,Cf(1));
			BOOST_CHECK(t_copy1.m_key == key_type{Expo(2)});
			// Move assignment.
			t = std::move(t_copy1);
			BOOST_CHECK_EQUAL(t.m_cf,Cf(1));
			BOOST_CHECK(t.m_key == key_type{Expo(2)});
			// Generic constructor.
			typedef polynomial_term<float,Expo> other_term_type;
			symbol_set other_ed;
			other_ed.add("x");
			other_term_type ot{float(7),key_type{Expo(2)}};
			term_type t_from_ot(Cf(ot.m_cf),key_type(ot.m_key,ed));
			BOOST_CHECK_EQUAL(t_from_ot.m_cf,Cf(float(7)));
			BOOST_CHECK(t_from_ot.m_key == key_type{Expo(2)});
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(polynomial_term_constructor_test)
{
	environment env;
	boost::mpl::for_each<cf_types>(constructor_tester());
}

struct multiplication_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef polynomial_term<Cf,Expo> term_type;
			BOOST_CONCEPT_ASSERT((concept::MultipliableTerm<term_type>));
			typedef typename term_type::key_type key_type;
			symbol_set ed;
			ed.add("x");
			term_type t1, t2, t3;
			t1.m_cf = Cf(2);
			t1.m_key = key_type{Expo(2)};
			t2.m_cf = Cf(3);
			t2.m_key = key_type{Expo(3)};
			t1.multiply(t3,t2,ed);
			BOOST_CHECK_EQUAL(t3.m_cf,t1.m_cf * t2.m_cf);
			BOOST_CHECK_EQUAL(t3.m_key[0],Expo(5));
			typedef polynomial_term<other_cf_type,Expo> other_term_type;
			symbol_set other_ed;
			other_ed.add("x");
			other_term_type t4, t5;
			t4.m_cf = other_cf_type(2);
			t4.m_key = key_type{Expo(2)};
			t4.multiply(t5,t2,other_ed);
			BOOST_CHECK_EQUAL(t5.m_cf,t4.m_cf * t2.m_cf);
			BOOST_CHECK_EQUAL(t5.m_key[0],Expo(5));
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(polynomial_term_multiplication_test)
{
	boost::mpl::for_each<cf_types>(multiplication_tester());
}

struct partial_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef polynomial_term<Cf,Expo> term_type;
			typedef typename term_type::key_type key_type;
			symbol_set ed;
			term_type t1;
			t1.m_cf = Cf(2);
			t1.m_key = key_type{Expo(2)};
			BOOST_CHECK_THROW(t1.partial(symbol("x"),ed),std::invalid_argument);
			ed.add("x");
			auto p_res = t1.partial(symbol("x"),ed);
			BOOST_CHECK_EQUAL(p_res.size(),1u);
			BOOST_CHECK(p_res[0u].m_cf == Cf(2) * Expo(2));
			BOOST_CHECK(p_res[0u].m_key.size() == 1u);
			BOOST_CHECK(p_res[0u].m_key[0u] == Expo(1));
			p_res = t1.partial(symbol("y"),ed);
			BOOST_CHECK(p_res.empty());
			t1.m_key = key_type{Expo(0)};
			p_res = t1.partial(symbol("x"),ed);
			BOOST_CHECK(p_res.empty());
			t1.m_key = key_type{Expo(2),Expo(3)};
			ed.add("y");
			p_res = t1.partial(symbol("y"),ed);
			BOOST_CHECK_EQUAL(p_res.size(),1u);
			BOOST_CHECK(p_res[0u].m_cf == Cf(2) * Expo(3));
			BOOST_CHECK(p_res[0u].m_key.size() == 2u);
			BOOST_CHECK(p_res[0u].m_key[0u] == Expo(2));
			BOOST_CHECK(p_res[0u].m_key[1u] == Expo(2));
			t1.m_cf = Cf(0);
			p_res = t1.partial(symbol("y"),ed);
			BOOST_CHECK_EQUAL(p_res.size(),1u);
			BOOST_CHECK(p_res[0u].m_cf == Cf(0));
			BOOST_CHECK(p_res[0u].m_key.size() == 2u);
			BOOST_CHECK(p_res[0u].m_key[0u] == Expo(2));
			BOOST_CHECK(p_res[0u].m_key[1u] == Expo(2));
		}
	};
	template <typename Cf>
	void operator()(const Cf &, typename std::enable_if<!std::is_base_of<detail::series_tag,Cf>::value>::type * = piranha_nullptr)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
	template <typename Cf>
	void operator()(const Cf &, typename std::enable_if<std::is_base_of<detail::series_tag,Cf>::value>::type * = piranha_nullptr)
	{
		typedef polynomial_term<Cf,int> term_type;
		typedef typename term_type::key_type key_type;
		symbol_set ed;
		ed.add("x");
		term_type t1;
		t1.m_cf = Cf{"x"};
		t1.m_key = key_type{2};
		auto p_res = t1.partial(symbol("x"),ed);
		BOOST_CHECK_EQUAL(p_res.size(),2u);
		BOOST_CHECK(p_res[0u].m_cf == Cf(1));
		BOOST_CHECK(p_res[0u].m_key == t1.m_key);
		BOOST_CHECK(p_res[1u].m_cf == t1.m_cf * 2);
		BOOST_CHECK(p_res[1u].m_key == key_type{1});
		t1.m_key = key_type{0};
		p_res = t1.partial(symbol("x"),ed);
		BOOST_CHECK_EQUAL(p_res.size(),1u);
		BOOST_CHECK(p_res[0u].m_cf == Cf(1));
		BOOST_CHECK(p_res[0u].m_key == t1.m_key);
	}
};

BOOST_AUTO_TEST_CASE(polynomial_term_partial_test)
{
	boost::mpl::for_each<cf_types>(partial_tester());
}

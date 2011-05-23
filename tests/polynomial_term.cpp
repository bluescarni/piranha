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

#include "../src/concepts/multipliable_term.hpp"
#include "../src/concepts/term.hpp"
#include "../src/echelon_descriptor.hpp"
#include "../src/integer.hpp"
#include "../src/numerical_coefficient.hpp"

using namespace piranha;

typedef boost::mpl::vector<numerical_coefficient<double>,numerical_coefficient<integer>> cf_types;
typedef boost::mpl::vector<unsigned,integer> expo_types;

typedef long double other_type;

struct constructor_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef polynomial_term<Cf,Expo> term_type;
			typedef echelon_descriptor<term_type> ed_type;
			BOOST_CONCEPT_ASSERT((concept::Term<term_type>));
			typedef typename term_type::key_type key_type;
			ed_type ed;
			ed.template add_symbol<term_type>("x");
			// Default constructor.
			BOOST_CHECK_EQUAL(term_type().m_cf.get_value(),Cf().get_value());
			BOOST_CHECK_EQUAL(term_type().m_key,key_type());
			// Copy constructor.
			term_type t;
			t.m_cf = Cf(1,ed);
			t.m_key = key_type{Expo(2)};
			BOOST_CHECK_EQUAL(term_type(t).m_cf.get_value(),Cf(1,ed).get_value());
			BOOST_CHECK_EQUAL(term_type(t).m_key,key_type{Expo(2)});
			// Move constructor.
			term_type t_copy1(t), t_copy2 = t;
			BOOST_CHECK_EQUAL(term_type(std::move(t_copy1)).m_cf.get_value(),Cf(1,ed).get_value());
			BOOST_CHECK_EQUAL(term_type(std::move(t_copy2)).m_key,key_type{Expo(2)});
			// Copy assignment.
			t_copy1 = t;
			BOOST_CHECK_EQUAL(t_copy1.m_cf.get_value(),Cf(1,ed).get_value());
			BOOST_CHECK_EQUAL(t_copy1.m_key,key_type{Expo(2)});
			// Move assignment.
			t = std::move(t_copy1);
			BOOST_CHECK_EQUAL(t.m_cf.get_value(),Cf(1,ed).get_value());
			BOOST_CHECK_EQUAL(t.m_key,key_type{Expo(2)});
			// Generic constructor.
			typedef polynomial_term<numerical_coefficient<float>,Expo> other_term_type;
			typedef echelon_descriptor<other_term_type> other_ed_type;
			other_ed_type other_ed;
			other_ed.template add_symbol<other_term_type>("x");
			other_term_type ot{numerical_coefficient<float>(7,other_ed),key_type{Expo(2)}};
			term_type t_from_ot(Cf(ot.m_cf,ed),key_type(ot.m_key,ed.template get_args<term_type>()));
			BOOST_CHECK_EQUAL(t_from_ot.m_cf.get_value(),Cf(float(7),ed).get_value());
			BOOST_CHECK_EQUAL(t_from_ot.m_key,key_type{Expo(2)});
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
			typedef echelon_descriptor<term_type> ed_type;
			BOOST_CONCEPT_ASSERT((concept::MultipliableTerm<term_type>));
			typedef typename term_type::key_type key_type;
			ed_type ed;
			ed.template add_symbol<term_type>("x");
			term_type t1, t2;
			t1.m_cf = Cf(2,ed);
			t1.m_key = key_type{Expo(2)};
			t2.m_cf = Cf(3,ed);
			t2.m_key = key_type{Expo(3)};
			auto t3 = t1.multiply(t2,ed);
			BOOST_CHECK_EQUAL(t3.m_cf.get_value(),t1.m_cf.get_value() * t2.m_cf.get_value());
			BOOST_CHECK_EQUAL(t3.m_key[0],Expo(5));
			typedef polynomial_term<numerical_coefficient<other_type>,Expo> other_term_type;
			typedef echelon_descriptor<other_term_type> other_ed_type;
			other_ed_type other_ed;
			other_ed.template add_symbol<other_term_type>("x");
			other_term_type t4;
			t4.m_cf = numerical_coefficient<other_type>(2,other_ed);
			t4.m_key = key_type{Expo(2)};
			auto t5 = t4.multiply(t2,other_ed);
			BOOST_CHECK_EQUAL(t5.m_cf.get_value(),t4.m_cf.get_value() * t2.m_cf.get_value());
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

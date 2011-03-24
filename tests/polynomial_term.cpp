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

#include "../src/concepts/term.hpp"
#include "../src/integer.hpp"
#include "../src/numerical_coefficient.hpp"

using namespace piranha;

typedef boost::mpl::vector<numerical_coefficient<double>,numerical_coefficient<integer>> cf_types;
typedef boost::mpl::vector<unsigned,integer> expo_types;

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
			// Default constructor.
			BOOST_CHECK_EQUAL(term_type().m_cf.get_value(),Cf().get_value());
			BOOST_CHECK_EQUAL(term_type().m_key,key_type());
			// Copy constructor.
			term_type t;
			t.m_cf = Cf(1);
			t.m_key = key_type{Expo(2)};
			BOOST_CHECK_EQUAL(term_type(t).m_cf.get_value(),Cf(1).get_value());
			BOOST_CHECK_EQUAL(term_type(t).m_key,key_type{Expo(2)});
			// Move constructor.
			term_type t_copy1(t), t_copy2 = t;
			BOOST_CHECK_EQUAL(term_type(std::move(t_copy1)).m_cf.get_value(),Cf(1).get_value());
			BOOST_CHECK_EQUAL(term_type(std::move(t_copy2)).m_key,key_type{Expo(2)});
			// Copy assignment.
			t_copy1 = t;
			BOOST_CHECK_EQUAL(t_copy1.m_cf.get_value(),Cf(1).get_value());
			BOOST_CHECK_EQUAL(t_copy1.m_key,key_type{Expo(2)});
			// Move assignment.
			t = std::move(t_copy1);
			BOOST_CHECK_EQUAL(t.m_cf.get_value(),Cf(1).get_value());
			BOOST_CHECK_EQUAL(t.m_key,key_type{Expo(2)});
			// Generic constructor.
			typedef polynomial_term<numerical_coefficient<float>,Expo> other_term_type;
			other_term_type ot{7,key_type{Expo(2)}};
			term_type t_from_ot(ot);
			BOOST_CHECK_EQUAL(t_from_ot.m_cf.get_value(),Cf(float(7)).get_value());
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

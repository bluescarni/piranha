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

#include "../src/base_series.hpp"

#define BOOST_TEST_MODULE base_series_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <stdexcept>

#include "../src/debug_access.hpp"
#include "../src/echelon_descriptor.hpp"
#include "../src/integer.hpp"
#include "../src/numerical_coefficient.hpp"
#include "../src/polynomial_term.hpp"
#include "../src/symbol.hpp"

using namespace piranha;

typedef boost::mpl::vector<numerical_coefficient<double>,numerical_coefficient<integer>> cf_types;
typedef boost::mpl::vector<unsigned,integer> expo_types;

struct construction_tag {};

namespace piranha
{

template <>
struct debug_access<construction_tag>
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef polynomial_term<Cf,Expo> term_type;
			typedef typename term_type::key_type key_type;
			typedef base_series<term_type,void> series_type;
			typedef echelon_descriptor<term_type> ed_type;
			ed_type ed;
			ed.template add_symbol<term_type>(symbol("x"));
			// Default constructor.
			BOOST_CHECK(series_type().empty());
			BOOST_CHECK_EQUAL(series_type().size(), (unsigned)0);
			// Copy constructor.
			series_type s;
			s.insert(term_type(1,key_type{Expo(1)}),ed);
			series_type t(s);
			BOOST_CHECK(*s.m_container.begin() == *t.m_container.begin());
			BOOST_CHECK(s.m_container.begin()->m_cf.get_value() == t.m_container.begin()->m_cf.get_value());
			// Move constructor.
			series_type u{series_type(s)};
			BOOST_CHECK(*u.m_container.begin() == *t.m_container.begin());
			BOOST_CHECK(u.m_container.begin()->m_cf.get_value() == t.m_container.begin()->m_cf.get_value());
			// Copy assignment.
			u = t;
			BOOST_CHECK(*u.m_container.begin() == *t.m_container.begin());
			BOOST_CHECK(u.m_container.begin()->m_cf.get_value() == t.m_container.begin()->m_cf.get_value());
			// Move assignment.
			u = std::move(t);
			BOOST_CHECK(*u.m_container.begin() == *s.m_container.begin());
			BOOST_CHECK(u.m_container.begin()->m_cf.get_value() == s.m_container.begin()->m_cf.get_value());			
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};
}

typedef debug_access<construction_tag> constructor_tester;

BOOST_AUTO_TEST_CASE(base_series_constructor_test)
{
	boost::mpl::for_each<cf_types>(constructor_tester());
}

struct insertion_tag {};

namespace piranha
{
template <>
class debug_access<insertion_tag>
{
	public:
		template <typename Cf>
		struct runner
		{
			template <typename Expo>
			void operator()(const Expo &)
			{
				typedef polynomial_term<Cf,Expo> term_type;
				typedef typename term_type::key_type key_type;
				typedef base_series<term_type,void> series_type;
				typedef echelon_descriptor<term_type> ed_type;
				ed_type ed;
				ed.template add_symbol<term_type>(symbol("x"));
				// Insert well-behaved term.
				series_type s;
				s.insert(term_type(1,key_type{Expo(1)}),ed);
				BOOST_CHECK(!s.empty());
				BOOST_CHECK_EQUAL(s.size(),unsigned(1));
				// Insert incompatible term.
				BOOST_CHECK_THROW(s.insert(term_type(1,key_type{}),ed),std::invalid_argument);
				BOOST_CHECK_EQUAL(s.size(),unsigned(1));
				// Insert ignorable term.
				s.insert(term_type(0,key_type{Expo(1)}),ed);
				BOOST_CHECK_EQUAL(s.size(),unsigned(1));
				// Insert another new term.
				s.insert(term_type(1,key_type{Expo(2)}),ed);
				BOOST_CHECK_EQUAL(s.size(),unsigned(2));
				// Insert equivalent terms.
				s.insert(term_type(2,key_type{Expo(2)}),ed);
				BOOST_CHECK_EQUAL(s.size(),unsigned(2));
				s.template insert<false>(term_type(-2,key_type{Expo(2)}),ed);
				BOOST_CHECK_EQUAL(s.size(),unsigned(2));
				// Insert terms that will prompt for erase of a term.
				s.insert(term_type(-2,key_type{Expo(2)}),ed);
				s.insert(term_type(-2,key_type{Expo(2)}),ed);
				s.insert(term_type(-1,key_type{Expo(2)}),ed);
				BOOST_CHECK_EQUAL(s.size(),unsigned(1));
				// Insertion of different term type.
				typedef polynomial_term<numerical_coefficient<float>,Expo> other_term_type;
				s.insert(other_term_type(1,key_type{Expo(1)}),ed);
				BOOST_CHECK_EQUAL(s.size(),unsigned(1));
				BOOST_CHECK_EQUAL(s.m_container.begin()->m_cf.get_value(),Cf(1).get_value() + float(1));
			}
		};
		template <typename Cf>
		void operator()(const Cf &)
		{
			boost::mpl::for_each<expo_types>(runner<Cf>());
		}
};
}

typedef debug_access<insertion_tag> insertion_tester;

BOOST_AUTO_TEST_CASE(base_series_insertion_test)
{
	boost::mpl::for_each<cf_types>(insertion_tester());
}

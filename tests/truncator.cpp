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

#include "../src/truncator.hpp"

#define BOOST_TEST_MODULE truncator_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <iostream>
#include <type_traits>

#include "../src/config.hpp"
#include "../src/environment.hpp"
#include "../src/integer.hpp"
#include "../src/polynomial_term.hpp"
#include "../src/rational.hpp"
#include "../src/series.hpp"

using namespace piranha;

typedef boost::mpl::vector<double,integer,rational> cf_types;
typedef boost::mpl::vector<int,integer> expo_types;

template <typename Cf, typename Expo>
class g_series_type: public series<polynomial_term<Cf,Expo>,g_series_type<Cf,Expo>>
{
	public:
		typedef series<polynomial_term<Cf,Expo>,g_series_type<Cf,Expo>> base;
		g_series_type() = default;
		g_series_type(const g_series_type &) = default;
		g_series_type(g_series_type &&) = default;
		explicit g_series_type(const char *name):base()
		{
			typedef typename base::term_type term_type;
			// Insert the symbol.
			this->m_symbol_set.add(name);
			// Construct and insert the term.
			this->insert(term_type(Cf(1),typename term_type::key_type{Expo(1)}));
		}
		g_series_type &operator=(const g_series_type &) = default;
		g_series_type &operator=(g_series_type &&other) piranha_noexcept_spec(true)
		{
			if (this != &other) {
				base::operator=(std::move(other));
			}
			return *this;
		}
		PIRANHA_FORWARDING_CTOR(g_series_type,base)
		PIRANHA_FORWARDING_ASSIGNMENT(g_series_type,base)
};

struct concept_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef g_series_type<Cf,Expo> series_type1;
			typedef g_series_type<int,Expo> series_type2;
			typedef truncator<series_type1> truncator_type1;
			typedef truncator<series_type1,series_type2> truncator_type2;
			series_type1 s1;
			series_type2 s2;
			truncator_type1 t1(s1);
			truncator_type2 t2(s1,s2);
			std::cout << t1 << '\n';
			std::cout << t2 << '\n';
			BOOST_CHECK((!t1.is_active()));
			BOOST_CHECK((!t2.is_active()));
			BOOST_CHECK((!truncator_traits<series_type1>::is_sorting));
			BOOST_CHECK((!truncator_traits<series_type2>::is_sorting));
			BOOST_CHECK((!truncator_traits<series_type1>::is_filtering));
			BOOST_CHECK((!truncator_traits<series_type2>::is_filtering));
			BOOST_CHECK((!truncator_traits<series_type1,series_type2>::is_sorting));
			BOOST_CHECK((!truncator_traits<series_type1,series_type1>::is_sorting));
			BOOST_CHECK((!truncator_traits<series_type1,series_type2>::is_filtering));
			BOOST_CHECK((!truncator_traits<series_type1,series_type1>::is_filtering));
			BOOST_CHECK((!truncator_traits<series_type1,series_type2>::is_skipping));
			BOOST_CHECK((!truncator_traits<series_type1,series_type1>::is_skipping));
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(truncator_concept_test)
{
	environment env;
	boost::mpl::for_each<cf_types>(concept_tester());
}

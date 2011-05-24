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

#include "../src/series_multiplier.hpp"

#define BOOST_TEST_MODULE series_multiplier_test
#include <boost/test/unit_test.hpp>

// #include <boost/concept/assert.hpp>
// #include <boost/mpl/for_each.hpp>
// #include <boost/mpl/vector.hpp>
// 
// #include "../src/concepts/multipliable_term.hpp"
// #include "../src/concepts/term.hpp"
#include "../src/debug_access.hpp"
#include "../src/echelon_descriptor.hpp"
#include "../src/integer.hpp"
#include "../src/numerical_coefficient.hpp"
#include "../src/polynomial_term.hpp"
#include "../src/top_level_series.hpp"

using namespace piranha;

// typedef boost::mpl::vector<numerical_coefficient<double>,numerical_coefficient<integer>> cf_types;
// typedef boost::mpl::vector<unsigned,integer> expo_types;
// 
// typedef long double other_type;

template <typename Cf, typename Expo>
class polynomial:
	public top_level_series<polynomial_term<Cf,Expo>,polynomial<Cf,Expo>>
{
		typedef top_level_series<polynomial_term<Cf,Expo>,polynomial<Cf,Expo>> base;
	public:
		polynomial() = default;
		polynomial(const polynomial &) = default;
		polynomial(polynomial &&) = default;
		explicit polynomial(const char *name):base()
		{
			typedef typename base::term_type term_type;
			// Insert the symbol.
			this->m_ed.template add_symbol<term_type>(symbol(name));
			// Construct and insert the term.
			this->insert(term_type(Cf(1,this->m_ed),typename term_type::key_type{Expo(1)}),this->m_ed);
		}
		~polynomial() = default;
		polynomial &operator=(const polynomial &) = default;
		polynomial &operator=(polynomial &&other) piranha_noexcept_spec(true)
		{
			base::operator=(std::move(other));
			return *this;
		}
};

typedef polynomial<numerical_coefficient<double>,int> p_type1;
typedef polynomial<numerical_coefficient<integer>,int> p_type2;

struct operator_tag {};

namespace piranha
{
template <>
struct debug_access<operator_tag>
{
	debug_access()
	{
		p_type1 p1("x"), p2("x");
		echelon_descriptor<p_type1::term_type> ed;
		ed.add_symbol<p_type1::term_type>("x");
		p1.m_container.begin()->m_cf.multiply_by(2,ed);
		p2.m_container.begin()->m_cf.multiply_by(3,ed);
		series_multiplier<p_type1,p_type1> sm1(p1,p2);
		auto retval = sm1(ed);
		BOOST_CHECK(retval.size() == 1u);
		BOOST_CHECK(retval.m_container.begin()->m_key.size() == 1u);
		BOOST_CHECK(retval.m_container.begin()->m_key[0] == 2);
		BOOST_CHECK(retval.m_container.begin()->m_cf.get_value() == (double(3) * double(1)) * (double(2) * double(1)));
		p_type2 p3("x");
		p3.m_container.begin()->m_cf.multiply_by(4,ed);
		series_multiplier<p_type1,p_type2> sm2(p1,p3);
		retval = sm2(ed);
		BOOST_CHECK(retval.size() == 1u);
		BOOST_CHECK(retval.m_container.begin()->m_key.size() == 1u);
		BOOST_CHECK(retval.m_container.begin()->m_key[0] == 2);
		BOOST_CHECK(retval.m_container.begin()->m_cf.get_value() == double((double(2) * double(1)) * (integer(1) * 4)));
	}
};
}

typedef debug_access<operator_tag> operator_tester;

BOOST_AUTO_TEST_CASE(series_multiplier_operator_test)
{
	operator_tester o;
}

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

#include "../src/substitutable_series.hpp"

#define BOOST_TEST_MODULE substitutable_series_test
#include <boost/test/unit_test.hpp>

#include "../src/environment.hpp"
#include "../src/forwarding.hpp"
#include "../src/monomial.hpp"
#include "../src/serialization.hpp"
#include "../src/series.hpp"

using namespace piranha;

template <typename Cf, typename Expo>
class g_series_type: public substitutable_series<series<Cf,monomial<Expo>,g_series_type<Cf,Expo>>,g_series_type<Cf,Expo>>
{
		using base = substitutable_series<series<Cf,monomial<Expo>,g_series_type<Cf,Expo>>,g_series_type<Cf,Expo>>;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
	public:
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
		g_series_type &operator=(g_series_type &&) = default;
		PIRANHA_FORWARDING_CTOR(g_series_type,base)
		PIRANHA_FORWARDING_ASSIGNMENT(g_series_type,base)
};

BOOST_AUTO_TEST_CASE(subs_series_subs_test)
{
	environment env;
}

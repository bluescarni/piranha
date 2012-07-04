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

#include "../src/echelon_size.hpp"

#define BOOST_TEST_MODULE echelon_size_test
#include <boost/test/unit_test.hpp>

#include "../src/base_term.hpp"
#include "../src/config.hpp"
#include "../src/environment.hpp"
#include "../src/monomial.hpp"
#include "../src/series.hpp"

using namespace piranha;

template <typename Cf>
class g_term_type: public base_term<Cf,monomial<int>,g_term_type<Cf>>
{
	public:
		g_term_type() = default;
		g_term_type(const g_term_type &) = default;
		g_term_type(g_term_type &&) = default;
		g_term_type &operator=(const g_term_type &) = default;
		g_term_type &operator=(g_term_type &&other) piranha_noexcept_spec(true)
		{
			base_term<Cf,monomial<int>,g_term_type>::operator=(std::move(other));
			return *this;
		}
		// Needed to satisfy concept checking.
		explicit g_term_type(const Cf &, const monomial<int> &) {}
};

template <typename Term>
class g_series_type: public series<Term,g_series_type<Term>>
{
	public:
		typedef series<Term,g_series_type<Term>> base;
		g_series_type() = default;
		g_series_type(const g_series_type &) = default;
		g_series_type(g_series_type &&) = default;
		g_series_type(const int &n):base(n) {}
		g_series_type &operator=(const g_series_type &) = default;
		g_series_type &operator=(g_series_type &&other) piranha_noexcept_spec(true)
		{
			if (this != &other) {
				base::operator=(std::move(other));
			}
			return *this;
		}
};

BOOST_AUTO_TEST_CASE(type_traits_echelon_size)
{
	environment env;
	BOOST_CHECK_EQUAL(echelon_size<g_term_type<double>>::value,std::size_t(1));
	typedef g_series_type<g_term_type<double>> series_type1;
	typedef g_series_type<g_term_type<series_type1>> series_type2;
	BOOST_CHECK_EQUAL(echelon_size<typename series_type2::term_type>::value,std::size_t(2));
	typedef g_series_type<g_term_type<series_type2>> series_type3;
	BOOST_CHECK_EQUAL(echelon_size<typename series_type3::term_type>::value,std::size_t(3));
}

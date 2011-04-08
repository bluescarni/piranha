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

#include "../src/echelon_position.hpp"

#define BOOST_TEST_MODULE echelon_position_test
#include <boost/test/unit_test.hpp>

#include "../src/base_term.hpp"
#include "../src/config.hpp"
#include "../src/monomial.hpp"
#include "../src/numerical_coefficient.hpp"

using namespace piranha;

// TODO: test with larger echelon sizes once we have coefficient series.

class term_type1: public base_term<numerical_coefficient<double>,monomial<int>,term_type1>
{
	public:
		term_type1() = default;
		term_type1(const term_type1 &) = default;
		term_type1(term_type1 &&) = default;
		term_type1 &operator=(const term_type1 &) = default;
		term_type1 &operator=(term_type1 &&other) piranha_noexcept_spec(true)
		{
			base_term<numerical_coefficient<double>,monomial<int>,term_type1>::operator=(std::move(other));
			return *this;
		}
		// Needed to satisfy concept checking.
		explicit term_type1(const numerical_coefficient<double> &, const monomial<int> &) {}
};

BOOST_AUTO_TEST_CASE(type_traits_echelon_position)
{
	BOOST_CHECK_EQUAL((echelon_position<term_type1,term_type1>::value),std::size_t(0));
}

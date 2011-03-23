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

#include "../src/monomial.hpp"

#define BOOST_TEST_MODULE monomial_test
#include <boost/test/unit_test.hpp>

#include <boost/concept/assert.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <vector>

#include "../src/concepts/key.hpp"
#include "../src/integer.hpp"
#include "../src/symbol.hpp"

using namespace piranha;

typedef boost::mpl::vector<unsigned,integer> expo_types;

// Constructors, assignments and element access.
struct constructor_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef monomial<T> monomial_type;
		BOOST_CONCEPT_ASSERT((concept::Key<monomial_type>));
		monomial_type m0;
		BOOST_CHECK_NO_THROW(monomial_type{});
		BOOST_CHECK_NO_THROW(monomial_type(monomial_type{}));
		BOOST_CHECK_NO_THROW(monomial_type(m0));
		// From init list.
		monomial_type m1{T(0),T(1),T(2),T(3)};
		BOOST_CHECK_EQUAL(m1.size(),static_cast<decltype(m1.size())>(4));
		for (typename monomial_type::size_type i = 0; i < m1.size(); ++i) {
			BOOST_CHECK_EQUAL(m1[i],i);
			BOOST_CHECK_NO_THROW(m1[i] = i + 1);
			BOOST_CHECK_EQUAL(m1[i],i + 1);
		}
		BOOST_CHECK_NO_THROW(m0 = m1);
		BOOST_CHECK_NO_THROW(m0 = std::move(m1));
	}
};

BOOST_AUTO_TEST_CASE(monomial_constructor_test)
{
	boost::mpl::for_each<expo_types>(constructor_tester());
}

struct hash_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef monomial<T> monomial_type;
		monomial_type m0;
		BOOST_CHECK_EQUAL(m0.hash(),std::size_t());
		BOOST_CHECK_EQUAL(m0.hash(),std::hash<monomial_type>()(m0));
		monomial_type m1{T(0),T(1),T(2),T(3)};
		BOOST_CHECK_EQUAL(m1.hash(),std::hash<monomial_type>()(m1));
	}
};

BOOST_AUTO_TEST_CASE(monomial_hash_test)
{
	boost::mpl::for_each<expo_types>(hash_tester());
}

struct compatibility_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef monomial<T> monomial_type;
		monomial_type m0;
		BOOST_CHECK(m0.is_compatible(std::vector<symbol>{}));
		monomial_type m1 = {T(0),T(1)};
		BOOST_CHECK(!m1.is_compatible(std::vector<symbol>(1,symbol{"foobarize"})));
		monomial_type m2 = {T(0)};
		BOOST_CHECK(m2.is_compatible(std::vector<symbol>(1,symbol{"foobarize"})));
	}
};

BOOST_AUTO_TEST_CASE(monomial_compatibility_test)
{
	boost::mpl::for_each<expo_types>(compatibility_tester());
}

struct ignorability_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef monomial<T> monomial_type;
		monomial_type m0;
		BOOST_CHECK(!m0.is_ignorable(std::vector<symbol>{}));
		monomial_type m1 = {T(0)};
		BOOST_CHECK(!m1.is_ignorable(std::vector<symbol>(1,symbol{"foobarize"})));
	}
};

BOOST_AUTO_TEST_CASE(monomial_ignorability_test)
{
	boost::mpl::for_each<expo_types>(ignorability_tester());
}

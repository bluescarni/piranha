/* Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)

This file is part of the Piranha library.

The Piranha library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The Piranha library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the Piranha library.  If not,
see https://www.gnu.org/licenses/. */

#include "../src/series.hpp"

#define BOOST_TEST_MODULE series_05_test
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <stdexcept>
#include <type_traits>

#include "../src/environment.hpp"
#include "../src/exceptions.hpp"
#include "../src/forwarding.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/serialization.hpp"
#include "../src/type_traits.hpp"


#include "../src/polynomial.hpp"

using namespace piranha;

template <typename Cf, typename Expo>
class g_series_type: public series<Cf,monomial<Expo>,g_series_type<Cf,Expo>>
{
	public:
		template <typename Cf2>
		using rebind = g_series_type<Cf2,Expo>;
		typedef series<Cf,monomial<Expo>,g_series_type<Cf,Expo>> base;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
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

// Mock coefficient.
struct mock_cf
{
	mock_cf();
	mock_cf(const int &);
	mock_cf(const mock_cf &);
	mock_cf(mock_cf &&) noexcept;
	mock_cf &operator=(const mock_cf &);
	mock_cf &operator=(mock_cf &&) noexcept;
	friend std::ostream &operator<<(std::ostream &, const mock_cf &);
	mock_cf operator-() const;
	bool operator==(const mock_cf &) const;
	bool operator!=(const mock_cf &) const;
	mock_cf &operator+=(const mock_cf &);
	mock_cf &operator-=(const mock_cf &);
	mock_cf operator+(const mock_cf &) const;
	mock_cf operator-(const mock_cf &) const;
	mock_cf &operator*=(const mock_cf &);
	mock_cf operator*(const mock_cf &) const;
};

// A few extra tests for division after the recent changes in implementation.
BOOST_AUTO_TEST_CASE(series_division_tests)
{
	environment env;
	{
	using s_type = g_series_type<integer,int>;
	using s_type2 = g_series_type<double,int>;
	BOOST_CHECK((is_divisible<s_type>::value));
	BOOST_CHECK((!is_divisible<g_series_type<mock_cf,int>>::value));
	s_type x{"x"}, y{"y"};
	BOOST_CHECK((std::is_same<s_type,decltype(s_type{} / s_type{})>::value));
	BOOST_CHECK_EQUAL(s_type{4} / s_type{-3},-1);
	BOOST_CHECK((s_type{} / s_type{-3}).empty());
	BOOST_CHECK_THROW(s_type{4} / s_type{},zero_division_error);
	BOOST_CHECK_THROW(x/x,std::invalid_argument);
	BOOST_CHECK_THROW(x/y,std::invalid_argument);
	BOOST_CHECK_THROW(s_type{1}/y,std::invalid_argument);
	BOOST_CHECK_THROW(s_type{1}/x,std::invalid_argument);
	BOOST_CHECK((s_type{} / x).empty());
	BOOST_CHECK((is_divisible<s_type2,s_type>::value));
	BOOST_CHECK((std::is_same<s_type2,decltype(s_type2{} / s_type{})>::value));
	BOOST_CHECK_EQUAL(s_type2{4} / s_type{-3},-4./3.);
	}
}

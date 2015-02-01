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

#include "../src/series.hpp"

#define BOOST_TEST_MODULE series_03_test
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <stdexcept>

#include "../src/environment.hpp"
#include "../src/forwarding.hpp"
#include "../src/monomial.hpp"
#include "../src/serialization.hpp"
#include "../src/symbol_set.hpp"

using namespace piranha;

// Mock coefficient.
struct mock_cf
{
	mock_cf() = default;
	mock_cf(const int &) {}
	mock_cf(const mock_cf &) = default;
	mock_cf(mock_cf &&) = default;
	mock_cf &operator=(const mock_cf &) = default;
	mock_cf &operator=(mock_cf &&) noexcept
	{
		return *this;
	}
	friend std::ostream &operator<<(std::ostream &, const mock_cf &);
	mock_cf operator-() const;
	bool operator==(const mock_cf &) const
	{
		return true;
	}
	bool operator!=(const mock_cf &) const
	{
		return false;
	}
	mock_cf &operator+=(const mock_cf &);
	mock_cf &operator-=(const mock_cf &);
	mock_cf operator+(const mock_cf &) const;
	mock_cf operator-(const mock_cf &) const;
	mock_cf &operator*=(const mock_cf &);
	mock_cf operator*(const mock_cf &) const;
};

// Normal usage via forwarding.
template <typename Cf, typename Expo>
class g_series_type: public series<Cf,monomial<Expo>,g_series_type<Cf,Expo>>
{
		using base = series<Cf,monomial<Expo>,g_series_type<Cf,Expo>>;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
	public:
		g_series_type() = default;
		g_series_type(const g_series_type &) = default;
		g_series_type(g_series_type &&) = default;
		g_series_type &operator=(const g_series_type &) = default;
		g_series_type &operator=(g_series_type &&) = default;
		PIRANHA_FORWARDING_CTOR(g_series_type,base)
		PIRANHA_FORWARDING_ASSIGNMENT(g_series_type,base)
};

// Copy ctor of base called explicitly: if the generic ctor is invoked, this should give a linker error
// as it will need the +/- operators from mock_cf which are declared but not defined.
template <typename Cf, typename Expo>
class g_series_type2: public series<Cf,monomial<Expo>,g_series_type2<Cf,Expo>>
{
		using base = series<Cf,monomial<Expo>,g_series_type2<Cf,Expo>>;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
	public:
		g_series_type2() = default;
		g_series_type2(const g_series_type2 &other):base(other) {}
		g_series_type2(g_series_type2 &&) = default;
		g_series_type2 &operator=(const g_series_type2 &) = default;
		g_series_type2 &operator=(g_series_type2 &&) = default;
		PIRANHA_FORWARDING_CTOR(g_series_type2,base)
		PIRANHA_FORWARDING_ASSIGNMENT(g_series_type2,base)
};

// A null toolbox, which will call explicitly the copy ctor of T.
template <typename T>
class null_toolbox: public T
{
		PIRANHA_SERIALIZE_THROUGH_BASE(T)
	public:
		null_toolbox() = default;
		null_toolbox(const null_toolbox &other):T(other) {}
		null_toolbox(null_toolbox &&) = default;
		null_toolbox &operator=(const null_toolbox &) = default;
		null_toolbox &operator=(null_toolbox &&) = default;
		PIRANHA_FORWARDING_CTOR(null_toolbox,T)
		PIRANHA_FORWARDING_ASSIGNMENT(null_toolbox,T)
};

template <typename T>
class null_toolbox2: public T
{
		PIRANHA_SERIALIZE_THROUGH_BASE(T)
	public:
		null_toolbox2() = default;
		null_toolbox2(const null_toolbox2 &) = default;
		null_toolbox2(null_toolbox2 &&) = default;
		null_toolbox2 &operator=(const null_toolbox2 &) = default;
		null_toolbox2 &operator=(null_toolbox2 &&) = default;
		PIRANHA_FORWARDING_CTOR(null_toolbox2,T)
		PIRANHA_FORWARDING_ASSIGNMENT(null_toolbox2,T)
};

// Another series type, using the toolbox this time.
template <typename Cf, typename Expo>
class g_series_type3: public null_toolbox<series<Cf,monomial<Expo>,g_series_type3<Cf,Expo>>>
{
		using base = null_toolbox<series<Cf,monomial<Expo>,g_series_type3<Cf,Expo>>>;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
	public:
		g_series_type3() = default;
		g_series_type3(const g_series_type3 &) = default;
		g_series_type3(g_series_type3 &&) = default;
		g_series_type3 &operator=(const g_series_type3 &) = default;
		g_series_type3 &operator=(g_series_type3 &&) = default;
		PIRANHA_FORWARDING_CTOR(g_series_type3,base)
		PIRANHA_FORWARDING_ASSIGNMENT(g_series_type3,base)
};

template <typename Cf, typename Expo>
class g_series_type4: public null_toolbox2<series<Cf,monomial<Expo>,g_series_type4<Cf,Expo>>>
{
		using base = null_toolbox2<series<Cf,monomial<Expo>,g_series_type4<Cf,Expo>>>;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
	public:
		g_series_type4() = default;
		g_series_type4(const g_series_type4 &other):base(other) {}
		g_series_type4(g_series_type4 &&) = default;
		g_series_type4 &operator=(const g_series_type4 &) = default;
		g_series_type4 &operator=(g_series_type4 &&) = default;
		PIRANHA_FORWARDING_CTOR(g_series_type4,base)
		PIRANHA_FORWARDING_ASSIGNMENT(g_series_type4,base)
};

BOOST_AUTO_TEST_CASE(series_generic_ctor_forwarding_test)
{
	environment env;
	using st0 = g_series_type<mock_cf,int>;
	BOOST_CHECK(is_series<st0>::value);
	// Test we are using copy ctor and copy assignment.
	st0 s0, s1(s0);
	s1 = s0;
	using st1 = g_series_type2<mock_cf,int>;
	BOOST_CHECK(is_series<st1>::value);
	st1 s2, s3(s2);
	s3 = s2;
	using st2 = g_series_type3<mock_cf,int>;
	BOOST_CHECK(is_series<st2>::value);
	st2 s4, s5(s4);
	s5 = s4;
	using st3 = g_series_type4<mock_cf,int>;
	BOOST_CHECK(is_series<st3>::value);
	st3 s6, s7(s6);
	s7 = s6;
}

BOOST_AUTO_TEST_CASE(series_symbol_set_test)
{
	using st0 = g_series_type<double,int>;
	symbol_set ss;
	ss.add("x");
	ss.add("y");
	st0 s;
	s.set_symbol_set(ss);
	BOOST_CHECK(ss == s.get_symbol_set());
	s += 1;
	BOOST_CHECK_THROW(s.set_symbol_set(ss),std::invalid_argument);
}

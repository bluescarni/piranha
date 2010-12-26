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

#include "../src/integer.hpp"

#define BOOST_TEST_MODULE integer_test
#include <boost/test/unit_test.hpp>

#define FUSION_MAX_VECTOR_SIZE 20

#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/numeric/conversion/bounds.hpp>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

const boost::fusion::vector<char,short,int,long,long long,unsigned char,unsigned short,unsigned,unsigned long,unsigned long long,float,double,long double> arithmetic_values(
	(char)-42,(short)-42,-42,-42L,-42LL,
	(unsigned char)42,(unsigned short)42,42U,42UL,42ULL,
	123.456f,-123.456,123.456L
);

const std::vector<std::string> invalid_strings{"-0","+0","01","+1","123f"," 123","123 ","123.56"};

struct check_arithmetic_construction
{
	template <typename T>
	void operator()(const T &value) const
	{
		BOOST_CHECK_EQUAL(static_cast<int>(value),static_cast<int>(piranha::integer(value)));
	}
};

BOOST_AUTO_TEST_CASE(integer_constructors_test)
{
	// Default construction.
	BOOST_CHECK_EQUAL(0,static_cast<int>(piranha::integer()));
	// Construction from arithmetic types.
	boost::fusion::for_each(arithmetic_values,check_arithmetic_construction());
	// Construction from string.
	BOOST_CHECK_EQUAL(123,static_cast<int>(piranha::integer("123")));
	BOOST_CHECK_EQUAL(-123,static_cast<int>(piranha::integer("-123")));
	// Construction from malformed strings.
	std::unique_ptr<piranha::integer> ptr;
	for (std::vector<std::string>::const_iterator it = invalid_strings.begin(); it != invalid_strings.end(); ++it) {
		BOOST_CHECK_THROW(ptr.reset(new piranha::integer(*it)),std::invalid_argument);
	}
	// Copy construction
	piranha::integer i("-30000"), j(i);
	BOOST_CHECK_EQUAL(-30000,static_cast<int>(j));
	// Construction with non-finite floating-point.
	BOOST_CHECK_THROW(ptr.reset(new piranha::integer(std::numeric_limits<float>::infinity())),std::invalid_argument);
	BOOST_CHECK_THROW(ptr.reset(new piranha::integer(std::numeric_limits<double>::infinity())),std::invalid_argument);
	BOOST_CHECK_THROW(ptr.reset(new piranha::integer(std::numeric_limits<long double>::infinity())),std::invalid_argument);
}

struct check_arithmetic_assignment
{
	check_arithmetic_assignment(piranha::integer &i):m_i(i) {}
	template <typename T>
	void operator()(const T &value) const
	{
		m_i = value;
		BOOST_CHECK_EQUAL(static_cast<int>(value),static_cast<int>(m_i));
	}
	piranha::integer &m_i;
};

BOOST_AUTO_TEST_CASE(integer_assignment_test)
{
	piranha::integer i, j;
	// Assignment from arithmetic types.
	boost::fusion::for_each(arithmetic_values,check_arithmetic_assignment(i));
	// Assignment from string.
	i = "123";
	BOOST_CHECK_EQUAL(123,static_cast<int>(i));
	i = "-123";
	BOOST_CHECK_EQUAL(-123,static_cast<int>(i));
	// Assignment from malformed strings.
	for (std::vector<std::string>::const_iterator it = invalid_strings.begin(); it != invalid_strings.end(); ++it) {
		BOOST_CHECK_THROW(i = *it,std::invalid_argument);
	}
	// Copy assignment.
	i = "30000";
	j = i;
	BOOST_CHECK_EQUAL(30000,static_cast<int>(j));
	// Assignment from non-finite floating-point.
	BOOST_CHECK_THROW(j = -std::numeric_limits<float>::infinity(),std::invalid_argument);
	BOOST_CHECK_THROW(j = -std::numeric_limits<double>::infinity(),std::invalid_argument);
	BOOST_CHECK_THROW(j = -std::numeric_limits<long double>::infinity(),std::invalid_argument);
}

struct check_arithmetic_move_construction
{
	template <typename T>
	void operator()(const T &value) const
	{
		piranha::integer i(value), j(std::move(i));
		BOOST_CHECK_EQUAL(static_cast<int>(value),static_cast<int>(j));
		// Revive i.
		i = value;
		BOOST_CHECK_EQUAL(static_cast<int>(value),static_cast<int>(i));
	}
};

struct check_arithmetic_move_assignment
{
	template <typename T>
	void operator()(const T &value) const
	{
		piranha::integer i(value), j;
		j = std::move(i);
		BOOST_CHECK_EQUAL(static_cast<int>(value),static_cast<int>(j));
		// Revive i.
		i = value;
		BOOST_CHECK_EQUAL(static_cast<int>(value),static_cast<int>(i));
	}
};

BOOST_AUTO_TEST_CASE(integer_move_semantics_test)
{
	boost::fusion::for_each(arithmetic_values,check_arithmetic_move_construction());
	boost::fusion::for_each(arithmetic_values,check_arithmetic_move_assignment());
	// Revive via plain assignment.
	{
		piranha::integer i(42), j, k(43);
		j = std::move(i);
		j = k;
		BOOST_CHECK_EQUAL(43,static_cast<int>(j));
	}
	// Revive via move assignment.
	{
		piranha::integer i(42), j, k(43);
		j = std::move(i);
		j = std::move(k);
		BOOST_CHECK_EQUAL(43,static_cast<int>(j));
	}
	// Revive via string assignment.
	{
		piranha::integer i(42), j;
		j = std::move(i);
		j = "42";
		BOOST_CHECK_EQUAL(42,static_cast<int>(j));
	}
}

BOOST_AUTO_TEST_CASE(integer_swap_test)
{
	piranha::integer i(42), j(43);
	i.swap(j);
	BOOST_CHECK_EQUAL(43,static_cast<int>(i));
}

template <typename T>
static inline void inf_conversion_test()
{
	{
		std::ostringstream oss;
		oss << piranha::integer(boost::numeric::bounds<T>::highest());
		std::string tmp(oss.str());
		tmp.append("0000000");
		BOOST_CHECK_EQUAL(static_cast<T>(piranha::integer(tmp)),std::numeric_limits<T>::infinity());
	}
	{
		std::ostringstream oss;
		oss << piranha::integer(boost::numeric::bounds<T>::lowest());
		std::string tmp(oss.str());
		tmp.append("0000000");
		BOOST_CHECK_EQUAL(static_cast<T>(piranha::integer(tmp)),-std::numeric_limits<T>::infinity());
	}
}

BOOST_AUTO_TEST_CASE(integer_conversion_test)
{
	piranha::integer bigint("6456895768945764589283127342389472389472389423799923942394823749238472394872389472389472389748923749223947234892374897");
	BOOST_CHECK_THROW(static_cast<int>(bigint),std::overflow_error);
	piranha::integer max_unsigned(boost::numeric::bounds<unsigned>::highest());
	BOOST_CHECK_THROW(static_cast<int>(max_unsigned),std::overflow_error);
	BOOST_CHECK_NO_THROW(static_cast<unsigned>(max_unsigned));
	// Conversion that will generate infinity.
	inf_conversion_test<float>();
	inf_conversion_test<double>();
	inf_conversion_test<long double>();
	// Implicit conversion to bool.
	piranha::integer true_int(1), false_int(0);
	if (!true_int) {
		BOOST_CHECK_EQUAL(0,1);
	}
	if (false_int) {
		BOOST_CHECK_EQUAL(0,1);
	}
}

BOOST_AUTO_TEST_CASE(integer_stream_test)
{
	{
		std::ostringstream oss;
		std::string tmp = "12843748347394832742398472398472389";
		oss << piranha::integer(tmp);
		BOOST_CHECK_EQUAL(tmp,oss.str());
	}
	{
		std::ostringstream oss;
		std::string tmp = "-2389472323272767078540934";
		oss << piranha::integer(tmp);
		BOOST_CHECK_EQUAL(tmp,oss.str());
	}
	{
		piranha::integer tmp;
		std::stringstream ss;
		ss << "123";
		ss >> tmp;
		BOOST_CHECK_EQUAL(static_cast<int>(tmp),123);
	}
	{
		piranha::integer tmp;
		std::stringstream ss;
		ss << "-30000";
		ss >> tmp;
		BOOST_CHECK_EQUAL(static_cast<int>(tmp),-30000);
	}
}

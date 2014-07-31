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

#include "../src/rational.hpp"

#define BOOST_TEST_MODULE rational_test
#include <boost/test/unit_test.hpp>

#define FUSION_MAX_VECTOR_SIZE 20

#include <algorithm>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/integer_traits.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/bounds.hpp>
#include <complex>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../src/environment.hpp"
#include "../src/integer.hpp"
#include "../src/exceptions.hpp"
#include "../src/math.hpp"
#include "../src/print_coefficient.hpp"
#include "../src/print_tex_coefficient.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

const boost::fusion::vector<char,signed char,short,int,long,long long,unsigned char,unsigned short,unsigned,unsigned long,unsigned long long,float,double> arithmetic_values(
	(char)42,(signed char)42,(short)42,-42,42L,-42LL,
	(unsigned char)42,(unsigned short)42,42U,42UL,42ULL,
	23.456f,-23.456
);

const std::vector<std::string> invalid_strings{"-0","+0","01","+1","123f"," 123","123 ","123.56","123 / 4","212/","/332"};

static inline rational get_big_int()
{
	std::string tmp = boost::lexical_cast<std::string>(boost::integer_traits<unsigned long long>::const_max);
	tmp += "123456789";
	return rational(tmp);
}

struct check_arithmetic_construction
{
	template <typename T>
	void operator()(const T &value) const
	{
		BOOST_CHECK_EQUAL(static_cast<int>(value),static_cast<int>(rational(value)));
	}
};

BOOST_AUTO_TEST_CASE(rational_constructors_test)
{
	environment env;
	// Default construction.
	BOOST_CHECK_EQUAL(0,static_cast<int>(rational()));
	// Construction from arithmetic types.
	boost::fusion::for_each(arithmetic_values,check_arithmetic_construction());
	// Construction from floating point.
	BOOST_CHECK(static_cast<float>(rational(1.23f)) == 1.23f);
	BOOST_CHECK(static_cast<double>(rational(1.23)) == 1.23);
	// Construction from integer.
	BOOST_CHECK_EQUAL(42,static_cast<int>(rational(integer(42))));
	BOOST_CHECK_EQUAL(-42,static_cast<int>(rational(integer(-42))));
	// Construction from string.
	BOOST_CHECK_EQUAL(123,static_cast<int>(rational("123")));
	BOOST_CHECK_EQUAL(-123,static_cast<int>(rational("-123")));
	BOOST_CHECK_EQUAL(128,static_cast<int>(rational("128/1")));
	BOOST_CHECK_EQUAL(-128,static_cast<int>(rational("128/-1")));
	BOOST_CHECK_EQUAL(128,static_cast<int>(rational("-128/-1")));
	BOOST_CHECK_EQUAL(128,static_cast<int>(rational("256/2")));
	BOOST_CHECK_EQUAL(-128,static_cast<int>(rational("256/-2")));
	BOOST_CHECK_THROW(rational{"3/0"},zero_division_error);
	BOOST_CHECK_THROW(rational{"-3/0"},zero_division_error);
	BOOST_CHECK_THROW(rational{"0/0"},zero_division_error);
	// Construction from malformed strings.
	std::unique_ptr<rational> ptr;
	for (std::vector<std::string>::const_iterator it = invalid_strings.begin(); it != invalid_strings.end(); ++it) {
		BOOST_CHECK_THROW(ptr.reset(new rational(*it)),std::invalid_argument);
	}
	// Copy construction
	rational i("-30"), j(i);
	BOOST_CHECK_EQUAL(-30,static_cast<int>(j));
	// Large value.
	rational i2(get_big_int()), j2(i2);
	BOOST_CHECK_EQUAL(i2,j2);
	// Move construction.
	rational i3("-30"), j3(std::move(i3));
	BOOST_CHECK(static_cast<int>(j3) == -30);
	rational i4(get_big_int()), j4(std::move(i4));
	BOOST_CHECK(j4 == i2);
	// Construction with non-finite floating-point.
	if (std::numeric_limits<float>::has_infinity && std::numeric_limits<double>::has_infinity) {
		BOOST_CHECK_THROW(ptr.reset(new rational(std::numeric_limits<float>::infinity())),std::invalid_argument);
		BOOST_CHECK_THROW(ptr.reset(new rational(std::numeric_limits<double>::infinity())),std::invalid_argument);
	}
	if (std::numeric_limits<float>::has_quiet_NaN) {
		BOOST_CHECK_THROW(ptr.reset(new rational(std::numeric_limits<float>::quiet_NaN())),std::invalid_argument);
	}
	if (std::numeric_limits<double>::has_quiet_NaN) {
		BOOST_CHECK_THROW(ptr.reset(new rational(std::numeric_limits<double>::quiet_NaN())),std::invalid_argument);
	}
	// Construction from numerator and denominator.
	BOOST_CHECK(static_cast<int>(rational(9,3)) == 3);
	BOOST_CHECK(static_cast<int>(rational(-9,3)) == -3);
	BOOST_CHECK(static_cast<int>(rational(9,-3)) == -3);
	BOOST_CHECK(static_cast<int>(rational(-9,-3)) == 3);
	BOOST_CHECK(static_cast<unsigned>(rational(9u,3u)) == 3u);
	BOOST_CHECK(static_cast<unsigned long long>(rational(9ull,3ull)) == 3ull);
	BOOST_CHECK(static_cast<long long>(rational(9ll,-3ll)) == -3ll);
	BOOST_CHECK(static_cast<int>(rational(integer(-9),integer(3))) == -3);
	BOOST_CHECK_THROW(ptr.reset(new rational(1,0)),zero_division_error);
	BOOST_CHECK_THROW(ptr.reset(new rational(integer(0),integer(0))),zero_division_error);
}

struct check_arithmetic_assignment
{
	check_arithmetic_assignment(rational &i):m_i(i) {}
	template <typename T>
	void operator()(const T &value) const
	{
		m_i = value;
		BOOST_CHECK_EQUAL(static_cast<int>(value),static_cast<int>(m_i));
	}
	rational &m_i;
};

BOOST_AUTO_TEST_CASE(rational_assignment_test)
{
	rational i, j;
	// Assignment from arithmetic types.
	boost::fusion::for_each(arithmetic_values,check_arithmetic_assignment(i));
	// Assignment from string.
	i = "123";
	BOOST_CHECK_EQUAL(123,static_cast<int>(i));
	i = std::string("-123");
	BOOST_CHECK_EQUAL(-123,static_cast<int>(i));
	// Assignment from malformed strings.
	for (std::vector<std::string>::const_iterator it = invalid_strings.begin(); it != invalid_strings.end(); ++it) {
		BOOST_CHECK_THROW(i = *it,std::invalid_argument);
	}
	// Copy assignment.
	i = "30000/2";
	j = i;
	BOOST_CHECK_EQUAL(15000,static_cast<int>(j));
	// Assignment from non-finite floating-point.
	if (std::numeric_limits<float>::has_infinity && std::numeric_limits<double>::has_infinity) {
		BOOST_CHECK_THROW(j = -std::numeric_limits<float>::infinity(),std::invalid_argument);
		BOOST_CHECK_THROW(j = std::numeric_limits<double>::infinity(),std::invalid_argument);
	}
	if (std::numeric_limits<float>::has_quiet_NaN) {
		BOOST_CHECK_THROW(j = std::numeric_limits<float>::quiet_NaN(),std::invalid_argument);
	}
	if (std::numeric_limits<double>::has_quiet_NaN) {
		BOOST_CHECK_THROW(j = std::numeric_limits<double>::quiet_NaN(),std::invalid_argument);
	}
	// Assignment from integer.
	i = integer(100);
	BOOST_CHECK_EQUAL(100,static_cast<int>(i));
}

struct check_arithmetic_move_construction
{
	template <typename T>
	void operator()(const T &value) const
	{
		rational i(value), j(std::move(i));
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
		rational i(value), j;
		j = std::move(i);
		BOOST_CHECK_EQUAL(static_cast<int>(value),static_cast<int>(j));
		// Revive i.
		i = value;
		BOOST_CHECK_EQUAL(static_cast<int>(value),static_cast<int>(i));
	}
};

BOOST_AUTO_TEST_CASE(rational_move_semantics_test)
{
	boost::fusion::for_each(arithmetic_values,check_arithmetic_move_construction());
	boost::fusion::for_each(arithmetic_values,check_arithmetic_move_assignment());
	// Revive via plain assignment.
	{
		rational i(42), j, k(43);
		j = std::move(i);
		j = k;
		BOOST_CHECK_EQUAL(43,static_cast<int>(j));
	}
	// Revive via move assignment.
	{
		rational i(42), j, k(43);
		j = std::move(i);
		j = std::move(k);
		BOOST_CHECK_EQUAL(43,static_cast<int>(j));
	}
	// Revive via string assignment.
	{
		rational i(42), j;
		j = std::move(i);
		j = "42";
		BOOST_CHECK_EQUAL(42,static_cast<int>(j));
	}
}

BOOST_AUTO_TEST_CASE(rational_swap_test)
{
	rational i(42), j(43), k(10,3);
	i.swap(j);
	BOOST_CHECK_EQUAL(43,static_cast<int>(i));
	i.swap(k);
	BOOST_CHECK_EQUAL(3,static_cast<int>(i));
	k = get_big_int();
	std::swap(i,k);
	BOOST_CHECK_EQUAL(3,static_cast<int>(k));
	k.swap(i);
	BOOST_CHECK_EQUAL(3,static_cast<int>(i));
}

template <typename T>
static inline void inf_conversion_test()
{
	if (!std::numeric_limits<T>::has_infinity) {
		return;
	}
	{
		std::ostringstream oss;
		oss << rational(boost::numeric::bounds<T>::highest());
		std::string tmp(oss.str());
		tmp.append("0000000");
		BOOST_CHECK_EQUAL(static_cast<T>(rational(tmp)),std::numeric_limits<T>::infinity());
	}
	{
		std::ostringstream oss;
		oss << rational(boost::numeric::bounds<T>::lowest());
		std::string tmp(oss.str());
		tmp.append("0000000");
		BOOST_CHECK_EQUAL(static_cast<T>(rational(tmp)),-std::numeric_limits<T>::infinity());
	}
}

BOOST_AUTO_TEST_CASE(rational_conversion_test)
{
	rational bigint(get_big_int());
	BOOST_CHECK_THROW((void)static_cast<int>(bigint),std::overflow_error);
	rational max_unsigned(boost::numeric::bounds<unsigned>::highest());
	BOOST_CHECK_THROW((void)static_cast<int>(max_unsigned),std::overflow_error);
	BOOST_CHECK_NO_THROW((void)static_cast<unsigned>(max_unsigned));
	// Conversion that will generate infinity.
	inf_conversion_test<float>();
	inf_conversion_test<double>();
	// Implicit conversion to bool.
	rational true_int(1), false_int(0);
	if (!true_int) {
		BOOST_CHECK_EQUAL(0,1);
	}
	if (false_int) {
		BOOST_CHECK_EQUAL(0,1);
	}
	// Conversion to integrals.
	BOOST_CHECK(static_cast<integer>(rational(3,2)) == 1);
	BOOST_CHECK(static_cast<int>(rational(-256,3)) == -85);
	BOOST_CHECK(static_cast<unsigned>(rational(256,3)) == 85u);
	BOOST_CHECK_THROW((void)static_cast<unsigned>(rational(-1)),std::overflow_error);
}

struct check_arithmetic_in_place_add
{
	template <typename T>
	void operator()(const T &x) const
	{
		{
			// In-place add, rational on the left.
			rational i(1);
			i += x;
			BOOST_CHECK_EQUAL(static_cast<int>(x) + 1, static_cast<int>(i));
		}
		{
			// In-place add, rational on the right.
			T y(x);
			rational i(1);
			y += i;
			BOOST_CHECK_EQUAL(x + 1, y);
			y += std::move(i);
			BOOST_CHECK_EQUAL(x + 2, y);
		}
	}
};

struct check_arithmetic_binary_add
{
	template <typename T>
	void operator()(const T &x) const
	{
		rational i(1);
		BOOST_CHECK_EQUAL(static_cast<T>(i + x),x + 1);
		BOOST_CHECK_EQUAL(static_cast<T>(x + i),x + 1);
		// Check also with move semantics.
		BOOST_CHECK_EQUAL(static_cast<T>(rational(1) + x),x + 1);
		BOOST_CHECK_EQUAL(static_cast<T>(x + rational(1)),x + 1);
	}
};

BOOST_AUTO_TEST_CASE(rational_addition_test)
{
	{
		// In-place addition.
		rational i(1), j(42);
		i += j;
		BOOST_CHECK_EQUAL(static_cast<int>(i),43);
		i += std::move(j);
		BOOST_CHECK_EQUAL(static_cast<int>(i),43 + 42);
		// Add with self.
		i += i;
		BOOST_CHECK_EQUAL(static_cast<int>(i), 2 * (43 + 42));
		// Add with self + move.
		i = 1;
		i += std::move(i);
		BOOST_CHECK_EQUAL(static_cast<int>(i), 2);
		boost::fusion::for_each(arithmetic_values,check_arithmetic_in_place_add());
		// Addition with integer.
		i = rational(3,4);
		i += integer(2);
		BOOST_CHECK(boost::lexical_cast<std::string>(i) == "11/4");
		i += 2u;
		BOOST_CHECK(boost::lexical_cast<std::string>(i) == "19/4");
		i += -2;
		BOOST_CHECK(boost::lexical_cast<std::string>(i) == "11/4");
		i += 0;
		BOOST_CHECK(boost::lexical_cast<std::string>(i) == "11/4");
		// In-place integer with rational.
		integer k(3);
		k += rational(4,2);
		BOOST_CHECK(k == 5);
		k += rational(1,2);
		BOOST_CHECK(k == 5);
		k += rational(3,2);
		BOOST_CHECK(k == 6);
	}
	{
		// Binary addition.
		rational i(1,2);
		// With this line we check all possible combinations of lvalue/rvalue.
		BOOST_CHECK_EQUAL(static_cast<int>(rational(1,2) + (i + ((i + i) + i))),2);
		boost::fusion::for_each(arithmetic_values,check_arithmetic_binary_add());
		// Binary addition with integer.
		BOOST_CHECK(boost::lexical_cast<std::string>(rational(3,2) + integer(2)) == "7/2");
		BOOST_CHECK(boost::lexical_cast<std::string>(integer(2) + rational(11,2)) == "15/2");
	}
	// Identity operation.
	rational i(123);
	BOOST_CHECK_EQUAL(static_cast<int>(+i), 123);
	// Increments.
	BOOST_CHECK_EQUAL(static_cast<int>(++i), 124);
	BOOST_CHECK_EQUAL(static_cast<int>(i++), 124);
	BOOST_CHECK_EQUAL(static_cast<int>(i), 125);
	i = rational(5,2);
	BOOST_CHECK(boost::lexical_cast<std::string>(++i) == "7/2");
	BOOST_CHECK(boost::lexical_cast<std::string>(i++) == "7/2");
	BOOST_CHECK(boost::lexical_cast<std::string>(i) == "9/2");
}

struct check_arithmetic_in_place_sub
{
	template <typename T>
	void operator()(const T &x) const
	{
		{
			rational i(1);
			i -= x;
			BOOST_CHECK_EQUAL(1 - static_cast<int>(x), static_cast<int>(i));
		}
		{
			T y(x);
			rational i(1);
			y -= i;
			BOOST_CHECK_EQUAL(x - 1, y);
			y -= std::move(i);
			BOOST_CHECK_EQUAL(x - 2, y);
		}
	}
};

struct check_arithmetic_binary_sub
{
	template <typename T>
	void operator()(const T &x) const
	{
		rational i(50), j(1);
		BOOST_CHECK_EQUAL(static_cast<T>(i - x),50 - x);
		BOOST_CHECK_EQUAL(static_cast<T>(x - j),x - 1);
		BOOST_CHECK_EQUAL(static_cast<T>(rational(50) - x),50 - x);
		BOOST_CHECK_EQUAL(static_cast<T>(x - rational(1)),x - 1);
	}
};

BOOST_AUTO_TEST_CASE(rational_subtraction_test)
{
	{
		rational i(1), j(42);
		i -= j;
		BOOST_CHECK_EQUAL(static_cast<int>(i),-41);
		i -= std::move(j);
		BOOST_CHECK_EQUAL(static_cast<int>(i),-41 - 42);
		// Sub with self.
		i -= i;
		BOOST_CHECK_EQUAL(static_cast<int>(i),0);
		// Sub with self + move.
		i = 1;
		i -= std::move(i);
		BOOST_CHECK_EQUAL(static_cast<int>(i),0);
		boost::fusion::for_each(arithmetic_values,check_arithmetic_in_place_sub());
		// Sub with integer.
		i = rational(3,4);
		i -= integer(2);
		BOOST_CHECK(boost::lexical_cast<std::string>(i) == "-5/4");
		i -= 2u;
		BOOST_CHECK(boost::lexical_cast<std::string>(i) == "-13/4");
		i -= -2;
		BOOST_CHECK(boost::lexical_cast<std::string>(i) == "-5/4");
		i += 0;
		BOOST_CHECK(boost::lexical_cast<std::string>(i) == "-5/4");
		// In-place integer with rational.
		integer k(3);
		k -= rational(4,2);
		BOOST_CHECK(k == 1);
		k -= rational(1,2);
		BOOST_CHECK(k == 0);
		k -= rational(3,2);
		BOOST_CHECK(k == -1);
	}
	{
		rational i(1);
		BOOST_CHECK_EQUAL(static_cast<int>(rational(1) - (i - ((i - i) - i))),-1);
		boost::fusion::for_each(arithmetic_values,check_arithmetic_binary_sub());
		BOOST_CHECK(boost::lexical_cast<std::string>(rational(3,2) - integer(2)) == "-1/2");
		BOOST_CHECK(boost::lexical_cast<std::string>(integer(2) - rational(11,2)) == "-7/2");
	}
	// Negation operation.
	rational i(123);
	i.negate();
	BOOST_CHECK_EQUAL(static_cast<int>(i), -123);
	BOOST_CHECK_EQUAL(static_cast<int>(-i), 123);
	// Decrements.
	BOOST_CHECK_EQUAL(static_cast<int>(--i), -124);
	BOOST_CHECK_EQUAL(static_cast<int>(i--), -124);
	BOOST_CHECK_EQUAL(static_cast<int>(i), -125);
	i = rational(5,2);
	BOOST_CHECK(boost::lexical_cast<std::string>(--i) == "3/2");
	BOOST_CHECK(boost::lexical_cast<std::string>(i--) == "3/2");
	BOOST_CHECK(boost::lexical_cast<std::string>(i) == "1/2");
}

struct check_arithmetic_in_place_mul
{
	template <typename T>
	void operator()(const T &x) const
	{
		{
			rational i(1);
			i *= x;
			BOOST_CHECK_EQUAL(static_cast<int>(x), static_cast<int>(i));
		}
		{
			T y(x);
			rational i(1);
			y *= i;
			BOOST_CHECK_EQUAL(x, y);
			y *= std::move(i);
			BOOST_CHECK_EQUAL(x, y);
		}
	}
};

struct check_arithmetic_binary_mul
{
	template <typename T>
	void operator()(const T &x) const
	{
		rational i(2), j(1);
		BOOST_CHECK_EQUAL(static_cast<T>(i * x),2 * x);
		BOOST_CHECK_EQUAL(static_cast<T>(x * j),x);
		BOOST_CHECK_EQUAL(static_cast<T>(rational(2) * x),2 * x);
		BOOST_CHECK_EQUAL(static_cast<T>(x * rational(1)),x);
	}
};

BOOST_AUTO_TEST_CASE(rational_multiplication_test)
{
	{
		rational i(1), j(42);
		i *= j;
		BOOST_CHECK_EQUAL(static_cast<int>(i),42);
		i *= std::move(j);
		BOOST_CHECK_EQUAL(static_cast<int>(i),42 * 42);
		// Mul with self.
		i = 2;
		i *= i;
		BOOST_CHECK_EQUAL(static_cast<int>(i),4);
		// Mul with self + move.
		i = 3;
		i *= std::move(i);
		BOOST_CHECK_EQUAL(static_cast<int>(i),9);
		boost::fusion::for_each(arithmetic_values,check_arithmetic_in_place_mul());
		// Sub with integer.
		i = rational(3,4);
		i *= integer(2);
		BOOST_CHECK(boost::lexical_cast<std::string>(i) == "3/2");
		i *= 2u;
		BOOST_CHECK(boost::lexical_cast<std::string>(i) == "3");
		i *= -2;
		BOOST_CHECK(boost::lexical_cast<std::string>(i) == "-6");
		i *= 0;
		BOOST_CHECK(boost::lexical_cast<std::string>(i) == "0");
		// In-place integer with rational.
		integer k(3);
		k *= rational(4,2);
		BOOST_CHECK(k == 6);
		k *= rational(1,2);
		BOOST_CHECK(k == 3);
		k *= rational(3,2);
		BOOST_CHECK(k == 4);
	}
	{
		rational i(2);
		BOOST_CHECK_EQUAL(static_cast<int>(rational(2) * (i * ((i * i) * i))),32);
		boost::fusion::for_each(arithmetic_values,check_arithmetic_binary_mul());
		// Binary multiplication with integer.
		BOOST_CHECK(boost::lexical_cast<std::string>(rational(3,2) * integer(2)) == "3");
		BOOST_CHECK(boost::lexical_cast<std::string>(integer(2) * rational(-11,3)) == "-22/3");
	}
}

const boost::fusion::vector<char,signed char,short,int,long,long long,unsigned char,unsigned short,unsigned,unsigned long,unsigned long long,float,double> arithmetic_zeroes(
	(char)0,(signed char)0,(short)0,0,0L,0LL,
	(unsigned char)0,(unsigned short)0,0U,0UL,0ULL,
	0.f,-0.
);

struct check_arithmetic_zeroes_div
{
	template <typename T>
	void operator()(const T &x) const
	{
		rational i(2);
		BOOST_CHECK_THROW(i /= x,zero_division_error);
	}
};

struct check_arithmetic_binary_div
{
	template <typename T>
	void operator()(const T &x, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr) const
	{
		rational i(100);
		BOOST_CHECK(boost::lexical_cast<std::string>(i / x) == "50/21" || boost::lexical_cast<std::string>(i / x) == "-50/21");
		BOOST_CHECK(boost::lexical_cast<std::string>(x / i) == "21/50" || boost::lexical_cast<std::string>(x / i) == "-21/50");
	}
	template <typename T>
	void operator()(const T &, typename std::enable_if<!std::is_integral<T>::value>::type * = nullptr) const
	{
		if (std::numeric_limits<T>::is_iec559) {
			rational i(100);
			BOOST_CHECK(i / T(2) == T(50));
			BOOST_CHECK(T(200) / i == T(2));
		}
	}
};

struct check_arithmetic_in_place_div
{
	template <typename T>
	void operator()(const T &x, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr) const
	{
		{
			rational i(100);
			i /= x;
			BOOST_CHECK(boost::lexical_cast<std::string>(i) == "50/21" || boost::lexical_cast<std::string>(i) == "-50/21");
		}
		{
			T y(x);
			rational i(21);
			y /= i;
			BOOST_CHECK(y == x / T(21));
		}
	}
	template <typename T>
	void operator()(const T &, typename std::enable_if<!std::is_integral<T>::value>::type * = nullptr) const
	{
		if (std::numeric_limits<T>::is_iec559) {
			rational i(100);
			i /= T(50);
			BOOST_CHECK(boost::lexical_cast<std::string>(i) == "2");
			T x(100);
			x /= rational(100,2);
			BOOST_CHECK(x == T(2));
		}
	}
};

BOOST_AUTO_TEST_CASE(rational_division_test)
{
	{
		rational i(42), j(2);
		i /= j;
		BOOST_CHECK_EQUAL(static_cast<int>(i),21);
		i /= -j;
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(i),"-21/2");
		BOOST_CHECK_THROW(i /= rational(),zero_division_error);
		boost::fusion::for_each(arithmetic_zeroes,check_arithmetic_zeroes_div());
		boost::fusion::for_each(arithmetic_values,check_arithmetic_in_place_div());
		// Div with integer.
		i = rational(3,4);
		i /= integer(2);
		BOOST_CHECK(boost::lexical_cast<std::string>(i) == "3/8");
		i /= 2u;
		BOOST_CHECK(boost::lexical_cast<std::string>(i) == "3/16");
		i /= -2;
		BOOST_CHECK(boost::lexical_cast<std::string>(i) == "-3/32");
		BOOST_CHECK_THROW(i /= integer(),zero_division_error);
		BOOST_CHECK(boost::lexical_cast<std::string>(rational(1,2) / 1) == "1/2");
		BOOST_CHECK(boost::lexical_cast<std::string>(1 / rational(1,2)) == "2");
		// In-place integer with rational.
		integer k(3);
		k /= rational(4,2);
		BOOST_CHECK(k == 1);
		k /= rational(1,2);
		BOOST_CHECK(k == 2);
		k /= rational(2,3);
		BOOST_CHECK(k == 3);
	}
	{
		rational i(2);
		BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(rational(2) / (i / ((i / i) / i))),"1/2");
		boost::fusion::for_each(arithmetic_values,check_arithmetic_binary_div());
		// Binary division with integer.
		BOOST_CHECK(boost::lexical_cast<std::string>(rational(3,2) / integer(2)) == "3/4");
		BOOST_CHECK(boost::lexical_cast<std::string>(integer(2) / rational(-11,3)) == "-6/11");
	}
}

struct check_arithmetic_comparisons
{
	template <typename T>
	void operator()(const T &x) const
	{
		rational i(x);
		BOOST_CHECK(i + 1 > x);
		BOOST_CHECK(x < i + 1);
		BOOST_CHECK(i + 1 >= x);
		BOOST_CHECK(x <= i + 1);
		BOOST_CHECK(i - 1 < x);
		BOOST_CHECK(x > i - 1);
		BOOST_CHECK(i - 1 <= x);
		BOOST_CHECK(x >= i - 1);
		BOOST_CHECK(i + 1 != x);
		BOOST_CHECK(x != i + 1);
		if (std::is_integral<T>::value) {
			BOOST_CHECK(i + 10 == x + 10);
			BOOST_CHECK(!(i + 10 != x + 10));
		}
	}
};

BOOST_AUTO_TEST_CASE(rational_comparisons_test)
{
	rational i(42), j(43);
	BOOST_CHECK(i != j);
	BOOST_CHECK(i < j);
	BOOST_CHECK(i <= j);
	BOOST_CHECK(j > i);
	BOOST_CHECK(j >= i);
	BOOST_CHECK(i + 1 == j);
	BOOST_CHECK(i + 1 <= j);
	BOOST_CHECK(i + 1 >= j);
	boost::fusion::for_each(arithmetic_values,check_arithmetic_comparisons());
	// Comparison with integer.
	BOOST_CHECK(i == integer(42));
	BOOST_CHECK(integer(42) == i);
	BOOST_CHECK(j != integer(42));
	BOOST_CHECK(integer(42) != j);
	BOOST_CHECK(rational(84,2) == integer(42));
	BOOST_CHECK(rational(84,4) != integer(42));
	BOOST_CHECK(integer(42) != rational(84,4));
	BOOST_CHECK(i < integer(43));
	BOOST_CHECK(rational(84,4) < integer(42));
	BOOST_CHECK(i <= integer(42));
	BOOST_CHECK(i <= integer(43));
	BOOST_CHECK(rational(84,4) <= integer(42));
	BOOST_CHECK(integer(42) <= i);
	BOOST_CHECK(integer(43) > i);
	BOOST_CHECK(integer(42) > rational(84,4));
	BOOST_CHECK(integer(42) >= rational(84,4));
	BOOST_CHECK(integer(42) >= rational(84,2));
	BOOST_CHECK(integer(43) >= i);
	BOOST_CHECK(integer(42) >= i);
}

BOOST_AUTO_TEST_CASE(rational_exponentiation_test)
{
	BOOST_CHECK_EQUAL(rational(10).pow(2),100);
	BOOST_CHECK_EQUAL(rational(10).pow(integer(2)),100);
	BOOST_CHECK_EQUAL(rational(10).pow(integer(-2)),rational(1,100));
	BOOST_CHECK_EQUAL(rational(1).pow(integer(-1)),1);
	BOOST_CHECK_EQUAL(rational(-1).pow(integer(-1)),-1);
	BOOST_CHECK_EQUAL(rational(1).pow(-1),1);
	BOOST_CHECK_EQUAL(rational(-1).pow(-2),1);
	BOOST_CHECK_EQUAL(rational(-1).pow(-3),-1);
	BOOST_CHECK_EQUAL(rational(-1).pow(2LL),1);
	BOOST_CHECK_EQUAL(rational(-1).pow(3ULL),-1);
	BOOST_CHECK_THROW(rational(1).pow(integer(boost::integer_traits<unsigned long>::const_max) * 10),std::invalid_argument);
	BOOST_CHECK_THROW(rational(1).pow(integer(boost::integer_traits<unsigned long>::const_max) * -1 - 1),std::invalid_argument);
	BOOST_CHECK_EQUAL(rational(10,3).pow(2),rational(100,9));
	BOOST_CHECK_EQUAL(rational(10,3).pow(-2),rational(9,100));
	BOOST_CHECK_EQUAL(rational(10,-3).pow(-3),rational(27,-1000));
	BOOST_CHECK_EQUAL(rational(10,-3).pow(3),rational(27,-1000).pow(-1));
	BOOST_CHECK_EQUAL(math::pow(rational(10),2),100);
	BOOST_CHECK_EQUAL(math::pow(rational(10),integer(2)),100);
	BOOST_CHECK_EQUAL(math::pow(rational(10),integer(-2)),rational(1,100));
	BOOST_CHECK_EQUAL(math::pow(rational(-1),3ULL),-1);
	BOOST_CHECK_THROW(math::pow(rational(),-1),zero_division_error);
	BOOST_CHECK_THROW(math::pow(rational(),integer(-1)),zero_division_error);
	BOOST_CHECK((is_exponentiable<rational,integer>::value));
	BOOST_CHECK((is_exponentiable<const rational,integer>::value));
	BOOST_CHECK((is_exponentiable<rational &,integer &>::value));
	BOOST_CHECK((is_exponentiable<rational &,integer const &>::value));
	BOOST_CHECK((is_exponentiable<rational,int>::value));
	BOOST_CHECK((is_exponentiable<rational,unsigned>::value));
	BOOST_CHECK((!is_exponentiable<rational,float>::value));
	BOOST_CHECK((!is_exponentiable<rational &,float>::value));
	BOOST_CHECK((!is_exponentiable<rational &,float &>::value));
	BOOST_CHECK((!is_exponentiable<rational,std::string>::value));
	// This was a bug in the signed/unsigned conversion+unary minus trick in the pow() method.
	BOOST_CHECK_EQUAL(math::pow(rational(1,2),(signed char)(-1)),2);
	BOOST_CHECK_EQUAL(math::pow(rational(1,2),(signed short)(-1)),2);
}

BOOST_AUTO_TEST_CASE(rational_hash_test)
{
	BOOST_CHECK_EQUAL((rational(1) + rational(1) - rational(1)).hash(),rational(1).hash());
	BOOST_CHECK_EQUAL((rational(-1) + rational(1) - rational(1)).hash(),rational(-1).hash());
	BOOST_CHECK_EQUAL((rational(1,2) + rational(1,2) - rational(1,2)).hash(),rational(1,2).hash());
	BOOST_CHECK_EQUAL((rational(-1,2) + rational(1,2) - rational(1,2)).hash(),rational(1,-2).hash());
	std::hash<rational> hasher;
	BOOST_CHECK_EQUAL(hasher(rational(1) + rational(1) - rational(1)),hasher(rational(1)));
	BOOST_CHECK_EQUAL(hasher(rational(-1) + rational(1) - rational(1)),hasher(rational(-1)));
	BOOST_CHECK_EQUAL((rational(1,2) + rational(1,2) - rational(1,2)).hash(),hasher(rational(1,2)));
	BOOST_CHECK_EQUAL((rational(-1,2) + rational(1,2) - rational(1,2)).hash(),hasher(rational(1,-2)));
}

BOOST_AUTO_TEST_CASE(rational_sign_test)
{
	BOOST_CHECK_EQUAL(rational().sign(),0);
	BOOST_CHECK_EQUAL(rational(-1).sign(),-1);
	BOOST_CHECK_EQUAL(rational(-1,2).sign(),-1);
	BOOST_CHECK_EQUAL(rational(-10).sign(),-1);
	BOOST_CHECK_EQUAL(rational(1,67).sign(),1);
	BOOST_CHECK_EQUAL(rational(10).sign(),1);
}

BOOST_AUTO_TEST_CASE(rational_math_overloads_test)
{
	BOOST_CHECK(math::is_zero(rational()));
	BOOST_CHECK(math::is_zero(rational(0)));
	BOOST_CHECK(!math::is_zero(rational(-1)));
	BOOST_CHECK(!math::is_zero(rational(-10)));
	BOOST_CHECK(!math::is_zero(rational(1)));
	BOOST_CHECK(!math::is_zero(rational(10)));
	rational n(0);
	math::negate(n);
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"0");
	n = 10;
	math::negate(n);
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"-10");
	math::negate(n);
	BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(n),"10");
	BOOST_CHECK(has_is_zero<rational>::value);
}

BOOST_AUTO_TEST_CASE(rational_stream_test)
{
	{
		std::ostringstream oss;
		std::string tmp = "12843748347394832742398472398472389/66786543";
		oss << rational(tmp);
		BOOST_CHECK_EQUAL(tmp,oss.str());
	}
	{
		std::ostringstream oss;
		std::string tmp = "-2389472323272767078540934/13";
		oss << rational(tmp);
		BOOST_CHECK_EQUAL(tmp,oss.str());
	}
	{
		rational tmp;
		std::stringstream ss;
		ss << "256/2";
		ss >> tmp;
		BOOST_CHECK_EQUAL(tmp,128);
	}
	{
		rational tmp;
		std::stringstream ss;
		ss << "-30000";
		ss >> tmp;
		BOOST_CHECK_EQUAL(tmp,-30000);
	}
}

BOOST_AUTO_TEST_CASE(rational_sin_cos_test)
{
	BOOST_CHECK_EQUAL(math::sin(rational()),0);
	BOOST_CHECK_THROW(math::sin(rational(1)),std::invalid_argument);
	BOOST_CHECK_EQUAL(math::cos(rational()),1);
	BOOST_CHECK_THROW(math::cos(rational(1)),std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(rational_numden_test)
{
	BOOST_CHECK((std::is_same<decltype(rational().get_numerator()),integer>::value));
	BOOST_CHECK((std::is_same<decltype(rational().get_denominator()),integer>::value));
	BOOST_CHECK_EQUAL(rational().get_numerator(),0);
	BOOST_CHECK_EQUAL(rational().get_denominator(),1);
	BOOST_CHECK_EQUAL(rational(1,2).get_numerator(),1);
	BOOST_CHECK_EQUAL(rational(4,-8).get_denominator(),2);
}

BOOST_AUTO_TEST_CASE(rational_integral_cast_test)
{
	BOOST_CHECK_EQUAL(math::integral_cast(rational()),0);
	BOOST_CHECK_EQUAL(math::integral_cast(rational(2)),2);
	BOOST_CHECK_EQUAL(math::integral_cast(rational(62,-2)),-31);
	BOOST_CHECK_THROW(math::integral_cast(rational(1,-2)),std::invalid_argument);
	BOOST_CHECK_THROW(math::integral_cast(rational("2/3") * 2),std::invalid_argument);
	BOOST_CHECK(has_integral_cast<rational>::value);
	BOOST_CHECK(has_integral_cast<rational &>::value);
	BOOST_CHECK(has_integral_cast<rational const &>::value);
	BOOST_CHECK(has_integral_cast<rational &&>::value);
}

BOOST_AUTO_TEST_CASE(rational_partial_test)
{
	BOOST_CHECK_EQUAL(math::partial(rational(),""),0);
	BOOST_CHECK_EQUAL(math::partial(rational(1),std::string("")),0);
	BOOST_CHECK_EQUAL(math::partial(rational(-10),std::string("")),0);
}

BOOST_AUTO_TEST_CASE(rational_evaluate_test)
{
	BOOST_CHECK_EQUAL(math::evaluate(rational(),std::unordered_map<std::string,integer>{}),rational());
	BOOST_CHECK_EQUAL(math::evaluate(rational(2),std::unordered_map<std::string,rational>{}),rational(2));
	BOOST_CHECK_EQUAL(math::evaluate(rational(-3.5),std::unordered_map<std::string,double>{}),rational(-3.5));
	BOOST_CHECK_EQUAL(math::evaluate(rational(4,5),std::unordered_map<std::string,int>{}),rational(8,10));
	BOOST_CHECK((std::is_same<decltype(math::evaluate(rational(),std::unordered_map<std::string,char>{})),rational>::value));
}

BOOST_AUTO_TEST_CASE(rational_subs_test)
{
	BOOST_CHECK_EQUAL(math::subs(rational(),"",1),rational());
	BOOST_CHECK_EQUAL(math::subs(rational(2),"foo",4.5),rational(2));
	BOOST_CHECK_EQUAL(math::subs(rational(-3.5),"bar",55),rational(-3.5));
	BOOST_CHECK_EQUAL(math::subs(rational(4,5),"","frob"),rational(-8,-10));
	BOOST_CHECK((std::is_same<decltype(math::subs(rational(4,5),"","frob")),rational>::value));
	BOOST_CHECK(has_subs<rational>::value);
	BOOST_CHECK((has_subs<rational,int>::value));
	BOOST_CHECK((has_subs<rational,std::string>::value));
	BOOST_CHECK((has_subs<rational,const double &>::value));
	BOOST_CHECK((has_subs<rational &&,const double &>::value));
}

BOOST_AUTO_TEST_CASE(rational_print_tex_test)
{
	std::ostringstream ss;
	print_tex_coefficient(ss,rational(0));
	BOOST_CHECK_EQUAL(ss.str(),"0");
	ss.str("");
	print_tex_coefficient(ss,rational(-1));
	BOOST_CHECK_EQUAL(ss.str(),"-1");
	ss.str("");
	print_tex_coefficient(ss,rational(1));
	BOOST_CHECK_EQUAL(ss.str(),"1");
	ss.str("");
	print_tex_coefficient(ss,rational(1,2));
	BOOST_CHECK_EQUAL(ss.str(),"\\frac{1}{2}");
	ss.str("");
	print_tex_coefficient(ss,rational(1,-2));
	BOOST_CHECK_EQUAL(ss.str(),"-\\frac{1}{2}");
	ss.str("");
	print_tex_coefficient(ss,rational(-14,21));
	BOOST_CHECK_EQUAL(ss.str(),"-\\frac{2}{3}");
}

BOOST_AUTO_TEST_CASE(rational_ipow_subs_test)
{
	BOOST_CHECK_EQUAL(math::ipow_subs(rational(-42,2),"a",integer(4),5),rational(-21));
	BOOST_CHECK_EQUAL(math::ipow_subs(rational(42,3),"a",integer(4),5),rational(14));
	BOOST_CHECK(has_ipow_subs<rational>::value);
	BOOST_CHECK((has_ipow_subs<rational,double>::value));
	BOOST_CHECK((has_ipow_subs<rational,integer>::value));
}

BOOST_AUTO_TEST_CASE(rational_abs_test)
{
	BOOST_CHECK_EQUAL(rational(42,2).abs(),21);
	BOOST_CHECK_EQUAL(rational(-42,2).abs(),21);
	BOOST_CHECK_EQUAL(math::abs(rational(42,2)),21);
	BOOST_CHECK_EQUAL(math::abs(rational(42,-2)),21);
}

BOOST_AUTO_TEST_CASE(rational_binomial_test)
{
	BOOST_CHECK((has_binomial<rational,int>::value));
	BOOST_CHECK((has_binomial<rational,char>::value));
	BOOST_CHECK((has_binomial<rational,unsigned>::value));
	BOOST_CHECK((has_binomial<rational,long>::value));
	BOOST_CHECK((!has_binomial<rational,std::string>::value));
	BOOST_CHECK((std::is_same<rational,decltype(math::binomial(rational{},2))>::value));
	BOOST_CHECK_EQUAL(math::binomial(rational(-14),12),integer("5200300"));
	BOOST_CHECK_EQUAL(math::binomial(rational(1,10),5),rational("64467/4000000"));
	BOOST_CHECK_EQUAL(math::binomial(rational(1,-10),5),rational("-97867/4000000"));
	BOOST_CHECK_EQUAL(math::binomial(rational(8,7),5),rational("-104/16807"));
	BOOST_CHECK_EQUAL(math::binomial(rational(8,-7),5),rational("-22968/16807"));
	BOOST_CHECK_EQUAL(math::binomial(rational(8,-7),0l),1);
	BOOST_CHECK_EQUAL(math::binomial(rational(8,7),0ull),1);
	BOOST_CHECK_EQUAL(math::binomial(rational(0,-7),1),0);
	BOOST_CHECK_EQUAL(math::binomial(rational(0,7),2),0);
	BOOST_CHECK_THROW(math::binomial(rational(3),-2),std::invalid_argument);
	BOOST_CHECK_THROW(math::binomial(rational(0),-2),std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(rational_is_equality_comparable_test)
{
	BOOST_CHECK(is_equality_comparable<rational>::value);
	BOOST_CHECK((is_equality_comparable<rational,integer>::value));
	BOOST_CHECK((is_equality_comparable<integer,rational>::value));
	BOOST_CHECK((is_equality_comparable<double,rational>::value));
	BOOST_CHECK((is_equality_comparable<rational,int>::value));
	BOOST_CHECK((!is_equality_comparable<rational,std::string>::value));
}

BOOST_AUTO_TEST_CASE(rational_t_subs_test)
{
	BOOST_CHECK(!has_t_subs<rational>::value);
	BOOST_CHECK((!has_t_subs<rational,int>::value));
	BOOST_CHECK((!has_t_subs<rational,int,double>::value));
	BOOST_CHECK((!has_t_subs<rational &,int>::value));
	BOOST_CHECK((!has_t_subs<const rational &,const int &,double &>::value));
	BOOST_CHECK((!has_t_subs<std::string,rational,double>::value));
}

BOOST_AUTO_TEST_CASE(rational_type_traits_test)
{
	BOOST_CHECK_EQUAL(is_nothrow_destructible<rational>::value,true);
	BOOST_CHECK_EQUAL(is_nothrow_destructible<const rational>::value,true);
	BOOST_CHECK(is_differentiable<rational>::value);
	BOOST_CHECK(is_differentiable<rational &>::value);
	BOOST_CHECK(is_differentiable<rational &&>::value);
	BOOST_CHECK(has_pbracket<rational>::value);
	BOOST_CHECK(has_pbracket<rational &>::value);
	BOOST_CHECK(has_pbracket<rational &&>::value);
	BOOST_CHECK(has_transformation_is_canonical<rational>::value);
	BOOST_CHECK(has_transformation_is_canonical<rational &>::value);
	BOOST_CHECK(has_transformation_is_canonical<rational &&>::value);
	BOOST_CHECK(!has_degree<rational>::value);
	BOOST_CHECK(is_addable<rational>::value);
	BOOST_CHECK((is_addable<rational,integer>::value));
	BOOST_CHECK((is_addable<integer,rational>::value));
	BOOST_CHECK((is_addable<double,rational>::value));
	BOOST_CHECK((is_addable<rational,double>::value));
	BOOST_CHECK((!is_addable<rational,std::complex<double>>::value));
	BOOST_CHECK((!is_addable<std::complex<double>,rational>::value));
	BOOST_CHECK(is_subtractable<rational>::value);
	BOOST_CHECK((is_subtractable<double,rational>::value));
	BOOST_CHECK((is_subtractable<rational,integer>::value));
	BOOST_CHECK((is_subtractable<integer,rational>::value));
	BOOST_CHECK((is_subtractable<rational,double>::value));
	BOOST_CHECK((!is_subtractable<rational,std::complex<double>>::value));
	BOOST_CHECK((!is_subtractable<std::complex<double>,rational>::value));
	BOOST_CHECK(is_container_element<rational>::value);
	BOOST_CHECK(is_ostreamable<rational>::value);
	BOOST_CHECK(has_print_coefficient<rational>::value);
	BOOST_CHECK(has_print_tex_coefficient<rational>::value);
	BOOST_CHECK((std::is_same<void,decltype(print_tex_coefficient(*(std::ostream *)nullptr,std::declval<rational>()))>::value));
	BOOST_CHECK(has_negate<rational>::value);
	BOOST_CHECK(has_negate<rational &>::value);
	BOOST_CHECK(!has_negate<const rational &>::value);
	BOOST_CHECK(!has_negate<const rational>::value);
	BOOST_CHECK((std::is_same<decltype(math::negate(*(rational *)nullptr)),void>::value));
	BOOST_CHECK(is_hashable<rational>::value);
	BOOST_CHECK(is_hashable<rational &>::value);
	BOOST_CHECK(is_hashable<const rational &>::value);
	BOOST_CHECK((is_evaluable<rational,int>::value));
	BOOST_CHECK((is_evaluable<rational,double>::value));
	BOOST_CHECK((is_evaluable<rational &,double>::value));
	BOOST_CHECK((is_evaluable<rational const &,double>::value));
	BOOST_CHECK((is_evaluable<rational &&,double>::value));
	BOOST_CHECK(has_sine<rational>::value);
	BOOST_CHECK(has_cosine<rational>::value);
	BOOST_CHECK(has_sine<rational &>::value);
	BOOST_CHECK(has_cosine<rational const &>::value);
	BOOST_CHECK(has_cosine<rational &&>::value);
}

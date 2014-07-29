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

#include <algorithm>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/integer_traits.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/bounds.hpp>
#include <complex>
#include <ctgmath>
#include <limits>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../src/environment.hpp"
#include "../src/exceptions.hpp"
#include "../src/math.hpp"
#include "../src/type_traits.hpp"

const boost::fusion::vector<char,signed char,short,int,long,long long,unsigned char,unsigned short,unsigned,unsigned long,unsigned long long,float,double> arithmetic_values(
	(char)42,(signed char)42,(short)42,-42,42L,-42LL,
	(unsigned char)42,(unsigned short)42,42U,42UL,42ULL,
	23.456f,-23.456
);

const std::vector<std::string> invalid_strings{"-0","+0","01","+1","123f"," 123","123 ","123.56"};

static inline piranha::integer get_big_int()
{
	std::string tmp = boost::lexical_cast<std::string>(boost::integer_traits<unsigned long long>::const_max);
	tmp += "123456789";
	return piranha::integer(tmp);
}

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
	piranha::environment env;
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
	piranha::integer i("-30"), j(i);
	BOOST_CHECK_EQUAL(-30,static_cast<int>(j));
	// Large value.
	piranha::integer i2(get_big_int()), j2(i2);
	BOOST_CHECK_EQUAL(i2,j2);
	// Move construction.
	piranha::integer i3("-30"), j3(std::move(i3));
	BOOST_CHECK(j3 == -30);
	piranha::integer i4(get_big_int()), j4(std::move(i4));
	BOOST_CHECK(j4 == i2);
	// Construction with non-finite floating-point.
	if (std::numeric_limits<float>::has_infinity && std::numeric_limits<double>::has_infinity) {
		BOOST_CHECK_THROW(ptr.reset(new piranha::integer(std::numeric_limits<float>::infinity())),std::invalid_argument);
		BOOST_CHECK_THROW(ptr.reset(new piranha::integer(std::numeric_limits<double>::infinity())),std::invalid_argument);
	}
	if (std::numeric_limits<float>::has_quiet_NaN) {
		BOOST_CHECK_THROW(ptr.reset(new piranha::integer(std::numeric_limits<float>::quiet_NaN())),std::invalid_argument);
	}
	if (std::numeric_limits<double>::has_quiet_NaN) {
		BOOST_CHECK_THROW(ptr.reset(new piranha::integer(std::numeric_limits<double>::quiet_NaN())),std::invalid_argument);
	}
	// Constructor from size.
	piranha::integer k(piranha::integer::nlimbs(4));
	BOOST_CHECK_EQUAL(k,0);
	BOOST_CHECK_NO_THROW(piranha::integer{piranha::integer::nlimbs(0)});
	piranha::integer k3(piranha::integer::nlimbs(1));
	BOOST_CHECK(k3.allocated_size() >= 1u);
	// High number of limbs.
	piranha::integer k2(piranha::integer::nlimbs(400));
	BOOST_CHECK_EQUAL(k2.allocated_size(),400u);
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
	i = std::string("-123");
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
	piranha::integer k(get_big_int());
	i.swap(k);
	BOOST_CHECK_EQUAL(43,static_cast<int>(k));
	std::swap(k,i);
	BOOST_CHECK_EQUAL(43,static_cast<int>(i));
	piranha::integer l(get_big_int() + 1);
	l.swap(k);
	BOOST_CHECK_EQUAL(get_big_int(),l);
}

template <typename T>
static inline void inf_conversion_test()
{
	if (!std::numeric_limits<T>::has_infinity) {
		return;
	}
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
	piranha::integer bigint(get_big_int());
	BOOST_CHECK_THROW((void)static_cast<int>(bigint),std::overflow_error);
	piranha::integer max_unsigned(boost::numeric::bounds<unsigned>::highest());
	BOOST_CHECK_THROW((void)static_cast<int>(max_unsigned),std::overflow_error);
	BOOST_CHECK_NO_THROW((void)static_cast<unsigned>(max_unsigned));
	// Conversion that will generate infinity.
	inf_conversion_test<float>();
	inf_conversion_test<double>();
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

struct check_arithmetic_in_place_add
{
	template <typename T>
	void operator()(const T &x) const
	{
		{
			// In-place add, integer on the left.
			piranha::integer i(1);
			i += x;
			BOOST_CHECK_EQUAL(static_cast<int>(x) + 1, static_cast<int>(i));
		}
		{
			// In-place add, integer on the right.
			T y(x);
			piranha::integer i(1);
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
		piranha::integer i(1);
		BOOST_CHECK_EQUAL(static_cast<T>(i + x),x + 1);
		BOOST_CHECK_EQUAL(static_cast<T>(x + i),x + 1);
		// Check also with move semantics.
		BOOST_CHECK_EQUAL(static_cast<T>(piranha::integer(1) + x),x + 1);
		BOOST_CHECK_EQUAL(static_cast<T>(x + piranha::integer(1)),x + 1);
	}
};

BOOST_AUTO_TEST_CASE(integer_addition_test)
{
	{
		// In-place addition.
		piranha::integer i(1), j(42);
		i += j;
		BOOST_CHECK_EQUAL(static_cast<int>(i),43);
		i += std::move(j);
		BOOST_CHECK_EQUAL(static_cast<int>(i),43 + 42);
		// Add with self.
		i += i;
		BOOST_CHECK_EQUAL(i, 2 * (43 + 42));
		// Add with self + move.
		i = 1;
		i += std::move(i);
		BOOST_CHECK_EQUAL(i, 2);
		boost::fusion::for_each(arithmetic_values,check_arithmetic_in_place_add());
	}
	{
		// Binary addition.
		piranha::integer i(1);
		// With this line we check all possible combinations of lvalue/rvalue.
		BOOST_CHECK_EQUAL(static_cast<int>(piranha::integer(1) + (i + ((i + i) + i))),5);
		boost::fusion::for_each(arithmetic_values,check_arithmetic_binary_add());
	}
	// Identity operation.
	piranha::integer i(123);
	BOOST_CHECK_EQUAL(static_cast<int>(+i), 123);
	// Increments.
	BOOST_CHECK_EQUAL(static_cast<int>(++i), 124);
	BOOST_CHECK_EQUAL(static_cast<int>(i++), 124);
	BOOST_CHECK_EQUAL(static_cast<int>(i), 125);
}

struct check_arithmetic_in_place_sub
{
	template <typename T>
	void operator()(const T &x) const
	{
		{
			piranha::integer i(1);
			i -= x;
			BOOST_CHECK_EQUAL(1 - static_cast<int>(x), static_cast<int>(i));
		}
		{
			T y(x);
			piranha::integer i(1);
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
		piranha::integer i(50), j(1);
		BOOST_CHECK_EQUAL(static_cast<T>(i - x),50 - x);
		BOOST_CHECK_EQUAL(static_cast<T>(x - j),x - 1);
		BOOST_CHECK_EQUAL(static_cast<T>(piranha::integer(50) - x),50 - x);
		BOOST_CHECK_EQUAL(static_cast<T>(x - piranha::integer(1)),x - 1);
	}
};

BOOST_AUTO_TEST_CASE(integer_subtraction_test)
{
	{
		piranha::integer i(1), j(42);
		i -= j;
		BOOST_CHECK_EQUAL(static_cast<int>(i),-41);
		i -= std::move(j);
		BOOST_CHECK_EQUAL(static_cast<int>(i),-41 - 42);
		// Sub with self.
		i -= i;
		BOOST_CHECK_EQUAL(i,0);
		// Sub with self + move.
		i = 1;
		i -= std::move(i);
		BOOST_CHECK_EQUAL(i,0);
		boost::fusion::for_each(arithmetic_values,check_arithmetic_in_place_sub());
	}
	{
		piranha::integer i(1);
		BOOST_CHECK_EQUAL(static_cast<int>(piranha::integer(1) - (i - ((i - i) - i))),-1);
		boost::fusion::for_each(arithmetic_values,check_arithmetic_binary_sub());
	}
	// Negation operation.
	piranha::integer i(123);
	i.negate();
	BOOST_CHECK_EQUAL(static_cast<int>(i), -123);
	BOOST_CHECK_EQUAL(static_cast<int>(-i), 123);
	// Decrements.
	BOOST_CHECK_EQUAL(static_cast<int>(--i), -124);
	BOOST_CHECK_EQUAL(static_cast<int>(i--), -124);
	BOOST_CHECK_EQUAL(static_cast<int>(i), -125);
}

struct check_arithmetic_in_place_mul
{
	template <typename T>
	void operator()(const T &x) const
	{
		{
			piranha::integer i(1);
			i *= x;
			BOOST_CHECK_EQUAL(static_cast<int>(x), static_cast<int>(i));
		}
		{
			T y(x);
			piranha::integer i(1);
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
		piranha::integer i(2), j(1);
		BOOST_CHECK_EQUAL(static_cast<T>(i * x),2 * x);
		BOOST_CHECK_EQUAL(static_cast<T>(x * j),x);
		BOOST_CHECK_EQUAL(static_cast<T>(piranha::integer(2) * x),2 * x);
		BOOST_CHECK_EQUAL(static_cast<T>(x * piranha::integer(1)),x);
	}
};

BOOST_AUTO_TEST_CASE(integer_multiplication_test)
{
	{
		piranha::integer i(1), j(42);
		i *= j;
		BOOST_CHECK_EQUAL(static_cast<int>(i),42);
		i *= std::move(j);
		BOOST_CHECK_EQUAL(static_cast<int>(i),42 * 42);
		// Mul with self.
		i = 2;
		i *= i;
		BOOST_CHECK_EQUAL(i,4);
		// Mul with self + move.
		i = 3;
		i *= std::move(i);
		BOOST_CHECK_EQUAL(i,9);
		boost::fusion::for_each(arithmetic_values,check_arithmetic_in_place_mul());
	}
	{
		piranha::integer i(2);
		BOOST_CHECK_EQUAL(static_cast<int>(piranha::integer(2) * (i * ((i * i) * i))),32);
		boost::fusion::for_each(arithmetic_values,check_arithmetic_binary_mul());
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
		piranha::integer i(2);
		BOOST_CHECK_THROW(i /= x,piranha::zero_division_error);
	}
};

struct check_arithmetic_binary_div
{
	template <typename T>
	void operator()(const T &x) const
	{
		piranha::integer i(100), j(105);
		BOOST_CHECK_EQUAL(static_cast<T>(i / x),100 / x);
		BOOST_CHECK_EQUAL(static_cast<T>(x / j),x / 105);
		BOOST_CHECK_EQUAL(static_cast<T>(piranha::integer(2) / x),2 / x);
		BOOST_CHECK_EQUAL(static_cast<T>(x / piranha::integer(1)),x);
	}
};

struct check_arithmetic_in_place_div
{
	template <typename T>
	void operator()(const T &x) const
	{
		{
			piranha::integer i(100);
			i /= x;
			BOOST_CHECK_EQUAL(static_cast<int>(100 / x), static_cast<int>(i));
		}
		{
			T y(x);
			piranha::integer i(1);
			y /= i;
			BOOST_CHECK_EQUAL(x, y);
		}
	}
};

BOOST_AUTO_TEST_CASE(integer_division_test)
{
	{
		piranha::integer i(42), j(2);
		i /= j;
		BOOST_CHECK_EQUAL(static_cast<int>(i),21);
		i /= -j;
		BOOST_CHECK_EQUAL(static_cast<int>(i),-10);
		BOOST_CHECK_THROW(i /= piranha::integer(),piranha::zero_division_error);
		boost::fusion::for_each(arithmetic_zeroes,check_arithmetic_zeroes_div());
		boost::fusion::for_each(arithmetic_values,check_arithmetic_in_place_div());
	}
	{
		piranha::integer i(1);
		BOOST_CHECK_EQUAL(static_cast<int>(piranha::integer(2) / (i / ((i / i) / i))),2);
		boost::fusion::for_each(arithmetic_values,check_arithmetic_binary_div());
	}
}

struct check_integral_zeroes_mod
{
	template <typename T>
	void operator()(const T &x,typename boost::enable_if_c<std::is_integral<T>::value>::type * = 0) const
	{
		piranha::integer i(2);
		BOOST_CHECK_THROW(i %= x,std::invalid_argument);
	}
	template <typename T>
	void operator()(const T &,typename boost::enable_if_c<std::is_floating_point<T>::value>::type * = 0) const
	{}
};

struct check_integral_in_place_mod
{
	template <typename T>
	void operator()(const T &x,typename boost::enable_if_c<std::is_integral<T>::value>::type * = 0) const
	{
		{
			piranha::integer i(100);
			if (x > 0) {
				i %= x;
				BOOST_CHECK_EQUAL(static_cast<int>(100 % x), static_cast<int>(i));
			} else {
				BOOST_CHECK_THROW(i %= x,std::invalid_argument);
			}
			i.negate();
			BOOST_CHECK_THROW(i %= x,std::invalid_argument);
		}
		{
			T y(piranha::math::abs(x));
			piranha::integer i(10);
			y %= i;
			BOOST_CHECK_EQUAL(static_cast<T>(2), y);
			if (std::is_signed<T>::value) {
				T z(piranha::math::abs(x));
				piranha::math::negate(z);
				BOOST_CHECK_THROW(z %= i,std::invalid_argument);
			}
		}
	}
	template <typename T>
	void operator()(const T &,typename boost::enable_if_c<std::is_floating_point<T>::value>::type * = 0) const
	{}
};

struct check_integral_binary_mod
{
	template <typename T>
	void operator()(const T &x,typename boost::enable_if_c<std::is_integral<T>::value>::type * = 0) const
	{
		piranha::integer i(100), j(105);
		BOOST_CHECK_EQUAL(static_cast<T>(i % piranha::math::abs(x)),static_cast<T>(100 % piranha::math::abs(x)));
		BOOST_CHECK_EQUAL(static_cast<T>(piranha::math::abs(x) % j),static_cast<T>(piranha::math::abs(x) % 105));
		BOOST_CHECK_EQUAL(static_cast<T>(piranha::integer(2) % piranha::math::abs(x)),static_cast<T>(2 % x));
		BOOST_CHECK_EQUAL(static_cast<T>(piranha::math::abs(x) % piranha::integer(1)),static_cast<T>(0));
		if (std::is_signed<T>::value) {
			BOOST_CHECK_THROW(-piranha::math::abs(x) % i,std::invalid_argument);
			BOOST_CHECK_THROW(i % -piranha::math::abs(x),std::invalid_argument);
		}
	}
	template <typename T>
	void operator()(const T &,typename boost::enable_if_c<std::is_floating_point<T>::value>::type * = 0) const
	{}
};

BOOST_AUTO_TEST_CASE(integer_modulo_test)
{
	{
		piranha::integer i(42), j(33);
		i %= j;
		BOOST_CHECK_EQUAL(static_cast<int>(i),9);
		BOOST_CHECK_THROW(i %= -j,std::invalid_argument);
		BOOST_CHECK_THROW(i %= piranha::integer(),std::invalid_argument);
		i.negate();
		BOOST_CHECK_THROW(i %= j,std::invalid_argument);
		i = 0;
		i %= j;
		BOOST_CHECK_EQUAL(static_cast<int>(i),0);
		boost::fusion::for_each(arithmetic_zeroes,check_integral_zeroes_mod());
		boost::fusion::for_each(arithmetic_values,check_integral_in_place_mod());
	}
	boost::fusion::for_each(arithmetic_values,check_integral_binary_mod());
}

struct check_arithmetic_comparisons
{
	template <typename T>
	void operator()(const T &x) const
	{
		piranha::integer i(x);
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

BOOST_AUTO_TEST_CASE(integer_comparisons_test)
{
	piranha::integer i(42), j(43);
	BOOST_CHECK(i != j);
	BOOST_CHECK(i < j);
	BOOST_CHECK(i <= j);
	BOOST_CHECK(j > i);
	BOOST_CHECK(j >= i);
	BOOST_CHECK(i + 1 == j);
	BOOST_CHECK(i + 1 <= j);
	BOOST_CHECK(i + 1 >= j);
	boost::fusion::for_each(arithmetic_values,check_arithmetic_comparisons());
}

struct no_fma{};

BOOST_AUTO_TEST_CASE(integer_multiply_accumulate_test)
{
	piranha::integer i(10);
	i.multiply_accumulate(piranha::integer(10),piranha::integer(-2));
	BOOST_CHECK_EQUAL(i,-10);
	i.multiply_accumulate(piranha::integer(-10),piranha::integer(2));
	BOOST_CHECK_EQUAL(i,-30);
	i.multiply_accumulate(piranha::integer(-10),piranha::integer(-3));
	BOOST_CHECK_EQUAL(i,0);
	// Same with function from math.
	i = 10;
	piranha::math::multiply_accumulate(i,piranha::integer(10),piranha::integer(-2));
	BOOST_CHECK_EQUAL(i,-10);
	piranha::math::multiply_accumulate(i,piranha::integer(-10),piranha::integer(2));
	BOOST_CHECK_EQUAL(i,-30);
	piranha::math::multiply_accumulate(i,piranha::integer(-10),piranha::integer(-3));
	BOOST_CHECK_EQUAL(i,0);
	BOOST_CHECK((piranha::has_multiply_accumulate<piranha::integer>::value));
	BOOST_CHECK((!piranha::has_multiply_accumulate<piranha::integer,no_fma>::value));
	BOOST_CHECK((piranha::has_multiply_accumulate<piranha::integer &, piranha::integer>::value));
	BOOST_CHECK((piranha::has_multiply_accumulate<piranha::integer &, piranha::integer &, const piranha::integer &>::value));
	BOOST_CHECK((!piranha::has_multiply_accumulate<const piranha::integer, piranha::integer>::value));
	BOOST_CHECK((!piranha::has_multiply_accumulate<const piranha::integer &, piranha::integer, piranha::integer &>::value));
}

BOOST_AUTO_TEST_CASE(integer_exponentiation_test)
{
	BOOST_CHECK_EQUAL(piranha::integer(10).pow(2),100);
	BOOST_CHECK_EQUAL(piranha::integer(10).pow(piranha::integer(2)),100);
	BOOST_CHECK_EQUAL(piranha::integer(10).pow(piranha::integer(-2)),0);
	BOOST_CHECK_EQUAL(piranha::integer(1).pow(piranha::integer(-1)),1);
	BOOST_CHECK_EQUAL(piranha::integer(-1).pow(piranha::integer(-1)),-1);
	BOOST_CHECK_EQUAL(piranha::integer(1).pow(-1),1);
	BOOST_CHECK_EQUAL(piranha::integer(-1).pow(-2),1);
	BOOST_CHECK_EQUAL(piranha::integer(-1).pow(-3),-1);
	BOOST_CHECK_EQUAL(piranha::integer(-1).pow(2LL),1);
	BOOST_CHECK_EQUAL(piranha::integer(-1).pow(3ULL),-1);
	BOOST_CHECK_THROW(piranha::integer(1).pow(piranha::integer(boost::integer_traits<unsigned long>::const_max) * 10),std::invalid_argument);
	BOOST_CHECK_THROW(piranha::integer(1).pow(piranha::integer(boost::integer_traits<unsigned long>::const_max) * -1 - 1),std::invalid_argument);
	BOOST_CHECK_EQUAL(piranha::math::pow(piranha::integer(10),2),100);
	BOOST_CHECK_EQUAL(piranha::math::pow(2,piranha::integer(10)),1024);
	BOOST_CHECK_EQUAL(piranha::math::pow(2ull,piranha::integer(10)),1024);
	BOOST_CHECK((std::is_same<decltype(piranha::math::pow(2ull,piranha::integer(10))),piranha::integer>::value));
	BOOST_CHECK((std::is_same<decltype(piranha::math::pow(char(2),piranha::integer(10))),piranha::integer>::value));
	BOOST_CHECK_EQUAL(piranha::math::pow(piranha::integer(10),-2),0);
	BOOST_CHECK_EQUAL(piranha::math::pow(char(3),piranha::integer(10)),59049ll);
	BOOST_CHECK_THROW(piranha::math::pow(piranha::integer(1),piranha::integer(boost::integer_traits<unsigned long>::const_max) * 10),std::invalid_argument);
	BOOST_CHECK((piranha::is_exponentiable<piranha::integer,piranha::integer>::value));
	BOOST_CHECK((piranha::is_exponentiable<piranha::integer,int>::value));
	BOOST_CHECK((piranha::is_exponentiable<piranha::integer,long>::value));
	BOOST_CHECK((piranha::is_exponentiable<piranha::integer,char>::value));
	BOOST_CHECK((piranha::is_exponentiable<const piranha::integer,char>::value));
	BOOST_CHECK((piranha::is_exponentiable<piranha::integer &,char>::value));
	BOOST_CHECK((piranha::is_exponentiable<const piranha::integer &,char>::value));
	BOOST_CHECK((piranha::is_exponentiable<piranha::integer &,char &>::value));
	BOOST_CHECK((piranha::is_exponentiable<piranha::integer,float>::value));
	BOOST_CHECK((piranha::is_exponentiable<piranha::integer,double>::value));
	BOOST_CHECK((!piranha::is_exponentiable<piranha::integer,std::string>::value));
	BOOST_CHECK((piranha::is_exponentiable<double,piranha::integer>::value));
	BOOST_CHECK((piranha::is_exponentiable<float,piranha::integer>::value));
	BOOST_CHECK_EQUAL(piranha::math::pow(piranha::integer(10),-2.),std::pow(10.,-2.));
	BOOST_CHECK_EQUAL(piranha::math::pow(-2.,piranha::integer(10)),std::pow(-2.,10.));
}

BOOST_AUTO_TEST_CASE(integer_hash_test)
{
	BOOST_CHECK_EQUAL(piranha::integer().hash(),static_cast<std::size_t>(0));
	BOOST_CHECK_EQUAL((piranha::integer() + piranha::integer(1) - piranha::integer(1)).hash(),static_cast<std::size_t>(0));
	BOOST_CHECK_EQUAL((piranha::integer(1) + piranha::integer(1) - piranha::integer(1)).hash(),piranha::integer(1).hash());
	BOOST_CHECK_EQUAL((piranha::integer(-1) + piranha::integer(1) - piranha::integer(1)).hash(),piranha::integer(-1).hash());
	std::hash<piranha::integer> hasher;
	BOOST_CHECK_EQUAL(hasher(piranha::integer()),static_cast<std::size_t>(0));
	BOOST_CHECK_EQUAL(hasher(piranha::integer() + piranha::integer(1) - piranha::integer(1)),static_cast<std::size_t>(0));
	BOOST_CHECK_EQUAL(hasher(piranha::integer(1) + piranha::integer(1) - piranha::integer(1)),hasher(piranha::integer(1)));
	BOOST_CHECK_EQUAL(hasher(piranha::integer(-1) + piranha::integer(1) - piranha::integer(1)),hasher(piranha::integer(-1)));
}

BOOST_AUTO_TEST_CASE(integer_sign_test)
{
	BOOST_CHECK_EQUAL(piranha::integer().sign(),0);
	BOOST_CHECK_EQUAL(piranha::integer(-1).sign(),-1);
	BOOST_CHECK_EQUAL(piranha::integer(-10).sign(),-1);
	BOOST_CHECK_EQUAL(piranha::integer(1).sign(),1);
	BOOST_CHECK_EQUAL(piranha::integer(10).sign(),1);
}

BOOST_AUTO_TEST_CASE(integer_math_overloads_test)
{
	BOOST_CHECK(piranha::math::is_zero(piranha::integer()));
	BOOST_CHECK(piranha::math::is_zero(piranha::integer(0)));
	BOOST_CHECK(!piranha::math::is_zero(piranha::integer(-1)));
	BOOST_CHECK(!piranha::math::is_zero(piranha::integer(-10)));
	BOOST_CHECK(!piranha::math::is_zero(piranha::integer(1)));
	BOOST_CHECK(!piranha::math::is_zero(piranha::integer(10)));
	piranha::integer n(0);
	piranha::math::negate(n);
	BOOST_CHECK_EQUAL(n,piranha::integer());
	n = 10;
	piranha::math::negate(n);
	BOOST_CHECK_EQUAL(n,piranha::integer(-10));
	piranha::math::negate(n);
	BOOST_CHECK_EQUAL(n,piranha::integer(10));
	BOOST_CHECK(piranha::has_is_zero<piranha::integer>::value);
}

BOOST_AUTO_TEST_CASE(integer_vector_accumulate_test)
{
	std::vector<piranha::integer> v;
	for (unsigned long i = 0; i < 10000; ++i) {
		v.push_back(piranha::integer(i));
	}
	BOOST_CHECK_EQUAL(std::accumulate(v.begin(),v.end(),piranha::integer(0),
		[](piranha::integer &n1, const piranha::integer &n2){return(std::move(n1) + n2);}),piranha::integer(10000) * piranha::integer(9999) / 2);
}

BOOST_AUTO_TEST_CASE(integer_size_test)
{
	piranha::integer k;
	BOOST_CHECK_EQUAL(k.size(),0u);
	k = 1;
	BOOST_CHECK(k.size() > 0u);
}

BOOST_AUTO_TEST_CASE(integer_allocated_size_test)
{
	piranha::integer k(piranha::integer::nlimbs(0));
	BOOST_CHECK(k.allocated_size() == 1u);
	k = piranha::integer(piranha::integer::nlimbs(10));
	BOOST_CHECK(k.allocated_size() == 10u);
	k = piranha::integer(piranha::integer::nlimbs(100));
	BOOST_CHECK(k.allocated_size() == 100u);
}

BOOST_AUTO_TEST_CASE(integer_sqrt_test)
{
	piranha::integer n(0);
	BOOST_CHECK(n.sqrt() == 0);
	n = -1;
	BOOST_CHECK_THROW(n.sqrt(),std::invalid_argument);
	n = 1;
	BOOST_CHECK(n.sqrt() == 1);
	n = 2;
	BOOST_CHECK(n.sqrt() == 1);
	n = 3;
	BOOST_CHECK(n.sqrt() == 1);
	n = 4;
	BOOST_CHECK(n.sqrt() == 2);
	n = 17;
	BOOST_CHECK(n.sqrt() == 4);
}

BOOST_AUTO_TEST_CASE(integer_primes_test)
{
	piranha::integer n(2);
	BOOST_CHECK(n.nextprime() == 3);
	n = -1;
	BOOST_CHECK_THROW(n.nextprime(),std::invalid_argument);
	n = 5;
	BOOST_CHECK(n.probab_prime_p() == 2);
	n = 6;
	BOOST_CHECK(n.probab_prime_p() == 0);
	BOOST_CHECK_THROW(n.probab_prime_p(-1),std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(integer_sin_cos_test)
{
	BOOST_CHECK_EQUAL(piranha::math::sin(piranha::integer()),0);
	BOOST_CHECK_THROW(piranha::math::sin(piranha::integer(1)),std::invalid_argument);
	BOOST_CHECK_EQUAL(piranha::math::cos(piranha::integer()),1);
	BOOST_CHECK_THROW(piranha::math::cos(piranha::integer(1)),std::invalid_argument);
}

struct check_integral_cast
{
	template <typename T>
	void operator()(const T &x) const
	{
		if (std::is_integral<T>::value) {
			BOOST_CHECK_EQUAL(piranha::math::integral_cast(x),x);
		}
	}
};

BOOST_AUTO_TEST_CASE(integer_integral_cast_test)
{
	BOOST_CHECK(!piranha::has_integral_cast<std::string>::value);
	if (std::numeric_limits<float>::is_iec559 && std::numeric_limits<float>::radix == 2 &&
		std::numeric_limits<float>::has_infinity && std::numeric_limits<float>::has_quiet_NaN)
	{
		BOOST_CHECK_EQUAL(piranha::math::integral_cast(2.f),2);
		BOOST_CHECK_EQUAL(piranha::math::integral_cast(-2.f),-2);
		BOOST_CHECK_THROW(piranha::math::integral_cast(2.5f),std::invalid_argument);
		BOOST_CHECK_THROW(piranha::math::integral_cast(-2.5f),std::invalid_argument);
		BOOST_CHECK_THROW(piranha::math::integral_cast(std::numeric_limits<float>::infinity()),std::invalid_argument);
		BOOST_CHECK_THROW(piranha::math::integral_cast(-std::numeric_limits<float>::infinity()),std::invalid_argument);
		BOOST_CHECK_THROW(piranha::math::integral_cast(std::numeric_limits<float>::quiet_NaN()),std::invalid_argument);
	}
	if (std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::radix == 2 &&
		std::numeric_limits<double>::has_infinity && std::numeric_limits<double>::has_quiet_NaN)
	{
		BOOST_CHECK_EQUAL(piranha::math::integral_cast(2.),2);
		BOOST_CHECK_EQUAL(piranha::math::integral_cast(-2.),-2);
		BOOST_CHECK_THROW(piranha::math::integral_cast(2.5),std::invalid_argument);
		BOOST_CHECK_THROW(piranha::math::integral_cast(-2.5),std::invalid_argument);
		BOOST_CHECK_THROW(piranha::math::integral_cast(std::numeric_limits<double>::infinity()),std::invalid_argument);
		BOOST_CHECK_THROW(piranha::math::integral_cast(-std::numeric_limits<double>::infinity()),std::invalid_argument);
		BOOST_CHECK_THROW(piranha::math::integral_cast(std::numeric_limits<double>::quiet_NaN()),std::invalid_argument);
	}
	// Long double is not interoperable.
	BOOST_CHECK(!piranha::has_integral_cast<long double>::value);
	boost::fusion::for_each(arithmetic_values,check_integral_cast());
	BOOST_CHECK_EQUAL(piranha::math::integral_cast(piranha::integer(-23)),-23);
	BOOST_CHECK(!piranha::has_integral_cast<void *>::value);
	BOOST_CHECK(!piranha::has_integral_cast<std::vector<int>>::value);
	BOOST_CHECK(!piranha::has_integral_cast<std::vector<int> &>::value);
	BOOST_CHECK(!piranha::has_integral_cast<std::vector<int> &&>::value);
	BOOST_CHECK(!piranha::has_integral_cast<const std::vector<int>>::value);
}

BOOST_AUTO_TEST_CASE(integer_partial_test)
{
	BOOST_CHECK_EQUAL(piranha::math::partial(piranha::integer(),""),0);
	BOOST_CHECK_EQUAL(piranha::math::partial(piranha::integer(1),std::string("")),0);
	BOOST_CHECK_EQUAL(piranha::math::partial(piranha::integer(-10),std::string("")),0);
}

BOOST_AUTO_TEST_CASE(integer_evaluate_test)
{
	BOOST_CHECK_EQUAL(piranha::math::evaluate(piranha::integer(),std::unordered_map<std::string,piranha::integer>{}),piranha::integer(0));
	BOOST_CHECK_EQUAL(piranha::math::evaluate(piranha::integer(-2),std::unordered_map<std::string,double>{}),piranha::integer(-2));
	BOOST_CHECK_EQUAL(piranha::math::evaluate(piranha::integer("3"),std::unordered_map<std::string,char>{}),piranha::integer(3));
	BOOST_CHECK((std::is_same<decltype(piranha::math::evaluate(piranha::integer(),std::unordered_map<std::string,int>{})),piranha::integer>::value));
}

BOOST_AUTO_TEST_CASE(integer_subs_test)
{
	BOOST_CHECK_EQUAL(piranha::math::subs(piranha::integer(),"",2),piranha::integer(0));
	BOOST_CHECK_EQUAL(piranha::math::subs(piranha::integer(-2),"foo",4.5),piranha::integer(-2));
	BOOST_CHECK_EQUAL(piranha::math::subs(piranha::integer("3"),"bar","frob"),piranha::integer(3));
	BOOST_CHECK((std::is_same<decltype(piranha::math::subs(piranha::integer(),"",2)),piranha::integer>::value));
	BOOST_CHECK(piranha::has_subs<piranha::integer>::value);
	BOOST_CHECK((piranha::has_subs<piranha::integer,int>::value));
	BOOST_CHECK((piranha::has_subs<piranha::integer,std::string>::value));
	BOOST_CHECK((piranha::has_subs<piranha::integer,const double &>::value));
	BOOST_CHECK((piranha::has_subs<piranha::integer &&,const double &>::value));
}

BOOST_AUTO_TEST_CASE(integer_factorial_test)
{
	BOOST_CHECK_EQUAL(piranha::integer().factorial(),1);
	BOOST_CHECK_EQUAL(piranha::integer(1).factorial(),1);
	BOOST_CHECK_EQUAL(piranha::integer(6).factorial(),1 * 2 * 3 * 4 * 5 * 6);
	BOOST_CHECK_THROW(piranha::integer(-1).factorial(),std::invalid_argument);
	BOOST_CHECK_THROW(piranha::integer(100001L).factorial(),std::invalid_argument);
	BOOST_CHECK_EQUAL(piranha::math::factorial(piranha::integer()),1);
	BOOST_CHECK_EQUAL(piranha::math::factorial(piranha::integer(1)),1);
	BOOST_CHECK_EQUAL(piranha::math::factorial(piranha::integer(6)),1 * 2 * 3 * 4 * 5 * 6);
	BOOST_CHECK_THROW(piranha::math::factorial(piranha::integer(-1)),std::invalid_argument);
	BOOST_CHECK_THROW(piranha::math::factorial(piranha::integer(100001L)),std::invalid_argument);
}

struct check_ipow_subs
{
	template <typename T>
	void operator()(const T &x) const
	{
		BOOST_CHECK_EQUAL(piranha::math::ipow_subs(x,"a",piranha::integer(4),5),x);
	}
};

BOOST_AUTO_TEST_CASE(integer_ipow_subs_test)
{
	boost::fusion::for_each(arithmetic_values,check_ipow_subs());
	BOOST_CHECK_EQUAL(piranha::math::ipow_subs(piranha::integer(-42),"a",piranha::integer(4),5),piranha::integer(-42));
	BOOST_CHECK_EQUAL(piranha::math::ipow_subs(piranha::integer(42),"a",piranha::integer(4),5),piranha::integer(42));
	BOOST_CHECK(piranha::has_ipow_subs<piranha::integer>::value);
	BOOST_CHECK((piranha::has_ipow_subs<piranha::integer,int>::value));
	BOOST_CHECK((piranha::has_ipow_subs<piranha::integer,long>::value));
	BOOST_CHECK((piranha::has_ipow_subs<piranha::integer,double>::value));
}

BOOST_AUTO_TEST_CASE(integer_abs_test)
{
	BOOST_CHECK_EQUAL(piranha::integer(42).abs(),42);
	BOOST_CHECK_EQUAL(piranha::integer(-42).abs(),42);
	BOOST_CHECK_EQUAL(piranha::math::abs(piranha::integer(43)),43);
	BOOST_CHECK_EQUAL(piranha::math::abs(piranha::integer(-43)),43);
}

BOOST_AUTO_TEST_CASE(integer_binomial_test)
{
	BOOST_CHECK_EQUAL(piranha::integer(0).binomial(0),1);
	BOOST_CHECK_EQUAL(piranha::integer(0).binomial(0u),1);
	BOOST_CHECK_EQUAL(piranha::integer(0).binomial(0l),1);
	BOOST_CHECK_EQUAL(piranha::integer(0).binomial(0ull),1);
	BOOST_CHECK_EQUAL(piranha::integer(0).binomial(piranha::integer(0)),1);
	BOOST_CHECK_EQUAL(piranha::integer(1).binomial(0),1);
	BOOST_CHECK_EQUAL(piranha::integer(1).binomial(0u),1);
	BOOST_CHECK_EQUAL(piranha::integer(1).binomial(0l),1);
	BOOST_CHECK_EQUAL(piranha::integer(1).binomial(0ull),1);
	BOOST_CHECK_EQUAL(piranha::integer(1).binomial(piranha::integer(0)),1);
	BOOST_CHECK_EQUAL(piranha::integer(-1).binomial(0),1);
	BOOST_CHECK_EQUAL(piranha::integer(-2).binomial(0u),1);
	BOOST_CHECK_EQUAL(piranha::integer(-3).binomial(0l),1);
	BOOST_CHECK_EQUAL(piranha::integer(-4).binomial(0ull),1);
	BOOST_CHECK_EQUAL(piranha::integer(-5).binomial(piranha::integer(0)),1);
	BOOST_CHECK_EQUAL(piranha::integer(1).binomial(1),1);
	BOOST_CHECK_EQUAL(piranha::integer(-1).binomial(1),-1);
	BOOST_CHECK_EQUAL(piranha::integer(-5).binomial(1),-5);
	BOOST_CHECK_THROW(piranha::integer(-5).binomial(-1),std::invalid_argument);
	BOOST_CHECK_THROW(piranha::integer(-5).binomial(-1l),std::invalid_argument);
	BOOST_CHECK_EQUAL(piranha::integer(5).binomial(1),5);
	BOOST_CHECK_EQUAL(piranha::integer(2).binomial(2),1);
	BOOST_CHECK_EQUAL(piranha::integer(-2).binomial(2),3);
	BOOST_CHECK_EQUAL(piranha::integer(-2).binomial(3),-4);
	BOOST_CHECK_EQUAL(piranha::integer(2).binomial(3),0);
	BOOST_CHECK_EQUAL(piranha::integer(20).binomial(3),1140);
	BOOST_CHECK_EQUAL(piranha::integer(-20).binomial(char(30)),piranha::integer("18851684897584"));
	BOOST_CHECK_EQUAL(piranha::math::binomial(piranha::integer(-20),char(30)),piranha::integer("18851684897584"));
	BOOST_CHECK((piranha::has_binomial<piranha::integer,int>::value));
	BOOST_CHECK((piranha::has_binomial<piranha::integer,unsigned short>::value));
	BOOST_CHECK((piranha::has_binomial<piranha::integer,long long>::value));
	BOOST_CHECK((piranha::has_binomial<piranha::integer,piranha::integer>::value));
	BOOST_CHECK((!piranha::has_binomial<piranha::integer,float>::value));
	BOOST_CHECK((!piranha::has_binomial<piranha::integer,double>::value));
	BOOST_CHECK((!piranha::has_binomial<piranha::integer,std::string>::value));
	// Int - integral.
	BOOST_CHECK((piranha::has_binomial<int,unsigned>::value));
	BOOST_CHECK((piranha::has_binomial<char,long>::value));
	BOOST_CHECK((piranha::has_binomial<int,piranha::integer>::value));
	BOOST_CHECK((!piranha::has_binomial<int,double>::value));
	BOOST_CHECK((!piranha::has_binomial<long,float>::value));
	BOOST_CHECK((!piranha::has_binomial<std::string,int>::value));
	BOOST_CHECK_EQUAL(piranha::math::binomial(20,3),1140);
	BOOST_CHECK_EQUAL(piranha::math::binomial(20,3ul),1140);
	BOOST_CHECK_EQUAL(piranha::math::binomial(20,piranha::integer(3)),1140);
	BOOST_CHECK_THROW(piranha::math::binomial(20,piranha::integer(-3)),std::invalid_argument);
	BOOST_CHECK_EQUAL(piranha::math::binomial(0,1l),0);
	BOOST_CHECK_EQUAL(piranha::math::binomial(0,0l),1);
	BOOST_CHECK_EQUAL(piranha::math::binomial(1,0l),1);
	BOOST_CHECK_EQUAL(piranha::math::binomial(-1,0ull),1);
	BOOST_CHECK_EQUAL(piranha::math::binomial(-3,5ull),-21);
	BOOST_CHECK_EQUAL(piranha::math::binomial(2,0ull),1);
	BOOST_CHECK_EQUAL(piranha::math::binomial(2,1ull),2);
	BOOST_CHECK_EQUAL(piranha::math::binomial(-2,1ull),-2);
	BOOST_CHECK_EQUAL(piranha::math::binomial(14,12),91);
	BOOST_CHECK_EQUAL(piranha::math::binomial(-14,12),piranha::integer("5200300"));
	BOOST_CHECK((std::is_same<piranha::integer,decltype(piranha::math::binomial(0,0))>::value));
	BOOST_CHECK((std::is_same<piranha::integer,decltype(piranha::math::binomial(0,piranha::integer(0)))>::value));
}

BOOST_AUTO_TEST_CASE(integer_is_equality_comparable_test)
{
	BOOST_CHECK(piranha::is_equality_comparable<piranha::integer>::value);
	BOOST_CHECK((piranha::is_equality_comparable<double,piranha::integer>::value));
	BOOST_CHECK((piranha::is_equality_comparable<piranha::integer,int>::value));
	BOOST_CHECK((!piranha::is_equality_comparable<piranha::integer,std::string>::value));
}

BOOST_AUTO_TEST_CASE(integer_t_subs_test)
{
	BOOST_CHECK(!piranha::has_t_subs<piranha::integer>::value);
	BOOST_CHECK((!piranha::has_t_subs<piranha::integer,int>::value));
	BOOST_CHECK((!piranha::has_t_subs<piranha::integer,int,double>::value));
	BOOST_CHECK((!piranha::has_t_subs<piranha::integer &,int>::value));
	BOOST_CHECK((!piranha::has_t_subs<const piranha::integer &,const int &,double &>::value));
	BOOST_CHECK((!piranha::has_t_subs<std::string,piranha::integer,double>::value));
}

BOOST_AUTO_TEST_CASE(integer_type_traits_test)
{
	BOOST_CHECK_EQUAL(piranha::is_nothrow_destructible<piranha::integer>::value,true);
	BOOST_CHECK_EQUAL(piranha::is_nothrow_destructible<const piranha::integer>::value,true);
	BOOST_CHECK(piranha::is_differentiable<piranha::integer>::value);
	BOOST_CHECK(piranha::is_differentiable<piranha::integer &>::value);
	BOOST_CHECK(piranha::is_differentiable<const piranha::integer &>::value);
	BOOST_CHECK(piranha::has_pbracket<piranha::integer>::value);
	BOOST_CHECK(piranha::has_pbracket<piranha::integer &>::value);
	BOOST_CHECK(piranha::has_pbracket<piranha::integer const &>::value);
	BOOST_CHECK(piranha::has_transformation_is_canonical<piranha::integer>::value);
	BOOST_CHECK(piranha::has_transformation_is_canonical<piranha::integer &>::value);
	BOOST_CHECK(piranha::has_transformation_is_canonical<piranha::integer const &>::value);
	BOOST_CHECK(!piranha::has_degree<piranha::integer>::value);
	BOOST_CHECK(piranha::is_addable<piranha::integer>::value);
	BOOST_CHECK((piranha::is_addable<double,piranha::integer>::value));
	BOOST_CHECK((piranha::is_addable<piranha::integer,double>::value));
	BOOST_CHECK((!piranha::is_addable<piranha::integer,std::complex<double>>::value));
	BOOST_CHECK((!piranha::is_addable<std::complex<double>,piranha::integer>::value));
	BOOST_CHECK(piranha::is_subtractable<piranha::integer>::value);
	BOOST_CHECK((piranha::is_subtractable<double,piranha::integer>::value));
	BOOST_CHECK((piranha::is_subtractable<piranha::integer,double>::value));
	BOOST_CHECK((!piranha::is_subtractable<piranha::integer,std::complex<double>>::value));
	BOOST_CHECK((!piranha::is_subtractable<std::complex<double>,piranha::integer>::value));
	BOOST_CHECK(piranha::is_container_element<piranha::integer>::value);
	BOOST_CHECK(piranha::is_ostreamable<piranha::integer>::value);
	BOOST_CHECK(piranha::has_negate<piranha::integer>::value);
	BOOST_CHECK(piranha::has_negate<piranha::integer &>::value);
	BOOST_CHECK(!piranha::has_negate<const piranha::integer &>::value);
	BOOST_CHECK(!piranha::has_negate<const piranha::integer>::value);
	BOOST_CHECK((std::is_same<decltype(piranha::math::negate(*(piranha::integer *)nullptr)),void>::value));
	BOOST_CHECK(piranha::is_hashable<piranha::integer>::value);
	BOOST_CHECK((piranha::is_evaluable<piranha::integer,int>::value));
	BOOST_CHECK((piranha::is_evaluable<piranha::integer,double>::value));
	BOOST_CHECK((piranha::is_evaluable<piranha::integer &,double>::value));
	BOOST_CHECK((piranha::is_evaluable<piranha::integer const &,double>::value));
	BOOST_CHECK((piranha::is_evaluable<piranha::integer &&,double>::value));
	BOOST_CHECK(piranha::has_sine<piranha::integer>::value);
	BOOST_CHECK(piranha::has_cosine<piranha::integer>::value);
	BOOST_CHECK(piranha::has_sine<piranha::integer &>::value);
	BOOST_CHECK(piranha::has_cosine<piranha::integer const &>::value);
	BOOST_CHECK(piranha::has_cosine<piranha::integer &&>::value);
}

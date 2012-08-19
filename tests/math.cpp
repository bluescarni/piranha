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

#include "../src/math.hpp"

#define BOOST_TEST_MODULE math_test
#include <boost/test/unit_test.hpp>

#define FUSION_MAX_VECTOR_SIZE 20

#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/integer_traits.hpp>
#include <cmath>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>

#include "../src/environment.hpp"
#include "../src/integer.hpp"
#include "../src/poisson_series.hpp"
#include "../src/polynomial.hpp"
#include "../src/rational.hpp"
#include "../src/real.hpp"

using namespace piranha;

const boost::fusion::vector<char,short,int,long,long long,unsigned char,unsigned short,unsigned,unsigned long,unsigned long long,float,double,long double> arithmetic_values(
	(char)-42,(short)42,-42,42L,-42LL,
	(unsigned char)42,(unsigned short)42,42U,42UL,42ULL,
	23.456f,-23.456,23.456L
);

struct check_negate
{
	template <typename T>
	void operator()(const T &value) const
	{
		if (std::is_signed<T>::value) {
			T negation(value);
			math::negate(negation);
			BOOST_CHECK_EQUAL(negation,-value);
		}
	}
};

BOOST_AUTO_TEST_CASE(negate_test)
{
	environment env;
	boost::fusion::for_each(arithmetic_values,check_negate());
}

const boost::fusion::vector<char,short,int,long,long long,unsigned char,unsigned short,unsigned,unsigned long,unsigned long long,float,double,long double> zeroes(
	(char)0,(short)0,0,0L,0LL,
	(unsigned char)0,(unsigned short)0,0U,0UL,0ULL,
	0.f,-0.,0.L
);

struct check_is_zero_01
{
	template <typename T>
	void operator()(const T &value) const
	{
		BOOST_CHECK(math::is_zero(value));
		BOOST_CHECK(math::is_zero(std::complex<T>(value)));
	}
};

struct check_is_zero_02
{
	template <typename T>
	void operator()(const T &value) const
	{
		BOOST_CHECK(!math::is_zero(value));
		BOOST_CHECK(!math::is_zero(std::complex<T>(value)));
	}
};

BOOST_AUTO_TEST_CASE(is_zero_test)
{
	boost::fusion::for_each(zeroes,check_is_zero_01());
	boost::fusion::for_each(arithmetic_values,check_is_zero_02());
}

struct check_multiply_accumulate
{
	template <typename T>
	void operator()(const T &) const
	{
		T x(2);
		math::multiply_accumulate(x,T(4),T(6));
		BOOST_CHECK_EQUAL(x,T(2) + T(4) * T(6));
		if (std::is_signed<T>::value || std::is_floating_point<T>::value) {
			x = T(-2);
			math::multiply_accumulate(x,T(5),T(-7));
			BOOST_CHECK_EQUAL(x,T(-2) + T(5) * T(-7));
		}
	}
};

BOOST_AUTO_TEST_CASE(multiply_accumulate_test)
{
	boost::fusion::for_each(arithmetic_values,check_multiply_accumulate());
}

BOOST_AUTO_TEST_CASE(pow_test)
{
	BOOST_CHECK(math::pow(2.,2.) == std::pow(2.,2.));
	BOOST_CHECK(math::pow(2.f,2.) == std::pow(2.f,2.));
	BOOST_CHECK(math::pow(2.,2.f) == std::pow(2,2.f));
	BOOST_CHECK((std::is_same<decltype(math::pow(2.,2.)),double>::value));
	BOOST_CHECK((std::is_same<decltype(math::pow(2.f,2.f)),float>::value));
	BOOST_CHECK((std::is_same<decltype(math::pow(2.,2.f)),double>::value));
	BOOST_CHECK((std::is_same<decltype(math::pow(2.f,2.)),double>::value));
	BOOST_CHECK(math::pow(2.,2) == std::pow(2.,2));
	BOOST_CHECK(math::pow(2.f,2) == std::pow(2.f,2));
	BOOST_CHECK((std::is_same<decltype(math::pow(2.,2)),double>::value));
	BOOST_CHECK((std::is_same<decltype(math::pow(2.f,2)),double>::value));
	BOOST_CHECK(math::pow(2.,integer(2)) == std::pow(2.,2));
	BOOST_CHECK((std::is_same<decltype(math::pow(2.,integer(2))),double>::value));
	if (boost::integer_traits<long long>::const_max > boost::integer_traits<int>::const_max) {
		BOOST_CHECK_THROW(math::pow(2.,static_cast<long long>(boost::integer_traits<int>::const_max)+1),std::bad_cast);
	}
	BOOST_CHECK_THROW(math::pow(2.,integer(boost::integer_traits<int>::const_max)+1),std::overflow_error);
	BOOST_CHECK((is_exponentiable<double,double>::value));
	BOOST_CHECK((is_exponentiable<float,double>::value));
	BOOST_CHECK((is_exponentiable<double,float>::value));
	BOOST_CHECK((is_exponentiable<double,int>::value));
	BOOST_CHECK((is_exponentiable<float,char>::value));
	BOOST_CHECK((is_exponentiable<float,integer>::value));
	BOOST_CHECK((is_exponentiable<double,integer>::value));
	BOOST_CHECK((!is_exponentiable<std::string,integer>::value));
	BOOST_CHECK((!is_exponentiable<int,integer>::value));
}

BOOST_AUTO_TEST_CASE(sin_cos_test)
{
	BOOST_CHECK(math::sin(1.f) == std::sin(1.f));
	BOOST_CHECK(math::sin(2.) == std::sin(2.));
	BOOST_CHECK(math::cos(1.f) == std::cos(1.f));
	BOOST_CHECK(math::cos(2.) == std::cos(2.));
	BOOST_CHECK(math::cos(1.L) == std::cos(1.L));
	BOOST_CHECK(math::cos(2.L) == std::cos(2.L));
}

BOOST_AUTO_TEST_CASE(partial_test)
{
	BOOST_CHECK(piranha::is_differentiable<int>::value);
	BOOST_CHECK(piranha::is_differentiable<long>::value);
	BOOST_CHECK(piranha::is_differentiable<double>::value);
	BOOST_CHECK(!piranha::is_differentiable<std::string>::value);
	BOOST_CHECK_EQUAL(math::partial(1,""),0);
	BOOST_CHECK_EQUAL(math::partial(1.,""),double(0));
	BOOST_CHECK_EQUAL(math::partial(2L,""),0L);
	BOOST_CHECK_EQUAL(math::partial(2LL,std::string("")),0LL);
}

BOOST_AUTO_TEST_CASE(evaluate_test)
{
	BOOST_CHECK_EQUAL(math::evaluate(5,std::unordered_map<std::string,double>{}),5);
	BOOST_CHECK((std::is_same<decltype(math::evaluate(5,std::unordered_map<std::string,double>{})),int>::value));
	BOOST_CHECK_EQUAL(math::evaluate(5.,std::unordered_map<std::string,int>{}),5.);
	BOOST_CHECK((std::is_same<decltype(math::evaluate(5.,std::unordered_map<std::string,short>{})),double>::value));
	BOOST_CHECK_EQUAL(math::evaluate(5ul,std::unordered_map<std::string,double>{}),5ul);
	BOOST_CHECK((std::is_same<decltype(math::evaluate(5ul,std::unordered_map<std::string,short>{})),unsigned long>::value));
}

BOOST_AUTO_TEST_CASE(subs_test)
{
	BOOST_CHECK_EQUAL(math::subs(5,"",6),5);
	BOOST_CHECK((std::is_same<decltype(math::subs(5,"",6)),int>::value));
	BOOST_CHECK_EQUAL(math::subs(5.,"",6),5.);
	BOOST_CHECK((std::is_same<decltype(math::subs(5.,"",6)),double>::value));
	BOOST_CHECK_EQUAL(math::subs(10ll,"","foo"),10ll);
	BOOST_CHECK((std::is_same<decltype(math::subs(10ll,"","foo")),long long>::value));
}

BOOST_AUTO_TEST_CASE(integrate_test)
{
	BOOST_CHECK(!piranha::is_integrable<int>::value);
	BOOST_CHECK(!piranha::is_integrable<long>::value);
	BOOST_CHECK(!piranha::is_integrable<double>::value);
	BOOST_CHECK(!piranha::is_integrable<integer>::value);
	BOOST_CHECK(!piranha::is_integrable<real>::value);
	BOOST_CHECK(!piranha::is_integrable<rational>::value);
	BOOST_CHECK(!piranha::is_integrable<std::string>::value);
}

BOOST_AUTO_TEST_CASE(pbracket_test)
{
	typedef polynomial<rational> p_type;
	BOOST_CHECK_EQUAL(math::pbracket(p_type{},p_type{},{},{}),p_type(0));
	BOOST_CHECK_THROW(math::pbracket(p_type{},p_type{},{"p"},{}),std::invalid_argument);
	BOOST_CHECK_THROW(math::pbracket(p_type{},p_type{},{"p"},{"q","r"}),std::invalid_argument);
	BOOST_CHECK_THROW(math::pbracket(p_type{},p_type{},{"p","p"},{"q","r"}),std::invalid_argument);
	BOOST_CHECK_THROW(math::pbracket(p_type{},p_type{},{"p","q"},{"q","q"}),std::invalid_argument);
	BOOST_CHECK_EQUAL(math::pbracket(p_type{},p_type{},{"x","y"},{"a","b"}),p_type(0));
	// Pendulum Hamiltonian.
	typedef poisson_series<polynomial<rational>> ps_type;
	auto m = ps_type{"m"}, p = ps_type{"p"}, l = ps_type{"l"}, g = ps_type{"g"}, th = ps_type{"theta"};
	auto H_p = p*p * (2*m*l*l).pow(-1) + m*g*l*math::cos(th);
	BOOST_CHECK_EQUAL(math::pbracket(H_p,H_p,{"p"},{"theta"}),0);
	// Two body problem.
	auto x = ps_type{"x"}, y = ps_type{"y"}, z = ps_type{"z"};
	auto vx = ps_type{"vx"}, vy = ps_type{"vy"}, vz = ps_type{"vz"}, r = ps_type{"r"};
	auto H_2 = (vx*vx + vy*vy + vz*vz) / 2 - r.pow(-1);
	ps_type::register_custom_derivative("x",[&x,&r](const ps_type &p) {
		return p.partial("x") - p.partial("r") * x * r.pow(-3);
	});
	ps_type::register_custom_derivative("y",[&y,&r](const ps_type &p) {
		return p.partial("y") - p.partial("r") * y * r.pow(-3);
	});
	ps_type::register_custom_derivative("z",[&z,&r](const ps_type &p) {
		return p.partial("z") - p.partial("r") * z * r.pow(-3);
	});
	BOOST_CHECK_EQUAL(math::pbracket(H_2,H_2,{"vx","vy","vz"},{"x","y","z"}),0);
	// Angular momentum integral.
	auto Gx = y*vz - z*vy, Gy = z*vx - x*vz, Gz = x*vy - y*vx;
	BOOST_CHECK_EQUAL(math::pbracket(H_2,Gx,{"vx","vy","vz"},{"x","y","z"}),0);
	BOOST_CHECK_EQUAL(math::pbracket(H_2,Gy,{"vx","vy","vz"},{"x","y","z"}),0);
	BOOST_CHECK_EQUAL(math::pbracket(H_2,Gz,{"vx","vy","vz"},{"x","y","z"}),0);
	BOOST_CHECK(math::pbracket(H_2,Gz + x,{"vx","vy","vz"},{"x","y","z"}) != 0);
}

BOOST_AUTO_TEST_CASE(abs_test)
{
	BOOST_CHECK_EQUAL(math::abs((signed char)(4)),(signed char)(4));
	BOOST_CHECK_EQUAL(math::abs((signed char)(-4)),(signed char)(4));
	BOOST_CHECK_EQUAL(math::abs(short(4)),short(4));
	BOOST_CHECK_EQUAL(math::abs(short(-4)),short(4));
	BOOST_CHECK_EQUAL(math::abs(4),4);
	BOOST_CHECK_EQUAL(math::abs(-4),4);
	BOOST_CHECK_EQUAL(math::abs(4l),4l);
	BOOST_CHECK_EQUAL(math::abs(-4l),4l);
	BOOST_CHECK_EQUAL(math::abs(4ll),4ll);
	BOOST_CHECK_EQUAL(math::abs(-4ll),4ll);
	BOOST_CHECK_EQUAL(math::abs((unsigned char)(4)),(unsigned char)(4));
	BOOST_CHECK_EQUAL(math::abs((unsigned short)(4)),(unsigned short)(4));
	BOOST_CHECK_EQUAL(math::abs(4u),4u);
	BOOST_CHECK_EQUAL(math::abs(4lu),4lu);;
	BOOST_CHECK_EQUAL(math::abs(4llu),4llu);
	BOOST_CHECK_EQUAL(math::abs(1.23f),1.23f);
	BOOST_CHECK_EQUAL(math::abs(-1.23f),1.23f);
	BOOST_CHECK_EQUAL(math::abs(1.23),1.23);
	BOOST_CHECK_EQUAL(math::abs(-1.23),1.23);
	BOOST_CHECK_EQUAL(math::abs(1.23l),1.23l);
	BOOST_CHECK_EQUAL(math::abs(-1.23l),1.23l);
}

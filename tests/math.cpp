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
#include <complex>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "../src/base_term.hpp"
#include "../src/environment.hpp"
#include "../src/forwarding.hpp"
#include "../src/integer.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/monomial.hpp"
#include "../src/poisson_series.hpp"
#include "../src/polynomial.hpp"
#include "../src/rational.hpp"
#include "../src/real.hpp"
#include "../src/symbol.hpp"
#include "../src/symbol_set.hpp"

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
		if (std::is_signed<T>::value && std::is_integral<T>::value) {
			T negation(value);
			math::negate(negation);
			BOOST_CHECK_EQUAL(negation,-value);
		}
		BOOST_CHECK(has_negate<T>::value);
	}
};

struct no_negate {};

struct no_negate2
{
	no_negate2 operator-() const;
	no_negate2 &operator=(no_negate2 &) = delete;
};

struct yes_negate
{
	yes_negate operator-() const;
};

BOOST_AUTO_TEST_CASE(math_negate_test)
{
	environment env;
	boost::fusion::for_each(arithmetic_values,check_negate());
	BOOST_CHECK(!has_negate<no_negate>::value);
	BOOST_CHECK(!has_negate<no_negate2>::value);
	BOOST_CHECK(has_negate<yes_negate>::value);
	BOOST_CHECK(has_negate<std::complex<double>>::value);
	BOOST_CHECK(has_negate<int &>::value);
	BOOST_CHECK(has_negate<int &&>::value);
	BOOST_CHECK(!has_negate<int const>::value);
	BOOST_CHECK(!has_negate<int const &>::value);
	BOOST_CHECK(!has_negate<std::complex<float> const &>::value);
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
	}
};

struct check_is_zero_02
{
	template <typename T>
	void operator()(const T &value) const
	{
		BOOST_CHECK(!math::is_zero(value));
	}
};

struct trivial
{};

struct trivial_a
{};

struct trivial_b
{};

struct trivial_c
{};

namespace piranha { namespace math {

template <>
struct is_zero_impl<trivial_a,void>
{
	char operator()(const trivial_a &);
};

template <>
struct is_zero_impl<trivial_b,void>
{
	char operator()(const trivial_a &);
};

template <>
struct is_zero_impl<trivial_c,void>
{
	std::string operator()(const trivial_c &);
};

}}

BOOST_AUTO_TEST_CASE(math_is_zero_test)
{
	boost::fusion::for_each(zeroes,check_is_zero_01());
	boost::fusion::for_each(arithmetic_values,check_is_zero_02());
	BOOST_CHECK(has_is_zero<int>::value);
	BOOST_CHECK(has_is_zero<char>::value);
	BOOST_CHECK(has_is_zero<double>::value);
	BOOST_CHECK(has_is_zero<double &>::value);
	BOOST_CHECK(has_is_zero<double &&>::value);
	BOOST_CHECK(has_is_zero<const double &>::value);
	BOOST_CHECK(has_is_zero<unsigned>::value);
	BOOST_CHECK(has_is_zero<std::complex<double>>::value);
	BOOST_CHECK(has_is_zero<std::complex<long double>>::value);
	BOOST_CHECK(has_is_zero<std::complex<float>>::value);
	BOOST_CHECK(math::is_zero(std::complex<double>(0.)));
	BOOST_CHECK(math::is_zero(std::complex<double>(0.,0.)));
	BOOST_CHECK(!math::is_zero(std::complex<double>(1.)));
	BOOST_CHECK(!math::is_zero(std::complex<double>(1.,-1.)));
	BOOST_CHECK(!has_is_zero<std::string>::value);
	BOOST_CHECK((!has_is_zero<trivial>::value));
	BOOST_CHECK((!has_is_zero<trivial &>::value));
	BOOST_CHECK((!has_is_zero<trivial &&>::value));
	BOOST_CHECK((!has_is_zero<const trivial &&>::value));
	BOOST_CHECK(has_is_zero<trivial_a>::value);
	BOOST_CHECK(has_is_zero<trivial_a &>::value);
	BOOST_CHECK(!has_is_zero<trivial_b>::value);
	BOOST_CHECK(!has_is_zero<trivial_c>::value);
}

struct no_fma{};

struct check_multiply_accumulate
{
	template <typename T>
	void operator()(const T &, typename std::enable_if<
		!std::is_same<T,char>::value && !std::is_same<T,unsigned char>::value && !std::is_same<T,signed char>::value &&
		!std::is_same<T,short>::value && !std::is_same<T,unsigned short>::value
		>::type * = nullptr) const
	{
		BOOST_CHECK(has_multiply_accumulate<T>::value);
		BOOST_CHECK(has_multiply_accumulate<T &>::value);
		BOOST_CHECK((has_multiply_accumulate<T,T>::value));
		BOOST_CHECK((has_multiply_accumulate<T &,T &>::value));
		BOOST_CHECK((has_multiply_accumulate<T &,T const &>::value));
		BOOST_CHECK((has_multiply_accumulate<T,T,T>::value));
		BOOST_CHECK((has_multiply_accumulate<T &,const T,T &>::value));
		BOOST_CHECK((has_multiply_accumulate<T &,const T,T const &>::value));
		BOOST_CHECK(!has_multiply_accumulate<const T>::value);
		BOOST_CHECK(!has_multiply_accumulate<const T &>::value);
		T x(2);
		math::multiply_accumulate(x,T(4),T(6));
		BOOST_CHECK_EQUAL(x,T(2) + T(4) * T(6));
		if ((std::is_signed<T>::value && std::is_integral<T>::value) || std::is_floating_point<T>::value) {
			x = T(-2);
			math::multiply_accumulate(x,T(5),T(-7));
			BOOST_CHECK_EQUAL(x,T(-2) + T(5) * T(-7));
		}
	}
	// NOTE: avoid testing with char and short types, the promotion rules will generate warnings about assigning int to
	// narrower types.
	template <typename T>
	void operator()(const T &, typename std::enable_if<std::is_same<T,char>::value || std::is_same<T,unsigned char>::value ||
		std::is_same<T,signed char>::value || std::is_same<T,short>::value || std::is_same<T,unsigned short>::value>::type * = nullptr) const
	{}
};

BOOST_AUTO_TEST_CASE(math_multiply_accumulate_test)
{
	boost::fusion::for_each(arithmetic_values,check_multiply_accumulate());
	BOOST_CHECK(!has_multiply_accumulate<no_fma>::value);
	BOOST_CHECK(!has_multiply_accumulate<no_fma const &>::value);
	BOOST_CHECK(!has_multiply_accumulate<no_fma &>::value);
}

BOOST_AUTO_TEST_CASE(math_pow_test)
{
	BOOST_CHECK(math::pow(2.,2.) == std::pow(2.,2.));
	BOOST_CHECK(math::pow(2.f,2.) == std::pow(2.f,2.));
	BOOST_CHECK(math::pow(2.,2.f) == std::pow(2.,2.f));
	BOOST_CHECK((std::is_same<decltype(math::pow(2.,2.)),double>::value));
	BOOST_CHECK((std::is_same<decltype(math::pow(2.f,2.f)),float>::value));
	BOOST_CHECK((std::is_same<decltype(math::pow(2.,2.f)),double>::value));
	BOOST_CHECK((std::is_same<decltype(math::pow(2.f,2.)),double>::value));
	BOOST_CHECK((std::is_same<decltype(math::pow(2.f,2)),double>::value));
	BOOST_CHECK((std::is_same<decltype(math::pow(2.f,2.L)),long double>::value));
	BOOST_CHECK(math::pow(2.,2) == std::pow(2.,2));
	BOOST_CHECK(math::pow(2.f,2) == std::pow(2.f,2));
	BOOST_CHECK((std::is_same<decltype(math::pow(2.,2)),double>::value));
	BOOST_CHECK((std::is_same<decltype(math::pow(2.f,2)),double>::value));
	BOOST_CHECK((std::is_same<decltype(math::pow(2.f,char(2))),double>::value));
	BOOST_CHECK((is_exponentiable<double,double>::value));
	BOOST_CHECK((is_exponentiable<double,unsigned short>::value));
	BOOST_CHECK((is_exponentiable<double &,double>::value));
	BOOST_CHECK((is_exponentiable<const double,double>::value));
	BOOST_CHECK((is_exponentiable<double &,double &>::value));
	BOOST_CHECK((is_exponentiable<double &,double const &>::value));
	BOOST_CHECK((is_exponentiable<double,double &>::value));
	BOOST_CHECK((is_exponentiable<float,double>::value));
	BOOST_CHECK((is_exponentiable<double,float>::value));
	BOOST_CHECK((is_exponentiable<double,int>::value));
	BOOST_CHECK((is_exponentiable<float,char>::value));
}

BOOST_AUTO_TEST_CASE(math_sin_cos_test)
{
	BOOST_CHECK(math::sin(1.f) == std::sin(1.f));
	BOOST_CHECK(math::sin(2.) == std::sin(2.));
	BOOST_CHECK(math::cos(1.f) == std::cos(1.f));
	BOOST_CHECK(math::cos(2.) == std::cos(2.));
	BOOST_CHECK(math::cos(1.L) == std::cos(1.L));
	BOOST_CHECK(math::cos(2.L) == std::cos(2.L));
}

BOOST_AUTO_TEST_CASE(math_partial_test)
{
	BOOST_CHECK(piranha::is_differentiable<int>::value);
	BOOST_CHECK(piranha::is_differentiable<long>::value);
	BOOST_CHECK(piranha::is_differentiable<double>::value);
	BOOST_CHECK(piranha::is_differentiable<double &>::value);
	BOOST_CHECK(piranha::is_differentiable<double &&>::value);
	BOOST_CHECK(piranha::is_differentiable<double const &>::value);
	BOOST_CHECK(!piranha::is_differentiable<std::string>::value);
	BOOST_CHECK(!piranha::is_differentiable<std::string &>::value);
	BOOST_CHECK_EQUAL(math::partial(1,""),0);
	BOOST_CHECK_EQUAL(math::partial(1.,""),double(0));
	BOOST_CHECK_EQUAL(math::partial(2L,""),0L);
	BOOST_CHECK_EQUAL(math::partial(2LL,std::string("")),0LL);
}

BOOST_AUTO_TEST_CASE(math_evaluate_test)
{
	BOOST_CHECK_EQUAL(math::evaluate(5,std::unordered_map<std::string,double>{}),5);
	BOOST_CHECK_EQUAL(math::evaluate(std::complex<float>(5.,4.),std::unordered_map<std::string,double>{}),std::complex<float>(5.,4.));
	BOOST_CHECK_EQUAL(math::evaluate(std::complex<double>(5.,4.),std::unordered_map<std::string,double>{}),std::complex<double>(5.,4.));
	BOOST_CHECK_EQUAL(math::evaluate(std::complex<long double>(5.,4.),std::unordered_map<std::string,double>{}),std::complex<long double>(5.,4.));
	BOOST_CHECK((std::is_same<decltype(math::evaluate(5,std::unordered_map<std::string,double>{})),int>::value));
	BOOST_CHECK_EQUAL(math::evaluate(5.,std::unordered_map<std::string,int>{}),5.);
	BOOST_CHECK((std::is_same<decltype(math::evaluate(5.,std::unordered_map<std::string,short>{})),double>::value));
	BOOST_CHECK_EQUAL(math::evaluate(5ul,std::unordered_map<std::string,double>{}),5ul);
	BOOST_CHECK((std::is_same<decltype(math::evaluate(5ul,std::unordered_map<std::string,short>{})),unsigned long>::value));
}

BOOST_AUTO_TEST_CASE(math_subs_test)
{
	BOOST_CHECK_EQUAL(math::subs(5,"",6),5);
	BOOST_CHECK((std::is_same<decltype(math::subs(5,"",6)),int>::value));
	BOOST_CHECK_EQUAL(math::subs(5.,"",6),5.);
	BOOST_CHECK((std::is_same<decltype(math::subs(5.,"",6)),double>::value));
	BOOST_CHECK_EQUAL(math::subs(10ll,"","foo"),10ll);
	BOOST_CHECK((std::is_same<decltype(math::subs(10ll,"","foo")),long long>::value));
	BOOST_CHECK(has_subs<double>::value);
	BOOST_CHECK((has_subs<int,double>::value));
	BOOST_CHECK((has_subs<int,char>::value));
	BOOST_CHECK((has_subs<int &,char>::value));
	BOOST_CHECK((has_subs<int,char &>::value));
	BOOST_CHECK((has_subs<int &,const char &>::value));
	BOOST_CHECK(!has_subs<std::string>::value);
	BOOST_CHECK((!has_subs<std::string,int>::value));
	BOOST_CHECK((!has_subs<std::string &,int>::value));
	BOOST_CHECK((has_subs<int,std::string>::value));
	BOOST_CHECK((has_subs<int,std::string &&>::value));
	BOOST_CHECK((has_subs<const int,std::string &&>::value));
}

BOOST_AUTO_TEST_CASE(math_integrate_test)
{
	BOOST_CHECK(!piranha::is_integrable<int>::value);
	BOOST_CHECK(!piranha::is_integrable<int const>::value);
	BOOST_CHECK(!piranha::is_integrable<int &>::value);
	BOOST_CHECK(!piranha::is_integrable<int const &>::value);
	BOOST_CHECK(!piranha::is_integrable<long>::value);
	BOOST_CHECK(!piranha::is_integrable<double>::value);
	BOOST_CHECK(!piranha::is_integrable<real>::value);
	BOOST_CHECK(!piranha::is_integrable<rational>::value);
	BOOST_CHECK(!piranha::is_integrable<std::string>::value);
}

BOOST_AUTO_TEST_CASE(math_pbracket_test)
{
	BOOST_CHECK(has_pbracket<int>::value);
	BOOST_CHECK(has_pbracket<double>::value);
	BOOST_CHECK(has_pbracket<long double>::value);
	BOOST_CHECK(!has_pbracket<std::string>::value);
	typedef polynomial<rational,int> p_type;
	BOOST_CHECK(has_pbracket<p_type>::value);
	BOOST_CHECK_EQUAL(math::pbracket(p_type{},p_type{},std::vector<std::string>{},std::vector<std::string>{}),p_type(0));
	BOOST_CHECK_THROW(math::pbracket(p_type{},p_type{},{"p"},std::vector<std::string>{}),std::invalid_argument);
	BOOST_CHECK_THROW(math::pbracket(p_type{},p_type{},{"p"},{"q","r"}),std::invalid_argument);
	BOOST_CHECK_THROW(math::pbracket(p_type{},p_type{},{"p","p"},{"q","r"}),std::invalid_argument);
	BOOST_CHECK_THROW(math::pbracket(p_type{},p_type{},{"p","q"},{"q","q"}),std::invalid_argument);
	BOOST_CHECK_EQUAL(math::pbracket(p_type{},p_type{},{"x","y"},{"a","b"}),p_type(0));
	// Pendulum Hamiltonian.
	typedef poisson_series<polynomial<rational,int>> ps_type;
	BOOST_CHECK(has_pbracket<ps_type>::value);
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

BOOST_AUTO_TEST_CASE(math_abs_test)
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

BOOST_AUTO_TEST_CASE(math_has_t_degree_test)
{
	BOOST_CHECK(!has_t_degree<int>::value);
	BOOST_CHECK(!has_t_degree<const int>::value);
	BOOST_CHECK(!has_t_degree<int &>::value);
	BOOST_CHECK(!has_t_degree<const int &>::value);
	BOOST_CHECK(!has_t_degree<std::string>::value);
	BOOST_CHECK(!has_t_degree<double>::value);
}

BOOST_AUTO_TEST_CASE(math_has_t_ldegree_test)
{
	BOOST_CHECK(!has_t_ldegree<int>::value);
	BOOST_CHECK(!has_t_ldegree<int const>::value);
	BOOST_CHECK(!has_t_ldegree<int &>::value);
	BOOST_CHECK(!has_t_ldegree<const int &>::value);
	BOOST_CHECK(!has_t_ldegree<std::string>::value);
	BOOST_CHECK(!has_t_ldegree<double>::value);
}

BOOST_AUTO_TEST_CASE(math_has_t_order_test)
{
	BOOST_CHECK(!has_t_order<int>::value);
	BOOST_CHECK(!has_t_order<const int>::value);
	BOOST_CHECK(!has_t_order<int &>::value);
	BOOST_CHECK(!has_t_order<int const &>::value);
	BOOST_CHECK(!has_t_order<std::string>::value);
	BOOST_CHECK(!has_t_order<double>::value);
}

BOOST_AUTO_TEST_CASE(math_has_t_lorder_test)
{
	BOOST_CHECK(!has_t_lorder<int>::value);
	BOOST_CHECK(!has_t_lorder<const int>::value);
	BOOST_CHECK(!has_t_lorder<int &>::value);
	BOOST_CHECK(!has_t_lorder<int const &>::value);
	BOOST_CHECK(!has_t_lorder<std::string>::value);
	BOOST_CHECK(!has_t_lorder<double>::value);
}

BOOST_AUTO_TEST_CASE(math_key_has_t_degree_test)
{
	BOOST_CHECK(!key_has_t_degree<monomial<int>>::value);
	BOOST_CHECK(!key_has_t_degree<kronecker_monomial<>>::value);
}

BOOST_AUTO_TEST_CASE(math_key_has_t_ldegree_test)
{
	BOOST_CHECK(!key_has_t_ldegree<monomial<int>>::value);
	BOOST_CHECK(!key_has_t_ldegree<kronecker_monomial<>>::value);
}

BOOST_AUTO_TEST_CASE(math_key_has_t_order_test)
{
	BOOST_CHECK(!key_has_t_order<monomial<int>>::value);
	BOOST_CHECK(!key_has_t_order<kronecker_monomial<>>::value);
}

BOOST_AUTO_TEST_CASE(math_key_has_t_lorder_test)
{
	BOOST_CHECK(!key_has_t_lorder<monomial<int>>::value);
	BOOST_CHECK(!key_has_t_lorder<kronecker_monomial<>>::value);
}

BOOST_AUTO_TEST_CASE(math_binomial_test)
{
	BOOST_CHECK((std::is_same<double,decltype(math::binomial(0.,0))>::value));
	BOOST_CHECK((std::is_same<double,decltype(math::binomial(0.,integer(0)))>::value));
	BOOST_CHECK((std::is_same<double,decltype(math::binomial(0.,0u))>::value));
	BOOST_CHECK((std::is_same<double,decltype(math::binomial(0.,0l))>::value));
	BOOST_CHECK((std::is_same<float,decltype(math::binomial(0.f,0))>::value));
	BOOST_CHECK((std::is_same<float,decltype(math::binomial(0.f,0u))>::value));
	BOOST_CHECK((std::is_same<float,decltype(math::binomial(0.f,0ll))>::value));
	BOOST_CHECK((std::is_same<long double,decltype(math::binomial(0.l,0))>::value));
	BOOST_CHECK((std::is_same<long double,decltype(math::binomial(0.l,char(0)))>::value));
	BOOST_CHECK((std::is_same<long double,decltype(math::binomial(0.l,short(0)))>::value));
	if (!std::numeric_limits<double>::is_iec559 || !std::numeric_limits<long double>::is_iec559 ||
		!std::numeric_limits<float>::is_iec559)
	{
		return;
	}
	BOOST_CHECK_EQUAL(math::binomial(0.,0),1.);
	BOOST_CHECK_EQUAL(math::binomial(0.,integer(0u)),1.);
	BOOST_CHECK_EQUAL(math::binomial(0.,0u),1.);
	BOOST_CHECK_EQUAL(math::binomial(0.f,0),1.f);
	BOOST_CHECK_EQUAL(math::binomial(0.l,0),1.l);
	BOOST_CHECK_THROW(math::binomial(0.,-1),std::invalid_argument);
	BOOST_CHECK_EQUAL(math::binomial(1.,0),1.);
	BOOST_CHECK_EQUAL(math::binomial(2.,0),1.);
	BOOST_CHECK_EQUAL(math::binomial(-1.,0),1.);
	BOOST_CHECK_EQUAL(math::binomial(-2.,0),1.);
	BOOST_CHECK_EQUAL(math::binomial(0.,1),0.);
	BOOST_CHECK_EQUAL(math::binomial(0.,2),0.);
	BOOST_CHECK_EQUAL(math::binomial(1.,0u),1.);
	BOOST_CHECK_EQUAL(math::binomial(2.,0u),1.);
	BOOST_CHECK_EQUAL(math::binomial(-1.,0u),1.);
	BOOST_CHECK_EQUAL(math::binomial(-2.,0u),1.);
	BOOST_CHECK_EQUAL(math::binomial(0.,1u),0.);
	BOOST_CHECK_EQUAL(math::binomial(0.,2u),0.);
	BOOST_CHECK_EQUAL(math::binomial(1.,0l),1.);
	BOOST_CHECK_EQUAL(math::binomial(2.,0l),1.);
	BOOST_CHECK_EQUAL(math::binomial(-1.,0l),1.);
	BOOST_CHECK_EQUAL(math::binomial(-2.,0l),1.);
	BOOST_CHECK_EQUAL(math::binomial(0.,1l),0.);
	BOOST_CHECK_EQUAL(math::binomial(0.,2l),0.);
	BOOST_CHECK_EQUAL(math::binomial(1.,0ull),1.);
	BOOST_CHECK_EQUAL(math::binomial(2.,0ull),1.);
	BOOST_CHECK_EQUAL(math::binomial(0.,1ull),0.);
	BOOST_CHECK_EQUAL(math::binomial(0.,2ull),0.);
	BOOST_CHECK_EQUAL(math::binomial(1.,1ull),1.);
	BOOST_CHECK_EQUAL(math::binomial(-1.,1ull),-1.);
	BOOST_CHECK_EQUAL(math::binomial(-5.,1ull),-5.);
	BOOST_CHECK_EQUAL(math::binomial(5.,1ull),5.);
	BOOST_CHECK_EQUAL(math::binomial(2.,2ull),1.);
	BOOST_CHECK_EQUAL(math::binomial(-2.,2ull),3.);
	BOOST_CHECK_EQUAL(math::binomial(-2.,3ull),-4.);
	BOOST_CHECK_EQUAL(math::binomial(2.,3ull),0.);
	BOOST_CHECK_EQUAL(math::binomial(20.,short(3)),1140.);
	BOOST_CHECK((has_binomial<double,int>::value));
	BOOST_CHECK((has_binomial<double,unsigned>::value));
	BOOST_CHECK((has_binomial<float,char>::value));
	BOOST_CHECK((!has_binomial<float,float>::value));
	BOOST_CHECK((!has_binomial<float,double>::value));
}

BOOST_AUTO_TEST_CASE(math_t_subs_test)
{
	BOOST_CHECK(!has_t_subs<double>::value);
	BOOST_CHECK((!has_t_subs<int,double>::value));
	BOOST_CHECK((!has_t_subs<int,char>::value));
	BOOST_CHECK((!has_t_subs<int &,char>::value));
	BOOST_CHECK((!has_t_subs<int,char &>::value));
	BOOST_CHECK((!has_t_subs<int,char &,short>::value));
	BOOST_CHECK((!has_t_subs<int &,const char &>::value));
	BOOST_CHECK((!has_t_subs<int &,const char &, const std::string &>::value));
	BOOST_CHECK(!has_t_subs<std::string>::value);
	BOOST_CHECK((!has_t_subs<std::string,int>::value));
	BOOST_CHECK((!has_t_subs<std::string &,int>::value));
	BOOST_CHECK((!has_t_subs<std::string &,int,std::string &&>::value));
}

BOOST_AUTO_TEST_CASE(math_canonical_test)
{
	typedef polynomial<rational,short> p_type1;
	BOOST_CHECK_THROW(math::transformation_is_canonical({p_type1{"p"},p_type1{"p"}},{p_type1{"q"}},{"p"},{"q"}),std::invalid_argument);
	BOOST_CHECK_THROW(math::transformation_is_canonical({p_type1{"p"}},{p_type1{"q"}},{"p","x"},{"q"}),std::invalid_argument);
	BOOST_CHECK_THROW(math::transformation_is_canonical({p_type1{"p"}},{p_type1{"q"}},{"p","x"},{"q","y"}),std::invalid_argument);
	BOOST_CHECK((math::transformation_is_canonical({p_type1{"p"}},{p_type1{"q"}},{"p"},{"q"})));
	p_type1 px{"px"}, py{"py"}, x{"x"}, y{"y"};
	BOOST_CHECK((math::transformation_is_canonical({py,px},{y,x},{"px","py"},{"x","y"})));
	BOOST_CHECK((!math::transformation_is_canonical({py,px},{x,y},{"px","py"},{"x","y"})));
	BOOST_CHECK((math::transformation_is_canonical({-x,-y},{px,py},{"px","py"},{"x","y"})));
	BOOST_CHECK((!math::transformation_is_canonical({x,y},{px,py},{"px","py"},{"x","y"})));
	BOOST_CHECK((math::transformation_is_canonical({px,px+py},{x-y,y},{"px","py"},{"x","y"})));
	BOOST_CHECK((!math::transformation_is_canonical({px,px-py},{x-y,y},{"px","py"},{"x","y"})));
	BOOST_CHECK((math::transformation_is_canonical(std::vector<p_type1>{px,px+py},std::vector<p_type1>{x-y,y},{"px","py"},{"x","y"})));
	// Linear transformation.
	p_type1 L{"L"}, G{"G"}, H{"H"}, l{"l"}, g{"g"}, h{"h"};
	BOOST_CHECK((math::transformation_is_canonical({L+G+H,L+G,L},{h,g-h,l-g},{"L","G","H"},{"l","g","h"})));
	// Unimodular matrices.
	BOOST_CHECK((math::transformation_is_canonical({L+2*G+3*H,-4*G+H,3*G-H},{l,11*l-g-3*h,14*l-g-4*h},{"L","G","H"},{"l","g","h"})));
	BOOST_CHECK((math::transformation_is_canonical({2*L+3*G+2*H,4*L+2*G+3*H,9*L+6*G+7*H},{-4*l-g+6*h,-9*l-4*g+15*h,5*l+2*g-8*h},{"L","G","H"},{"l","g","h"})));
	BOOST_CHECK((!math::transformation_is_canonical({2*L+3*G+2*H,4*L+2*G+3*H,9*L+6*G+7*H},{-4*l-g+6*h,-9*l-4*g+15*h,5*l+2*g-7*h},{"L","G","H"},{"l","g","h"})));
	BOOST_CHECK((math::transformation_is_canonical(std::vector<p_type1>{2*L+3*G+2*H,4*L+2*G+3*H,9*L+6*G+7*H},std::vector<p_type1>{-4*l-g+6*h,-9*l-4*g+15*h,5*l+2*g-8*h},{"L","G","H"},{"l","g","h"})));
	BOOST_CHECK((!math::transformation_is_canonical(std::vector<p_type1>{2*L+3*G+2*H,4*L+2*G+3*H,9*L+6*G+7*H},std::vector<p_type1>{-4*l-g+6*h,-9*l-4*g+15*h,5*l+2*g-7*h},{"L","G","H"},{"l","g","h"})));
	typedef poisson_series<p_type1> p_type2;
	// Poincare' variables.
	p_type2 P{"P"}, Q{"Q"}, p{"p"}, q{"q"}, P2{"P2"}, Q2{"Q2"};
	p_type2::register_custom_derivative("P",[&P2](const p_type2 &arg) {return arg.partial("P") + arg.partial("P2") * math::pow(P2,-1);});
	p_type2::register_custom_derivative("Q",[&Q2](const p_type2 &arg) {return arg.partial("Q") + arg.partial("Q2") * math::pow(Q2,-1);});
	BOOST_CHECK((math::transformation_is_canonical({P2*math::cos(p),Q2*math::cos(q)},{P2*math::sin(p),Q2*math::sin(q)},{"P","Q"},{"p","q"})));
	BOOST_CHECK((!math::transformation_is_canonical({P*Q*math::cos(p)*q,Q*P*math::sin(3*q)*p*math::pow(q,-1)},{P*math::sin(p),Q*math::sin(q)},{"P","Q"},{"p","q"})));
	BOOST_CHECK((!math::transformation_is_canonical(std::vector<p_type2>{P2*math::cos(p)*q,Q2*math::cos(q)*p},std::vector<p_type2>{P2*math::sin(p),Q2*math::sin(q)},
		{"P","Q"},{"p","q"})));
	BOOST_CHECK(has_transformation_is_canonical<p_type1>::value);
	BOOST_CHECK(has_transformation_is_canonical<p_type1 &>::value);
	BOOST_CHECK(has_transformation_is_canonical<p_type1 const &>::value);
	BOOST_CHECK(has_transformation_is_canonical<p_type2>::value);
	BOOST_CHECK(has_transformation_is_canonical<int>::value);
	BOOST_CHECK(has_transformation_is_canonical<double>::value);
	BOOST_CHECK(has_transformation_is_canonical<double &&>::value);
	BOOST_CHECK(!has_transformation_is_canonical<std::string>::value);
	BOOST_CHECK(!has_transformation_is_canonical<std::string &>::value);
	BOOST_CHECK(!has_transformation_is_canonical<std::string const &>::value);
}

struct term1: base_term<double,monomial<int>,term1>
{
	typedef base_term<double,monomial<int>,term1> base;
	PIRANHA_FORWARDING_CTOR(term1,base)
	PIRANHA_FORWARDING_ASSIGNMENT(term1,base)
	term1() = default;
	term1(const term1 &) = default;
	term1(term1 &&) = default;
	term1 &operator=(const term1 &) = default;
	term1 &operator=(term1 &&) = default;
};

struct term2: base_term<double,monomial<int>,term2>
{
	typedef base_term<double,monomial<int>,term2> base;
	PIRANHA_FORWARDING_CTOR(term2,base)
	PIRANHA_FORWARDING_ASSIGNMENT(term2,base)
	term2() = default;
	term2(const term2 &) = default;
	term2(term2 &&) = default;
	term2 &operator=(const term2 &) = default;
	term2 &operator=(term2 &&) = default;
	std::vector<term2> partial(const symbol &, const symbol_set &) const;
};

struct term3: base_term<double,monomial<int>,term3>
{
	typedef base_term<double,monomial<int>,term3> base;
	PIRANHA_FORWARDING_CTOR(term3,base)
	PIRANHA_FORWARDING_ASSIGNMENT(term3,base)
	term3() = default;
	term3(const term3 &) = default;
	term3(term3 &&) = default;
	term3 &operator=(const term3 &) = default;
	term3 &operator=(term3 &&) = default;
	std::vector<term3> partial(const symbol &, const symbol_set &);
};

struct term4: base_term<double,monomial<int>,term4>
{
	typedef base_term<double,monomial<int>,term4> base;
	PIRANHA_FORWARDING_CTOR(term4,base)
	PIRANHA_FORWARDING_ASSIGNMENT(term4,base)
	term4() = default;
	term4(const term4 &) = default;
	term4(term4 &&) = default;
	term4 &operator=(const term4 &) = default;
	term4 &operator=(term4 &&) = default;
	std::vector<term4> partial(const symbol &, symbol_set &) const;
};

BOOST_AUTO_TEST_CASE(math_term_is_differentiable_test)
{
	BOOST_CHECK(!term_is_differentiable<term1>::value);
	BOOST_CHECK(term_is_differentiable<term2>::value);
	BOOST_CHECK(!term_is_differentiable<term3>::value);
	BOOST_CHECK(!term_is_differentiable<term4>::value);
}

BOOST_AUTO_TEST_CASE(math_is_evaluable_test)
{
	BOOST_CHECK((is_evaluable<int,int>::value));
	BOOST_CHECK((is_evaluable<double,double>::value));
	BOOST_CHECK((is_evaluable<double,int>::value));
	BOOST_CHECK((is_evaluable<int &,int>::value));
	BOOST_CHECK((is_evaluable<double,int>::value));
	BOOST_CHECK((is_evaluable<double &,int>::value));
	BOOST_CHECK((is_evaluable<double &&,int>::value));
	BOOST_CHECK((!is_evaluable<std::string,int>::value));
	BOOST_CHECK((!is_evaluable<std::set<int>,int>::value));
	BOOST_CHECK((!is_evaluable<std::string &,int>::value));
	BOOST_CHECK((!is_evaluable<std::set<int> &&,int>::value));
}

BOOST_AUTO_TEST_CASE(math_has_sine_cosine_test)
{
	BOOST_CHECK(has_sine<float>::value);
	BOOST_CHECK(has_sine<float &>::value);
	BOOST_CHECK(has_sine<float const>::value);
	BOOST_CHECK(has_sine<float &&>::value);
	BOOST_CHECK(has_sine<double>::value);
	BOOST_CHECK(has_sine<long double &>::value);
	BOOST_CHECK(has_sine<double const>::value);
	BOOST_CHECK(has_sine<long double &&>::value);
	BOOST_CHECK(has_cosine<float>::value);
	BOOST_CHECK(has_cosine<float &>::value);
	BOOST_CHECK(has_cosine<float const>::value);
	BOOST_CHECK(has_cosine<float &&>::value);
	BOOST_CHECK(has_cosine<double>::value);
	BOOST_CHECK(has_cosine<long double &>::value);
	BOOST_CHECK(has_cosine<double const>::value);
	BOOST_CHECK(has_cosine<long double &&>::value);
	BOOST_CHECK(!has_sine<int>::value);
	BOOST_CHECK(!has_sine<long &>::value);
	BOOST_CHECK(!has_sine<long long &&>::value);
	BOOST_CHECK(!has_sine<long long const &>::value);
	BOOST_CHECK(!has_cosine<int>::value);
	BOOST_CHECK(!has_cosine<long &>::value);
	BOOST_CHECK(!has_cosine<long long &&>::value);
	BOOST_CHECK(!has_cosine<long long const &>::value);
	BOOST_CHECK(!has_cosine<std::string>::value);
	BOOST_CHECK(!has_cosine<void *>::value);
}

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

#define BOOST_TEST_MODULE series_02_test
#include <boost/test/unit_test.hpp>

#include <cstddef>
#include <functional>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>

#include "../src/base_term.hpp"
#include "../src/environment.hpp"
#include "../src/forwarding.hpp"
#include "../src/math.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/polynomial_term.hpp"
#include "../src/real.hpp"
#include "../src/serialization.hpp"
#include "../src/symbol_set.hpp"

static const int ntries = 1000;
static std::mt19937 rng;

using namespace piranha;

template <typename Cf, typename Expo>
class g_series_type: public series<polynomial_term<Cf,Expo>,g_series_type<Cf,Expo>>
{
		typedef series<polynomial_term<Cf,Expo>,g_series_type<Cf,Expo>> base;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
	public:
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

template <typename Cf, typename Key>
class g_term_type: public base_term<Cf,Key,g_term_type<Cf,Key>>
{
		typedef base_term<Cf,Key,g_term_type> base;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
	public:
		g_term_type() = default;
		g_term_type(const g_term_type &) = default;
		g_term_type(g_term_type &&) = default;
		g_term_type &operator=(const g_term_type &) = default;
		g_term_type &operator=(g_term_type &&) = default;
		PIRANHA_FORWARDING_CTOR(g_term_type,base)
};

template <typename Cf, typename Key>
class g_series_type3: public series<g_term_type<Cf,Key>,g_series_type3<Cf,Key>>
{
		typedef series<g_term_type<Cf,Key>,g_series_type3<Cf,Key>> base;
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

// Mock coefficient, not differentiable.
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

BOOST_AUTO_TEST_CASE(series_partial_test)
{
	environment env;
	{
	typedef g_series_type<rational,int> p_type1;
	p_type1 x1{"x"};
	BOOST_CHECK(is_differentiable<p_type1>::value);
	BOOST_CHECK((std::is_same<decltype(x1.partial("foo")),p_type1>::value));
	p_type1 x{"x"}, y{"y"};
	BOOST_CHECK_EQUAL(math::partial(x,"x"),1);
	BOOST_CHECK_EQUAL(math::partial(x,"y"),0);
	BOOST_CHECK_EQUAL(math::partial(-4 * x.pow(2),"x"),-8 * x);
	BOOST_CHECK_EQUAL(math::partial(-4 * x.pow(2) + y * x,"y"),x);
	BOOST_CHECK_EQUAL(math::partial(math::partial(-4 * x.pow(2),"x"),"x"),-8);
	BOOST_CHECK_EQUAL(math::partial(math::partial(math::partial(-4 * x.pow(2),"x"),"x"),"x"),0);
	BOOST_CHECK_EQUAL(math::partial(-x + 1,"x"),-1);
	BOOST_CHECK_EQUAL(math::partial((1 + 2 * x).pow(10),"x"),20 * (1 + 2 * x).pow(9));
	BOOST_CHECK_EQUAL(math::partial((1 + 2 * x + y).pow(10),"x"),20 * (1 + 2 * x + y).pow(9));
	BOOST_CHECK_EQUAL(math::partial(x * (1 + 2 * x + y).pow(10),"x"),20 * x * (1 + 2 * x + y).pow(9) + (1 + 2 * x + y).pow(10));
	BOOST_CHECK(math::partial((1 + 2 * x + y).pow(0),"x").empty());
	// Custom derivatives.
	p_type1::register_custom_derivative("x",[](const p_type1 &) {return p_type1{rational(1,314)};});
	BOOST_CHECK_EQUAL(math::partial(x,"x"),rational(1,314));
	p_type1::register_custom_derivative("x",[](const p_type1 &) {return p_type1{rational(1,315)};});
	BOOST_CHECK_EQUAL(math::partial(x,"x"),rational(1,315));
	p_type1::unregister_custom_derivative("x");
	p_type1::unregister_custom_derivative("x");
	BOOST_CHECK_EQUAL(math::partial(x,"x"),1);
	// y as implicit function of x: y = x**2.
	p_type1::register_custom_derivative("x",[x](const p_type1 &p) -> p_type1 {
		return p.partial("x") + math::partial(p,"y") * 2 * x;
	});
	BOOST_CHECK_EQUAL(math::partial(x + y,"x"),1 + 2 * x);
	p_type1::unregister_custom_derivative("y");
	p_type1::unregister_custom_derivative("x");
	BOOST_CHECK_EQUAL(math::partial(x + y,"x"),1);
	BOOST_CHECK_EQUAL(math::partial(x + 2 * y,"y"),2);
	p_type1::register_custom_derivative("x",[](const p_type1 &p) {return p.partial("x");});
	BOOST_CHECK_EQUAL(math::partial(x + y,"x"),1);
	BOOST_CHECK_EQUAL(math::partial(x + y * x,"x"),y + 1);
	p_type1::register_custom_derivative("x",[x](const p_type1 &p) -> p_type1 {
		return p.partial("x") + math::partial(p,"y") * 2 * x;
	});
	p_type1::register_custom_derivative("y",[](const p_type1 &p) -> p_type1 {
		return 2 * p;
	});
	BOOST_CHECK_EQUAL(math::partial(x + y,"x"),1 + 4 * x * (x + y));
	BOOST_CHECK_EQUAL(math::partial(x + y,"y"),2 * (x + y));
	p_type1::unregister_all_custom_derivatives();
	BOOST_CHECK_EQUAL(math::partial(x + y,"x"),1);
	BOOST_CHECK_EQUAL(math::partial(x + 3 * y,"y"),3);
	}
	{
	typedef g_series_type<integer,rational> p_type2;
	p_type2 x2{"x"};
	BOOST_CHECK(is_differentiable<p_type2>::value);
	// NOTE: this will have to be changed once we rework series arithmetics with mixed
	// types. The correct type then will be not p_type2 but g_series_type<rational,int>.
	// This should anyway trigger the second algorithm, the non-optimised one.
	BOOST_CHECK((std::is_same<decltype(x2.partial("foo")),p_type2>::value));
	p_type2 x{"x"}, y{"y"};
	BOOST_CHECK_EQUAL(math::partial(x,"x"),1);
	BOOST_CHECK_EQUAL(math::partial(x,"y"),0);
	BOOST_CHECK_EQUAL(math::partial(-4 * x.pow(2),"x"),-8 * x);
	BOOST_CHECK_EQUAL(math::partial(-4 * x.pow(2) + y * x,"y"),x);
	BOOST_CHECK_EQUAL(math::partial(math::partial(-4 * x.pow(2),"x"),"x"),-8);
	BOOST_CHECK_EQUAL(math::partial(math::partial(math::partial(-4 * x.pow(2),"x"),"x"),"x"),0);
	BOOST_CHECK_EQUAL(math::partial(-x + 1,"x"),-1);
	BOOST_CHECK_EQUAL(math::partial((1 + 2 * x).pow(10),"x"),20 * (1 + 2 * x).pow(9));
	BOOST_CHECK_EQUAL(math::partial((1 + 2 * x + y).pow(10),"x"),20 * (1 + 2 * x + y).pow(9));
	BOOST_CHECK_EQUAL(math::partial(x * (1 + 2 * x + y).pow(10),"x"),20 * x * (1 + 2 * x + y).pow(9) + (1 + 2 * x + y).pow(10));
	BOOST_CHECK(math::partial((1 + 2 * x + y).pow(0),"x").empty());
	// Custom derivatives.
	// NOTE: restore these tests once we rework series arithmetics.
	/*
	p_type2::register_custom_derivative("x",[](const p_type2 &) {return p_type2{rational(1,314)};});
	BOOST_CHECK_EQUAL(math::partial(x,"x"),rational(1,314));
	p_type2::register_custom_derivative("x",[](const p_type2 &) {return p_type2{rational(1,315)};});
	BOOST_CHECK_EQUAL(math::partial(x,"x"),rational(1,315));
	p_type2::unregister_custom_derivative("x");
	p_type2::unregister_custom_derivative("x");
	BOOST_CHECK_EQUAL(math::partial(x,"x"),1);*/
	// y as implicit function of x: y = x**2.
	p_type2::register_custom_derivative("x",[x](const p_type2 &p) -> p_type2 {
		return p.partial("x") + math::partial(p,"y") * 2 * x;
	});
	BOOST_CHECK_EQUAL(math::partial(x + y,"x"),1 + 2 * x);
	p_type2::unregister_custom_derivative("y");
	p_type2::unregister_custom_derivative("x");
	BOOST_CHECK_EQUAL(math::partial(x + y,"x"),1);
	BOOST_CHECK_EQUAL(math::partial(x + 2 * y,"y"),2);
	p_type2::register_custom_derivative("x",[](const p_type2 &p) {return p.partial("x");});
	BOOST_CHECK_EQUAL(math::partial(x + y,"x"),1);
	BOOST_CHECK_EQUAL(math::partial(x + y * x,"x"),y + 1);
	p_type2::register_custom_derivative("x",[x](const p_type2 &p) -> p_type2 {
		return p.partial("x") + math::partial(p,"y") * 2 * x;
	});
	p_type2::register_custom_derivative("y",[](const p_type2 &p) -> p_type2 {
		return 2 * p;
	});
	BOOST_CHECK_EQUAL(math::partial(x + y,"x"),1 + 4 * x * (x + y));
	BOOST_CHECK_EQUAL(math::partial(x + y,"y"),2 * (x + y));
	p_type2::unregister_all_custom_derivatives();
	BOOST_CHECK_EQUAL(math::partial(x + y,"x"),1);
	BOOST_CHECK_EQUAL(math::partial(x + 3 * y,"y"),3);
	}
	// Check with mock_cf.
	BOOST_CHECK((!is_differentiable<g_series_type<mock_cf,rational>>::value));
	{
	using s0 = g_series_type<double,rational>;
	using ss0 = g_series_type<s0,rational>;
	// Series as coefficient.
	BOOST_CHECK((is_differentiable<ss0>::value));
	BOOST_CHECK_EQUAL(math::partial(s0{"y"} * ss0{"x"},"y"),ss0{"x"});
	BOOST_CHECK_EQUAL(math::partial(s0{"y"} * ss0{"x"},"x"),s0{"y"});
	BOOST_CHECK_EQUAL(math::partial(s0{"y"} * math::pow(ss0{"x"},5),"x"),5 * s0{"y"} * math::pow(ss0{"x"},4));
	}
}

BOOST_AUTO_TEST_CASE(series_serialization_test)
{
	// Serialization test done with a randomly-generated series.
	typedef g_series_type<rational,int> p_type1;
	auto x = p_type1{"x"}, y = p_type1{"y"}, z = p_type1{"z"};
	std::uniform_int_distribution<int> int_dist(0,5);
	std::uniform_int_distribution<unsigned> size_dist(0u,10u);
	p_type1 tmp;
	for (int i = 0; i < ntries; ++i) {
		p_type1 p;
		const unsigned size = size_dist(rng);
		for (unsigned j = 0u; j < size; ++j) {
			p += math::pow(x,int_dist(rng)) * math::pow(y,int_dist(rng)) * math::pow(z,int_dist(rng));
		}
		p *= int_dist(rng);
		const auto div = int_dist(rng);
		if (div) {
			p /= div;
		}
		std::stringstream ss;
		{
		boost::archive::text_oarchive oa(ss);
		oa << p;
		}
		{
		boost::archive::text_iarchive ia(ss);
		ia >> tmp;
		}
		BOOST_CHECK_EQUAL(tmp,p);
	}
	// Test bad archives.
	std::stringstream ss;
	{
	// This originally corresponded to:
	// math::pow(x,2)/2 + math::pow(y,2)/3
	// An extra exponent was added to the only term of math::pow(y,2)/3.
	const std::string ba = "22 serialization::archive 10 0 0 0 0 0 0 2 1 x 1 y 2 0 0 0 0 0 0 0 0 1 1 1 2 0 0 0 0 0 0 2 2 0 1 1 1 3 3 0 2 0";
	ss.str(ba);
	boost::archive::text_iarchive ia(ss);
	BOOST_CHECK_THROW(ia >> tmp,std::invalid_argument);
	}
	{
	// This is equivalent to:
	// math::pow(x,2)/2 + math::pow(x,2)/3
	// (y replaced with x wrt the original example).
	std::stringstream ss;
	const std::string ba = "22 serialization::archive 10 0 0 0 0 0 0 2 1 x 1 y 2 0 0 0 0 0 0 0 0 1 1 1 2 0 0 0 0 0 0 2 2 0 1 1 1 3 2 2 0";
	ss.str(ba);
	boost::archive::text_iarchive ia(ss);
	ia >> tmp;
	BOOST_CHECK_EQUAL(tmp,math::pow(x,2)/2 + math::pow(x,2)/3);
	BOOST_CHECK_EQUAL(tmp.size(),1u);
	}
	{
	// This is equivalent to:
	// math::pow(x,2)/2 + math::pow(x,2)/3
	// with the numerators of the fractions replaced by zero.
	std::stringstream ss;
	const std::string ba = "22 serialization::archive 10 0 0 0 0 0 0 2 1 x 1 y 2 0 0 0 0 0 0 0 0 1 0 1 2 0 0 0 0 0 0 2 2 0 1 0 1 3 2 2 0";
	ss.str(ba);
	boost::archive::text_iarchive ia(ss);
	ia >> tmp;
	BOOST_CHECK_EQUAL(tmp.size(),0u);
	}
}

struct mock_key
{
	mock_key() = default;
	mock_key(const mock_key &) = default;
	mock_key(mock_key &&) noexcept;
	mock_key &operator=(const mock_key &) = default;
	mock_key &operator=(mock_key &&) noexcept;
	mock_key(const symbol_set &);
	bool operator==(const mock_key &) const;
	bool operator!=(const mock_key &) const;
	bool is_compatible(const symbol_set &) const noexcept;
	bool is_ignorable(const symbol_set &) const noexcept;
	mock_key merge_args(const symbol_set &, const symbol_set &) const;
	bool is_unitary(const symbol_set &) const;
	void print(std::ostream &, const symbol_set &) const;
	void print_tex(std::ostream &, const symbol_set &) const;
};

namespace std
{

template <>
struct hash<mock_key>
{
	std::size_t operator()(const mock_key &) const noexcept;
};

}

BOOST_AUTO_TEST_CASE(series_evaluate_test)
{
	typedef g_series_type<rational,int> p_type1;
	typedef std::unordered_map<std::string,rational> dict_type;
	typedef std::unordered_map<std::string,int> dict_type_int;
	typedef std::unordered_map<std::string,long> dict_type_long;
	BOOST_CHECK((is_evaluable<p_type1,rational>::value));
	BOOST_CHECK((is_evaluable<p_type1,integer>::value));
	BOOST_CHECK((is_evaluable<p_type1,int>::value));
	BOOST_CHECK((is_evaluable<p_type1,long>::value));
	BOOST_CHECK((std::is_same<rational,decltype(p_type1{}.evaluate(dict_type_int{}))>::value));
	BOOST_CHECK((std::is_same<rational,decltype(p_type1{}.evaluate(dict_type_long{}))>::value));
	BOOST_CHECK_EQUAL(p_type1{}.evaluate(dict_type{}),0);
	p_type1 x{"x"}, y{"y"};
	BOOST_CHECK_THROW(x.evaluate(dict_type{}),std::invalid_argument);
	BOOST_CHECK_EQUAL(x.evaluate(dict_type{{"x",rational(1)}}),1);
	BOOST_CHECK_THROW((x + (2 * y).pow(3)).evaluate(dict_type{{"x",rational(1)}}),std::invalid_argument);
	BOOST_CHECK_EQUAL((x + (2 * y).pow(3)).evaluate(dict_type{{"x",rational(1)},{"y",rational(2,3)}}),rational(1) + (2 * rational(2,3)).pow(3));
	BOOST_CHECK_EQUAL((x + (2 * y).pow(3)).evaluate(dict_type{{"x",rational(1)},{"y",rational(2,3)}}),
		math::evaluate(x + (2 * y).pow(3),dict_type{{"x",rational(1)},{"y",rational(2,3)}}));
	BOOST_CHECK((std::is_same<decltype(p_type1{}.evaluate(dict_type{})),rational>::value));
	typedef std::unordered_map<std::string,real> dict_type2;
	BOOST_CHECK((is_evaluable<p_type1,real>::value));
	BOOST_CHECK_EQUAL((x + (2 * y).pow(3)).evaluate(dict_type2{{"x",real(1.234)},{"y",real(-5.678)},{"z",real()}}),
		real(1.234) + math::pow(2 * real(-5.678),3));
	BOOST_CHECK_EQUAL((x + (2 * y).pow(3)).evaluate(dict_type2{{"x",real(1.234)},{"y",real(-5.678)},{"z",real()}}),
		math::evaluate(x + math::pow(2 * y,3),dict_type2{{"x",real(1.234)},{"y",real(-5.678)},{"z",real()}}));
	BOOST_CHECK((std::is_same<decltype(p_type1{}.evaluate(dict_type2{})),real>::value));
	typedef std::unordered_map<std::string,double> dict_type3;
	BOOST_CHECK((is_evaluable<p_type1,double>::value));
	BOOST_CHECK_EQUAL((x + (2 * y).pow(3)).evaluate(dict_type3{{"x",1.234},{"y",-5.678},{"z",0.0001}}),
		1.234 + math::pow(2 * -5.678,3));
	BOOST_CHECK_EQUAL((x + (2 * y).pow(3)).evaluate(dict_type3{{"x",1.234},{"y",-5.678},{"z",0.0001}}),
		math::evaluate(x + math::pow(2 * y,3),dict_type3{{"x",1.234},{"y",-5.678},{"z",0.0001}}));
	BOOST_CHECK((std::is_same<decltype(p_type1{}.evaluate(dict_type3{})),double>::value));
	BOOST_CHECK((!is_evaluable<g_series_type3<double,mock_key>,double>::value));
	BOOST_CHECK((is_evaluable<g_series_type3<mock_cf,monomial<int>>,double>::value));
	BOOST_CHECK((!is_evaluable<g_series_type3<mock_cf,mock_key>,double>::value));
	BOOST_CHECK((is_evaluable<g_series_type3<double,monomial<int>>,double>::value));
	// Check the syntax from initializer list with explicit template parameter.
	BOOST_CHECK_EQUAL(p_type1{}.evaluate<int>({{"foo",4.}}),0);
	BOOST_CHECK_EQUAL(p_type1{}.evaluate<double>({{"foo",4.},{"bar",7}}),0);
	BOOST_CHECK_EQUAL(math::evaluate<int>(p_type1{},{{"foo",4.}}),0);
	BOOST_CHECK_EQUAL(math::evaluate<double>(p_type1{},{{"foo",4.},{"bar",7}}),0);
}

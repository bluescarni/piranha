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

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <functional>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "../src/base_term.hpp"
#include "../src/debug_access.hpp"
#include "../src/environment.hpp"
#include "../src/forwarding.hpp"
#include "../src/math.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/polynomial_term.hpp"
#include "../src/real.hpp"
#include "../src/serialization.hpp"
#include "../src/symbol.hpp"
#include "../src/symbol_set.hpp"
#include "../src/type_traits.hpp"

static const int ntries = 1000;
static std::mt19937 rng;

using namespace piranha;

typedef boost::mpl::vector<double,integer,rational,real> cf_types;
typedef boost::mpl::vector<unsigned,integer> expo_types;

template <typename Cf, typename Expo>
class g_series_type: public series<polynomial_term<Cf,Expo>,g_series_type<Cf,Expo>>
{
		typedef series<polynomial_term<Cf,Expo>,g_series_type<Cf,Expo>> base;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
	public:
		template <typename Cf2>
		using rebind = g_series_type<Cf2,Expo>;
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

// This is essentially the same as above, just a different type.
template <typename Cf, typename Expo>
class g_series_type2: public series<polynomial_term<Cf,Expo>,g_series_type2<Cf,Expo>>
{
	public:
		typedef series<polynomial_term<Cf,Expo>,g_series_type2<Cf,Expo>> base;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
		g_series_type2() = default;
		g_series_type2(const g_series_type2 &) = default;
		g_series_type2(g_series_type2 &&) = default;
		explicit g_series_type2(const char *name):base()
		{
			typedef typename base::term_type term_type;
			// Insert the symbol.
			this->m_symbol_set.add(name);
			// Construct and insert the term.
			this->insert(term_type(Cf(1),typename term_type::key_type{Expo(1)}));
		}
		g_series_type2 &operator=(const g_series_type2 &) = default;
		g_series_type2 &operator=(g_series_type2 &&) = default;
		PIRANHA_FORWARDING_CTOR(g_series_type2,base)
		PIRANHA_FORWARDING_ASSIGNMENT(g_series_type2,base)
		// Provide fake sin/cos methods to test math overloads.
		g_series_type2 sin() const
		{
			return g_series_type2(42);
		}
		g_series_type2 cos() const
		{
			return g_series_type2(-42);
		}
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
		template <typename Cf2>
		using rebind = g_series_type3<Cf2,Key>;
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
	explicit mock_cf(const int &);
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
	using p_type2_diff = g_series_type<rational,rational>;
	p_type2 x2{"x"};
	BOOST_CHECK(is_differentiable<p_type2>::value);
	BOOST_CHECK((std::is_same<decltype(x2.partial("foo")),p_type2_diff>::value));
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
	p_type2::register_custom_derivative("x",[](const p_type2 &) {return p_type2_diff{rational(1,314)};});
	BOOST_CHECK_EQUAL(math::partial(x,"x"),rational(1,314));
	p_type2::register_custom_derivative("x",[](const p_type2 &) {return p_type2_diff{rational(1,315)};});
	BOOST_CHECK_EQUAL(math::partial(x,"x"),rational(1,315));
	p_type2::unregister_custom_derivative("x");
	BOOST_CHECK_EQUAL(math::partial(x,"x"),1);
	// y as implicit function of x: y = x**2.
	p_type2::register_custom_derivative("x",[x](const p_type2 &p) {
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
	p_type2::register_custom_derivative("x",[x](const p_type2 &p) {
		return p.partial("x") + math::partial(p,"y") * 2 * x;
	});
	p_type2::register_custom_derivative("y",[](const p_type2 &p) {
		return 2 * p_type2_diff{p};
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
	// NOTE: this used to be true before we changed the ctor from int of mock_cf to explicit.
	BOOST_CHECK((!is_evaluable<g_series_type3<mock_cf,monomial<int>>,double>::value));
	BOOST_CHECK((!is_evaluable<g_series_type3<mock_cf,mock_key>,double>::value));
	BOOST_CHECK((is_evaluable<g_series_type3<double,monomial<int>>,double>::value));
	// Check the syntax from initializer list with explicit template parameter.
	BOOST_CHECK_EQUAL(p_type1{}.evaluate<int>({{"foo",4.}}),0);
	BOOST_CHECK_EQUAL(p_type1{}.evaluate<double>({{"foo",4.},{"bar",7}}),0);
	BOOST_CHECK_EQUAL(math::evaluate<int>(p_type1{},{{"foo",4.}}),0);
	BOOST_CHECK_EQUAL(math::evaluate<double>(p_type1{},{{"foo",4.},{"bar",7}}),0);
}

template <typename Expo>
class g_series_type_nr: public series<polynomial_term<float,Expo>,g_series_type_nr<Expo>>
{
		typedef series<polynomial_term<float,Expo>,g_series_type_nr<Expo>> base;
	public:
		g_series_type_nr() = default;
		g_series_type_nr(const g_series_type_nr &) = default;
		g_series_type_nr(g_series_type_nr &&) = default;
		g_series_type_nr &operator=(const g_series_type_nr &) = default;
		g_series_type_nr &operator=(g_series_type_nr &&) = default;
		PIRANHA_FORWARDING_CTOR(g_series_type_nr,base)
		PIRANHA_FORWARDING_ASSIGNMENT(g_series_type_nr,base)
};

template <typename Expo>
class g_series_type_nr2: public series<polynomial_term<short,Expo>,g_series_type_nr2<Expo>>
{
		typedef series<polynomial_term<short,Expo>,g_series_type_nr2<Expo>> base;
	public:
		template <typename Expo2>
		using rebind = g_series_type_nr2<Expo2>;
		g_series_type_nr2() = default;
		g_series_type_nr2(const g_series_type_nr2 &) = default;
		g_series_type_nr2(g_series_type_nr2 &&) = default;
		g_series_type_nr2 &operator=(const g_series_type_nr2 &) = default;
		g_series_type_nr2 &operator=(g_series_type_nr2 &&) = default;
		PIRANHA_FORWARDING_CTOR(g_series_type_nr2,base)
		PIRANHA_FORWARDING_ASSIGNMENT(g_series_type_nr2,base)
};

template <typename Expo>
class g_series_type_nr3: public series<polynomial_term<float,Expo>,g_series_type_nr3<Expo>>
{
		typedef series<polynomial_term<float,Expo>,g_series_type_nr3<Expo>> base;
	public:
		template <typename Expo2>
		using rebind = void;
		g_series_type_nr3() = default;
		g_series_type_nr3(const g_series_type_nr3 &) = default;
		g_series_type_nr3(g_series_type_nr3 &&) = default;
		g_series_type_nr3 &operator=(const g_series_type_nr3 &) = default;
		g_series_type_nr3 &operator=(g_series_type_nr3 &&) = default;
		PIRANHA_FORWARDING_CTOR(g_series_type_nr3,base)
		PIRANHA_FORWARDING_ASSIGNMENT(g_series_type_nr3,base)
};

BOOST_AUTO_TEST_CASE(series_series_is_rebindable_test)
{
	typedef g_series_type<rational,int> p_type1;
	BOOST_CHECK((series_is_rebindable<p_type1,int>::value));
	BOOST_CHECK((std::is_same<series_rebind<p_type1,int>,g_series_type<int,int>>::value));
	BOOST_CHECK((series_is_rebindable<p_type1,rational>::value));
	BOOST_CHECK((std::is_same<series_rebind<p_type1,rational>,p_type1>::value));
	BOOST_CHECK((std::is_same<series_rebind<p_type1 &,rational const>,p_type1>::value));
	BOOST_CHECK((series_is_rebindable<p_type1,p_type1>::value));
	BOOST_CHECK((series_is_rebindable<p_type1 &,p_type1>::value));
	BOOST_CHECK((series_is_rebindable<p_type1 &,const p_type1>::value));
	BOOST_CHECK((std::is_same<series_rebind<p_type1,p_type1>,g_series_type<p_type1,int>>::value));
	typedef g_series_type_nr<int> p_type_nr;
	BOOST_CHECK((!series_is_rebindable<p_type_nr,unsigned>::value));
	BOOST_CHECK((!series_is_rebindable<p_type_nr,integer>::value));
	BOOST_CHECK((!series_is_rebindable<p_type_nr &,unsigned const>::value));
	BOOST_CHECK((!series_is_rebindable<p_type_nr &&,const integer &>::value));
	typedef g_series_type_nr2<int> p_type_nr2;
	BOOST_CHECK((!series_is_rebindable<p_type_nr2,unsigned>::value));
	BOOST_CHECK((!series_is_rebindable<p_type_nr2,integer>::value));
	typedef g_series_type_nr3<int> p_type_nr3;
	BOOST_CHECK((!series_is_rebindable<p_type_nr3,unsigned>::value));
	BOOST_CHECK((!series_is_rebindable<p_type_nr3,integer>::value));
}

BOOST_AUTO_TEST_CASE(series_series_recursion_index_test)
{
	BOOST_CHECK_EQUAL(series_recursion_index<int>::value,0u);
	BOOST_CHECK_EQUAL(series_recursion_index<double>::value,0u);
	BOOST_CHECK_EQUAL(series_recursion_index<float>::value,0u);
	BOOST_CHECK_EQUAL((series_recursion_index<g_series_type<rational,int>>::value),1u);
	BOOST_CHECK_EQUAL((series_recursion_index<g_series_type<float,int>>::value),1u);
	BOOST_CHECK_EQUAL((series_recursion_index<g_series_type<double,int>>::value),1u);
	BOOST_CHECK_EQUAL((series_recursion_index<g_series_type<g_series_type<double,int>,int>>::value),2u);
	BOOST_CHECK_EQUAL((series_recursion_index<g_series_type<g_series_type<double,int>,long>>::value),2u);
	BOOST_CHECK_EQUAL((series_recursion_index<g_series_type<g_series_type<g_series_type<double,int>,int>,long>>::value),3u);
	BOOST_CHECK_EQUAL((series_recursion_index<g_series_type<g_series_type<g_series_type<rational,int>,int>,long>>::value),3u);
	BOOST_CHECK_EQUAL((series_recursion_index<g_series_type<g_series_type<g_series_type<rational,int>,int>,long> &>::value),3u);
	BOOST_CHECK_EQUAL((series_recursion_index<g_series_type<g_series_type<g_series_type<rational,int>,int>,long> const>::value),3u);
	BOOST_CHECK_EQUAL((series_recursion_index<g_series_type<g_series_type<g_series_type<rational,int>,int>,long> const &>::value),3u);
}

PIRANHA_DECLARE_HAS_TYPEDEF(type);

template <typename T, typename U>
using binary_series_op_return_type = detail::binary_series_op_return_type<T,U,0>;

BOOST_AUTO_TEST_CASE(series_binary_series_op_return_type_test)
{
	// Check missing type in case both operands are not series.
	BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<int,int>>::value));
	BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<int,double>>::value));
	BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<float,double>>::value));
	// Case 0.
	// NOTE: this cannot fail in any way as we require coefficients to be addable in is_cf.
	typedef g_series_type<rational,int> p_type1;
	BOOST_CHECK((std::is_same<p_type1,binary_series_op_return_type<p_type1,p_type1>::type>::value));
	// Case 1 and 2.
	typedef g_series_type<double,int> p_type2;
	BOOST_CHECK((std::is_same<p_type2,binary_series_op_return_type<p_type2,p_type1>::type>::value));
	BOOST_CHECK((std::is_same<p_type2,binary_series_op_return_type<p_type1,p_type2>::type>::value));
	// mock_cf supports only multiplication vs mock_cf.
	BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<g_series_type<double,int>,g_series_type<mock_cf,int>>>::value));
	BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<g_series_type<mock_cf,int>,g_series_type<double,int>>>::value));
	// Case 3.
	typedef g_series_type<short,int> p_type3;
	BOOST_CHECK((std::is_same<g_series_type<int,int>,binary_series_op_return_type<p_type3,p_type3>::type>::value));
	typedef g_series_type<char,int> p_type4;
	BOOST_CHECK((std::is_same<g_series_type<int,int>,binary_series_op_return_type<p_type3,p_type4>::type>::value));
	BOOST_CHECK((std::is_same<g_series_type<int,int>,binary_series_op_return_type<p_type4,p_type3>::type>::value));
	// Wrong rebind implementations.
	BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<g_series_type_nr2<int>,g_series_type<char,int>>>::value));
	BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<g_series_type<char,int>,g_series_type_nr2<int>>>::value));
	// Case 4 and 6.
	BOOST_CHECK((std::is_same<p_type2,binary_series_op_return_type<p_type2,int>::type>::value));
	BOOST_CHECK((std::is_same<p_type2,binary_series_op_return_type<int,p_type2>::type>::value));
	// mock_cf does not support multiplication with int.
	BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<g_series_type<mock_cf,int>,int>>::value));
	BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<int,g_series_type<mock_cf,int>>>::value));
	// Case 5 and 7.
	BOOST_CHECK((std::is_same<p_type2,binary_series_op_return_type<p_type3,double>::type>::value));
	BOOST_CHECK((std::is_same<p_type2,binary_series_op_return_type<double,p_type3>::type>::value));
	BOOST_CHECK((std::is_same<g_series_type<int,int>,binary_series_op_return_type<p_type4,short>::type>::value));
	BOOST_CHECK((std::is_same<g_series_type<int,int>,binary_series_op_return_type<short,p_type4>::type>::value));
	// These need rebinding, but rebind is not supported.
	BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<g_series_type_nr<int>,double>>::value));
	BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<double,g_series_type_nr<int>>>::value));
	// Wrong implementation of rebind.
	BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<g_series_type_nr2<char>,g_series_type<char,char>>>::value));
	BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<g_series_type<char,char>,g_series_type_nr2<char>>>::value));
	// Same coefficients, amibguity in series type.
	BOOST_CHECK((!has_typedef_type<binary_series_op_return_type<g_series_type_nr<int>,g_series_type<float,int>>>::value));
}

struct arithmetics_add_tag {};

namespace piranha
{
template <>
class debug_access<arithmetics_add_tag>
{
	public:
		template <typename Cf>
		struct runner
		{
			template <typename Expo>
			void operator()(const Expo &)
			{
				typedef g_series_type<Cf,Expo> p_type1;
				typedef g_series_type2<Cf,Expo> p_type2;
				typedef g_series_type<int,Expo> p_type3;
				// Binary add first.
				// Some type checks - these are not addable as they result in an ambiguity
				// between two series with same coefficient but different series types.
				BOOST_CHECK((!is_addable<p_type1,p_type2>::value));
				BOOST_CHECK((!is_addable<p_type2,p_type1>::value));
				BOOST_CHECK((!is_addable_in_place<p_type1,p_type2>::value));
				BOOST_CHECK((!is_addable_in_place<p_type2,p_type1>::value));
				// Various subcases of case 0.
				p_type1 x{"x"}, y{"y"};
				// No need to merge args.
				auto tmp = x + x;
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1) + Cf(1));
				BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
				// Try with moves on both sides.
				tmp = p_type1{x} + x;
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1) + Cf(1));
				BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
				tmp = x + p_type1{x};
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1) + Cf(1));
				BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
				// Now with merging.
				tmp = x + y;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				auto it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(1));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				// With moves.
				tmp = p_type1{x} + y;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(1));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = x + p_type1{y};
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(1));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				// Test the swapping of operands when one series is larger than the other.
				tmp = (x + y) + x;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(2));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(2));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = x + (y + x);
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(2));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(2));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				// Some tests for case 1/4.
				tmp = x + p_type3{"y"};
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 1);
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == 1);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = x + (p_type3{"y"} + p_type3{"x"});
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = x + 1;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 1);
				BOOST_CHECK(it->m_key.size() == 1u);
				++it;
				BOOST_CHECK(it->m_cf == 1);
				BOOST_CHECK(it->m_key.size() == 1u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
				// Symmetric of the previous case.
				tmp = p_type3{"y"} + x;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 1);
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == 1);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = (p_type3{"y"} + p_type3{"x"}) + x;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = 1 + x;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 1);
				BOOST_CHECK(it->m_key.size() == 1u);
				++it;
				BOOST_CHECK(it->m_cf == 1);
				BOOST_CHECK(it->m_key.size() == 1u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
				// Case 3/5 and symmetric.
				using p_type4 = g_series_type<g_series_type<int,Expo>,Expo>;
				using p_type5 = g_series_type<double,Expo>;
				auto tmp2 = p_type4{"x"} + p_type5{"y"};
				BOOST_CHECK_EQUAL(tmp2.size(),2u);
				BOOST_CHECK((std::is_same<decltype(tmp2),g_series_type<g_series_type<double,Expo>,Expo>>::value));
				auto it2 = tmp2.m_container.begin();
				BOOST_CHECK((it2->m_cf == g_series_type<double,Expo>{"y"} || it2->m_cf == 1));
				BOOST_CHECK(it2->m_key.size() == 1u);
				++it2;
				BOOST_CHECK((it2->m_cf == g_series_type<double,Expo>{"y"} || it2->m_cf == 1));
				BOOST_CHECK(it2->m_key.size() == 1u);
				BOOST_CHECK((tmp2.m_symbol_set == symbol_set{symbol{"x"}}));
				tmp2 = p_type5{"y"} + p_type4{"x"};
				BOOST_CHECK_EQUAL(tmp2.size(),2u);
				BOOST_CHECK((std::is_same<decltype(tmp2),g_series_type<g_series_type<double,Expo>,Expo>>::value));
				it2 = tmp2.m_container.begin();
				BOOST_CHECK((it2->m_cf == g_series_type<double,Expo>{"y"} || it2->m_cf == 1));
				BOOST_CHECK(it2->m_key.size() == 1u);
				++it2;
				BOOST_CHECK((it2->m_cf == g_series_type<double,Expo>{"y"} || it2->m_cf == 1));
				BOOST_CHECK(it2->m_key.size() == 1u);
				BOOST_CHECK((tmp2.m_symbol_set == symbol_set{symbol{"x"}}));
				// Now in-place.
				// Case 0.
				tmp = x;
				tmp += x;
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1) + Cf(1));
				BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
				// Move.
				tmp = x;
				tmp += p_type1{x};
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1) + Cf(1));
				BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
				// Now with merging.
				tmp = x;
				tmp += y;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(1));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				// With moves.
				tmp = x;
				tmp += p_type1{y};
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(1));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				// Test the swapping of operands when one series is larger than the other.
				tmp = x + y;
				tmp += x;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = x;
				tmp += y + x;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				// Some tests for case 1/4.
				tmp = x;
				tmp += p_type3{"y"};
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 1);
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == 1);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = x;
				tmp += p_type3{"y"} + p_type3{"x"};
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = x;
				tmp += 1;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 1);
				BOOST_CHECK(it->m_key.size() == 1u);
				++it;
				BOOST_CHECK(it->m_cf == 1);
				BOOST_CHECK(it->m_key.size() == 1u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
				// Symmetric of the previous case.
				p_type3 tmp3{"y"};
				tmp3 += x;
				BOOST_CHECK_EQUAL(tmp3.size(),2u);
				auto it3 = tmp3.m_container.begin();
				BOOST_CHECK(it3->m_cf == 1);
				BOOST_CHECK(it3->m_key.size() == 2u);
				++it3;
				BOOST_CHECK(it3->m_cf == 1);
				BOOST_CHECK(it3->m_key.size() == 2u);
				BOOST_CHECK((tmp3.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp3 += p_type3{"y"} + p_type3{"x"};
				tmp3 += x;
				BOOST_CHECK_EQUAL(tmp3.size(),2u);
				it3 = tmp3.m_container.begin();
				BOOST_CHECK(it3->m_cf == 2 || it3->m_cf == 3);
				BOOST_CHECK(it3->m_key.size() == 2u);
				++it3;
				BOOST_CHECK(it3->m_cf == 2 || it3->m_cf == 3);
				BOOST_CHECK(it3->m_key.size() == 2u);
				BOOST_CHECK((tmp3.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				// Case 3/5.
				auto tmp4 = p_type4{"x"};
				tmp4 += p_type5{"y"};
				BOOST_CHECK_EQUAL(tmp4.size(),2u);
				auto it4 = tmp4.m_container.begin();
				BOOST_CHECK((std::is_same<decltype(it4->m_cf),g_series_type<int,Expo>>::value));
				BOOST_CHECK((it4->m_cf == g_series_type<int,Expo>{"y"} || it4->m_cf == 1));
				BOOST_CHECK(it4->m_key.size() == 1u);
				++it4;
				BOOST_CHECK((it4->m_cf == g_series_type<int,Expo>{"y"} || it4->m_cf == 1));
				BOOST_CHECK(it4->m_key.size() == 1u);
				BOOST_CHECK((tmp4.m_symbol_set == symbol_set{symbol{"x"}}));
			}
		};
		template <typename Cf>
		void operator()(const Cf &)
		{
			boost::mpl::for_each<expo_types>(runner<Cf>());
		}
};
}

typedef debug_access<arithmetics_add_tag> arithmetics_add_tester;

BOOST_AUTO_TEST_CASE(series_arithmetics_add_test)
{
	// Functional testing.
	boost::mpl::for_each<cf_types>(arithmetics_add_tester());
	// Type testing for binary addition.
	typedef g_series_type<rational,int> p_type1;
	typedef g_series_type<int,rational> p_type2;
	typedef g_series_type<short,rational> p_type3;
	typedef g_series_type<char,rational> p_type4;
	// First let's check the output type.
	// Case 0.
	BOOST_CHECK((std::is_same<p_type1,decltype(p_type1{} + p_type1{})>::value));
	// Case 1.
	BOOST_CHECK((std::is_same<p_type1,decltype(p_type1{} + p_type2{})>::value));
	// Case 2.
	BOOST_CHECK((std::is_same<p_type1,decltype(p_type2{} + p_type1{})>::value));
	// Case 3, symmetric.
	BOOST_CHECK((std::is_same<p_type2,decltype(p_type3{} + p_type4{})>::value));
	BOOST_CHECK((std::is_same<p_type2,decltype(p_type4{} + p_type3{})>::value));
	// Case 4.
	BOOST_CHECK((std::is_same<p_type1,decltype(p_type1{} + 0)>::value));
	// Case 5.
	BOOST_CHECK((std::is_same<p_type2,decltype(p_type3{} + 0)>::value));
	// Case 6.
	BOOST_CHECK((std::is_same<p_type1,decltype(0 + p_type1{})>::value));
	// Case 7.
	BOOST_CHECK((std::is_same<p_type2,decltype(0 + p_type3{})>::value));
	// Check non-addable series.
	typedef g_series_type2<rational,int> p_type5;
	BOOST_CHECK((!is_addable<p_type1,p_type5>::value));
	BOOST_CHECK((!is_addable<p_type5,p_type1>::value));
	// Check coefficient series.
	typedef g_series_type<p_type1,int> p_type11;
	typedef g_series_type<p_type2,rational> p_type22;
	typedef g_series_type<p_type1,rational> p_type21;
	BOOST_CHECK((std::is_same<p_type11,decltype(p_type1{} + p_type11{})>::value));
	BOOST_CHECK((std::is_same<p_type11,decltype(p_type11{} + p_type1{})>::value));
	BOOST_CHECK((std::is_same<p_type21,decltype(p_type1{} + p_type22{})>::value));
	BOOST_CHECK((std::is_same<p_type21,decltype(p_type22{} + p_type1{})>::value));
	BOOST_CHECK((std::is_same<p_type11,decltype(p_type11{} + p_type22{})>::value));
	BOOST_CHECK((std::is_same<p_type11,decltype(p_type22{} + p_type11{})>::value));
	// Type testing for in-place addition.
	// Case 0.
	BOOST_CHECK((std::is_same<p_type1 &,decltype(std::declval<p_type1 &>() += p_type1{})>::value));
	// Case 1.
	BOOST_CHECK((std::is_same<p_type1 &,decltype(std::declval<p_type1 &>() += p_type2{})>::value));
	// Case 2.
	BOOST_CHECK((std::is_same<p_type2 &,decltype(std::declval<p_type2 &>() += p_type1{})>::value));
	// Case 3, symmetric.
	BOOST_CHECK((std::is_same<p_type3 &,decltype(std::declval<p_type3 &>() += p_type4{})>::value));
	BOOST_CHECK((std::is_same<p_type4 &,decltype(std::declval<p_type4 &>() += p_type3{})>::value));
	// Case 4.
	BOOST_CHECK((std::is_same<p_type1 &,decltype(std::declval<p_type1 &>() += 0)>::value));
	// Case 5.
	BOOST_CHECK((std::is_same<p_type3 &,decltype(std::declval<p_type3 &>() += 0)>::value));
	// Cases 6 and 7 do not make sense at the moment.
	BOOST_CHECK((!is_addable_in_place<int,p_type3>::value));
	BOOST_CHECK((!is_addable_in_place<p_type1,p_type11>::value));
	// Checks for coefficient series.
	p_type11 tmp;
	BOOST_CHECK((std::is_same<p_type11 &,decltype(tmp += p_type1{})>::value));
	p_type22 tmp2;
	BOOST_CHECK((std::is_same<p_type22 &,decltype(tmp2 += p_type1{})>::value));
}

struct arithmetics_sub_tag {};

namespace piranha
{
template <>
class debug_access<arithmetics_sub_tag>
{
	public:
		template <typename Cf>
		struct runner
		{
			template <typename Expo>
			void operator()(const Expo &)
			{
				typedef g_series_type<Cf,Expo> p_type1;
				typedef g_series_type2<Cf,Expo> p_type2;
				typedef g_series_type<int,Expo> p_type3;
				// Binary sub first.
				// Some type checks - these are not subtractable as they result in an ambiguity
				// between two series with same coefficient but different series types.
				BOOST_CHECK((!is_subtractable<p_type1,p_type2>::value));
				BOOST_CHECK((!is_subtractable<p_type2,p_type1>::value));
				BOOST_CHECK((!is_subtractable_in_place<p_type1,p_type2>::value));
				BOOST_CHECK((!is_subtractable_in_place<p_type2,p_type1>::value));
				// Various subcases of case 0.
				p_type1 x{"x"}, y{"y"}, x2 = x + x;
				// No need to merge args.
				auto tmp = x2 - x;
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1));
				BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
				// Try with moves on both sides.
				tmp = p_type1{x} - x2;
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(-1));
				BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
				tmp = x2 - p_type1{x};
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1));
				BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
				// Now with merging.
				tmp = x - y;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				auto it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				// With moves.
				tmp = p_type1{x} - y;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = x - p_type1{y};
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				// Test the swapping of operands when one series is larger than the other.
				tmp = (x2 - y) - x;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == -1);
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == -1);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = x2 - (y - x);
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 3 || it->m_cf == -1);
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == 3 || it->m_cf == -1);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				// Some tests for case 1/4.
				tmp = x - p_type3{"y"};
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == -1);
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == -1);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = x2 - (p_type3{"y"} - p_type3{"x"});
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 3 || it->m_cf == -1);
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == 3 || it->m_cf == -1);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = x - 1;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == -1);
				BOOST_CHECK(it->m_key.size() == 1u);
				++it;
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == -1);
				BOOST_CHECK(it->m_key.size() == 1u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
				// Symmetric of the previous case.
				tmp = p_type3{"y"} - x;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == -1);
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == -1);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = (p_type3{"y"} - p_type3{"x"}) - x2;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == -3);
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == -3);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = 1 - x;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == -1);
				BOOST_CHECK(it->m_key.size() == 1u);
				++it;
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == -1);
				BOOST_CHECK(it->m_key.size() == 1u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
				// Case 3/5 and symmetric.
				using p_type4 = g_series_type<g_series_type<int,Expo>,Expo>;
				using p_type5 = g_series_type<double,Expo>;
				auto tmp2 = p_type4{"x"} - p_type5{"y"};
				BOOST_CHECK_EQUAL(tmp2.size(),2u);
				BOOST_CHECK((std::is_same<decltype(tmp2),g_series_type<g_series_type<double,Expo>,Expo>>::value));
				auto it2 = tmp2.m_container.begin();
				BOOST_CHECK((it2->m_cf == -g_series_type<double,Expo>{"y"} || it2->m_cf == 1));
				BOOST_CHECK(it2->m_key.size() == 1u);
				++it2;
				BOOST_CHECK((it2->m_cf == -g_series_type<double,Expo>{"y"} || it2->m_cf == 1));
				BOOST_CHECK(it2->m_key.size() == 1u);
				BOOST_CHECK((tmp2.m_symbol_set == symbol_set{symbol{"x"}}));
				tmp2 = p_type5{"y"} - p_type4{"x"};
				BOOST_CHECK_EQUAL(tmp2.size(),2u);
				BOOST_CHECK((std::is_same<decltype(tmp2),g_series_type<g_series_type<double,Expo>,Expo>>::value));
				it2 = tmp2.m_container.begin();
				BOOST_CHECK((it2->m_cf == g_series_type<double,Expo>{"y"} || it2->m_cf == -1));
				BOOST_CHECK(it2->m_key.size() == 1u);
				++it2;
				BOOST_CHECK((it2->m_cf == g_series_type<double,Expo>{"y"} || it2->m_cf == -1));
				BOOST_CHECK(it2->m_key.size() == 1u);
				BOOST_CHECK((tmp2.m_symbol_set == symbol_set{symbol{"x"}}));
				// Now in-place.
				// Case 0.
				tmp = x2;
				tmp -= x;
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1));
				BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
				// Move.
				tmp = x2;
				tmp -= p_type1{x};
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(1));
				BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
				// Now with merging.
				tmp = x;
				tmp -= y;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				// With moves.
				tmp = x;
				tmp -= p_type1{y};
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				// Test the swapping of operands when one series is larger than the other.
				tmp = x2 - y;
				tmp -= x;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = x;
				tmp -= y - x2;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(3) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(3) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				// Some tests for case 1/4.
				tmp = x;
				tmp -= p_type3{"y"};
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = x2;
				tmp -= p_type3{"y"} - p_type3{"x"};
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(3) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(3) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = x;
				tmp -= 1;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 1u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 1u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
				// Symmetric of the previous case.
				p_type3 tmp3{"y"};
				tmp3 -= x;
				BOOST_CHECK_EQUAL(tmp3.size(),2u);
				auto it3 = tmp3.m_container.begin();
				BOOST_CHECK(it3->m_cf == Cf(1) || it3->m_cf == Cf(-1));
				BOOST_CHECK(it3->m_key.size() == 2u);
				++it3;
				BOOST_CHECK(it3->m_cf == Cf(1) || it3->m_cf == Cf(-1));
				BOOST_CHECK(it3->m_key.size() == 2u);
				BOOST_CHECK((tmp3.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp3 = p_type3{"x"};
				tmp3 -= p_type3{"y"} - p_type3{"x"};
				tmp3 -= x;
				BOOST_CHECK_EQUAL(tmp3.size(),2u);
				it3 = tmp3.m_container.begin();
				BOOST_CHECK(it3->m_cf == Cf(1) || it3->m_cf == Cf(-1));
				BOOST_CHECK(it3->m_key.size() == 2u);
				++it3;
				BOOST_CHECK(it3->m_cf == Cf(1) || it3->m_cf == Cf(-1));
				BOOST_CHECK(it3->m_key.size() == 2u);
				BOOST_CHECK((tmp3.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				// Case 3/5.
				auto tmp4 = p_type4{"x"};
				tmp4 -= p_type5{"y"};
				BOOST_CHECK_EQUAL(tmp4.size(),2u);
				auto it4 = tmp4.m_container.begin();
				BOOST_CHECK((std::is_same<decltype(it4->m_cf),g_series_type<int,Expo>>::value));
				BOOST_CHECK((it4->m_cf == -g_series_type<int,Expo>{"y"} || it4->m_cf == 1));
				BOOST_CHECK(it4->m_key.size() == 1u);
				++it4;
				BOOST_CHECK((it4->m_cf == -g_series_type<int,Expo>{"y"} || it4->m_cf == 1));
				BOOST_CHECK(it4->m_key.size() == 1u);
				BOOST_CHECK((tmp4.m_symbol_set == symbol_set{symbol{"x"}}));
			}
		};
		template <typename Cf>
		void operator()(const Cf &)
		{
			boost::mpl::for_each<expo_types>(runner<Cf>());
		}
};
}

typedef debug_access<arithmetics_sub_tag> arithmetics_sub_tester;

BOOST_AUTO_TEST_CASE(series_arithmetics_sub_test)
{
	// Functional testing.
	boost::mpl::for_each<cf_types>(arithmetics_sub_tester());
	// Type testing for binary subtraction.
	typedef g_series_type<rational,int> p_type1;
	typedef g_series_type<int,rational> p_type2;
	typedef g_series_type<short,rational> p_type3;
	typedef g_series_type<char,rational> p_type4;
	// First let's check the output type.
	// Case 0.
	BOOST_CHECK((std::is_same<p_type1,decltype(p_type1{} - p_type1{})>::value));
	// Case 1.
	BOOST_CHECK((std::is_same<p_type1,decltype(p_type1{} - p_type2{})>::value));
	// Case 2.
	BOOST_CHECK((std::is_same<p_type1,decltype(p_type2{} - p_type1{})>::value));
	// Case 3, symmetric.
	BOOST_CHECK((std::is_same<p_type2,decltype(p_type3{} - p_type4{})>::value));
	BOOST_CHECK((std::is_same<p_type2,decltype(p_type4{} - p_type3{})>::value));
	// Case 4.
	BOOST_CHECK((std::is_same<p_type1,decltype(p_type1{} - 0)>::value));
	// Case 5.
	BOOST_CHECK((std::is_same<p_type2,decltype(p_type3{} - 0)>::value));
	// Case 6.
	BOOST_CHECK((std::is_same<p_type1,decltype(0 - p_type1{})>::value));
	// Case 7.
	BOOST_CHECK((std::is_same<p_type2,decltype(0 - p_type3{})>::value));
	// Check non-subtractable series.
	typedef g_series_type2<rational,int> p_type5;
	BOOST_CHECK((!is_subtractable<p_type1,p_type5>::value));
	BOOST_CHECK((!is_subtractable<p_type5,p_type1>::value));
	// Check coefficient series.
	typedef g_series_type<p_type1,int> p_type11;
	typedef g_series_type<p_type2,rational> p_type22;
	typedef g_series_type<p_type1,rational> p_type21;
	BOOST_CHECK((std::is_same<p_type11,decltype(p_type1{} - p_type11{})>::value));
	BOOST_CHECK((std::is_same<p_type11,decltype(p_type11{} - p_type1{})>::value));
	BOOST_CHECK((std::is_same<p_type21,decltype(p_type1{} - p_type22{})>::value));
	BOOST_CHECK((std::is_same<p_type21,decltype(p_type22{} - p_type1{})>::value));
	BOOST_CHECK((std::is_same<p_type11,decltype(p_type11{} - p_type22{})>::value));
	BOOST_CHECK((std::is_same<p_type11,decltype(p_type22{} - p_type11{})>::value));
	// Type testing for in-place subtraction.
	// Case 0.
	BOOST_CHECK((std::is_same<p_type1 &,decltype(std::declval<p_type1 &>() -= p_type1{})>::value));
	// Case 1.
	BOOST_CHECK((std::is_same<p_type1 &,decltype(std::declval<p_type1 &>() -= p_type2{})>::value));
	// Case 2.
	BOOST_CHECK((std::is_same<p_type2 &,decltype(std::declval<p_type2 &>() -= p_type1{})>::value));
	// Case 3, symmetric.
	BOOST_CHECK((std::is_same<p_type3 &,decltype(std::declval<p_type3 &>() -= p_type4{})>::value));
	BOOST_CHECK((std::is_same<p_type4 &,decltype(std::declval<p_type4 &>() -= p_type3{})>::value));
	// Case 4.
	BOOST_CHECK((std::is_same<p_type1 &,decltype(std::declval<p_type1 &>() -= 0)>::value));
	// Case 5.
	BOOST_CHECK((std::is_same<p_type3 &,decltype(std::declval<p_type3 &>() -= 0)>::value));
	// Cases 6 and 7 do not make sense at the moment.
	BOOST_CHECK((!is_subtractable_in_place<int,p_type3>::value));
	BOOST_CHECK((!is_subtractable_in_place<p_type1,p_type11>::value));
	// Checks for coefficient series.
	p_type11 tmp;
	BOOST_CHECK((std::is_same<p_type11 &,decltype(tmp -= p_type1{})>::value));
	p_type22 tmp2;
	BOOST_CHECK((std::is_same<p_type22 &,decltype(tmp2 -= p_type1{})>::value));
}

struct arithmetics_mul_tag {};

namespace piranha
{
template <>
class debug_access<arithmetics_mul_tag>
{
	public:
		template <typename Cf>
		struct runner
		{
			template <typename Expo>
			void operator()(const Expo &)
			{
				typedef g_series_type<Cf,Expo> p_type1;
				typedef g_series_type2<Cf,Expo> p_type2;
				typedef g_series_type<int,Expo> p_type3;
				// Binary mul first.
				// Some type checks - these are not multipliable as they result in an ambiguity
				// between two series with same coefficient but different series types.
				BOOST_CHECK((!is_multipliable<p_type1,p_type2>::value));
				BOOST_CHECK((!is_multipliable<p_type2,p_type1>::value));
				BOOST_CHECK((!is_multipliable_in_place<p_type1,p_type2>::value));
				BOOST_CHECK((!is_multipliable_in_place<p_type2,p_type1>::value));
				// Various subcases of case 0.
				p_type1 x{"x"}, y{"y"};
				// No need to merge args.
				auto tmp = 2 * x * x;
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(2) * Cf(1));
				BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
				// Try with moves on both sides.
				tmp = 3 * p_type1{x} * 2 * x;
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(3) * Cf(2));
				BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
				tmp = 2 * x * 3 * p_type1{x};
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(2) * Cf(3));
				BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
				// Now with merging.
				tmp = x * y;
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				auto it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1) * Cf(1));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				// With moves.
				tmp = p_type1{x} * y;
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1) * Cf(1));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = x * p_type1{y};
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1) * Cf(1));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				// Test the swapping of operands when one series is larger than the other.
				tmp = (x + y) * 2 * x;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(2) * Cf(1));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(2) * Cf(1));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = x * (2 * y + 2 * x);
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(2) * Cf(1));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(2) * Cf(1));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				// Some tests for case 1/4.
				tmp = 3 * x * p_type3{"y"};
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 3);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = 3 * x * (p_type3{"y"} + p_type3{"x"});
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 3);
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == 3);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = x * 2;
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 2);
				BOOST_CHECK(it->m_key.size() == 1u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
				// Symmetric of the previous case.
				tmp = p_type3{"y"} * x * 3;
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 3);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = (p_type3{"y"} + p_type3{"x"}) * 4 * x;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 4);
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == 4);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = -2 * x;
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == -2);
				BOOST_CHECK(it->m_key.size() == 1u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
				// Case 3/5 and symmetric.
				using p_type4 = g_series_type<g_series_type<int,Expo>,Expo>;
				using p_type5 = g_series_type<double,Expo>;
				auto tmp2 = p_type4{"x"} * p_type5{"y"} * -1;
				BOOST_CHECK_EQUAL(tmp2.size(),1u);
				BOOST_CHECK((std::is_same<decltype(tmp2),g_series_type<g_series_type<double,Expo>,Expo>>::value));
				auto it2 = tmp2.m_container.begin();
				BOOST_CHECK((it2->m_cf == -g_series_type<double,Expo>{"y"}));
				BOOST_CHECK(it2->m_key.size() == 1u);
				BOOST_CHECK((tmp2.m_symbol_set == symbol_set{symbol{"x"}}));
				tmp2 = p_type5{"y"} * p_type4{"x"} * 2;
				BOOST_CHECK_EQUAL(tmp2.size(),1u);
				BOOST_CHECK((std::is_same<decltype(tmp2),g_series_type<g_series_type<double,Expo>,Expo>>::value));
				it2 = tmp2.m_container.begin();
				BOOST_CHECK((it2->m_cf == 2 * g_series_type<double,Expo>{"y"}));
				BOOST_CHECK(it2->m_key.size() == 1u);
				BOOST_CHECK((tmp2.m_symbol_set == symbol_set{symbol{"x"}}));
				// Now in-place.
				// Case 0.
				tmp = 2 * x;
				tmp *= x;
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(2));
				BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
				// Move.
				tmp = 2 * x;
				tmp *= p_type1{x};
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				BOOST_CHECK(tmp.m_container.begin()->m_cf == Cf(2));
				BOOST_CHECK(tmp.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK(tmp.m_symbol_set == symbol_set{symbol{"x"}});
				// Now with merging.
				tmp = -3 * x;
				tmp *= y;
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(-3));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				// With moves.
				tmp = 4 * x;
				tmp *= p_type1{y};
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(4));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				// Test the swapping of operands when one series is larger than the other.
				tmp = 4 * (x + y);
				tmp *= x;
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 4);
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == 4);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = x;
				tmp *= 3 * (y + x);
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 3);
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == 3);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				// Some tests for case 1/4.
				tmp = 4 * x;
				tmp *= p_type3{"y"};
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 4);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = x;
				tmp *= -4 * (p_type3{"y"} + p_type3{"x"});
				BOOST_CHECK_EQUAL(tmp.size(),2u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == -4);
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == -4);
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp = x;
				tmp *= 3;
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == 3);
				BOOST_CHECK(it->m_key.size() == 1u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
				// Symmetric of the previous case.
				p_type3 tmp3{"y"};
				tmp3 *= -4 * x;
				BOOST_CHECK_EQUAL(tmp3.size(),1u);
				auto it3 = tmp3.m_container.begin();
				BOOST_CHECK(it3->m_cf == -4);
				BOOST_CHECK(it3->m_key.size() == 2u);
				BOOST_CHECK((tmp3.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				tmp3 *= p_type3{"y"} + p_type3{"x"};
				tmp3 *= -x;
				BOOST_CHECK_EQUAL(tmp3.size(),2u);
				it3 = tmp3.m_container.begin();
				BOOST_CHECK(it3->m_cf == 4);
				BOOST_CHECK(it3->m_key.size() == 2u);
				BOOST_CHECK((tmp3.m_symbol_set == symbol_set{symbol{"x"},symbol{"y"}}));
				// Case 3/5.
				auto tmp4 = p_type4{"x"};
				tmp4 *= p_type5{"y"} * 3;
				BOOST_CHECK_EQUAL(tmp4.size(),1u);
				auto it4 = tmp4.m_container.begin();
				BOOST_CHECK((std::is_same<decltype(it4->m_cf),g_series_type<int,Expo>>::value));
				BOOST_CHECK((it4->m_cf == 3 * g_series_type<int,Expo>{"y"}));
				BOOST_CHECK(it4->m_key.size() == 1u);
				BOOST_CHECK((tmp4.m_symbol_set == symbol_set{symbol{"x"}}));
			}
		};
		template <typename Cf>
		void operator()(const Cf &)
		{
			boost::mpl::for_each<expo_types>(runner<Cf>());
		}
};
}

typedef debug_access<arithmetics_mul_tag> arithmetics_mul_tester;

BOOST_AUTO_TEST_CASE(series_arithmetics_mul_test)
{
	// Functional testing.
	boost::mpl::for_each<cf_types>(arithmetics_mul_tester());
	// Type testing for binary multiplication.
	typedef g_series_type<rational,int> p_type1;
	typedef g_series_type<int,rational> p_type2;
	typedef g_series_type<short,rational> p_type3;
	typedef g_series_type<char,rational> p_type4;
	// First let's check the output type.
	// Case 0.
	BOOST_CHECK((std::is_same<p_type1,decltype(p_type1{} * p_type1{})>::value));
	// Case 1.
	BOOST_CHECK((std::is_same<p_type1,decltype(p_type1{} * p_type2{})>::value));
	// Case 2.
	BOOST_CHECK((std::is_same<p_type1,decltype(p_type2{} * p_type1{})>::value));
	// Case 3, symmetric.
	BOOST_CHECK((std::is_same<p_type2,decltype(p_type3{} * p_type4{})>::value));
	BOOST_CHECK((std::is_same<p_type2,decltype(p_type4{} * p_type3{})>::value));
	// Case 4.
	BOOST_CHECK((std::is_same<p_type1,decltype(p_type1{} * 0)>::value));
	// Case 5.
	BOOST_CHECK((std::is_same<p_type2,decltype(p_type3{} * 0)>::value));
	// Case 6.
	BOOST_CHECK((std::is_same<p_type1,decltype(0 * p_type1{})>::value));
	// Case 7.
	BOOST_CHECK((std::is_same<p_type2,decltype(0 * p_type3{})>::value));
	// Check non-multipliable series.
	typedef g_series_type2<rational,int> p_type5;
	BOOST_CHECK((!is_multipliable<p_type1,p_type5>::value));
	BOOST_CHECK((!is_multipliable<p_type5,p_type1>::value));
	// Check coefficient series.
	typedef g_series_type<p_type1,int> p_type11;
	typedef g_series_type<p_type2,rational> p_type22;
	typedef g_series_type<p_type1,rational> p_type21;
	BOOST_CHECK((std::is_same<p_type11,decltype(p_type1{} * p_type11{})>::value));
	BOOST_CHECK((std::is_same<p_type11,decltype(p_type11{} * p_type1{})>::value));
	BOOST_CHECK((std::is_same<p_type21,decltype(p_type1{} * p_type22{})>::value));
	BOOST_CHECK((std::is_same<p_type21,decltype(p_type22{} * p_type1{})>::value));
	BOOST_CHECK((std::is_same<p_type11,decltype(p_type11{} * p_type22{})>::value));
	BOOST_CHECK((std::is_same<p_type11,decltype(p_type22{} * p_type11{})>::value));
	// Type testing for in-place multiplication.
	// Case 0.
	BOOST_CHECK((std::is_same<p_type1 &,decltype(std::declval<p_type1 &>() *= p_type1{})>::value));
	// Case 1.
	BOOST_CHECK((std::is_same<p_type1 &,decltype(std::declval<p_type1 &>() *= p_type2{})>::value));
	// Case 2.
	BOOST_CHECK((std::is_same<p_type2 &,decltype(std::declval<p_type2 &>() *= p_type1{})>::value));
	// Case 3, symmetric.
	BOOST_CHECK((std::is_same<p_type3 &,decltype(std::declval<p_type3 &>() *= p_type4{})>::value));
	BOOST_CHECK((std::is_same<p_type4 &,decltype(std::declval<p_type4 &>() *= p_type3{})>::value));
	// Case 4.
	BOOST_CHECK((std::is_same<p_type1 &,decltype(std::declval<p_type1 &>() *= 0)>::value));
	// Case 5.
	BOOST_CHECK((std::is_same<p_type3 &,decltype(std::declval<p_type3 &>() *= 0)>::value));
	// Cases 6 and 7 do not make sense at the moment.
	BOOST_CHECK((!is_multipliable_in_place<int,p_type3>::value));
	BOOST_CHECK((!is_multipliable_in_place<p_type1,p_type11>::value));
	// Checks for coefficient series.
	p_type11 tmp;
	BOOST_CHECK((std::is_same<p_type11 &,decltype(tmp *= p_type1{})>::value));
	p_type22 tmp2;
	BOOST_CHECK((std::is_same<p_type22 &,decltype(tmp2 *= p_type1{})>::value));
}

struct arithmetics_div_tag {};

namespace piranha
{
template <>
class debug_access<arithmetics_div_tag>
{
	public:
		template <typename Cf>
		struct runner
		{
			template <typename Expo>
			void operator()(const Expo &)
			{
				typedef g_series_type<Cf,Expo> p_type1;
				p_type1 x{"x"};
				// Some tests for case 4.
				auto tmp = 3 * x / 2;
				BOOST_CHECK_EQUAL(tmp.size(),1u);
				auto it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(3) / 2);
				BOOST_CHECK(it->m_key.size() == 1u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
				// Case 5.
				auto tmp2 = 3 * x / 2.;
				auto it2 = tmp2.m_container.begin();
				BOOST_CHECK(it2->m_cf == Cf(3) / 2.);
				BOOST_CHECK(it2->m_key.size() == 1u);
				BOOST_CHECK((tmp2.m_symbol_set == symbol_set{symbol{"x"}}));
				// In-place.
				// Case 4.
				tmp = 3 * x;
				tmp /= 2;
				it = tmp.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(3) / 2);
				BOOST_CHECK(it->m_key.size() == 1u);
				BOOST_CHECK((tmp.m_symbol_set == symbol_set{symbol{"x"}}));
				// Case 5.
				tmp2 = 3 * x;
				tmp2 /= 2.;
				it2 = tmp2.m_container.begin();
				BOOST_CHECK(it2->m_cf == Cf(3) / 2.);
				BOOST_CHECK(it2->m_key.size() == 1u);
				BOOST_CHECK((tmp2.m_symbol_set == symbol_set{symbol{"x"}}));
			}
		};
		template <typename Cf>
		void operator()(const Cf &)
		{
			boost::mpl::for_each<expo_types>(runner<Cf>());
		}
};
}

typedef debug_access<arithmetics_div_tag> arithmetics_div_tester;

BOOST_AUTO_TEST_CASE(series_arithmetics_div_test)
{
	// Functional testing.
	boost::mpl::for_each<cf_types>(arithmetics_div_tester());
	// Type testing for binary division.
	typedef g_series_type<rational,int> p_type1;
	typedef g_series_type<p_type1,int> p_type11;
	typedef g_series_type<double,int> p_type1d;
	typedef g_series_type<float,int> p_type1f;
	// First let's check the output type.
	// Case 4.
	BOOST_CHECK((std::is_same<p_type1,decltype(p_type1{} / 0)>::value));
	BOOST_CHECK((std::is_same<p_type1,decltype(p_type1{} / integer{})>::value));
	BOOST_CHECK((std::is_same<p_type1,decltype(p_type1{} / rational{})>::value));
	// Case 5.
	BOOST_CHECK((std::is_same<p_type1d,decltype(p_type1{} / 0.)>::value));
	BOOST_CHECK((std::is_same<p_type1f,decltype(p_type1{} / 0.f)>::value));
	// Check non-divisible series.
	BOOST_CHECK((!is_divisible<double,p_type1>::value));
	BOOST_CHECK((!is_divisible<int,p_type1>::value));
	BOOST_CHECK((!is_divisible<integer,p_type1>::value));
	// Type testing for in-place multiplication.
	// Case 4.
	BOOST_CHECK((std::is_same<p_type1 &,decltype(std::declval<p_type1 &>() /= 0)>::value));
	// Case 5.
	BOOST_CHECK((std::is_same<p_type1 &,decltype(std::declval<p_type1 &>() /= 0.)>::value));
	// Not divisible in-place.
	BOOST_CHECK((!is_divisible_in_place<int,p_type1>::value));
	BOOST_CHECK((!is_divisible_in_place<p_type11,p_type1>::value));
	// Special cases to test the erasing of terms.
	using pint = g_series_type<integer,int>;
	pint x{"x"}, y{"y"};
	auto tmp = 2 * x + y;
	tmp /= 2;
	BOOST_CHECK_EQUAL(tmp,x);
	tmp = 2 * x + 2 * y;
	tmp /= 3;
	BOOST_CHECK(tmp.empty());
}

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

#include "../src/t_substitutable_series.hpp"

#define BOOST_TEST_MODULE t_substitutable_series_test
#include <boost/test/unit_test.hpp>

#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../src/base_term.hpp"
#include "../src/environment.hpp"
#include "../src/forwarding.hpp"
#include "../src/math.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/polynomial.hpp"
#include "../src/poisson_series.hpp"
#include "../src/real.hpp"
#include "../src/symbol_set.hpp"

using namespace piranha;

// NOTE: when series mutliplication SFINAEs out, change the signature of t_subs in key02 to be the correct one
// and check that has_t_subs fails for the series type because g_series is not multipliable by std::string any more.

struct key02
{
	key02() = default;
	key02(const key02 &) = default;
	key02(key02 &&) noexcept;
	key02 &operator=(const key02 &) = default;
	key02 &operator=(key02 &&) noexcept;
	key02(const symbol_set &);
	bool operator==(const key02 &) const;
	bool operator!=(const key02 &) const;
	bool is_compatible(const symbol_set &) const noexcept;
	bool is_ignorable(const symbol_set &) const noexcept;
	key02 merge_args(const symbol_set &, const symbol_set &) const;
	bool is_unitary(const symbol_set &) const;
	void print(std::ostream &, const symbol_set &) const;
	void print_tex(std::ostream &, const symbol_set &) const;
	template <typename T, typename U>
	std::vector<std::pair<std::string,int>> t_subs(const std::string &, const T &, const U &, const symbol_set &) const;
};

namespace std
{

template <>
struct hash<key02>
{
	std::size_t operator()(const key02 &) const noexcept;
};

}

template <typename Cf, typename Key>
class g_term_type: public base_term<Cf,Key,g_term_type<Cf,Key>>
{
		typedef base_term<Cf,Key,g_term_type> base;
	public:
		g_term_type() = default;
		g_term_type(const g_term_type &) = default;
		g_term_type(g_term_type &&) = default;
		g_term_type &operator=(const g_term_type &) = default;
		g_term_type &operator=(g_term_type &&) = default;
		PIRANHA_FORWARDING_CTOR(g_term_type,base)
};

template <typename Cf, typename Key>
class g_series_type: public t_substitutable_series<series<g_term_type<Cf,Key>,g_series_type<Cf,Key>>,g_series_type<Cf,Key>>
{
	public:
		typedef t_substitutable_series<series<g_term_type<Cf,Key>,g_series_type<Cf,Key>>,g_series_type<Cf,Key>> base;
		g_series_type() = default;
		g_series_type(const g_series_type &) = default;
		g_series_type(g_series_type &&) = default;
		g_series_type &operator=(const g_series_type &) = default;
		g_series_type &operator=(g_series_type &&) = default;
		PIRANHA_FORWARDING_CTOR(g_series_type,base)
		PIRANHA_FORWARDING_ASSIGNMENT(g_series_type,base)
};

BOOST_AUTO_TEST_CASE(t_subs_series_t_subs_test)
{
	environment env;
	typedef poisson_series<polynomial<rational,short>> p_type1;
	p_type1 x{"x"}, y{"y"};
	BOOST_CHECK((has_t_subs<p_type1>::value));
	BOOST_CHECK((has_t_subs<p_type1,rational>::value));
	BOOST_CHECK((has_t_subs<p_type1,rational,rational>::value));
	BOOST_CHECK((has_t_subs<p_type1,double>::value));
	BOOST_CHECK((!has_t_subs<p_type1,int,double>::value));
	BOOST_CHECK_EQUAL(p_type1{}.t_subs("a",2,3),0);
	BOOST_CHECK_EQUAL(math::t_subs(p_type1{},"a",2,3),0);
	BOOST_CHECK_EQUAL(p_type1{"x"}.t_subs("a",2,3),p_type1{"x"});
	BOOST_CHECK_EQUAL(math::t_subs(p_type1{"x"},"a",2,3),p_type1{"x"});
	BOOST_CHECK_EQUAL(math::cos(p_type1{"x"}).t_subs("a",2,3),p_type1{"x"}.cos());
	BOOST_CHECK_EQUAL(math::t_subs(math::cos(p_type1{"x"}),"a",2,3),p_type1{"x"}.cos());
	BOOST_CHECK_EQUAL(math::t_subs(math::cos(p_type1{"x"}),"x",2,3),2);
	BOOST_CHECK_EQUAL(math::t_subs(math::sin(p_type1{"x"}),"x",2,3),3);
	BOOST_CHECK_EQUAL(math::t_subs(math::cos(p_type1{"x"})+math::sin(p_type1{"x"}),"x",2,3),5);
	auto tmp1 = math::t_subs(4 * math::cos(p_type1{"x"}+p_type1{"y"}) +
		3 * math::sin(p_type1{"x"}+p_type1{"y"}),"x",2,3);
	BOOST_CHECK((std::is_same<decltype(tmp1),p_type1>::value));
	BOOST_CHECK_EQUAL(tmp1,4*2*math::cos(y)-4*3*math::sin(y)+3*3*math::cos(y)+3*2*math::sin(y));
	auto tmp2 = math::t_subs(4 * math::cos(p_type1{"x"}-p_type1{"y"}) +
		3 * math::sin(p_type1{"x"}-p_type1{"y"}),"x",2,3);
	BOOST_CHECK((std::is_same<decltype(tmp2),p_type1>::value));
	BOOST_CHECK_EQUAL(tmp2,4*2*math::cos(y)+4*3*math::sin(y)+3*3*math::cos(y)-3*2*math::sin(y));
	auto tmp3 = math::t_subs(4 * math::cos(-p_type1{"x"}-p_type1{"y"}) +
		3 * math::sin(-p_type1{"x"}-p_type1{"y"}),"x",2,3);
	BOOST_CHECK((std::is_same<decltype(tmp3),p_type1>::value));
	BOOST_CHECK_EQUAL(tmp3,4*2*math::cos(y)-4*3*math::sin(y)-3*3*math::cos(y)-3*2*math::sin(y));
	// Some trigonometric identities from Wikipedia.
	p_type1 c{"c"}, s{"s"};
	BOOST_CHECK_EQUAL(math::sin(3*x).t_subs("x",c,s),3*c*c*s-s*s*s);
	BOOST_CHECK_EQUAL(math::cos(3*x).t_subs("x",c,s),c*c*c-3*s*s*c);
	BOOST_CHECK_EQUAL((math::t_subs((10*math::sin(x)-5*math::sin(3*x)+math::sin(5*x))/16,"x",c,s)).ipow_subs("c",integer(2),1-s*s),s*s*s*s*s);
	BOOST_CHECK_EQUAL((math::t_subs((10*math::cos(x)+5*math::cos(3*x)+math::cos(5*x))/16,"x",c,s)).ipow_subs("s",integer(2),1-c*c),c*c*c*c*c);
	BOOST_CHECK_EQUAL((math::t_subs((10*math::sin(2*x)-5*math::sin(6*x)+math::sin(10*x))/512,"x",c,s)).subs("c",math::cos(x)).subs("s",math::sin(x)),
		math::pow(math::cos(x),5)*math::pow(math::sin(x),5));
	BOOST_CHECK_EQUAL((math::cos(x)*math::cos(y)).t_subs("x",c,s),c*math::cos(y));
	BOOST_CHECK_EQUAL((math::sin(x)*math::sin(y)).t_subs("x",c,s),s*math::sin(y));
	BOOST_CHECK_EQUAL((math::sin(x)*math::cos(y)).t_subs("x",c,s),s*math::cos(y));
	BOOST_CHECK_EQUAL((math::cos(x)*math::sin(y)).t_subs("x",c,s),c*math::sin(y));
	BOOST_CHECK_EQUAL(4*math::sin(2*x).t_subs("x",c,s),8*s*c);
	BOOST_CHECK_EQUAL(5*math::cos(2*x).t_subs("x",c,s),5*(c*c-s*s));
	BOOST_CHECK_EQUAL((2*math::sin(x+y)*math::cos(x-y)).t_subs("x",c,s),2*c*s+math::sin(2*y));
	BOOST_CHECK_EQUAL(math::sin(x+p_type1{"pi2"}).t_subs("pi2",0,1),math::cos(x));
	BOOST_CHECK_EQUAL(math::cos(x+p_type1{"pi2"}).t_subs("pi2",0,1),-math::sin(x));
	BOOST_CHECK_EQUAL(math::sin(x+p_type1{"pi"}).t_subs("pi",-1,0),-math::sin(x));
	BOOST_CHECK_EQUAL(math::cos(x+p_type1{"pi"}).t_subs("pi",-1,0),-math::cos(x));
	BOOST_CHECK_EQUAL(math::sin(-x+p_type1{"pi"}).t_subs("pi",-1,0),math::sin(x));
	BOOST_CHECK_EQUAL(math::cos(-x+p_type1{"pi"}).t_subs("pi",-1,0),-math::cos(x));
	BOOST_CHECK_EQUAL((math::cos(-x+p_type1{"pi"}) + math::cos(y)).t_subs("pi",-1,0),-math::cos(x) + math::cos(y));
	BOOST_CHECK_EQUAL((math::cos(-x+p_type1{"pi"}) + math::cos(y+p_type1{"pi"})).t_subs("pi",-1,0),-math::cos(x) - math::cos(y));
	BOOST_CHECK_EQUAL(math::cos(x+p_type1{"2pi"}).t_subs("2pi",1,0),math::cos(x));
	BOOST_CHECK_EQUAL(math::sin(x+p_type1{"2pi"}).t_subs("2pi",1,0),math::sin(x));
	BOOST_CHECK_EQUAL(math::cos(-x+p_type1{"2pi"}).t_subs("2pi",1,0),math::cos(x));
	BOOST_CHECK_EQUAL(math::sin(-x+p_type1{"2pi"}).t_subs("2pi",1,0),-math::sin(x));
	BOOST_CHECK_EQUAL(math::t_subs(math::sin(-x+p_type1{"2pi"}),"2pi",1,0),-math::sin(x));
	// Real and mixed-series subs.
	typedef poisson_series<polynomial<real,short>> p_type3;
	BOOST_CHECK((std::is_same<decltype(p_type3{"x"}.t_subs("x",c,s)),p_type3>::value));
	BOOST_CHECK((std::is_same<decltype(math::t_subs(p_type3{"x"},"x",c,s)),p_type3>::value));
	BOOST_CHECK_EQUAL(p_type3{"x"}.cos().t_subs("x",c,s),c);
	BOOST_CHECK_EQUAL(p_type3{"x"}.cos().t_subs("x",real(.5),real(1.)),real(.5));
	BOOST_CHECK((std::is_same<decltype(x.t_subs("x",p_type3{"c"},p_type3{"s"})),p_type3>::value));
	BOOST_CHECK_EQUAL(x.t_subs("x",p_type3{"c"},p_type3{"s"}),p_type3{"x"});
	BOOST_CHECK_EQUAL(math::sin(x).t_subs("x",p_type3{"c"},p_type3{3.}),p_type3{3.});
	BOOST_CHECK_EQUAL(math::pow(math::cos(p_type3{"x"}),7).t_subs("x",real(.5),real(math::pow(real(3),.5))/2),math::pow(real(.5),7));
	BOOST_CHECK_EQUAL(math::pow(math::sin(p_type3{"x"}),7).t_subs("x",real(math::pow(real(3),.5))/2,real(.5)),math::pow(real(.5),7));
	BOOST_CHECK_EQUAL(math::t_subs(math::pow(math::sin(p_type3{"x"}),7),"x",real(math::pow(real(3),.5))/2,real(.5)),math::pow(real(.5),7));
	BOOST_CHECK(math::abs(((math::pow(math::sin(p_type3{"x"}),5)*math::pow(math::cos(p_type3{"x"}),5)).t_subs(
		"x",real(math::pow(real(3),.5))/2,real(.5)) - math::pow(real(.5),5)*math::pow(real(math::pow(real(3),.5))/2,5)).trim().evaluate(std::unordered_map<std::string,real>{})) < 1E-9);
	BOOST_CHECK(has_t_subs<p_type3>::value);
	BOOST_CHECK((has_t_subs<p_type3,double>::value));
	BOOST_CHECK((has_t_subs<p_type3,real>::value));
	BOOST_CHECK((has_t_subs<p_type3,double,double>::value));
	BOOST_CHECK((!has_t_subs<p_type3,double,int>::value));
	// Trig subs in the coefficient.
	typedef polynomial<poisson_series<rational>,short> p_type2;
	BOOST_CHECK_EQUAL(p_type2{}.t_subs("x",1,2),p_type2{});
	BOOST_CHECK_EQUAL(p_type2{3}.t_subs("x",1,2),p_type2{3});
	BOOST_CHECK((std::is_same<decltype(p_type2{}.t_subs("x",1,2)),p_type2>::value));
	BOOST_CHECK((std::is_same<decltype(p_type2{}.t_subs("x",rational(1),rational(2))),p_type2>::value));
	BOOST_CHECK(has_t_subs<p_type2>::value);
	BOOST_CHECK((has_t_subs<p_type2,double>::value));
	BOOST_CHECK((has_t_subs<p_type2,double,double>::value));
	BOOST_CHECK((!has_t_subs<p_type2,double,int>::value));
	BOOST_CHECK((!key_has_t_subs<key02,int,int>::value));
	BOOST_CHECK((!has_t_subs<g_series_type<double,key02>,double,double>::value));
}

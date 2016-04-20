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

#include "../src/rational_function.hpp"

#define BOOST_TEST_MODULE rational_function_01_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <initializer_list>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "../src/environment.hpp"
#include "../src/exceptions.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/math.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/pow.hpp"
#include "../src/print_tex_coefficient.hpp"
#include "../src/real.hpp"
#include "../src/serialization.hpp"
#include "../src/symbol_set.hpp"
#include "../src/symbol.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

using key_types = boost::mpl::vector<k_monomial,monomial<unsigned char>,monomial<integer>>;

static std::mt19937 rng;
static const int ntrials = 200;

template <typename Poly>
inline static Poly rn_poly(const Poly &x, const Poly &y, const Poly &z, std::uniform_int_distribution<int> &dist)
{
	int nterms = dist(rng);
	Poly retval;
	for (int i = 0; i < nterms; ++i) {
		int m = dist(rng);
		retval += m * (m % 2 ? 1 : -1 ) * x.pow(dist(rng)) * y.pow(dist(rng)) * z.pow(dist(rng));
	}
	return retval;
}

struct constructor_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using r_type = rational_function<Key>;
		using p_type = typename r_type::p_type;
		using q_type = typename r_type::q_type;
		p_type x{"x"}, y{"y"}, z{"z"};
		q_type xq{"x"}, yq{"y"}, zq{"z"};
		// Standard constructors.
		r_type r;
		BOOST_CHECK(r.is_canonical());
		BOOST_CHECK_EQUAL(r.num(),0);
		BOOST_CHECK_EQUAL(r.den(),1);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		r = r_type{5};
		auto s = r;
		BOOST_CHECK(s.is_canonical());
		BOOST_CHECK_EQUAL(s.num(),5);
		BOOST_CHECK_EQUAL(s.den(),1);
		BOOST_CHECK(s.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(s.den().get_symbol_set().size() == 0u);
		// Check status of moved-from objects.
		r_type t = std::move(s);
		BOOST_CHECK(t.is_canonical());
		BOOST_CHECK(!s.is_canonical());
		BOOST_CHECK_EQUAL(t.num(),5);
		BOOST_CHECK_EQUAL(t.den(),1);
		BOOST_CHECK(t.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(t.den().get_symbol_set().size() == 0u);
		BOOST_CHECK_EQUAL(s.num(),0);
		BOOST_CHECK_EQUAL(s.den(),0);
		BOOST_CHECK_THROW(s.canonicalise(),zero_division_error);
		BOOST_CHECK(s.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(s.den().get_symbol_set().size() == 0u);
		// Revive.
		s = t;
		BOOST_CHECK(s.is_canonical());
		BOOST_CHECK_EQUAL(s.num(),5);
		BOOST_CHECK_EQUAL(s.den(),1);
		BOOST_CHECK(s.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(s.den().get_symbol_set().size() == 0u);
		// Revive with move.
		t = std::move(s);
		BOOST_CHECK(!s.is_canonical());
		BOOST_CHECK(t.is_canonical());
		s = std::move(t);
		BOOST_CHECK(!t.is_canonical());
		BOOST_CHECK(s.is_canonical());
		BOOST_CHECK_EQUAL(s.num(),5);
		BOOST_CHECK_EQUAL(s.den(),1);
		BOOST_CHECK(s.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(s.den().get_symbol_set().size() == 0u);
		BOOST_CHECK_EQUAL(t.num(),0);
		BOOST_CHECK_EQUAL(t.den(),0);
		BOOST_CHECK(t.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(t.den().get_symbol_set().size() == 0u);
		// Unary ctors.
		BOOST_CHECK((std::is_constructible<r_type,p_type>::value));
		BOOST_CHECK((std::is_constructible<r_type,p_type &>::value));
		BOOST_CHECK((std::is_constructible<r_type,p_type &&>::value));
		BOOST_CHECK((std::is_constructible<r_type,q_type>::value));
		BOOST_CHECK((std::is_constructible<r_type,q_type &>::value));
		BOOST_CHECK((std::is_constructible<r_type,q_type &&>::value));
		BOOST_CHECK((std::is_constructible<r_type,int>::value));
		BOOST_CHECK((std::is_constructible<r_type,char &>::value));
		BOOST_CHECK((std::is_constructible<r_type,const integer &>::value));
		BOOST_CHECK((std::is_constructible<r_type,rational>::value));
		BOOST_CHECK((std::is_constructible<r_type,const rational &>::value));
		BOOST_CHECK((std::is_constructible<r_type,std::string>::value));
		BOOST_CHECK((std::is_constructible<r_type,char *>::value));
		BOOST_CHECK((std::is_constructible<r_type,const char *>::value));
		BOOST_CHECK((!std::is_constructible<r_type,double>::value));
		BOOST_CHECK((!std::is_constructible<r_type,const float>::value));
		BOOST_CHECK((!std::is_constructible<r_type,real &&>::value));
		// Ctor from ints.
		r = r_type{0};
		BOOST_CHECK_EQUAL(r.num(),0);
		BOOST_CHECK_EQUAL(r.den(),1);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		r = r_type{1u};
		BOOST_CHECK_EQUAL(r.num(),1);
		BOOST_CHECK_EQUAL(r.den(),1);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		r = r_type{-2_z};
		BOOST_CHECK_EQUAL(r.num(),-2);
		BOOST_CHECK_EQUAL(r.den(),1);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		// Ctor from string.
		r = r_type{"x"};
		BOOST_CHECK_EQUAL(r.num(),x);
		BOOST_CHECK_EQUAL(r.den(),1);
		BOOST_CHECK(r.num().get_symbol_set().size() == 1u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		r = r_type{std::string("y")};
		BOOST_CHECK_EQUAL(r.num(),y);
		BOOST_CHECK_EQUAL(r.den(),1);
		BOOST_CHECK(r.num().get_symbol_set().size() == 1u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		// Ctor from p_type.
		r = r_type{p_type{}};
		BOOST_CHECK_EQUAL(r.num(),0);
		BOOST_CHECK_EQUAL(r.den(),1);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		r = r_type{x+2*y};
		BOOST_CHECK_EQUAL(r.num(),x+2*y);
		BOOST_CHECK_EQUAL(r.den(),1);
		BOOST_CHECK(r.num().get_symbol_set().size() == 2u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		// Ctor from rational.
		r = r_type{0_q};
		BOOST_CHECK_EQUAL(r.num(),0);
		BOOST_CHECK_EQUAL(r.den(),1);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		r = r_type{8/-12_q};
		BOOST_CHECK_EQUAL(r.num(),-2);
		BOOST_CHECK_EQUAL(r.den(),3);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		// Ctor from q_type.
		r = r_type{q_type{}};
		BOOST_CHECK_EQUAL(r.num(),0);
		BOOST_CHECK_EQUAL(r.den(),1);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		r = r_type{xq/3+2*yq};
		BOOST_CHECK_EQUAL(r.num(),x+6*y);
		BOOST_CHECK_EQUAL(r.den(),3);
		BOOST_CHECK(r.num().get_symbol_set().size() == 2u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		r = r_type{xq+xq.pow(2)/2};
		BOOST_CHECK_EQUAL(r.num(),2*x+x*x);
		BOOST_CHECK_EQUAL(r.den(),2);
		BOOST_CHECK(r.num().get_symbol_set().size() == 1u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		// Binary ctors.
		BOOST_CHECK((std::is_constructible<r_type,r_type,r_type>::value));
		BOOST_CHECK((std::is_constructible<r_type,const r_type,r_type &&>::value));
		BOOST_CHECK((std::is_constructible<r_type,p_type,p_type>::value));
		BOOST_CHECK((std::is_constructible<r_type,p_type,int>::value));
		BOOST_CHECK((std::is_constructible<r_type,const int &,p_type>::value));
		BOOST_CHECK((std::is_constructible<r_type,p_type &,p_type>::value));
		BOOST_CHECK((std::is_constructible<r_type,p_type &&,const p_type &>::value));
		BOOST_CHECK((std::is_constructible<r_type,q_type,q_type>::value));
		BOOST_CHECK((std::is_constructible<r_type,q_type &,q_type>::value));
		BOOST_CHECK((std::is_constructible<r_type,q_type &&,const q_type &>::value));
		BOOST_CHECK((std::is_constructible<r_type,int,int>::value));
		BOOST_CHECK((std::is_constructible<r_type,char &,char>::value));
		BOOST_CHECK((std::is_constructible<r_type,const integer &,integer>::value));
		BOOST_CHECK((std::is_constructible<r_type,rational, rational>::value));
		BOOST_CHECK((std::is_constructible<r_type,const rational &, rational &&>::value));
		BOOST_CHECK((std::is_constructible<r_type,std::string,std::string>::value));
		BOOST_CHECK((std::is_constructible<r_type,std::string,int>::value));
		BOOST_CHECK((std::is_constructible<r_type,q_type &&,std::string>::value));
		BOOST_CHECK((std::is_constructible<r_type,std::string,std::string &>::value));
		BOOST_CHECK((std::is_constructible<r_type,char *,char*>::value));
		BOOST_CHECK((std::is_constructible<r_type,const char *,const char*>::value));
		BOOST_CHECK((!std::is_constructible<r_type,double,double>::value));
		BOOST_CHECK((!std::is_constructible<r_type,int,double>::value));
		BOOST_CHECK((!std::is_constructible<r_type,double,rational>::value));
		BOOST_CHECK((!std::is_constructible<r_type,const float,float>::value));
		BOOST_CHECK((!std::is_constructible<r_type,real &&,real>::value));
		// From ints.
		r = r_type{4,-12};
		BOOST_CHECK_EQUAL(r.num(),-1);
		BOOST_CHECK_EQUAL(r.den(),3);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		r = r_type{0u,12u};
		BOOST_CHECK_EQUAL(r.num(),0);
		BOOST_CHECK_EQUAL(r.den(),1);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		BOOST_CHECK_THROW((r = r_type{0_z,0_z}),zero_division_error);
		BOOST_CHECK_THROW((r = r_type{1,0}),zero_division_error);
		r = r_type{4,1};
		BOOST_CHECK_EQUAL(r.num(),4);
		BOOST_CHECK_EQUAL(r.den(),1);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		// From strings.
		r = r_type{"x","x"};
		BOOST_CHECK_EQUAL(r.num(),1);
		BOOST_CHECK_EQUAL(r.den(),1);
		BOOST_CHECK(r.num().get_symbol_set().size() == 1u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 1u);
		r = r_type{std::string("x"),std::string("y")};
		BOOST_CHECK_EQUAL(r.num(),x);
		BOOST_CHECK_EQUAL(r.den(),y);
		BOOST_CHECK(r.num().get_symbol_set().size() == 2u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 2u);
		// From p_type.
		r = r_type{p_type{6},p_type{-15}};
		BOOST_CHECK_EQUAL(r.num(),-2);
		BOOST_CHECK_EQUAL(r.den(),5);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		r = r_type{(x+y)*(x-y),(2*x+2*y)*z};
		BOOST_CHECK_EQUAL(r.num(),x-y);
		BOOST_CHECK_EQUAL(r.den(),2*z);
		BOOST_CHECK(r.num().get_symbol_set().size() == 3u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 3u);
		r = r_type{x,p_type{1}};
		BOOST_CHECK_EQUAL(r.num(),x);
		BOOST_CHECK_EQUAL(r.den(),1);
		BOOST_CHECK(r.num().get_symbol_set().size() == 1u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		r = r_type{p_type{0},(2*x+2*y)*z};
		BOOST_CHECK_EQUAL(r.num(),0);
		BOOST_CHECK_EQUAL(r.den(),1);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		BOOST_CHECK_THROW((r = r_type{(x+y)*(x-y),p_type{}}),zero_division_error);
		BOOST_CHECK_THROW((r = r_type{(x+y)*(x-y),x.pow(-1)}),std::invalid_argument);
		BOOST_CHECK_THROW((r = r_type{x.pow(-1),(x+y)*(x-y)}),std::invalid_argument);
		// From rational.
		r = r_type{0_q,-6_q};
		BOOST_CHECK_EQUAL(r.num(),0);
		BOOST_CHECK_EQUAL(r.den(),1);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		r = r_type{3_q,-6_q};
		BOOST_CHECK_EQUAL(r.num(),-1);
		BOOST_CHECK_EQUAL(r.den(),2);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		r = r_type{3/2_q,-7/6_q};
		BOOST_CHECK_EQUAL(r.num(),-9);
		BOOST_CHECK_EQUAL(r.den(),7);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		BOOST_CHECK_THROW((r = r_type{0_q,0_q}),zero_division_error);
		BOOST_CHECK_THROW((r = r_type{3/2_q,0_q}),zero_division_error);
		// From q_type.
		r = r_type{q_type{6},q_type{-15}};
		BOOST_CHECK_EQUAL(r.num(),-2);
		BOOST_CHECK_EQUAL(r.den(),5);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		r = r_type{q_type{6},q_type{1}};
		BOOST_CHECK_EQUAL(r.num(),6);
		BOOST_CHECK_EQUAL(r.den(),1);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		r = r_type{q_type{},xq+yq};
		BOOST_CHECK_EQUAL(r.num(),0);
		BOOST_CHECK_EQUAL(r.den(),1);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		r = r_type{(xq/3+3*xq*yq/4)*(xq*xq+yq*yq),xq.pow(3)*(4*xq+9*xq*yq)*(xq-yq)/2};
		// NOTE: k_monomial orders in revlex order.
		if (std::is_same<Key,k_monomial>::value) {
			BOOST_CHECK_EQUAL(r.num(),-(x*x+y*y));
			BOOST_CHECK_EQUAL(r.den(),-(6*x.pow(3)*(x-y)));
		} else {
			BOOST_CHECK_EQUAL(r.num(),x*x+y*y);
			BOOST_CHECK_EQUAL(r.den(),6*x.pow(3)*(x-y));
		}
		BOOST_CHECK(r.num().get_symbol_set().size() == 2u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 2u);
		BOOST_CHECK_THROW((r = r_type{q_type{1},q_type{0}}),zero_division_error);
		BOOST_CHECK_THROW((r = r_type{xq.pow(-1),xq}),std::invalid_argument);
		BOOST_CHECK_THROW((r = r_type{xq,xq.pow(-1)}),std::invalid_argument);
		// Some mixed binary ctor.
		r = r_type{q_type{6},-15};
		BOOST_CHECK_EQUAL(r.num(),-2);
		BOOST_CHECK_EQUAL(r.den(),5);
		r = r_type{r_type{6},-15/2_q};
		BOOST_CHECK_EQUAL(r.num(),-4);
		BOOST_CHECK_EQUAL(r.den(),5);
		r = r_type{1_z,-15/2_q};
		BOOST_CHECK_EQUAL(r.num(),-2);
		BOOST_CHECK_EQUAL(r.den(),15);
		r = r_type{1_q,x+3*y};
		BOOST_CHECK_EQUAL(r.num(),1);
		BOOST_CHECK_EQUAL(r.den(),x+3*y);
		r = r_type{1_q,r_type{1,x+3*y}};
		BOOST_CHECK_EQUAL(r.num(),x+3*y);
		BOOST_CHECK_EQUAL(r.den(),1);
		r = r_type{r_type{1,x+3*y},x*2};
		BOOST_CHECK_EQUAL(r.num(),1);
		BOOST_CHECK_EQUAL(r.den(),2*x*(x+3*y));
		r = r_type{-x,"x"};
		BOOST_CHECK_EQUAL(r.num(),-1);
		BOOST_CHECK_EQUAL(r.den(),1);
		r = r_type{"y","x"};
		BOOST_CHECK_EQUAL(r.num(),y);
		BOOST_CHECK_EQUAL(r.den(),x);
		BOOST_CHECK_THROW((r = r_type{r_type{1,x+3*y},0}),zero_division_error);
		BOOST_CHECK_THROW((r = r_type{r_type{1,x+3*y},q_type{}}),zero_division_error);
		// Some assignment checks.
		BOOST_CHECK((std::is_same<decltype(s = s),r_type &>::value));
		BOOST_CHECK((std::is_same<decltype(s = std::move(s)),r_type &>::value));
		// Self assignment.
		s = s;
		BOOST_CHECK_EQUAL(s.num(),5);
		BOOST_CHECK_EQUAL(s.den(),1);
		BOOST_CHECK(s.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(s.den().get_symbol_set().size() == 0u);
		s = std::move(s);
		BOOST_CHECK_EQUAL(s.num(),5);
		BOOST_CHECK_EQUAL(s.den(),1);
		BOOST_CHECK(s.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(s.den().get_symbol_set().size() == 0u);
		// Generic assignments.
		BOOST_CHECK((std::is_assignable<r_type &,p_type>::value));
		BOOST_CHECK((std::is_assignable<r_type &,p_type &>::value));
		BOOST_CHECK((std::is_assignable<r_type &,p_type &&>::value));
		BOOST_CHECK((std::is_assignable<r_type &,q_type>::value));
		BOOST_CHECK((std::is_assignable<r_type &,q_type &>::value));
		BOOST_CHECK((std::is_assignable<r_type &,q_type &&>::value));
		BOOST_CHECK((std::is_assignable<r_type &,int>::value));
		BOOST_CHECK((std::is_assignable<r_type &,char &>::value));
		BOOST_CHECK((std::is_assignable<r_type &,const integer &>::value));
		BOOST_CHECK((std::is_assignable<r_type &,rational>::value));
		BOOST_CHECK((std::is_assignable<r_type &,const rational &>::value));
		BOOST_CHECK((std::is_assignable<r_type &,std::string>::value));
		BOOST_CHECK((std::is_assignable<r_type &,char *>::value));
		BOOST_CHECK((std::is_assignable<r_type &,const char *>::value));
		BOOST_CHECK((!std::is_assignable<r_type &,double>::value));
		BOOST_CHECK((!std::is_assignable<r_type &,const float>::value));
		BOOST_CHECK((!std::is_assignable<r_type &,real &&>::value));
		BOOST_CHECK((std::is_same<decltype(s = 0),r_type &>::value));
		BOOST_CHECK((std::is_same<decltype(s = xq),r_type &>::value));
		BOOST_CHECK((std::is_same<decltype(s = std::move(x)),r_type &>::value));
		s = 0;
		BOOST_CHECK_EQUAL(s.num(),0);
		BOOST_CHECK_EQUAL(s.den(),1);
		BOOST_CHECK(s.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(s.den().get_symbol_set().size() == 0u);
		s = 1_z;
		BOOST_CHECK_EQUAL(s.num(),1);
		BOOST_CHECK_EQUAL(s.den(),1);
		BOOST_CHECK(s.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(s.den().get_symbol_set().size() == 0u);
		s = x+y;
		BOOST_CHECK_EQUAL(s.num(),x+y);
		BOOST_CHECK_EQUAL(s.den(),1);
		BOOST_CHECK(s.num().get_symbol_set().size() == 2u);
		BOOST_CHECK(s.den().get_symbol_set().size() == 0u);
		s = -3/6_q;
		BOOST_CHECK_EQUAL(s.num(),-1);
		BOOST_CHECK_EQUAL(s.den(),2);
		BOOST_CHECK(s.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(s.den().get_symbol_set().size() == 0u);
		s = xq - zq;
		BOOST_CHECK_EQUAL(s.num(),-z+x);
		BOOST_CHECK_EQUAL(s.den(),1);
		BOOST_CHECK(s.num().get_symbol_set().size() == 2u);
		BOOST_CHECK(s.den().get_symbol_set().size() == 0u);
		// A check to trigger a code path in canonicalise() when the den is unitary.
		s = 0;
		s._num() = -10;
		s._den() = 1;
		s.canonicalise();
		BOOST_CHECK(s.is_canonical());
		BOOST_CHECK_EQUAL(s,-10);
	}
};

BOOST_AUTO_TEST_CASE(rational_function_ctor_test)
{
	environment env;
	boost::mpl::for_each<key_types>(constructor_tester());
}

struct stream_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using r_type = rational_function<Key>;
		using p_type = typename r_type::p_type;
		auto str_cmp = [](const r_type &x, const std::string &cmp) {
			std::ostringstream oss;
			oss << x;
			BOOST_CHECK_EQUAL(oss.str(),cmp);
		};
		r_type r;
		str_cmp(r,"0");
		r = -123;
		str_cmp(r,"-123");
		r = -123/7_q;
		str_cmp(r,"-123/7");
		p_type x{"x"}, y{"y"}, z{"z"};
		r = -123/7_q + x;
		str_cmp(r,"(-123+7*x)/7");
		r = r_type{-123 + x,x+1};
		str_cmp(r,"(-123+x)/(1+x)");
		r = r_type{-123 + x,2*x};
		str_cmp(r,"(-123+x)/(2*x)");
		r = r_type{-123 + x,-x};
		str_cmp(r,"(123-x)/x");
		r = r_type{x,y};
		str_cmp(r,"x/y");
		// This was printed incorrectly (without brackets in den) in a previous version.
		r = r_type{y,x*z};
		str_cmp(r,"y/(x*z)");
	}
};

BOOST_AUTO_TEST_CASE(rational_function_stream_test)
{
	boost::mpl::for_each<key_types>(stream_tester());
}

struct canonical_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using r_type = rational_function<Key>;
		r_type r;
		r._num() = 0;
		r._den() = 2;
		BOOST_CHECK(!r.is_canonical());
		r._num() = 0;
		r._den() = -1;
		BOOST_CHECK(!r.is_canonical());
		r._num() = 2;
		r._den() = 2;
		BOOST_CHECK(!r.is_canonical());
		r._den() = 0;
		BOOST_CHECK(!r.is_canonical());
		r._den() = -1;
		BOOST_CHECK(!r.is_canonical());
	}
};

BOOST_AUTO_TEST_CASE(rational_function_canonical_test)
{
	boost::mpl::for_each<key_types>(canonical_tester());
}

struct add_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using r_type = rational_function<Key>;
		using p_type = typename r_type::p_type;
		using q_type = typename r_type::q_type;
		BOOST_CHECK(is_addable<r_type>::value);
		BOOST_CHECK((is_addable<r_type,int>::value));
		BOOST_CHECK((is_addable<int,r_type>::value));
		BOOST_CHECK((is_addable<r_type,integer>::value));
		BOOST_CHECK((is_addable<integer,r_type>::value));
		BOOST_CHECK((is_addable<r_type,rational>::value));
		BOOST_CHECK((is_addable<rational,r_type>::value));
		BOOST_CHECK((is_addable<r_type,p_type>::value));
		BOOST_CHECK((is_addable<p_type,r_type>::value));
		BOOST_CHECK((is_addable<r_type,q_type>::value));
		BOOST_CHECK((is_addable<q_type,r_type>::value));
		BOOST_CHECK(is_addable_in_place<r_type>::value);
		BOOST_CHECK((is_addable_in_place<r_type,int>::value));
		BOOST_CHECK((is_addable_in_place<r_type,integer>::value));
		BOOST_CHECK((is_addable_in_place<r_type,rational>::value));
		BOOST_CHECK((is_addable_in_place<r_type,p_type>::value));
		BOOST_CHECK((is_addable_in_place<r_type,q_type>::value));
		BOOST_CHECK((!is_addable<r_type,double>::value));
		BOOST_CHECK((!is_addable<long double,r_type>::value));
		BOOST_CHECK((!is_addable_in_place<r_type,double>::value));
		BOOST_CHECK((!is_addable_in_place<r_type,float>::value));
		BOOST_CHECK((std::is_same<decltype(r_type{} + r_type{}),r_type>::value));
		BOOST_CHECK((std::is_same<decltype(r_type{} + 1_z),r_type>::value));
		BOOST_CHECK((std::is_same<decltype(1_q + r_type{}),r_type>::value));
		// This is mostly for checking that we are picking the overridden operators.
		BOOST_CHECK((std::is_same<decltype(r_type{} + p_type{}),r_type>::value));
		BOOST_CHECK((std::is_same<decltype(q_type{} + r_type{}),r_type>::value));
		p_type x{"x"}, y{"y"}, z{"z"};
		auto checker = [](const r_type &a, const r_type &b) {
			BOOST_CHECK_EQUAL(a,b);
			BOOST_CHECK(a.is_canonical());
		};
		checker(r_type{} + r_type{},r_type{});
		checker(r_type{} + r_type{x,y},r_type{x,y});
		checker(r_type{x,y} + r_type{},r_type{x,y});
		checker(r_type{x,y} + 2,r_type{x+2*y,y});
		checker(1_z + r_type{x,y},r_type{x+y,y});
		checker(1/3_q + r_type{x,y},r_type{3*x+y,3*y});
		checker(r_type{2*x,y} + r_type{y,x},r_type{2*x*x+y*y,y*x});
		checker(r_type{x,y+x} + x,r_type{x+x*x+x*y,x+y});
		checker(x + r_type{x,y+x},r_type{x+x*x+x*y,x+y});
		checker(q_type{"x"}/2 + r_type{x,y+x},r_type{2*x+x*x+x*y,2*(x+y)});
		checker(r_type{x,y+x} + q_type{"x"}/2,r_type{2*x+x*x+x*y,2*(x+y)});
		// Random testing.
		std::uniform_int_distribution<int> dist(0,4);
		for (int i = 0; i < ntrials; ++i) {
			auto n1 = rn_poly(x,y,z,dist);
			auto d1 = rn_poly(x,y,z,dist);
			if (math::is_zero(d1)) {
				BOOST_CHECK_THROW((r_type{n1,d1}),zero_division_error);
				continue;
			}
			auto n2 = rn_poly(x,y,z,dist);
			auto d2 = rn_poly(x,y,z,dist);
			if (math::is_zero(d2)) {
				BOOST_CHECK_THROW((r_type{n2,d2}),zero_division_error);
				continue;
			}
			r_type r1{n1,d1}, r2{n2,d2};
			auto add = r1 + r2;
			BOOST_CHECK(add.is_canonical());
			auto check = add - r1;
			BOOST_CHECK(check.is_canonical());
			BOOST_CHECK_EQUAL(check,r2);
			check = add - r2;
			BOOST_CHECK(check.is_canonical());
			BOOST_CHECK_EQUAL(check,r1);
			// Vs interop.
			BOOST_CHECK_EQUAL(-1 + r1 + 1,r1);
			BOOST_CHECK_EQUAL(-1_z + r1 + 1_z,r1);
			BOOST_CHECK_EQUAL(-1/2_q + r1 + 1/2_q,r1);
			BOOST_CHECK_EQUAL(-n2 + r1 + n2,r1);
			BOOST_CHECK_EQUAL(q_type{-n2}/2 + r1 + q_type{n2}/2,r1);
			// Check the in-place version.
			r1 += r2;
			BOOST_CHECK_EQUAL(add,r1);
			r1 += 1/2_q;
			BOOST_CHECK_EQUAL(add + 1/2_q,r1);
			r1 += 1;
			BOOST_CHECK_EQUAL(add + 1/2_q + 1,r1);
			r1 += n2;
			BOOST_CHECK_EQUAL(add + 1/2_q + 1 + n2,r1);
			r1 += q_type{n2}/3;
			BOOST_CHECK_EQUAL(add + 1/2_q + 1 + n2 + q_type{n2}/3,r1);
		}
		// Identity operator.
		BOOST_CHECK_EQUAL((+r_type{2*x*x+y*y,y*x}),(r_type{2*x*x+y*y,y*x}));
	}
};

BOOST_AUTO_TEST_CASE(rational_function_add_test)
{
	boost::mpl::for_each<key_types>(add_tester());
}

struct sub_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using r_type = rational_function<Key>;
		using p_type = typename r_type::p_type;
		using q_type = typename r_type::q_type;
		BOOST_CHECK(is_subtractable<r_type>::value);
		BOOST_CHECK((is_subtractable<r_type,int>::value));
		BOOST_CHECK((is_subtractable<int,r_type>::value));
		BOOST_CHECK((is_subtractable<r_type,integer>::value));
		BOOST_CHECK((is_subtractable<integer,r_type>::value));
		BOOST_CHECK((is_subtractable<r_type,rational>::value));
		BOOST_CHECK((is_subtractable<rational,r_type>::value));
		BOOST_CHECK((is_subtractable<r_type,p_type>::value));
		BOOST_CHECK((is_subtractable<p_type,r_type>::value));
		BOOST_CHECK((is_subtractable<r_type,q_type>::value));
		BOOST_CHECK((is_subtractable<q_type,r_type>::value));
		BOOST_CHECK(is_subtractable_in_place<r_type>::value);
		BOOST_CHECK((is_subtractable_in_place<r_type,int>::value));
		BOOST_CHECK((is_subtractable_in_place<r_type,integer>::value));
		BOOST_CHECK((is_subtractable_in_place<r_type,rational>::value));
		BOOST_CHECK((is_subtractable_in_place<r_type,p_type>::value));
		BOOST_CHECK((is_subtractable_in_place<r_type,q_type>::value));
		BOOST_CHECK((!is_subtractable<r_type,double>::value));
		BOOST_CHECK((!is_subtractable<long double,r_type>::value));
		BOOST_CHECK((!is_subtractable_in_place<r_type,double>::value));
		BOOST_CHECK((!is_subtractable_in_place<r_type,float>::value));
		BOOST_CHECK((std::is_same<decltype(r_type{} - r_type{}),r_type>::value));
		BOOST_CHECK((std::is_same<decltype(r_type{} - 1_z),r_type>::value));
		BOOST_CHECK((std::is_same<decltype(1_q - r_type{}),r_type>::value));
		BOOST_CHECK((std::is_same<decltype(r_type{} - p_type{}),r_type>::value));
		BOOST_CHECK((std::is_same<decltype(q_type{} - r_type{}),r_type>::value));
		p_type x{"x"}, y{"y"}, z{"z"};
		auto checker = [](const r_type &a, const r_type &b) {
			BOOST_CHECK_EQUAL(a,b);
			BOOST_CHECK(a.is_canonical());
		};
		checker(r_type{} - r_type{},r_type{});
		checker(r_type{} - r_type{x,y},-r_type{x,y});
		checker(r_type{x,y} - r_type{},r_type{x,y});
		checker(r_type{x,y} - 2,r_type{x-2*y,y});
		checker(1_z - r_type{x,y},r_type{-x+y,y});
		checker(1/3_q - r_type{x,y},r_type{y-3*x,3*y});
		checker(r_type{2*x,y} - r_type{y,x},r_type{2*x*x-y*y,y*x});
		checker(r_type{x,y+x} - x,r_type{x-x*x-x*y,x+y});
		checker(x - r_type{x,y+x},r_type{-x+x*x+x*y,x+y});
		checker(q_type{"x"}/2 - r_type{x,y+x},r_type{-2*x+x*x+x*y,2*(x+y)});
		checker(r_type{x,y+x} - q_type{"x"}/2,r_type{2*x-x*x-x*y,2*(x+y)});
		// Random testing.
		std::uniform_int_distribution<int> dist(0,4);
		for (int i = 0; i < ntrials; ++i) {
			auto n1 = rn_poly(x,y,z,dist);
			auto d1 = rn_poly(x,y,z,dist);
			if (math::is_zero(d1)) {
				BOOST_CHECK_THROW((r_type{n1,d1}),zero_division_error);
				continue;
			}
			auto n2 = rn_poly(x,y,z,dist);
			auto d2 = rn_poly(x,y,z,dist);
			if (math::is_zero(d2)) {
				BOOST_CHECK_THROW((r_type{n2,d2}),zero_division_error);
				continue;
			}
			r_type r1{n1,d1}, r2{n2,d2};
			auto sub = r1 - r2;
			BOOST_CHECK(sub.is_canonical());
			auto check = sub - r1;
			BOOST_CHECK(check.is_canonical());
			BOOST_CHECK_EQUAL(check,-r2);
			check = -sub - r2;
			BOOST_CHECK(check.is_canonical());
			BOOST_CHECK_EQUAL(check,-r1);
			// Vs interop.
			BOOST_CHECK_EQUAL(1 - r1 - 1,-r1);
			BOOST_CHECK_EQUAL(1_z - r1 - 1_z,-r1);
			BOOST_CHECK_EQUAL(1/2_q - r1 - 1/2_q,-r1);
			BOOST_CHECK_EQUAL(n2 - r1 - n2,-r1);
			BOOST_CHECK_EQUAL(q_type{n2}/2 - r1 - q_type{n2}/2,-r1);
			// Check the in-place version.
			r1 -= r2;
			BOOST_CHECK_EQUAL(sub,r1);
			r1 -= 1/2_q;
			BOOST_CHECK_EQUAL(sub - 1/2_q,r1);
			r1 -= 1;
			BOOST_CHECK_EQUAL(sub - 1/2_q - 1,r1);
			r1 -= n2;
			BOOST_CHECK_EQUAL(sub - 1/2_q - 1 - n2,r1);
			r1 -= q_type{n2}/3;
			BOOST_CHECK_EQUAL(sub - 1/2_q - 1 - n2 - q_type{n2}/3,r1);
		}
		// Identity operator.
		BOOST_CHECK_EQUAL((-r_type{2*x*x+y*y,y*x}),(r_type{-2*x*x-y*y,y*x}));
	}
};

BOOST_AUTO_TEST_CASE(rational_function_sub_test)
{
	boost::mpl::for_each<key_types>(sub_tester());
}

struct mul_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using r_type = rational_function<Key>;
		using p_type = typename r_type::p_type;
		using q_type = typename r_type::q_type;
		BOOST_CHECK(is_multipliable<r_type>::value);
		BOOST_CHECK((is_multipliable<r_type,int>::value));
		BOOST_CHECK((is_multipliable<int,r_type>::value));
		BOOST_CHECK((is_multipliable<r_type,integer>::value));
		BOOST_CHECK((is_multipliable<integer,r_type>::value));
		BOOST_CHECK((is_multipliable<r_type,rational>::value));
		BOOST_CHECK((is_multipliable<rational,r_type>::value));
		BOOST_CHECK((is_multipliable<r_type,p_type>::value));
		BOOST_CHECK((is_multipliable<p_type,r_type>::value));
		BOOST_CHECK((is_multipliable<r_type,q_type>::value));
		BOOST_CHECK((is_multipliable<q_type,r_type>::value));
		BOOST_CHECK(is_multipliable_in_place<r_type>::value);
		BOOST_CHECK((is_multipliable_in_place<r_type,int>::value));
		BOOST_CHECK((is_multipliable_in_place<r_type,integer>::value));
		BOOST_CHECK((is_multipliable_in_place<r_type,rational>::value));
		BOOST_CHECK((is_multipliable_in_place<r_type,p_type>::value));
		BOOST_CHECK((is_multipliable_in_place<r_type,q_type>::value));
		BOOST_CHECK((!is_multipliable<r_type,double>::value));
		BOOST_CHECK((!is_multipliable<long double,r_type>::value));
		BOOST_CHECK((!is_multipliable_in_place<r_type,double>::value));
		BOOST_CHECK((!is_multipliable_in_place<r_type,float>::value));
		BOOST_CHECK((std::is_same<decltype(r_type{} * r_type{}),r_type>::value));
		BOOST_CHECK((std::is_same<decltype(r_type{} * 1_z),r_type>::value));
		BOOST_CHECK((std::is_same<decltype(1_q * r_type{}),r_type>::value));
		BOOST_CHECK((std::is_same<decltype(r_type{} * p_type{}),r_type>::value));
		BOOST_CHECK((std::is_same<decltype(q_type{} * r_type{}),r_type>::value));
		p_type x{"x"}, y{"y"}, z{"z"};
		auto checker = [](const r_type &a, const r_type &b) {
			BOOST_CHECK_EQUAL(a,b);
			BOOST_CHECK(a.is_canonical());
		};
		checker(r_type{} * r_type{},r_type{});
		checker(r_type{} * r_type{x,y},r_type{});
		checker(r_type{x,y} * r_type{},r_type{});
		checker(r_type{1} * r_type{x,y},r_type{x,y});
		checker(r_type{x,y} * r_type{1},r_type{x,y});
		checker(r_type{x,y} * 2,r_type{2*x,y});
		checker(2_z * r_type{x,y},r_type{2*x,y});
		checker(1/3_q * r_type{x,y},r_type{x,3*y});
		checker(r_type{2*x,y} * r_type{y,x},r_type{2});
		checker(r_type{x,y+x} * x,r_type{x*x,x+y});
		checker(x * r_type{x,y+x},r_type{x*x,x+y});
		checker((q_type{"x"}/2) * r_type{x,y+x},r_type{x*x,2*(x+y)});
		checker(r_type{x,y+x} * (q_type{"x"}/2),r_type{x*x,2*(x+y)});
		// Random testing.
		std::uniform_int_distribution<int> dist(0,4);
		for (int i = 0; i < ntrials; ++i) {
			auto n1 = rn_poly(x,y,z,dist);
			auto d1 = rn_poly(x,y,z,dist);
			if (math::is_zero(d1)) {
				BOOST_CHECK_THROW((r_type{n1,d1}),zero_division_error);
				continue;
			}
			auto n2 = rn_poly(x,y,z,dist);
			auto d2 = rn_poly(x,y,z,dist);
			if (math::is_zero(d2)) {
				BOOST_CHECK_THROW((r_type{n2,d2}),zero_division_error);
				continue;
			}
			r_type r1{n1,d1}, r2{n2,d2};
			auto mul = r1 * r2;
			BOOST_CHECK(mul.is_canonical());
			r_type check;
			if (math::is_zero(r1)) {
				BOOST_CHECK_THROW(mul / r1,zero_division_error);
			} else {
				check = mul / r1;
				BOOST_CHECK(check.is_canonical());
				BOOST_CHECK_EQUAL(check,r2);
			}
			if (math::is_zero(r2)) {
				BOOST_CHECK_THROW(mul / r2,zero_division_error);
			} else {
				check = mul / r2;
				BOOST_CHECK(check.is_canonical());
				BOOST_CHECK_EQUAL(check,r1);
			}
			// Vs interop.
			BOOST_CHECK_EQUAL((r1 * 2) / 2,r1);
			BOOST_CHECK_EQUAL((r1 * 2_z) / 2_z,r1);
			BOOST_CHECK_EQUAL((r1 * (1/2_q)) / (1/2_q),r1);
			if (math::is_zero(n2)) {
				BOOST_CHECK_THROW((r1 * n2) / n2,zero_division_error);
			} else {
				BOOST_CHECK_EQUAL((r1 * n2) / n2,r1);
				BOOST_CHECK_EQUAL((q_type{n2}/2 * r1) / (q_type{n2}/2),r1);
			}
			// Check the in-place version.
			r1 *= r2;
			BOOST_CHECK_EQUAL(mul,r1);
			r1 *= 1/2_q;
			BOOST_CHECK_EQUAL(mul * 1/2_q,r1);
			r1 *= 1;
			BOOST_CHECK_EQUAL(mul * 1/2_q,r1);
			r1 *= n2;
			BOOST_CHECK_EQUAL(mul * 1/2_q * n2,r1);
			r1 *= q_type{n2}/3;
			BOOST_CHECK_EQUAL(mul * 1/2_q * n2 * q_type{n2}/3,r1);
		}
	}
};

BOOST_AUTO_TEST_CASE(rational_function_mul_test)
{
	boost::mpl::for_each<key_types>(mul_tester());
}

struct div_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using r_type = rational_function<Key>;
		using p_type = typename r_type::p_type;
		using q_type = typename r_type::q_type;
		BOOST_CHECK(is_divisible<r_type>::value);
		BOOST_CHECK((is_divisible<r_type,int>::value));
		BOOST_CHECK((is_divisible<int,r_type>::value));
		BOOST_CHECK((is_divisible<r_type,integer>::value));
		BOOST_CHECK((is_divisible<integer,r_type>::value));
		BOOST_CHECK((is_divisible<r_type,rational>::value));
		BOOST_CHECK((is_divisible<rational,r_type>::value));
		BOOST_CHECK((is_divisible<r_type,p_type>::value));
		BOOST_CHECK((is_divisible<p_type,r_type>::value));
		BOOST_CHECK((is_divisible<r_type,q_type>::value));
		BOOST_CHECK((is_divisible<q_type,r_type>::value));
		BOOST_CHECK(is_divisible_in_place<r_type>::value);
		BOOST_CHECK((is_divisible_in_place<r_type,int>::value));
		BOOST_CHECK((is_divisible_in_place<r_type,integer>::value));
		BOOST_CHECK((is_divisible_in_place<r_type,rational>::value));
		BOOST_CHECK((is_divisible_in_place<r_type,p_type>::value));
		BOOST_CHECK((is_divisible_in_place<r_type,q_type>::value));
		BOOST_CHECK((!is_divisible<r_type,double>::value));
		BOOST_CHECK((!is_divisible<long double,r_type>::value));
		BOOST_CHECK((!is_divisible_in_place<r_type,double>::value));
		BOOST_CHECK((!is_divisible_in_place<r_type,float>::value));
		BOOST_CHECK((std::is_same<decltype(r_type{} / r_type{}),r_type>::value));
		BOOST_CHECK((std::is_same<decltype(r_type{} / 1_z),r_type>::value));
		BOOST_CHECK((std::is_same<decltype(1_q / r_type{}),r_type>::value));
		BOOST_CHECK((std::is_same<decltype(r_type{} / p_type{}),r_type>::value));
		BOOST_CHECK((std::is_same<decltype(q_type{} / r_type{}),r_type>::value));
		p_type x{"x"}, y{"y"}, z{"z"};
		auto checker = [](const r_type &a, const r_type &b) {
			BOOST_CHECK_EQUAL(a,b);
			BOOST_CHECK(a.is_canonical());
		};
		checker(r_type{1} / r_type{1},r_type{1});
		checker(r_type{1} / r_type{x,y},r_type{y,x});
		checker(r_type{x,y} / r_type{1},r_type{x,y});
		checker(r_type{x,y} / 2,r_type{x,2*y});
		checker(2_z / r_type{x,y},r_type{2*y,x});
		checker(1/3_q / r_type{x,y},r_type{y,3*x});
		checker(r_type{2*x,y} / r_type{y,x},r_type{2*x*x,y*y});
		checker(r_type{x,y+x} / x,r_type{p_type{1},x+y});
		checker(x / r_type{x,y+x},r_type{y+x,p_type{1}});
		checker((q_type{"x"}/2) / r_type{x,y+x},r_type{y+x,p_type{2}});
		checker(r_type{x,y+x} / (q_type{"x"}/2),r_type{p_type{2},(x+y)});
		// Random testing.
		std::uniform_int_distribution<int> dist(0,4);
		for (int i = 0; i < ntrials; ++i) {
			auto n1 = rn_poly(x,y,z,dist);
			auto d1 = rn_poly(x,y,z,dist);
			if (math::is_zero(d1)) {
				BOOST_CHECK_THROW((r_type{n1,d1}),zero_division_error);
				continue;
			}
			auto n2 = rn_poly(x,y,z,dist);
			auto d2 = rn_poly(x,y,z,dist);
			if (math::is_zero(d2)) {
				BOOST_CHECK_THROW((r_type{n2,d2}),zero_division_error);
				continue;
			}
			r_type r1{n1,d1}, r2{n2,d2};
			if (math::is_zero(r2)) {
				continue;
			}
			auto div = r1 / r2;
			BOOST_CHECK(div.is_canonical());
			auto check = div * r2;
			BOOST_CHECK(check.is_canonical());
			BOOST_CHECK_EQUAL(check,r1);
			// Vs interop.
			BOOST_CHECK_EQUAL((r1 / 2) * 2,r1);
			BOOST_CHECK_EQUAL((r1 / 2_z) * 2_z,r1);
			BOOST_CHECK_EQUAL((r1 / (1/2_q)) * (1/2_q),r1);
			if (math::is_zero(n2)) {
				BOOST_CHECK_THROW((r1 / n2) * n2,zero_division_error);
			} else {
				BOOST_CHECK_EQUAL((r1 / n2) * n2,r1);
				BOOST_CHECK_EQUAL((q_type{n2}/2 * r2) / (q_type{n2}/2),r2);
			}
			// Check the in-place version.
			r1 /= r2;
			BOOST_CHECK_EQUAL(div,r1);
			r1 /= 1/2_q;
			BOOST_CHECK_EQUAL(div / (1/2_q),r1);
			r1 /= 1;
			BOOST_CHECK_EQUAL(div / (1/2_q),r1);
			if (math::is_zero(n2)) {
				continue;
			}
			r1 /= n2;
			BOOST_CHECK_EQUAL((div / (1/2_q)) / n2,r1);
			r1 /= q_type{n2}/3;
			BOOST_CHECK_EQUAL(((div / (1/2_q)) / n2) / (q_type{n2}/3),r1);
		}
	}
};

BOOST_AUTO_TEST_CASE(rational_function_div_test)
{
	boost::mpl::for_each<key_types>(div_tester());
}

struct is_zero_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using r_type = rational_function<Key>;
		BOOST_CHECK((has_is_zero<r_type>::value));
		BOOST_CHECK(math::is_zero(r_type{}));
		BOOST_CHECK(math::is_zero(r_type{0,1}));
		BOOST_CHECK(math::is_zero(r_type{0,-123}));
		BOOST_CHECK(!math::is_zero(r_type{1,-1}));
	}
};

BOOST_AUTO_TEST_CASE(rational_function_is_zero_test)
{
	boost::mpl::for_each<key_types>(is_zero_tester());
}

struct comparison_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using r_type = rational_function<Key>;
		using p_type = typename r_type::p_type;
		using q_type = typename r_type::q_type;
		p_type x{"x"}, y{"y"}, z{"z"};
		BOOST_CHECK((is_equality_comparable<r_type>::value));
		BOOST_CHECK((is_equality_comparable<r_type,p_type>::value));
		BOOST_CHECK((is_equality_comparable<p_type,r_type>::value));
		BOOST_CHECK((is_equality_comparable<r_type,q_type>::value));
		BOOST_CHECK((is_equality_comparable<q_type,r_type>::value));
		BOOST_CHECK((is_equality_comparable<r_type,int>::value));
		BOOST_CHECK((is_equality_comparable<integer,r_type>::value));
		BOOST_CHECK((is_equality_comparable<r_type,rational>::value));
		BOOST_CHECK((!is_equality_comparable<r_type,double>::value));
		BOOST_CHECK((!is_equality_comparable<std::string,r_type>::value));
		BOOST_CHECK_EQUAL(r_type{0},p_type{});
		BOOST_CHECK_EQUAL(p_type{0},r_type{});
		BOOST_CHECK_EQUAL(r_type{},q_type{0});
		BOOST_CHECK_EQUAL(q_type{},r_type{});
		BOOST_CHECK_EQUAL(r_type{1},1);
		BOOST_CHECK_EQUAL(1_z,r_type{1});
		BOOST_CHECK_EQUAL((r_type{1,2}),1/2_q);
		BOOST_CHECK_EQUAL((r_type{(x+y+z)*2,p_type{2}}),x+y+z);
		BOOST_CHECK((r_type{x,y} != r_type{1}));
		BOOST_CHECK((r_type{x,y} != 1/2_q));
		BOOST_CHECK((-6 != r_type{x,p_type{2}}));
		BOOST_CHECK((r_type{x,y} != q_type{x} / 2));
		BOOST_CHECK((p_type{x} != r_type{x,p_type{2}}));
	}
};

BOOST_AUTO_TEST_CASE(rational_function_comparison_test)
{
	boost::mpl::for_each<key_types>(comparison_tester());
}

struct pow_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using math::pow;
		using r_type = rational_function<Key>;
		using p_type = typename r_type::p_type;
		{
		r_type x{"x"}, y{"y"}, z{"z"};
		BOOST_CHECK((is_exponentiable<r_type,int>::value));
		BOOST_CHECK((is_exponentiable<r_type,integer>::value));
		BOOST_CHECK((is_exponentiable<r_type,long long>::value));
		BOOST_CHECK((!is_exponentiable<r_type,double>::value));
		BOOST_CHECK((!is_exponentiable<r_type,rational>::value));
		BOOST_CHECK((!is_exponentiable<r_type,r_type>::value));
		BOOST_CHECK_EQUAL(pow(x/y,char(2)),x*x/(y*y));
		BOOST_CHECK_EQUAL(pow(x/y,0_z),1);
		BOOST_CHECK_EQUAL(pow(r_type{},0_z),1);
		BOOST_CHECK_EQUAL(pow(x/y,-2),y*y/(x*x));
		BOOST_CHECK_THROW((pow(r_type{},-1)),zero_division_error);
		}
		// Random testing.
		p_type x{"x"}, y{"y"}, z{"z"};
		std::uniform_int_distribution<int> dist(0,4), p_dist(-4,4), clear_dist(0,9);
		for (int i = 0; i < ntrials; ++i) {
			auto n1 = rn_poly(x,y,z,dist);
			auto d1 = rn_poly(x,y,z,dist);
			if (math::is_zero(d1)) {
				BOOST_CHECK_THROW((r_type{n1,d1}),zero_division_error);
				continue;
			}
			r_type r1{n1,d1};
			auto expo = p_dist(rng);
			if (expo == 0) {
				BOOST_CHECK_EQUAL(pow(r1,expo),1);
			} else if (expo > 0) {
				auto p = pow(r1,expo);
				BOOST_CHECK(p.is_canonical());
				r_type acc{1};
				for (auto j = 0; j < expo; ++j) {
					acc *= r1;
				}
				BOOST_CHECK_EQUAL(acc,p);
			} else if (!math::is_zero(r1)) {
				auto p = pow(r1,expo);
				BOOST_CHECK(p.is_canonical());
				r_type acc{1};
				for (auto j = 0; j < -expo; ++j) {
					acc /= r1;
				}
				BOOST_CHECK_EQUAL(acc,p);
			}
			if (clear_dist(rng) == 0) {
				r_type::clear_pow_cache();
			}
		}
	}
};

BOOST_AUTO_TEST_CASE(rational_function_pow_test)
{
	boost::mpl::for_each<key_types>(pow_tester());
}

struct subs_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using math::pow;
		using r_type = rational_function<Key>;
		using p_type = typename r_type::p_type;
		using q_type = typename r_type::q_type;
		r_type x{"x"}, y{"y"}, z{"z"};
		BOOST_CHECK((has_subs<r_type,int>::value));
		BOOST_CHECK((has_subs<r_type,r_type>::value));
		BOOST_CHECK((has_subs<r_type,p_type>::value));
		BOOST_CHECK((has_subs<r_type,q_type>::value));
		BOOST_CHECK((has_subs<r_type,integer>::value));
		// More checks for these in rational_function_02.
		BOOST_CHECK((has_subs<r_type,double>::value));
		BOOST_CHECK((has_subs<r_type,float>::value));
		BOOST_CHECK((!has_subs<r_type,std::string>::value));
		BOOST_CHECK_EQUAL(x.subs("x",1),1);
		BOOST_CHECK((std::is_same<r_type,decltype(x.subs("x",1))>::value));
		BOOST_CHECK_THROW((1/x).subs("x",0),zero_division_error);
		BOOST_CHECK_EQUAL(math::subs((x+y)/z,"z",-x-y),-1);
		BOOST_CHECK((std::is_same<r_type,decltype(math::subs((x+y)/z,"z",-x-y))>::value));
		BOOST_CHECK_EQUAL(math::subs((x+y)/z,"x",123_z),(123+y)/z);
		BOOST_CHECK((std::is_same<r_type,decltype(math::subs((x+y)/z,"x",123_z))>::value));
		BOOST_CHECK_EQUAL(math::subs((x+y)/z,"x",3/2_q),(3+2*y)/(2*z));
		BOOST_CHECK((std::is_same<r_type,decltype(math::subs((x+y)/z,"x",3/2_q))>::value));
		BOOST_CHECK_EQUAL(math::subs((x+y)/z,"y",p_type("z")*3),(x+3*z)/z);
		BOOST_CHECK((std::is_same<r_type,decltype(math::subs((x+y)/z,"y",p_type("z")*3))>::value));
		BOOST_CHECK_EQUAL(math::subs((x+y)/z,"z",q_type("z")/6),6*(x+y)/z);
		BOOST_CHECK((std::is_same<r_type,decltype(math::subs((x+y)/z,"z",q_type("z")/6))>::value));
		BOOST_CHECK_EQUAL(math::subs((x+y)/z,"a",123_z),(x+y)/z);
		BOOST_CHECK_EQUAL(math::subs(x/(z+y),"x",0),0);
		// Check that using negative powers throws.
		BOOST_CHECK_THROW(math::subs(x/(z+y),"x",p_type{"x"}.pow(-1)),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(rational_function_subs_test)
{
	boost::mpl::for_each<key_types>(subs_tester());
}

struct serialization_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using r_type = rational_function<Key>;
		using p_type = typename r_type::p_type;
		auto checker = [](const r_type &r) -> void {
			r_type tmp;
			std::stringstream ss;
			{
			boost::archive::text_oarchive oa(ss);
			oa << r;
			}
			{
			boost::archive::text_iarchive ia(ss);
			ia >> tmp;
			}
			BOOST_CHECK_EQUAL(tmp,r);
		};
		// Random testing.
		p_type x{"x"}, y{"y"}, z{"z"};
		std::uniform_int_distribution<int> dist(0,4), p_dist(-4,4);
		for (int i = 0; i < ntrials; ++i) {
			auto n1 = rn_poly(x,y,z,dist);
			auto d1 = rn_poly(x,y,z,dist);
			if (math::is_zero(d1)) {
				BOOST_CHECK_THROW((r_type{n1,d1}),zero_division_error);
				continue;
			}
			r_type r1{n1,d1};
			checker(r1);
		}
	}
};

BOOST_AUTO_TEST_CASE(rational_function_serialization_test)
{
	boost::mpl::for_each<key_types>(serialization_tester());
}

struct ipow_subs_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using math::pow;
		using r_type = rational_function<Key>;
		using p_type = typename r_type::p_type;
		using q_type = typename r_type::q_type;
		r_type x{"x"}, y{"y"}, z{"z"};
		BOOST_CHECK((has_ipow_subs<r_type,int>::value));
		BOOST_CHECK((has_ipow_subs<r_type,r_type>::value));
		BOOST_CHECK((has_ipow_subs<r_type,p_type>::value));
		BOOST_CHECK((has_ipow_subs<r_type,q_type>::value));
		BOOST_CHECK((has_ipow_subs<r_type,integer>::value));
		BOOST_CHECK((!has_ipow_subs<r_type,std::string>::value));
		// More checks for these in rational_function_02.
		BOOST_CHECK((has_ipow_subs<r_type,double>::value));
		BOOST_CHECK((has_ipow_subs<r_type,float>::value));
		BOOST_CHECK_EQUAL(x.ipow_subs("x",1_z,y),y);
		BOOST_CHECK((std::is_same<r_type,decltype(x.ipow_subs("x",1_z,y))>::value));
		BOOST_CHECK_THROW((1/(x*x)).ipow_subs("x",2_z,0),zero_division_error);
		BOOST_CHECK((std::is_same<r_type,decltype((1/(x*x)).ipow_subs("x",2_z,0))>::value));
		BOOST_CHECK_EQUAL((1/(x*x)).ipow_subs("x",2_z,y),1/y);
		BOOST_CHECK_EQUAL(math::ipow_subs((x+y)/(z*z),"z",2,-x-y),-1);
		BOOST_CHECK_EQUAL(math::ipow_subs((x*x*x+y)/z,"x",2,123_z),(123*x+y)/z);
		BOOST_CHECK_EQUAL(math::ipow_subs((x+y)/z,"x",2,3/2_q),(x+y)/z);
		BOOST_CHECK((std::is_same<r_type,decltype(math::ipow_subs((x+y)/z,"x",2,3/2_q))>::value));
		BOOST_CHECK_EQUAL(math::ipow_subs((x+y*y*y*y)/z,"y",2,p_type("z")*3),(x+9*z*z)/z);
		BOOST_CHECK((std::is_same<r_type,decltype(math::ipow_subs((x+y*y*y*y)/z,"y",2,p_type("z")*3))>::value));
		BOOST_CHECK_EQUAL(math::ipow_subs((x+y)/(z*z),"z",2,q_type("z")/6),6*(x+y)/z);
		BOOST_CHECK((std::is_same<r_type,decltype(math::ipow_subs((x+y)/(z*z),"z",2,q_type("z")/6))>::value));
		BOOST_CHECK_EQUAL(math::ipow_subs((x+y)/z,"a",123,123_z),(x+y)/z);
		BOOST_CHECK_EQUAL(math::ipow_subs(x/(z+y),"x",1,0),0);
		BOOST_CHECK_EQUAL(math::ipow_subs(x/(z+y),"x",-1,0),x/(z+y));
		// Check that using negative powers throws.
		BOOST_CHECK_THROW(math::ipow_subs(x/(z+y),"x",1,p_type{"x"}.pow(-1)),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(rational_function_ipow_subs_test)
{
	boost::mpl::for_each<key_types>(ipow_subs_tester());
}

struct partial_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using math::pow;
		using math::partial;
		using r_type = rational_function<Key>;
		using p_type = typename r_type::p_type;
		{
		BOOST_CHECK(is_differentiable<r_type>::value);
		r_type x{"x"}, y{"y"};
		BOOST_CHECK_EQUAL(partial(r_type{3,4},"x"),0);
		BOOST_CHECK_EQUAL(partial(x,"x"),1);
		BOOST_CHECK_EQUAL(partial(x/y,"z"),0);
		BOOST_CHECK_EQUAL(partial(x/y,"x"),1/y);
		BOOST_CHECK_EQUAL(partial((4*x-2)/(x*x+1),"x"),(-4*x*x+4*x+4)/pow(x*x+1,2));
		}
		// Random testing.
		p_type x{"x"}, y{"y"}, z{"z"};
		std::uniform_int_distribution<int> dist(0,4), p_dist(-4,4);
		for (int i = 0; i < ntrials; ++i) {
			auto n1 = rn_poly(x,y,z,dist);
			auto d1 = rn_poly(x,y,z,dist);
			if (math::is_zero(d1)) {
				BOOST_CHECK_THROW((r_type{n1,d1}),zero_division_error);
				continue;
			}
			r_type r1{n1,d1};
			BOOST_CHECK_EQUAL(partial(r1,"x"),(r_type{partial(r1.num(),"x")*r1.den()-partial(r1.den(),"x")*r1.num(),
				r1.den()*r1.den()}));
		}
	}
};

BOOST_AUTO_TEST_CASE(rational_function_partial_test)
{
	boost::mpl::for_each<key_types>(partial_tester());
}

struct integrate_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using math::pow;
		using math::integrate;
		using r_type = rational_function<Key>;
		BOOST_CHECK(is_integrable<r_type>::value);
		r_type x{"x"}, y{"y"};
		BOOST_CHECK_EQUAL(integrate(r_type{},"x"),0);
		BOOST_CHECK_EQUAL(integrate(r_type{3,4},"x"),(r_type{3,4}*x));
		BOOST_CHECK_EQUAL(integrate(x,"x"),x*x/2);
		BOOST_CHECK_EQUAL(integrate(x,"y"),x*y);
		BOOST_CHECK_THROW(integrate(1/x,"x"),std::invalid_argument);
		BOOST_CHECK_EQUAL(integrate(1/x,"y"),y/x);
		BOOST_CHECK_EQUAL(integrate((7*x*x+y*x)/(2*y),"x"),(14*x*x*x+3*x*x*y)/(12*y));
	}
};

BOOST_AUTO_TEST_CASE(rational_function_integrate_test)
{
	boost::mpl::for_each<key_types>(integrate_tester());
}

template <typename T>
inline void tex_checker(const T &r, std::string cmp1, std::string cmp2 = "")
{
	std::ostringstream oss;
	r.print_tex(oss);
	auto oss_str = oss.str();
	oss.str("");
	print_tex_coefficient(oss,r);
	auto oss_str2 = oss.str();
	BOOST_CHECK_EQUAL(oss_str,oss_str2);
	if (cmp2 != oss_str) {
		BOOST_CHECK_EQUAL(cmp1,oss_str);
	}
}

struct print_tex_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using r_type = rational_function<Key>;
		tex_checker(r_type{},"0");
		r_type x{"x"}, y{"y"};
		tex_checker(x,"{x}");
		tex_checker(x*y,"{x}{y}");
		tex_checker(-x,"-{x}");
		tex_checker(-x*y,"-{x}{y}");
		tex_checker(x*y/2,"\\frac{{x}{y}}{2}");
		tex_checker(-x*y/2,"-\\frac{{x}{y}}{2}");
		tex_checker(-x*y/(x+2),"-\\frac{{x}{y}}{{x}+2}","-\\frac{{x}{y}}{2+{x}}");
		tex_checker(x*y/2,"\\frac{{x}{y}}{2}");
		tex_checker(x*y/(x+2),"\\frac{{x}{y}}{{x}+2}","\\frac{{x}{y}}{2+{x}}");
		tex_checker(x*y/(x-2),"\\frac{{x}{y}}{{x}-2}","-\\frac{{x}{y}}{2-{x}}");
		tex_checker((x-3*y)/x,"\\frac{{x}-3{y}}{{x}}","-\\frac{3{y}-{x}}{{x}}");
		tex_checker((x-2*y)/(x-y),"\\frac{{x}-2{y}}{{x}-{y}}","\\frac{2{y}-{x}}{{y}-{x}}");
	}
};

BOOST_AUTO_TEST_CASE(rational_function_print_tex_test)
{
	boost::mpl::for_each<key_types>(print_tex_tester());
}

struct evaluate_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using r_type = rational_function<Key>;
		using math::evaluate;
		BOOST_CHECK((is_evaluable<r_type,int>::value));
		BOOST_CHECK((is_evaluable<r_type,double>::value));
		BOOST_CHECK((is_evaluable<r_type,integer>::value));
		BOOST_CHECK((is_evaluable<r_type,rational>::value));
		BOOST_CHECK((is_evaluable<r_type,r_type>::value));
		BOOST_CHECK((!is_evaluable<r_type,std::string>::value));
		r_type x{"x"}, y{"y"};
		BOOST_CHECK((std::is_same<decltype(evaluate<rational>((x+y)/(3*y),{{"x",1/2_q},{"y",-3/5_q}})),rational>::value));
		BOOST_CHECK((std::is_same<decltype(evaluate<double>((x+y)/(3*y),{{"x",1.},{"y",2.}})),double>::value));
		BOOST_CHECK((std::is_same<decltype(evaluate<integer>((x+y)/(3*y),{{"x",1_z},{"y",2_z}})),integer>::value));
		BOOST_CHECK((std::is_same<decltype(evaluate<r_type>((x+y)/(3*y),{{"x",r_type{}},{"y",r_type{}}})),r_type>::value));
		BOOST_CHECK_EQUAL((evaluate<rational>((2*x*x+y)/(3*y),{{"x",1/2_q},{"y",-3/5_q}})),1/18_q);
		BOOST_CHECK_EQUAL((evaluate<integer>((2*x*x+y)/(3*y),{{"x",2_z},{"y",3_z}})),1_z);
		BOOST_CHECK_EQUAL((evaluate<r_type>((2*x*x+y)/(3*y),{{"x",r_type{"y"}},{"y",r_type{"x"}}})),(2*y*y+x)/(3*x));
		BOOST_CHECK_THROW((evaluate<rational>((2*x*x+y)/(3*y),{{"x",1/2_q},{"y",0_q}})),zero_division_error);
	}
};

BOOST_AUTO_TEST_CASE(rational_function_evaluate_test)
{
	boost::mpl::for_each<key_types>(evaluate_tester());
}

struct trim_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using r_type = rational_function<Key>;
		using p_type = typename r_type::p_type;
		{
		r_type x{"x"}, y{"y"};
		auto r = (x+y)/x;
		auto r_trim = r.trim();
		BOOST_CHECK(r.num().get_symbol_set() == r_trim.num().get_symbol_set());
		BOOST_CHECK(r.den().get_symbol_set() != r_trim.den().get_symbol_set());
		r = (x-x+y)/x;
		r_trim = r.trim();
		BOOST_CHECK(r.num().get_symbol_set() != r_trim.num().get_symbol_set());
		BOOST_CHECK(r.den().get_symbol_set() != r_trim.den().get_symbol_set());
		BOOST_CHECK((r.num().get_symbol_set() == symbol_set{symbol{"x"},symbol{"y"}}));
		BOOST_CHECK((r_trim.num().get_symbol_set() == symbol_set{symbol{"y"}}));
		}
		// Random testing.
		p_type x{"x"}, y{"y"}, z{"z"};
		std::uniform_int_distribution<int> dist(0,4), p_dist(-4,4);
		for (int i = 0; i < ntrials; ++i) {
			auto n1 = rn_poly(x,y,z,dist);
			auto d1 = rn_poly(x,y,z,dist);
			if (math::is_zero(d1)) {
				BOOST_CHECK_THROW((r_type{n1,d1}),zero_division_error);
				continue;
			}
			r_type r1{n1,d1};
			auto r1_trim = r1.trim();
			BOOST_CHECK(r1_trim.is_canonical());
			BOOST_CHECK_EQUAL(r1,r1_trim);
		}
	}
};

BOOST_AUTO_TEST_CASE(rational_function_trim_test)
{
	boost::mpl::for_each<key_types>(trim_tester());
}

struct sin_cos_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using r_type = rational_function<Key>;
		BOOST_CHECK(has_sine<r_type>::value);
		BOOST_CHECK(has_cosine<r_type>::value);
		BOOST_CHECK_EQUAL(math::sin(r_type{}),0);
		BOOST_CHECK_EQUAL(math::cos(r_type{}),1);
		BOOST_CHECK_THROW(math::sin(r_type{"x"}),std::invalid_argument);
		BOOST_CHECK_THROW(math::cos(r_type{"x"}),std::invalid_argument);
	}
};

BOOST_AUTO_TEST_CASE(rational_function_sin_cos_test)
{
	boost::mpl::for_each<key_types>(sin_cos_tester());
}

struct degree_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using math::degree;
		using r_type = rational_function<Key>;
		BOOST_CHECK(has_degree<r_type>::value);
		r_type x{"x"}, y{"y"};
		BOOST_CHECK_EQUAL(degree(r_type{}),0);
		BOOST_CHECK_EQUAL(degree(x),1);
		BOOST_CHECK_EQUAL(degree(y),1);
		BOOST_CHECK_EQUAL(degree(x*x/y),2);
		BOOST_CHECK_EQUAL(degree(y/(x*x)),2);
		BOOST_CHECK_EQUAL(degree(y/(x*x),{"y"}),1);
		BOOST_CHECK_EQUAL(degree(y/(x*x),{"x"}),2);
		BOOST_CHECK_EQUAL(degree(y/(x*x),{"z"}),0);
		// Check nothing funky is going on with the return type
		// (during development, a bad use of std::max with trailing
		// return type would end up returning a reference).
		BOOST_CHECK((!std::is_reference<decltype(degree(x))>::value));
		BOOST_CHECK((!std::is_reference<decltype(degree(x,{"x"}))>::value));
	}
};

BOOST_AUTO_TEST_CASE(rational_function_degree_test)
{
	boost::mpl::for_each<key_types>(degree_tester());
}

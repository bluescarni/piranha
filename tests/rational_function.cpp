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

#define BOOST_TEST_MODULE rational_function_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "../src/environment.hpp"
#include "../src/exceptions.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/real.hpp"

using namespace piranha;

using key_types = boost::mpl::vector<k_monomial,monomial<unsigned char>,monomial<integer>>;

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
		BOOST_CHECK_EQUAL(r.num(),0);
		BOOST_CHECK_EQUAL(r.den(),1);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		r = r_type{5};
		auto s = r;
		BOOST_CHECK_EQUAL(s.num(),5);
		BOOST_CHECK_EQUAL(s.den(),1);
		BOOST_CHECK(s.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(s.den().get_symbol_set().size() == 0u);
		// Check status of moved-from objects.
		r_type t = std::move(s);
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
		BOOST_CHECK_EQUAL(s.num(),5);
		BOOST_CHECK_EQUAL(s.den(),1);
		BOOST_CHECK(s.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(s.den().get_symbol_set().size() == 0u);
		// Revive with move.
		t = std::move(s);
		s = std::move(t);
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
		BOOST_CHECK((std::is_constructible<r_type,p_type,p_type>::value));
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
		BOOST_CHECK((std::is_constructible<r_type,std::string,std::string &>::value));
		BOOST_CHECK((std::is_constructible<r_type,char *,char*>::value));
		BOOST_CHECK((std::is_constructible<r_type,const char *,const char*>::value));
		BOOST_CHECK((!std::is_constructible<r_type,double,double>::value));
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
		p_type x{"x"}, y{"y"};
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
	}
};

BOOST_AUTO_TEST_CASE(rational_function_stream_test)
{
	boost::mpl::for_each<key_types>(stream_tester());
}

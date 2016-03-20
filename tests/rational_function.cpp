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
#include <string>
#include <type_traits>

#include "../src/environment.hpp"
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
		BOOST_CHECK(s.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(s.den().get_symbol_set().size() == 0u);
		// Revive.
		s = t;
		BOOST_CHECK_EQUAL(s.num(),5);
		BOOST_CHECK_EQUAL(s.den(),1);
		BOOST_CHECK(s.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(s.den().get_symbol_set().size() == 0u);
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
		r = r_type{1};
		BOOST_CHECK_EQUAL(r.num(),1);
		BOOST_CHECK_EQUAL(r.den(),1);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);
		r = -2;
		BOOST_CHECK_EQUAL(r.num(),-2);
		BOOST_CHECK_EQUAL(r.den(),1);
		BOOST_CHECK(r.num().get_symbol_set().size() == 0u);
		BOOST_CHECK(r.den().get_symbol_set().size() == 0u);

		std::cout << r_type{q_type{"x"}+q_type{"x"}.pow(2)/2} << '\n';
		std::cout << r_type{123_z} << '\n';
		std::cout << r_type{122/244_q} << '\n';
		std::cout << r_type{-122/244_q} << '\n';
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
		std::cout << r_type{1_z,-2_z} << '\n';
		std::cout << r_type{(x+y)*(x-y),(2*x+2*y)*z} << '\n';
		std::cout << r_type{(x+y)*(x-y),(2*x+2*y)*-z} << '\n';
		std::cout << r_type{(xq+yq)*(xq-yq),(2*xq+2*yq)*-zq} << '\n';
		std::cout << r_type{4*(xq+yq)*(xq-yq),2*(2*xq+2*yq)*-zq} << '\n';
		// Assignments.
	}
};

BOOST_AUTO_TEST_CASE(rational_function_ctor_test)
{
	environment env;
	boost::mpl::for_each<key_types>(constructor_tester());
}

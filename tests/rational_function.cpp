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
		BOOST_CHECK((std::is_constructible<r_type,p_type>::value));
		BOOST_CHECK((std::is_constructible<r_type,p_type &>::value));
		BOOST_CHECK((std::is_constructible<r_type,p_type &&>::value));
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
		std::cout << r_type{q_type{"x"}+q_type{"x"}.pow(2)/2} << '\n';
		std::cout << r_type{123_z} << '\n';
		std::cout << r_type{122/244_q} << '\n';
		std::cout << r_type{-122/244_q} << '\n';
	}
};

BOOST_AUTO_TEST_CASE(rational_function_ctor_test)
{
	environment env;
	boost::mpl::for_each<key_types>(constructor_tester());
}

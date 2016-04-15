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

#define BOOST_TEST_MODULE rational_function_02_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <type_traits>

#include "../src/environment.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/math.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/pow.hpp"

using namespace piranha;

using key_types = boost::mpl::vector<k_monomial,monomial<unsigned char>,monomial<integer>>;

struct hash_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using r_type = rational_function<Key>;
		using p_type = typename r_type::p_type;
		r_type x{"x"}, y{"y"};
		BOOST_CHECK_EQUAL(r_type{}.hash(),p_type{1}.hash());
		BOOST_CHECK_EQUAL((x/y).hash(),
			static_cast<std::size_t>((x + y - y).hash() + (y + x -x).hash()));
		BOOST_CHECK_EQUAL((x/y).hash(),(y/x).hash());
		BOOST_CHECK((std::is_same<std::size_t,decltype(x.hash())>::value));
	}
};

BOOST_AUTO_TEST_CASE(rational_function_ctor_test)
{
	environment env;
	boost::mpl::for_each<key_types>(hash_tester());
}

struct is_identical_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using r_type = rational_function<Key>;
		r_type x{"x"}, y{"y"}, z{"z"};
		BOOST_CHECK((std::is_same<bool,decltype(x.is_identical(y))>::value));
		BOOST_CHECK((!x.is_identical(y)));
		BOOST_CHECK((!y.is_identical(x)));
		BOOST_CHECK((x.is_identical(x)));
		BOOST_CHECK(((x / z).is_identical(x / z)));
		BOOST_CHECK((!(x + y - y).is_identical(x)));
		BOOST_CHECK((!(x / y).is_identical((x + z - z) / y)));
		BOOST_CHECK((!(x / y).is_identical(x / (y + z - z))));
		BOOST_CHECK((!(x / y).is_identical((x + z - z) / (y + z - z))));
	}
};

BOOST_AUTO_TEST_CASE(rational_function_is_identical_test)
{
	boost::mpl::for_each<key_types>(is_identical_tester());
}

struct partial_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using r_type = rational_function<Key>;
		using p_type = typename r_type::p_type;
		r_type x{"x"}, y{"y"}, z{"z"};
		r_type::register_custom_derivative("x",[](const r_type &) {return r_type{42};});
		BOOST_CHECK_EQUAL(math::partial(x,"x"),42);
		BOOST_CHECK_EQUAL(math::partial(x + 2*y,"x"),42);
		r_type::unregister_custom_derivative("x");
		r_type::register_custom_derivative("x",[&x](const r_type &r) {
			return r.partial("x") + r.partial("y") * 2*x;
		});
		BOOST_CHECK_EQUAL(math::partial((x+y)/(x-z+y),"x"),
			((1+2*x)*(x-z+y)-(x+y)*(1+2*x))/math::pow(x-z+y,2));
		r_type::unregister_all_custom_derivatives();
		BOOST_CHECK_EQUAL(math::partial((x+y)/(x-z+y),"x"),
			((x-z+y)-(x+y))/math::pow(x-z+y,2));
		r_type::register_custom_derivative("x",[&x](const r_type &r) {
			return r.partial("x") + r.partial("y") * 2*x + r.partial("z") * -1/(x*x);
		});
		BOOST_CHECK_EQUAL(math::subs(math::subs(math::partial((x+y)/(x-z+y),"x"),"y",x*x),"z",1/x),
			((1+2*x)*(x-1/x+x*x)-(x+x*x)*(1+1/(x*x)+2*x))/math::pow(x-1/x+x*x,2));
		r_type::unregister_all_custom_derivatives();
		// Try also with custom diffs on the poly type.
		p_type::register_custom_derivative("x",[](const p_type &p) {
			return p.partial("x") + p.partial("y") * 2*p_type{"x"};
		});
		BOOST_CHECK_EQUAL(math::partial((x+y)/(x-z+y),"x"),
			((1+2*x)*(x-z+y)-(x+y)*(1+2*x))/math::pow(x-z+y,2));
		p_type::unregister_all_custom_derivatives();
	}
};

BOOST_AUTO_TEST_CASE(rational_function_partial_test)
{
	boost::mpl::for_each<key_types>(partial_tester());
}

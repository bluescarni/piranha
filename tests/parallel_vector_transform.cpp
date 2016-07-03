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

#include "../src/detail/parallel_vector_transform.hpp"

#define BOOST_TEST_MODULE parallel_vector_transform_test
#include <boost/test/unit_test.hpp>

#include <stdexcept>
#include <vector>

#include "../src/init.hpp"
#include "../src/settings.hpp"

using namespace piranha;
using namespace piranha::detail;

BOOST_AUTO_TEST_CASE(pvt_test_00)
{
	init();
	// First check the throwing conditions.
	{
	std::vector<int> v1, v2;
	BOOST_CHECK_THROW(parallel_vector_transform(0u,v1,v2,[](int n) {return n;}),std::invalid_argument);
	v1.push_back(1);
	BOOST_CHECK_THROW(parallel_vector_transform(1u,v1,v2,[](int n) {return n;}),std::invalid_argument);
	v1.pop_back();
	}
	for (unsigned nt = 1u; nt <= 20u; ++nt) {
		settings::set_n_threads(nt);
		// Two empty vectors to start.
		std::vector<int> v1, v2;
		BOOST_CHECK_NO_THROW(parallel_vector_transform(nt,v1,v2,[](int n) {return n;}));
		BOOST_CHECK(v1.empty());
		BOOST_CHECK(v2.empty());
		// Fill them in with some data.
		v1 = {1,2,3,4,5,6,7,8};
		v2.resize(8u);
		BOOST_CHECK_NO_THROW(parallel_vector_transform(nt,v1,v2,[](int n) {return 3*n;}));
		BOOST_CHECK((v2 == std::vector<int>{3,6,9,12,15,18,21,24}));
		// Check with a throwing functor.
		v2 = std::vector<int>(8u,0);
		BOOST_CHECK_THROW(parallel_vector_transform(nt,v1,v2,[](int n) -> int {
			if (n == 8) {
				throw std::invalid_argument("");
			}
			return 3*n;
		}),std::invalid_argument);
		BOOST_CHECK_EQUAL(v2[0u],3);
		BOOST_CHECK_EQUAL(v2[7u],0);
		// Try with a different type.
		std::vector<double> v3(8u,0.);
		BOOST_CHECK_NO_THROW(parallel_vector_transform(nt,v1,v3,[](int n) {return 3. * n;}));
		BOOST_CHECK((v3 == std::vector<double>{3.,6.,9.,12.,15.,18.,21.,24.}));
		// Reset the number of threads before going out.
		settings::reset_n_threads();
	}
}

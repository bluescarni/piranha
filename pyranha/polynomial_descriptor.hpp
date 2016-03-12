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

#ifndef PYRANHA_POLYNOMIAL_DESCRIPTOR_HPP
#define PYRANHA_POLYNOMIAL_DESCRIPTOR_HPP

#include "python_includes.hpp"

#include <tuple>

#include "../src/kronecker_monomial.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/polynomial.hpp"
#include "../src/real.hpp"
#include "type_system.hpp"

namespace pyranha
{

DECLARE_TT_NAMER(piranha::polynomial,"polynomial")

// Descriptor for polynomial exposition.
struct polynomial_descriptor
{
	using params = std::tuple<
		// Double precision.
		std::tuple<double,piranha::monomial<piranha::rational>>,
		std::tuple<double,piranha::monomial<short>>,
		std::tuple<double,piranha::kronecker_monomial<>>,
		// Integer.
		std::tuple<piranha::integer,piranha::monomial<piranha::rational>>,
		std::tuple<piranha::integer,piranha::monomial<short>>,
		std::tuple<piranha::integer,piranha::kronecker_monomial<>>,
		// Rational.
		std::tuple<piranha::rational,piranha::monomial<piranha::rational>>,
		std::tuple<piranha::rational,piranha::monomial<short>>,
		std::tuple<piranha::rational,piranha::kronecker_monomial<>>,
		// Real.
		std::tuple<piranha::real,piranha::monomial<piranha::rational>>,
		std::tuple<piranha::real,piranha::monomial<short>>,
		std::tuple<piranha::real,piranha::kronecker_monomial<>>
	>;
	using interop_types = std::tuple<double,piranha::integer,piranha::real,piranha::rational>;
	using pow_types = std::tuple<double,piranha::integer,piranha::real,piranha::rational>;
	using eval_types = interop_types;
	using subs_types = interop_types;
	// For now, we have only degrees computed as integers or rationals.
	using degree_truncation_types = std::tuple<piranha::integer,piranha::rational>;
	// Need to refer to these to silence a warning in GCC.
	interop_types		it;
	pow_types		pt;
	eval_types		et;
	subs_types		st;
	degree_truncation_types	dtt;
};

}

#endif

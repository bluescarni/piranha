/* Copyright 2009-2017 Francesco Biscani (bluescarni@gmail.com)

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

#include <cstdint>
#include <tuple>

#include <piranha/kronecker_monomial.hpp>
#include <piranha/monomial.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/mp_rational.hpp>
#include <piranha/polynomial.hpp>
#include <piranha/real.hpp>

namespace pyranha
{

// Descriptor for polynomial exposition.
struct polynomial_descriptor {
    using params = std::tuple<
        // Double precision.
        std::tuple<double, piranha::monomial<piranha::rational>>,
        std::tuple<double, piranha::monomial<std::int_least16_t>>, std::tuple<double, piranha::kronecker_monomial<>>,
        // Integer.
        std::tuple<piranha::integer, piranha::monomial<piranha::rational>>,
        std::tuple<piranha::integer, piranha::monomial<std::int_least16_t>>,
        std::tuple<piranha::integer, piranha::kronecker_monomial<>>,
        // Integer recursive.
        // NOTE: this is not really part of the public API, but it can be useful when experimenting
        // with recursive poly algorithms.
        std::tuple<piranha::polynomial<piranha::integer, piranha::monomial<std::int_least16_t>>,
                   piranha::monomial<std::int_least16_t>>,
        std::tuple<piranha::polynomial<piranha::integer, piranha::kronecker_monomial<>>, piranha::kronecker_monomial<>>,
        // Rational.
        std::tuple<piranha::rational, piranha::monomial<piranha::rational>>,
        std::tuple<piranha::rational, piranha::monomial<std::int_least16_t>>,
        std::tuple<piranha::rational, piranha::kronecker_monomial<>>>;
    using interop_types = std::tuple<double, piranha::integer, piranha::rational>;
    using pow_types = interop_types;
    using eval_types = std::tuple<double, piranha::integer, piranha::rational, piranha::real>;
    using subs_types = interop_types;
    // For now, we have only degrees computed as integers or rationals.
    using degree_truncation_types = std::tuple<piranha::integer, piranha::rational>;
    // Need to refer to these to silence a warning in GCC.
    interop_types it;
    pow_types pt;
    eval_types et;
    subs_types st;
    degree_truncation_types dtt;
};
}

#endif

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

#ifndef PYRANHA_POISSON_SERIES_DESCRIPTOR_HPP
#define PYRANHA_POISSON_SERIES_DESCRIPTOR_HPP

#include "python_includes.hpp"

#include <cstdint>
#include <tuple>

#include <piranha/divisor.hpp>
#include <piranha/divisor_series.hpp>
#include <piranha/kronecker_monomial.hpp>
#include <piranha/monomial.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/mp_rational.hpp>
#include <piranha/poisson_series.hpp>
#include <piranha/polynomial.hpp>
#include <piranha/real.hpp>

namespace pyranha
{

struct poisson_series_descriptor {
    using params = std::tuple<
        // Polynomials with double coefficients.
        std::tuple<piranha::polynomial<double, piranha::monomial<piranha::rational>>>,
        std::tuple<piranha::polynomial<double, piranha::monomial<std::int_least16_t>>>,
        std::tuple<piranha::polynomial<double, piranha::kronecker_monomial<>>>,
        // Polynomials with rational coefficients.
        std::tuple<piranha::polynomial<piranha::rational, piranha::monomial<piranha::rational>>>,
        std::tuple<piranha::polynomial<piranha::rational, piranha::monomial<std::int_least16_t>>>,
        std::tuple<piranha::polynomial<piranha::rational, piranha::kronecker_monomial<>>>,
        // Divisor series for the rational polynomial coefficients.
        std::tuple<piranha::divisor_series<piranha::polynomial<piranha::rational, piranha::monomial<piranha::rational>>,
                                           piranha::divisor<std::int_least16_t>>>,
        std::
            tuple<piranha::divisor_series<piranha::polynomial<piranha::rational, piranha::monomial<std::int_least16_t>>,
                                          piranha::divisor<std::int_least16_t>>>,
        std::tuple<piranha::divisor_series<piranha::polynomial<piranha::rational, piranha::kronecker_monomial<>>,
                                           piranha::divisor<std::int_least16_t>>>,
        // Divisor series for the double polynomial coefficients.
        std::tuple<piranha::divisor_series<piranha::polynomial<double, piranha::monomial<piranha::rational>>,
                                           piranha::divisor<std::int_least16_t>>>,
        std::tuple<piranha::divisor_series<piranha::polynomial<double, piranha::monomial<std::int_least16_t>>,
                                           piranha::divisor<std::int_least16_t>>>,
        std::tuple<piranha::divisor_series<piranha::polynomial<double, piranha::kronecker_monomial<>>,
                                           piranha::divisor<std::int_least16_t>>>>;
    using interop_types = std::tuple<double, piranha::integer, piranha::rational>;
    using pow_types = interop_types;
    using eval_types = std::tuple<double, piranha::integer, piranha::rational, piranha::real>;
    using subs_types = interop_types;
    using degree_truncation_types = std::tuple<piranha::integer, piranha::rational>;
    interop_types it;
    pow_types pt;
    eval_types et;
    subs_types st;
    degree_truncation_types dtt;
};
}

#endif

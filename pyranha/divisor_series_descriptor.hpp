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

#ifndef PYRANHA_DIVISOR_SERIES_DESCRIPTOR_HPP
#define PYRANHA_DIVISOR_SERIES_DESCRIPTOR_HPP

#include "python_includes.hpp"

#include <cstdint>
#include <tuple>

#include <mp++/config.hpp>

#include <piranha/divisor.hpp>
#include <piranha/divisor_series.hpp>
#include <piranha/integer.hpp>
#include <piranha/kronecker_monomial.hpp>
#include <piranha/monomial.hpp>
#include <piranha/polynomial.hpp>
#include <piranha/rational.hpp>
#if defined(MPPP_WITH_MPFR)
#include <piranha/real.hpp>
#endif

namespace pyranha
{
struct divisor_series_descriptor {
    using d_type = piranha::divisor<std::int_least16_t>;
    using params = std::tuple<
        // Poly double.
        std::tuple<piranha::polynomial<double, piranha::monomial<piranha::rational>>, d_type>,
        std::tuple<piranha::polynomial<double, piranha::monomial<std::int_least16_t>>, d_type>,
        std::tuple<piranha::polynomial<double, piranha::kronecker_monomial<>>, d_type>,
        // Poly rational.
        std::tuple<piranha::polynomial<piranha::rational, piranha::monomial<piranha::rational>>, d_type>,
        std::tuple<piranha::polynomial<piranha::rational, piranha::monomial<std::int_least16_t>>, d_type>,
        std::tuple<piranha::polynomial<piranha::rational, piranha::kronecker_monomial<>>, d_type>>;
    using interop_types = std::tuple<double, piranha::integer, piranha::rational>;
    using pow_types = interop_types;
    using eval_types = std::tuple<double, piranha::integer, piranha::rational
#if defined(MPPP_WITH_MPFR)
                                  ,
                                  piranha::real
#endif
                                  >;
    interop_types it;
    pow_types pt;
    eval_types et;
};
}

#endif

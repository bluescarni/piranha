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

#ifndef PYRANHA_DIVISOR_SERIES_DESCRIPTOR_HPP
#define PYRANHA_DIVISOR_SERIES_DESCRIPTOR_HPP

#include "python_includes.hpp"

#include <tuple>

#include "../src/divisor.hpp"
#include "../src/divisor_series.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/polynomial.hpp"
#include "../src/real.hpp"
#include "expose_utils.hpp"

namespace pyranha
{

PYRANHA_DECLARE_TT_NAMER(piranha::divisor_series, "divisor_series")

struct divisor_series_descriptor {
    using d_type = piranha::divisor<short>;
    using params = std::tuple<
        // Poly double.
        std::tuple<piranha::polynomial<double, piranha::monomial<piranha::rational>>, d_type>,
        std::tuple<piranha::polynomial<double, piranha::monomial<short>>, d_type>,
        std::tuple<piranha::polynomial<double, piranha::kronecker_monomial<>>, d_type>,
        // Poly rational.
        std::tuple<piranha::polynomial<piranha::rational, piranha::monomial<piranha::rational>>, d_type>,
        std::tuple<piranha::polynomial<piranha::rational, piranha::monomial<short>>, d_type>,
        std::tuple<piranha::polynomial<piranha::rational, piranha::kronecker_monomial<>>, d_type>,
        // Poly real.
        std::tuple<piranha::polynomial<piranha::real, piranha::monomial<piranha::rational>>, d_type>,
        std::tuple<piranha::polynomial<piranha::real, piranha::monomial<short>>, d_type>,
        std::tuple<piranha::polynomial<piranha::real, piranha::kronecker_monomial<>>, d_type>>;
    using interop_types = std::tuple<double, piranha::integer, piranha::real, piranha::rational>;
    using pow_types = interop_types;
    using eval_types = interop_types;
    interop_types it;
    pow_types pt;
    eval_types et;
};
}

#endif

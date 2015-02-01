/***************************************************************************
 *   Copyright (C) 2009-2011 by Francesco Biscani                          *
 *   bluescarni@gmail.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

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
		// Long double precision.
		std::tuple<long double,piranha::monomial<piranha::rational>>,
		std::tuple<long double,piranha::monomial<short>>,
		std::tuple<long double,piranha::kronecker_monomial<>>,
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

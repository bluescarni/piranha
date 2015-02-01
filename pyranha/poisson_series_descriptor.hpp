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

#ifndef PYRANHA_POISSON_SERIES_DESCRIPTOR_HPP
#define PYRANHA_POISSON_SERIES_DESCRIPTOR_HPP

#include "python_includes.hpp"

#include <tuple>

#include "../src/divisor.hpp"
#include "../src/divisor_series.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/poisson_series.hpp"
#include "../src/polynomial.hpp"
#include "../src/real.hpp"
#include "type_system.hpp"

namespace pyranha
{

DECLARE_TT_NAMER(piranha::poisson_series,"poisson_series")

struct poisson_series_descriptor
{
	using params = std::tuple<
		// Numerical coefficients.
		std::tuple<double>,
		std::tuple<piranha::rational>,
		std::tuple<piranha::real>,
		// Polynomials with double coefficients.
		std::tuple<piranha::polynomial<double,piranha::monomial<piranha::rational>>>,
		std::tuple<piranha::polynomial<double,piranha::monomial<short>>>,
		std::tuple<piranha::polynomial<double,piranha::kronecker_monomial<>>>,
		// Polynomials with rational coefficients.
		std::tuple<piranha::polynomial<piranha::rational,piranha::monomial<piranha::rational>>>,
		std::tuple<piranha::polynomial<piranha::rational,piranha::monomial<short>>>,
		std::tuple<piranha::polynomial<piranha::rational,piranha::kronecker_monomial<>>>,
		// Polynomials with real coefficients.
		std::tuple<piranha::polynomial<piranha::real,piranha::monomial<piranha::rational>>>,
		std::tuple<piranha::polynomial<piranha::real,piranha::monomial<short>>>,
		std::tuple<piranha::polynomial<piranha::real,piranha::kronecker_monomial<>>>,
		// Divisor series for the rational polynomial coefficients.
		std::tuple<piranha::divisor_series<piranha::polynomial<piranha::rational,piranha::monomial<piranha::rational>>,piranha::divisor<short>>>,
		std::tuple<piranha::divisor_series<piranha::polynomial<piranha::rational,piranha::monomial<short>>,piranha::divisor<short>>>,
		std::tuple<piranha::divisor_series<piranha::polynomial<piranha::rational,piranha::kronecker_monomial<>>,piranha::divisor<short>>>
	>;
	using interop_types = std::tuple<double,piranha::rational,piranha::integer,piranha::real>;
	using pow_types = std::tuple<double,piranha::integer,piranha::real,piranha::rational>;
	using eval_types = std::tuple<double,piranha::real,piranha::rational>;
	using subs_types = eval_types;
	using degree_truncation_types = std::tuple<piranha::integer,piranha::rational>;
	interop_types		it;
	pow_types		pt;
	eval_types		et;
	subs_types		st;
	degree_truncation_types	dtt;
};

}

#endif

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

#include "python_includes.hpp"

#include <tuple>

#include "../src/kronecker_monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/poisson_series.hpp"
#include "../src/polynomial.hpp"
#include "../src/real.hpp"
#include "expose_poisson_series.hpp"
#include "expose_utils.hpp"
#include "type_system.hpp"

namespace pyranha
{

DECLARE_TT_NAMER(piranha::poisson_series,"poisson_series")

void expose_poisson_series()
{
	struct ps_desc
	{
		using params = std::tuple<std::tuple<double>,std::tuple<piranha::rational>,std::tuple<piranha::real>,
			std::tuple<piranha::polynomial<double,signed char>>,std::tuple<piranha::polynomial<double,short>>,std::tuple<piranha::polynomial<double,piranha::kronecker_monomial<>>>,
			std::tuple<piranha::polynomial<piranha::rational,signed char>>,std::tuple<piranha::polynomial<piranha::rational,short>>,
			std::tuple<piranha::polynomial<piranha::rational,piranha::kronecker_monomial<>>>,
			std::tuple<piranha::polynomial<piranha::real,signed char>>,std::tuple<piranha::polynomial<piranha::real,short>>,
			std::tuple<piranha::polynomial<piranha::real,piranha::kronecker_monomial<>>>>;
		using interop_types = std::tuple<double,piranha::rational,piranha::integer,piranha::real>;
		using pow_types = std::tuple<double,piranha::integer,piranha::real>;
		using eval_types = std::tuple<double,piranha::real,piranha::rational>;
		using subs_types = eval_types;
		interop_types	it;
		pow_types	pt;
		eval_types	et;
		subs_types	st;
	};
	series_exposer<piranha::poisson_series,ps_desc> ps_exposer;
}

}

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

#ifndef PIRANHA_POISSON_SERIES_COEFFICIENT_HPP
#define PIRANHA_POISSON_SERIES_COEFFICIENT_HPP

#include <boost/concept_check.hpp>

#include "multipliable_coefficient.hpp"

namespace piranha
{

namespace concept
{

/// Concept for Poisson series coefficients.
/**
 * The requisites for type \p T are the following:
 * 
 * - must be a model of piranha::concept::MultipliableCoefficient,
 * - must be dividable by int.
 */
template <typename T>
struct PoissonSeriesCoefficient:
	MultipliableCoefficient<T>
{
	/// Concept usage pattern.
	BOOST_CONCEPT_USAGE(PoissonSeriesCoefficient)
	{
		T inst_mut = T();
		int n = 2;
		inst_mut /= n;
	}
};

}
}

#endif

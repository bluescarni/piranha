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

#ifndef PIRANHA_DETAIL_DEGREE_COMMONS_HPP
#define PIRANHA_DETAIL_DEGREE_COMMONS_HPP

#include <set>
#include <stdexcept>
#include <string>

#include "../config.hpp"
#include "../exceptions.hpp"
#include "../symbol_set.hpp"

// Common routines for use in monomial classes for computation of degree and order.

namespace piranha { namespace detail {

// Total degree.
template <typename Retval, typename Container, typename Op>
inline Retval monomial_degree(const Container &c, const Op &op, const symbol_set &args)
{
	if (unlikely(args.size() != c.size())) {
		piranha_throw(std::invalid_argument,"invalid arguments set");
	}
	Retval retval(0);
	for (decltype(c.size()) i = 0u; i < c.size(); ++i) {
		op(retval,c[i]);
	}
	return retval;
}

// Partial degree.
template <typename Retval, typename Container, typename Op>
inline Retval monomial_partial_degree(const Container &c, const Op &op, const std::set<std::string> &active_args, const symbol_set &args)
{
	if (unlikely(args.size() != c.size())) {
		piranha_throw(std::invalid_argument,"invalid arguments set");
	}
	Retval retval(0);
	// Just return zero if the size of the container is also zero.
	if (!c.size()) {
		return retval;
	}
	auto it1 = args.begin();
	auto it2 = active_args.begin();
	// Move forward until element in active args does not preceed the first
	// element in the reference arguments set.
	while (it2 != active_args.end() && *it2 < it1->get_name()) {
		++it2;
	}
	for (decltype(c.size()) i = 0u; i < c.size(); ++i, ++it1) {
		if (it2 == active_args.end()) {
			break;
		}
		if (it1->get_name() == *it2) {
			op(retval,c[i]);
			++it2;
		}
	}
	return retval;
}

}}

#endif

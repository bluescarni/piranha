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

#ifndef PIRANHA_CONCEPTS_HPP
#define PIRANHA_CONCEPTS_HPP

/** \file concepts.hpp
 * \brief Include this file to include all concepts defined in Piranha.
 */

namespace piranha
{
/// Concepts namespace.
/**
 * All concepts in Piranha are defined within this namespace.
 */
namespace concept {}
}

// Include all concepts.
#include "concepts/array_key_value_type.hpp"
#include "concepts/coefficient.hpp"
#include "concepts/container_element.hpp"
#include "concepts/crtp.hpp"
#include "concepts/key.hpp"
#include "concepts/multipliable_coefficient.hpp"
#include "concepts/multipliable_term.hpp"
#include "concepts/power_series_term.hpp"
#include "concepts/series.hpp"
#include "concepts/term.hpp"

#endif

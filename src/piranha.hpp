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

#ifndef PIRANHA_PIRANHA_HPP
#define PIRANHA_PIRANHA_HPP

/** \file piranha.hpp
 * Global piranha header file.
 * 
 * Include this file to import piranha's entire public interface.
 */

/// Root piranha namespace.
/**
 * \todo Check if in the end destructors will be implicitly marked as noexcept(true) in the final c++0x standard,
 * and if this is not the case add it.
 * 
 * \todo implement is_instance_of when GCC support for variadic templates improves, and remove the tag structs
 * (see http://stackoverflow.com/questions/4749863/variadic-templates-and-copy-construction-via-assignment)
 */
namespace piranha {}

// NOTES FOR DOCUMENTATION:
// - thread safety: assume none unless specified
// - bad_cast due to boost numeric cast: say that it might be thrown in many places, too cumbersome
//   to document every occurrence.
// - c++0x features not implemented yet in GCC latest version: what impact they have and piranha_* macros used
//   to signal/emulate them.

#include "config.hpp"
#include "crtp_helper.hpp"
#include "cvector.hpp"
#include "exceptions.hpp"
#include "hop_table.hpp"
#include "integer.hpp"
#include "mf_int.hpp"
#include "numerical_coefficient.hpp"
#include "runtime_info.hpp"
#include "settings.hpp"
#include "symbol.hpp"
#include "thread_barrier.hpp"
#include "thread_group.hpp"
#include "thread_management.hpp"
#include "type_traits.hpp"

#endif

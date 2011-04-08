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
 * \brief Global piranha header file.
 * 
 * Include this file to import piranha's entire public interface.
 */

/// Root piranha namespace.
/**
 * \todo implement is_instance_of when GCC support for variadic templates improves, and remove the tag structs
 * (see http://stackoverflow.com/questions/4749863/variadic-templates-and-copy-construction-via-assignment)
 * \todo better piranha_throw macro with support of boost_throw when using boost thread instead of native c++0x threads.
 * \todo switch to auto -> decltype declarations of member functions for complicated types (e.g., tuples) when decltype on this
 * becomes available.
 * \todo explain in general section the base assumptions of move semantics and thread safety (e.g., require implicitly that
 * all moved-from objects are assignable and destructable, and everything not thread-safe by default).
 * \todo document classes in detail:: use to implement mathematical functions?
 * \todo fix type traits such as is_nothrow_move_constructible/assignable. They must work also for references,
 * make them use SFINAE, declval and decltype as in the current GCC 4.6 type_traits header (e.g., see is_constructible). Keep in mind
 * reference collapsing rules, etc. etc.
 * \todo modify concepts to use declval where applicable, instead of dereferencing nullptr.
 */
namespace piranha
{

/// Namespace for implementation details.
/**
 * Classes and functions defined in this namespace are non-documented implementation details.
 * Users should never employ functionality implemented in this namespace.
 */
namespace detail {}

}

// NOTES FOR DOCUMENTATION:
// - thread safety: assume none unless specified
// - bad_cast due to boost numeric cast: say that it might be thrown in many places, too cumbersome
//   to document every occurrence.
// - c++0x features not implemented yet in GCC latest version: what impact they have and piranha_* macros used
//   to signal/emulate them.

#include "array_key.hpp"
#include "base_series.hpp"
#include "base_term.hpp"
#include "concepts.hpp"
#include "config.hpp"
#include "cvector.hpp"
#include "debug_access.hpp"
#include "echelon_descriptor.hpp"
#include "exceptions.hpp"
#include "hop_table.hpp"
#include "integer.hpp"
#include "math.hpp"
#include "mf_int.hpp"
#include "monomial.hpp"
#include "numerical_coefficient.hpp"
#include "polynomial_term.hpp"
#include "runtime_info.hpp"
#include "settings.hpp"
#include "symbol.hpp"
#include "thread_barrier.hpp"
#include "thread_group.hpp"
#include "thread_management.hpp"
#include "threading.hpp"
#include "top_level_series.hpp"
#include "type_traits.hpp"
#include "utils.hpp"

#endif

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

#include "degree_truncator_settings.hpp"
#include "kronecker_array.hpp"
#include "runtime_info.hpp"
#include "settings.hpp"
#include "threading.hpp"

// Translation unit to enforce order of initialisation of static variables.
// NOTE: the important thing here so far is that memory allocation functions of GMP
// are set up correctly before any GMP object is used. Namely:
// - degree truncator setup,
// - kronecker array codification limits.
// IOW, the startup class must be created before any GMP object is initialised.

namespace piranha
{

// Instantiate the thread id, in case in the future we need to check on this in GMP related checks.
const thread_id runtime_info::m_main_thread_id = this_thread::get_id();

// This will be set to true in the startup code below.
bool settings::m_status = false;

// Piranha runtime environment initial setup.
const settings::startup settings::m_startup;

integer degree_truncator_settings::m_limit = integer(0);

// Instantiate explicitly the static data of kronecker array for all valid types, i.e., all signed integer types.
template <>
const typename kronecker_array<signed char>::limits_type kronecker_array<signed char>::m_limits = kronecker_array<signed char>::determine_limits();

template <>
const typename kronecker_array<signed short>::limits_type kronecker_array<signed short>::m_limits = kronecker_array<signed short>::determine_limits();

template <>
const typename kronecker_array<signed int>::limits_type kronecker_array<signed int>::m_limits = kronecker_array<signed int>::determine_limits();

template <>
const typename kronecker_array<signed long>::limits_type kronecker_array<signed long>::m_limits = kronecker_array<signed long>::determine_limits();

template <>
const typename kronecker_array<signed long long>::limits_type kronecker_array<signed long long>::m_limits = kronecker_array<signed long long>::determine_limits();

}

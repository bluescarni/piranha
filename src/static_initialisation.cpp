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

#include <algorithm>
#include <set>
#include <string>
#include <unordered_set>

#include "degree_truncator_settings.hpp"
#include "kronecker_array.hpp"
#include "runtime_info.hpp"
#include "settings.hpp"
#include "symbol.hpp"
#include "thread_management.hpp"
#include "threading.hpp"
#include "tracing.hpp"

// Translation unit to enforce order of initialisation of static variables.

namespace piranha
{

// Instantiate the thread id, in case in the future we need to check on this.
const thread_id runtime_info::m_main_thread_id = this_thread::get_id();

// Piranha runtime environment initial setup.
const settings::startup settings::m_startup;

// Settings.
mutex settings::m_mutex;
unsigned settings::m_n_threads = std::max<unsigned>(runtime_info::determine_hardware_concurrency(),1u);
bool settings::m_tracing = false;
unsigned settings::m_max_char_output = settings::m_default_max_char_output;

// Symbol.
symbol::container_type symbol::m_symbol_list;
mutex symbol::m_mutex;

// Thread management.
mutex thread_management::m_mutex;
mutex thread_management::binder::m_binder_mutex;
std::unordered_set<unsigned> thread_management::binder::m_used_procs;

// Runtime info.
const unsigned runtime_info::m_hardware_concurrency = runtime_info::determine_hardware_concurrency();
const unsigned runtime_info::m_cache_line_size = runtime_info::determine_cache_line_size();

// Tracing.
tracing::container_type tracing::m_container;
mutex tracing::m_mutex;

// Degree truncator.
degree_truncator_settings::mode degree_truncator_settings::m_mode = degree_truncator_settings::inactive;
mutex degree_truncator_settings::m_mutex;
integer degree_truncator_settings::m_limit = integer(0);
std::set<std::string> degree_truncator_settings::m_args = {};

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

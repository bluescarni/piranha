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
#include <iostream>
#include <stdexcept>

#include "config.hpp"
#include "exceptions.hpp"
#include "malloc_allocator.hpp"
#include "runtime_info.hpp"
#include "settings.hpp"
#include "threading.hpp"

namespace piranha
{

mutex settings::m_mutex;
unsigned settings::m_n_threads = std::max<unsigned>(runtime_info::determine_hardware_concurrency(),1u);
bool settings::m_tracing = false;
settings::startup settings::m_startup;
static const unsigned default_max_char_output = 10000u;
unsigned settings::m_max_char_output = default_max_char_output;

settings::startup::startup()
{
	std::cout << "Piranha version: " << PIRANHA_VERSION << '\n';
	std::cout << "Hardware concurrency: " << runtime_info::determine_hardware_concurrency() << '\n';
	std::cout << "Cache line size: " << runtime_info::determine_cache_line_size() << '\n';
	std::cout << "Memory alignment primitives: " <<
		(malloc_allocator<char>::have_memalign_primitives ? "available" : "unavailable") << '\n';
	std::cout << "Threading primitives: " <<
#ifdef PIRANHA_USE_BOOST_THREAD
		"Boost.Thread"
#else
		"native C++"
#endif
		<< '\n';
	std::cout << "Piranha is ready.\n";
	std::cout << "__________________________________\n";
}

/// Get the number of threads available for use by piranha.
/**
 * The initial value upon program startup is set to the maximum between 1 and piranha::runtime_info::determine_hardware_concurrency().
 * 
 * @return the number of threads that will be available for use by piranha.
 * 
 * @throws std::system_error in case of failure(s) by threading primitives.
 */
unsigned settings::get_n_threads()
{
	lock_guard<mutex>::type lock(m_mutex);
	return m_n_threads;
}

/// Set the number of threads available for use by piranha.
/**
 * @param[in] n the desired number of threads.
 * 
 * @throws std::invalid_argument if <tt>n == 0</tt>.
 * @throws std::system_error in case of failure(s) by threading primitives.
 */
void settings::set_n_threads(unsigned n)
{
	lock_guard<mutex>::type lock(m_mutex);
	if (n == 0) {
		piranha_throw(std::invalid_argument,"the number of threads must be strictly positive");
	}
	m_n_threads = n;
}

/// Reset the number of threads available for use by piranha.
/**
 * Will set the number of threads to the maximum between 1 and piranha::runtime_info::get_hardware_concurrency().
 * 
 * @throws std::system_error in case of failure(s) by threading primitives.
 */
void settings::reset_n_threads()
{
	lock_guard<mutex>::type lock(m_mutex);
	m_n_threads = std::max<unsigned>(runtime_info::get_hardware_concurrency(),static_cast<unsigned>(1));
}

/// Get tracing status.
/**
 * Tracing is disabled by default on program startup.
 * 
 * @return \p true if tracing is enabled, \p false otherwise.
 * 
 * @throws std::system_error in case of failure(s) by threading primitives.
 */
bool settings::get_tracing()
{
	lock_guard<mutex>::type lock(m_mutex);
	return m_tracing;
}

/// Set tracing status.
/**
 * Tracing is disabled by default on program startup.
 * 
 * @param[in] flag \p true to enable tracing, \p false to disable it.
 * 
 * @throws std::system_error in case of failure(s) by threading primitives.
 */
void settings::set_tracing(bool flag)
{
	lock_guard<mutex>::type lock(m_mutex);
	m_tracing = flag;
}

/// Get max char output.
/**
 * @return the maximum number of character displayed when printing series.
 * 
 * @throws std::system_error in case of failure(s) by threading primitives.
 */
unsigned settings::get_max_char_output()
{
	lock_guard<mutex>::type lock(m_mutex);
	return m_max_char_output;
}

/// Set max char output.
/**
 * @param[in] n the maximum number of character to be displayed when printing series.
 * 
 * @throws std::system_error in case of failure(s) by threading primitives.
 */
void settings::set_max_char_output(unsigned n)
{
	lock_guard<mutex>::type lock(m_mutex);
	m_max_char_output = n;
}

/// Reset max char output.
/**
 * Will set the max char output value to the default.
 */
void settings::reset_max_char_output()
{
	lock_guard<mutex>::type lock(m_mutex);
	m_max_char_output = default_max_char_output;
}

}

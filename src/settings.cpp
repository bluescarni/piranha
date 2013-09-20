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

#include <mutex>
#include <stdexcept>
#include <utility>

#include "config.hpp"
#include "runtime_info.hpp"
#include "settings.hpp"

namespace piranha
{

// Static init.
std::mutex settings::m_mutex;
std::pair<bool,unsigned> settings::m_cache_line_size(false,0u);
bool settings::m_tracing = false;
unsigned long settings::m_max_term_output = settings::m_default_max_term_output;

/// Get the cache line size.
/**
 * The initial value is set to the output of piranha::runtime_info::get_cache_line_size(). The value
 * can be overridden with set_cache_line_size() in case the detection fails and the value is set to zero.
 * 
 * @return data cache line size (in bytes).
 * 
 * @throws std::system_error in case of failure(s) by threading primitives.
 */
unsigned settings::get_cache_line_size()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (unlikely(!m_cache_line_size.first)) {
		m_cache_line_size.second = runtime_info::get_cache_line_size();
		m_cache_line_size.first = true;
	}
	return m_cache_line_size.second;
}

/// Set the cache line size.
/**
 * Overrides the detected cache line size. This method should be used only if the automatic
 * detection fails.
 * 
 * @param[in] n data cache line size (in bytes).
 * 
 * @throws std::system_error in case of failure(s) by threading primitives.
 */
void settings::set_cache_line_size(unsigned n)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_cache_line_size.first = true;
	m_cache_line_size.second = n;
}

/// Reset the cache line size.
/**
 * Will set the value to the output of piranha::runtime_info::get_cache_line_size().
 * 
 * @throws std::system_error in case of failure(s) by threading primitives.
 */
void settings::reset_cache_line_size()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_cache_line_size.second = runtime_info::get_cache_line_size();
	m_cache_line_size.first = true;
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
	std::lock_guard<std::mutex> lock(m_mutex);
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
	std::lock_guard<std::mutex> lock(m_mutex);
	m_tracing = flag;
}

/// Get max term output.
/**
 * @return maximum number of terms displayed when printing series.
 * 
 * @throws std::system_error in case of failure(s) by threading primitives.
 */
unsigned long settings::get_max_term_output()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_max_term_output;
}

/// Set max term output.
/**
 * @param[in] n maximum number of terms to be displayed when printing series.
 * 
 * @throws std::system_error in case of failure(s) by threading primitives.
 */
void settings::set_max_term_output(unsigned long n)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_max_term_output = n;
}

/// Reset max term output.
/**
 * Will set the max term output value to the default.
 * 
 * @throws std::system_error in case of failure(s) by threading primitives.
 */
void settings::reset_max_term_output()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_max_term_output = m_default_max_term_output;
}

}

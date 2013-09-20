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

#ifndef PIRANHA_SETTINGS_HPP
#define PIRANHA_SETTINGS_HPP

#include <mutex>
#include <utility>

#include "config.hpp"
#include "runtime_info.hpp"
#include "thread_pool.hpp"

namespace piranha
{

/// Global settings.
/**
 * This class stores the global settings of piranha's runtime environment.
 * The methods of this class are thread-safe.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
class PIRANHA_PUBLIC settings
{
	public:
		/// Get the number of threads available for use by piranha.
		/**
		 * The initial value is set to the maximum between 1 and piranha::runtime_info::get_hardware_concurrency().
		 * This function is equivalent to piranha::thread_pool::size().
		 *
		 * @return the number of threads that will be available for use by piranha.
		 *
		 * @throws unspecified any exception thrown by piranha::thread_pool::size().
		 */
		static unsigned get_n_threads()
		{
			return thread_pool::size();
		}
		/// Set the number of threads available for use by piranha.
		/**
		 * This function is equivalent to piranha::thread_pool::resize().
		 *
		 * @param[in] n the desired number of threads.
		 *
		 * @throws unspecfied any exception thrown by piranha::thread_pool::resize().
		 */
		static void set_n_threads(unsigned n)
		{
			thread_pool::resize(n);
		}
		/// Reset the number of threads available for use by piranha.
		/**
		 * Will set the number of threads to the maximum between 1 and piranha::runtime_info::get_hardware_concurrency().
		 *
		 * @throws unspecfied any exception thrown by set_n_threads().
		 */
		static void reset_n_threads()
		{
			const auto candidate = runtime_info::get_hardware_concurrency();
			set_n_threads((candidate > 0u) ? candidate : 1u);
		}
		static unsigned get_cache_line_size();
		static void set_cache_line_size(unsigned);
		static void reset_cache_line_size();
		static bool get_tracing();
		static void set_tracing(bool);
		static unsigned long get_max_term_output();
		static void set_max_term_output(unsigned long);
		static void reset_max_term_output();
	private:
		static std::mutex		m_mutex;
		static std::pair<bool,unsigned>	m_cache_line_size;
		static bool			m_tracing;
		static unsigned long		m_max_term_output;
		static const unsigned long	m_default_max_term_output = 20ul;
};

}

#endif

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

#ifndef PIRANHA_ENVIRONMENT_HPP
#define PIRANHA_ENVIRONMENT_HPP

#include <mutex>

#include "config.hpp"

namespace piranha
{

/// Piranha environment.
/**
 * An instance of this class should be created in the main routine of the program before accessing any
 * other functionality of the library. Its constructor will set up the runtime environment and register
 * cleanup functions that will be run on program exit (e.g., the MPFR <tt>mpfr_free_cache()</tt> function).
 * 
 * It is allowed to construct multiple instances of this class even from multiple threads: after the first
 * instance has been created, additional instances will not perform any action.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
class PIRANHA_PUBLIC environment
{
	public:
		environment();
		/// Deleted copy constructor.
		environment(const environment &) = delete;
		/// Deleted move constructor.
		environment(environment &&) = delete;
		/// Deleted copy assignment operator.
		environment &operator=(const environment &) = delete;
		/// Deleted move assignment operator.
		environment &operator=(environment &&) = delete;
		/// Query shutdown flag.
		/**
		 * If called before <tt>main()</tt> returns, this method will return \p false.
		 * The shutdown flag will be set to \p true after <tt>main()</tt> has returned
		 * but before the destruction of static objects begins (i.e., the flag is set to \p true
		 * by a function registered with <tt>std::atexit()</tt>).
		 * 
		 * @return shutdown flag.
		 */
		static bool shutdown()
		{
			return m_shutdown;
		}
	private:
		static void cleanup_function();
	private:
		static std::mutex	m_mutex;
		static bool		m_inited;
		static bool		m_shutdown;
};

}

#endif

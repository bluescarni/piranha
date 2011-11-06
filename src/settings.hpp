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

#include "config.hpp"
#include "threading.hpp"

namespace piranha
{

/// Global settings.
/**
 * This class stores the global settings of piranha's runtime environment.
 * The methods of this class, unless otherwise specified, are thread-safe.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
class PIRANHA_PUBLIC settings
{
	public:
		static unsigned get_n_threads();
		static void set_n_threads(unsigned);
		static void reset_n_threads();
		static bool get_tracing();
		static void set_tracing(bool);
		static unsigned get_max_char_output();
		static void set_max_char_output(unsigned);
		static void reset_max_char_output();
	private:
		struct startup
		{
			startup();
		};
	private:
		static mutex	m_mutex;
		static unsigned	m_n_threads;
		static bool	m_tracing;
		static startup	m_startup;
		static unsigned m_max_char_output;
};

}

#endif

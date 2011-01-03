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

#include <cstddef>

#include "mf_int.hpp"

namespace piranha
{

mf_int_traits::table_type mf_int_traits::init_log_table_256()
{
	table_type retval;
	retval[0] = retval[1] = 0;
	for (table_type::size_type i = 2; i < retval.size(); ++i) {
		retval[i] = 1 + retval[i / static_cast<table_type::size_type>(2)];
	}
	retval[0] = -1;
	return retval;
}

const mf_int_traits::table_type mf_int_traits::log_table_256 = mf_int_traits::init_log_table_256();

}

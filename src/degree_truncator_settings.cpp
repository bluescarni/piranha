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

#include <set>
#include <string>

#include "degree_truncator_settings.hpp"
#include "integer.hpp"
#include "threading.hpp"

namespace piranha
{

mutex degree_truncator_settings::m_mutex;
degree_truncator_settings::mode degree_truncator_settings::m_mode = degree_truncator_settings::inactive;
integer degree_truncator_settings::m_limit = integer(0);
std::set<std::string> degree_truncator_settings::m_args = {};

/// Disable truncation.
/**
 * Will disable any type of truncation.
 * 
 * @throws unspecified any exception thrown by threading primitives.
 */
void degree_truncator_settings::unset()
{
	lock_guard<mutex>::type lock(m_mutex);
	m_mode = inactive;
	m_limit = 0;
	m_args = {};
}

/// Set total degree truncation.
/**
 * @param[in] limit maximum total degree that will be retained during truncation.
 * 
 * @throws unspecified any exception thrown by threading primitives or memory allocation
 * errors in standard containers.
 */
void degree_truncator_settings::set(const int &limit)
{
	set_impl(total,std::set<std::string>{},integer(limit));
}

/// Set total degree truncation.
/**
 * @param[in] limit maximum total degree that will be retained during truncation.
 * 
 * @throws unspecified any exception thrown by threading primitives or memory allocation
 * errors in standard containers.
 */
void degree_truncator_settings::set(const integer &limit)
{
	set_impl(total,std::set<std::string>{},limit);
}

/// Set partial degree truncation.
/**
 * @param[in] arg argument that will be considered in the computation of the partial degree.
 * @param[in] limit maximum partial degree that will be retained during truncation.
 * 
 * @throws unspecified any exception thrown by threading primitives or memory allocation
 * errors in standard containers.
 */
void degree_truncator_settings::set(const std::string &arg, const int &limit)
{
	set_impl(partial,std::set<std::string>{arg},integer(limit));
}

/// Set partial degree truncation.
/**
 * @param[in] arg argument that will be considered in the computation of the partial degree.
 * @param[in] limit maximum partial degree that will be retained during truncation.
 * 
 * @throws unspecified any exception thrown by threading primitives or memory allocation
 * errors in standard containers.
 */
void degree_truncator_settings::set(const std::string &arg, const integer &limit)
{
	set_impl(partial,std::set<std::string>{arg},limit);
}

/// Set partial degree truncation.
/**
 * @param[in] args arguments that will be considered in the computation of the partial degree.
 * @param[in] limit maximum partial degree that will be retained during truncation.
 * 
 * @throws unspecified any exception thrown by threading primitives or memory allocation
 * errors in standard containers.
 */
void degree_truncator_settings::set(const std::set<std::string> &args, const int &limit)
{
	set_impl(partial,args,integer(limit));
}

/// Set partial degree truncation.
/**
 * @param[in] args arguments that will be considered in the computation of the partial degree.
 * @param[in] limit maximum partial degree that will be retained during truncation.
 * 
 * @throws unspecified any exception thrown by threading primitives or memory allocation
 * errors in standard containers.
 */
void degree_truncator_settings::set(const std::set<std::string> &args, const integer &limit)
{
	set_impl(partial,args,limit);
}

/// Get truncation mode.
/**
 * @return current truncation mode.
 * 
 * @throws unspecified any exception thrown by threading primitives.
 */
degree_truncator_settings::mode degree_truncator_settings::get_mode()
{
	lock_guard<mutex>::type lock(m_mutex);
	return m_mode;
}

/// Get truncation limit.
/**
 * @return current truncation limit or zero if the truncation mode is inactive.
 * 
 * @throws unspecified any exception thrown by threading primitives.
 */
integer degree_truncator_settings::get_limit()
{
	lock_guard<mutex>::type lock(m_mutex);
	return m_limit;
}

/// Get truncation arguments.
/**
 * @return arguments considered for truncation, or an empty set of arguments if the truncation
 * mode is total.
 * 
 * @throws unspecified any exception thrown by threading primitives or memory allocation
 * errors in standard containers.
 */
std::set<std::string> degree_truncator_settings::get_args()
{
	lock_guard<mutex>::type lock(m_mutex);
	return m_args;
}

}

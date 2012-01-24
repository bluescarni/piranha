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

#include <boost/any.hpp>
#include <cstddef>
#include <iostream>
#include <string>
#include <typeinfo>

#include "threading.hpp"
#include "tracing.hpp"

namespace piranha
{

// Static init.
tracing::container_type tracing::m_container;
mutex tracing::m_mutex;

namespace detail
{

#define piranha_tracing_print_case(T) \
if (typeid(T) == x.type()) { \
	os << boost::any_cast<T>(x); \
} else

struct generic_printer
{
	static void run(std::ostream &os,const boost::any &x)
	{
		piranha_tracing_print_case(char)
		piranha_tracing_print_case(wchar_t)
		piranha_tracing_print_case(char16_t)
		piranha_tracing_print_case(char32_t)
		piranha_tracing_print_case(unsigned char)
		piranha_tracing_print_case(signed char)
		piranha_tracing_print_case(unsigned short)
		piranha_tracing_print_case(short)
		piranha_tracing_print_case(unsigned)
		piranha_tracing_print_case(int)
		piranha_tracing_print_case(unsigned long)
		piranha_tracing_print_case(long)
		piranha_tracing_print_case(unsigned long long)
		piranha_tracing_print_case(long long)
		piranha_tracing_print_case(float)
		piranha_tracing_print_case(double)
		piranha_tracing_print_case(long double)
		piranha_tracing_print_case(std::string)
		piranha_tracing_print_case(const char *)
		{
			os << "unprintable value of type '" << x.type().name() << "'";
		}
	}
};

#undef piranha_tracing_print_case

}

/// Dump contents of the events database.
/**
 * Write the contents of the events database to stream in human-readable
 * format. Currently, the tracing data types for which
 * visualization is supported are the fundamental C++ arithmetic types,
 * \p std::string and <tt>const char *</tt>.
 * 
 * @param[in] os target output stream.
 * 
 * @throws unspecified any exception thrown by threading primitives.
 */
void tracing::dump(std::ostream &os)
{
	lock_guard<mutex>::type lock(m_mutex);
	for (auto it = m_container.begin(); it != m_container.end(); ++it) {
		os << it->first << '=';
		if (it->second.empty()) {
			os << "empty\n";
		} else {
			detail::generic_printer::run(os,it->second);
			os << '\n';
		}
	}
}

/// Get data associated to an event (C string version).
/**
 * @param[in] str event descriptor.
 * 
 * @return an instance of \p boost::any containing the data associated to the event described by \p str,
 * or an empty \p boost::any instance if the event is not in the database.
 * 
 * @throws unspecified any exception thrown by the constructor of \p std::string from a C string, or
 * by the other overload of the method.
 */
boost::any tracing::get(const char *str)
{
	return get(std::string(str));
}

/// Get data associated to an event.
/**
 * @param[in] str event descriptor.
 * 
 * @return an instance of \p boost::any containing the data associated to the event described by \p str,
 * or an empty \p boost::any instance if the event is not in the database.
 * 
 * @throws unspecified any exception thrown by threading primitives, or by the copy constructor
 * of \p boost::any.
 */
boost::any tracing::get(const std::string &str)
{
	lock_guard<mutex>::type lock(m_mutex);
	const auto it = m_container.find(str);
	if (it == m_container.end()) {
		return boost::any();
	} else {
		return it->second;
	}
}

/// Reset the contents of the database of events.
/**
 * @throws unspecified any exception thrown by threading primitives or by the <tt>clear()</tt> method
 * of the internal container.
 */
void tracing::reset()
{
	lock_guard<mutex>::type lock(m_mutex);
	m_container.clear();
}

}

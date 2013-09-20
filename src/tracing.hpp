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

#ifndef PIRANHA_TRACING_HPP
#define PIRANHA_TRACING_HPP

#include <boost/any.hpp>
#include <cstddef>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <typeinfo>
#include <utility>

#include "config.hpp"
#include "settings.hpp"

namespace piranha
{

namespace detail
{

template <typename = int>
struct base_tracing
{
	static std::map<std::string,boost::any>	m_container;
	static std::mutex			m_mutex;
};

template <typename T>
std::map<std::string,boost::any> base_tracing<T>::m_container;

template <typename T>
std::mutex base_tracing<T>::m_mutex;

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

/// Tracing class.
/**
 * This class is used to trace events for analysis and statistical purposes.
 * It is comprised of static methods that allow to interact at runtime with a database
 * of events in which string descriptors are paired with arbitrary data.
 * 
 * All methods in this class are thread-safe and provide the strong
 * exception safety guarantee, unless otherwise specified. Methods
 * and data members of this class are static, hence this class provides trivial
 * move semantics.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 * 
 * \todo settings::get_tracing() should use an atomic variable instead of mutex, to maximize performance
 * in those cases in which we are not tracing.
 */
class PIRANHA_PUBLIC tracing: private detail::base_tracing<>
{
	public:
		/// Trace event.
		/**
		 * Trace an event uniquely identified by \p str, and apply the functor
		 * \p f to the data associated to it.
		 * 
		 * \p Functor is expected to be an unary function object taking
		 * as parameter the reference to a \p boost::any instance representing
		 * the data associated to the descriptor \p str. The first time
		 * a descriptor is used, the \p boost::any object passed to \p f will be empty.
		 * 
		 * If tracing is disabled in piranha::settings, this method will be a no-op.
		 * 
		 * @param[in] str the descriptor of the event being traced.
		 * @param[in] f the functor to be applied to the data associated to \p str.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - copy construction of \p std::string,
		 * - memory errors in standard containers,
		 * - failures in threading primitives,
		 * - the execution of functor \p f.
		 * 
		 * @see http://www.boost.org/doc/libs/release/libs/any
		 */
		template <typename Functor>
		static void trace(const std::string &str, const Functor &f)
		{
			if (likely(!settings::get_tracing())) {
				return;
			}
			trace_impl(str,f);
		}
		/// Reset the contents of the database of events.
		/**
		 * @throws unspecified any exception thrown by threading primitives or by the <tt>clear()</tt> method
		 * of the internal container.
		 */
		static void reset()
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_container.clear();
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
		static boost::any get(const std::string &str)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			const auto it = m_container.find(str);
			if (it == m_container.end()) {
				return boost::any();
			} else {
				return it->second;
			}
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
		static void dump(std::ostream &os = std::cout)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
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
	private:
		template <typename Functor>
		static void trace_impl(const char *str, const Functor &f)
		{
			trace_impl(std::string(str),f);
		}
		template <typename Functor>
		static void trace_impl(const std::string &str, const Functor &f)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			bool new_item = false;
			auto it = m_container.find(str);
			if (it == m_container.end()) {
				auto tmp = m_container.insert(std::make_pair(str,boost::any()));
				piranha_assert(tmp.second);
				it = tmp.first;
				new_item = true;
			}
			try {
				f(it->second);
			} catch (...) {
				if (new_item) {
					m_container.erase(it);
				}
				throw;
			}
		}
};

}

#endif

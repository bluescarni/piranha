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

#ifndef PIRANHA_DEGREE_TRUNCATOR_SETTINGS_HPP
#define PIRANHA_DEGREE_TRUNCATOR_SETTINGS_HPP

#include <iostream>
#include <set>
#include <string>
#include <tuple>

#include "config.hpp"
#include "integer.hpp"
#include "threading.hpp"

namespace piranha
{

/// Settings for degree-based truncators.
/**
 * This class manages the global settings for degree-based piranha::truncator types. It consists of a set of static variables
 * representing the global truncation mode (total or partial degree), its limit and the variables involved.
 * 
 * All the methods in this class are thread-safe.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * This class provides the strong exception safety guarantee for all operations.
 * 
 * \section move_semantics Move semantics
 * 
 * This class has only global state, and hence has trivial move semantics.
 */
// NOTE: depending on performance measurements, a possible way to reduce the
// ovrhead of mutexes here is to use an atomic variable for the mode: when getting
// the state of the truncator settings, first query the mode and lock+copy the full state
// only if the truncator is active.
class degree_truncator_settings
{
	public:
		/// Truncation mode.
		enum mode
		{
			inactive	= 0, /**< Inactive: no truncation will be performed. */
			total		= 1, /**< Total: truncation is performed according to the total degree. */
			partial		= 2  /**< Partial: truncation is performed according to the degree of a set of variables. */
		};
		static void unset();
		static void set(const int &);
		static void set(const integer &);
		static void set(const std::string &, const int &);
		static void set(const std::string &, const integer &);
		static void set(const std::set<std::string> &, const int &);
		static void set(const std::set<std::string> &, const integer &);
		static mode get_mode();
		static integer get_limit();
		static std::set<std::string> get_args();
		/// Print degree truncator settings to stream.
		/**
		 * Will print to stream a human-readable representation of the current degree truncation settings.
		 * 
		 * @param[in] os target stream.
		 * @param[in] t instance to be printed.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - errors in threading primitives,
		 * - printing to stream the variables representing the truncator settings.
		 */
		friend std::ostream &operator<<(std::ostream &os, const degree_truncator_settings &t)
		{
			lock_guard<mutex>::type lock(m_mutex);
			os <<	"Degree truncator\n"
				"================\nMode: ";
			switch (t.m_mode) {
				case inactive:
					os << "inactive\n";
					break;
				case total:
					os << "total degree\n";
					break;
				case partial:
					os << "partial degree\n";
			}
			if (t.m_mode == partial) {
				os << "Arguments: {";
				for (auto it = t.m_args.begin(); it != t.m_args.end();) {
					os << *it;
					++it;
					if (it != t.m_args.end()) {
						os << ',';
					}
				}
				os << "}\n";
			}
			if (t.m_mode != inactive) {
				os << "Limit: " << t.m_limit << '\n';
			}
			return os;
		}
	protected:
		/// Get truncator state.
		/**
		 * @return an \p std::tuple containing the mode, limit and arguments of truncation.
		 * 
		 * @throws unspecified any exception thrown by threading primitives or memory allocation
		 * errors in standard containers.
		 */
		static std::tuple<mode,integer,std::set<std::string>> get_state()
		{
			lock_guard<mutex>::type lock(m_mutex);
			return std::make_tuple(m_mode,m_limit,m_args);
		}
	private:
		template <typename Set, typename Integer>
		static void set_impl(mode m, Set &&args, Integer &&limit)
		{
			piranha_assert(m == total || m == partial);
			// Exception safety.
			auto new_args(std::forward<Set>(args));
			lock_guard<mutex>::type lock(m_mutex);
			m_mode = m;
			m_args = std::move(new_args);
			m_limit = std::forward<Integer>(limit);
		}
	private:
		static mutex			m_mutex;
		static mode			m_mode;
		static integer			m_limit;
		static std::set<std::string>	m_args;
};

}

#endif
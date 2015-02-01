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

#ifndef PIRANHA_DIVISOR_SERIES_HPP
#define PIRANHA_DIVISOR_SERIES_HPP

#include "divisor.hpp"
#include "forwarding.hpp"
#include "is_cf.hpp"
#include "power_series.hpp"
#include "serialization.hpp"
#include "series.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

// Type trait to check the key type in divisor_series.
template <typename T>
struct is_divisor_series_key
{
	static const bool value = false;
};

template <typename T>
struct is_divisor_series_key<divisor<T>>
{
	static const bool value = true;
};

}

/// Divisor series.
/**
 * This class represents series in which the keys are divisor (see piranha::divisor) and the coefficient type
 * is generic. This class satisfies the piranha::is_series and piranha::is_cf type traits.
 *
 * ## Type requirements ##
 *
 * \p Cf must be suitable for use in piranha::series as first template argument, \p Key must be an instance
 * of piranha::divisor.
 *
 * ## Exception safety guarantee ##
 *
 * This class provides the same guarantee as the base series type it derives from.
 *
 * ## Move semantics ##
 *
 * Move semantics is equivalent to the move semantics of the base series type it derives from.
 *
 * ## Serialization ##
 *
 * This class supports serialization if the underlying coefficient type does.
 */
template <typename Cf, typename Key>
class divisor_series: public power_series<series<Cf,Key,divisor_series<Cf,Key>>,divisor_series<Cf,Key>>
{
		PIRANHA_TT_CHECK(detail::is_divisor_series_key,Key);
		using base = power_series<series<Cf,Key,divisor_series<Cf,Key>>,divisor_series<Cf,Key>>;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
	public:
		/// Series rebind alias.
		template <typename Cf2>
		using rebind = divisor_series<Cf2,Key>;
		/// Defaulted default constructor.
		divisor_series() = default;
		/// Defaulted copy constructor.
		divisor_series(const divisor_series &) = default;
		/// Defaulted move constructor.
		divisor_series(divisor_series &&) = default;
		PIRANHA_FORWARDING_CTOR(divisor_series,base)
		/// Trivial destructor.
		~divisor_series()
		{
			PIRANHA_TT_CHECK(is_series,divisor_series);
			PIRANHA_TT_CHECK(is_cf,divisor_series);
		}
		/// Defaulted copy assignment operator.
		divisor_series &operator=(const divisor_series &) = default;
		/// Defaulted move assignment operator.
		divisor_series &operator=(divisor_series &&) = default;
		PIRANHA_FORWARDING_ASSIGNMENT(divisor_series,base)
};

}

#endif

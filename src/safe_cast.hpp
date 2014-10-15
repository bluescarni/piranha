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

#ifndef PIRANHA_SAFE_CAST_HPP
#define PIRANHA_SAFE_CAST_HPP

#include <boost/numeric/conversion/cast.hpp>
#include <stdexcept>
#include <type_traits>

#include "detail/sfinae_types.hpp"
#include "exceptions.hpp"
#include "mp_integer.hpp"

namespace piranha
{

/// Default functor for the implementation of piranha::safe_cast().
template <typename To, typename From, typename = void>
struct safe_cast_impl
{
	/// Call operator.
	/**
	 * \note
	 * This call operator is enabled only if \p To and \p From2 are the same type
	 * and \p From2 is copy-constructible.
	 *
	 * @param[in] f conversion argument.
	 *
	 * @return a copy of \p f.
	 *
	 * @throws unspecified any exception thrown by the copy constructor of \p From2.
	 */
	template <typename From2, typename std::enable_if<std::is_same<To,From2>::value &&
		std::is_copy_constructible<From2>::value,
		int>::type = 0>
	To operator()(const From2 &f) const
	{
		return f;
	}
};

namespace detail
{

template <typename To, typename From>
using sc_int_int_enabler = typename std::enable_if<
	std::is_integral<To>::value && std::is_integral<From>::value
	>::type;

}

/// Implementation of piranha::safe_cast() for integral types.
/**
 * This implementation is enabled when both \p To and \p From are integral types.
 */
template <typename To, typename From>
struct safe_cast_impl<To,From,detail::sc_int_int_enabler<To,From>>
{
	/// Call operator.
	/**
	 * The call operator uses \p boost::numeric_cast() to perform a safe conversion
	 * between integral types.
	 *
	 * @param[in] f conversion argument.
	 *
	 * @return a copy of \p f cast safely to \p To.
	 *
	 * @throws unspecified any exception thrown by <tt>boost::numeric_cast()</tt>.
	 */
	To operator()(const From &f) const
	{
		return boost::numeric_cast<To>(f);
	}
};

namespace detail
{

template <typename To, typename From>
using sc_int_float_enabler = typename std::enable_if<
	((std::is_integral<To>::value || detail::is_mp_integer<To>::value) && std::is_floating_point<From>::value) ||
	(std::is_integral<To>::value && detail::is_mp_integer<From>::value) ||
	(std::is_integral<From>::value && detail::is_mp_integer<To>::value)
	>::type;

}

/// Specialisation of piranha::safe_cast() for conversions involving integral and floating-point types.
/**
 * This specialisation is enabled in the following cases:
 * - \p From is a floating-point type and \p To is a C++ integral type or piranha::mp_integer,
 * - one type is a C++ integral type and the other piranha::mp_integer.
 */
template <typename To, typename From>
struct safe_cast_impl<To,From,detail::sc_int_float_enabler<To,From>>
{
		/// Call operator, float to C++ integral overload.
		/**
		 * The call operator will first construct a piranha::integer from the input
		 * float, and will then attempt to convert the result to \p To.
		 *
		 * If the input float is not finite or does not represent an integral value,
		 * an error will be raised.
		 *
		 * @param[in] f conversion argument.
		 *
		 * @return a copy of \p f cast safely to \p To.
		 *
		 * @throws std::invalid_argument if \p f is not finite or if \p f does not represent
		 * an integral value.
		 * @throws unspecified any exception thrown by the conversion operator of piranha::integer.
		 */
		template <typename From2, typename To2 = To, typename std::enable_if<
			std::is_integral<To2>::value && std::is_floating_point<From2>::value,int>::type = 0>
		To2 operator()(const From2 &f) const
		{
			common_float_checks(f);
			return static_cast<To2>(integer{f});
		}
		/// Call operator, float to piranha::mp_integer overload.
		/**
		 * The call operator will construct a piranha::mp_integer from the input
		 * float.
		 *
		 * If the input float is not finite or does not represent an integral value,
		 * an error will be raised.
		 *
		 * @param[in] f conversion argument.
		 *
		 * @return a copy of \p f cast safely to \p To.
		 *
		 * @throws std::invalid_argument if \p f is not finite or if \p f does not represent
		 * an integral value.
		 * @throws unspecified any exception thrown by the conversion operator of piranha::mp_integer.
		 */
		template <typename From2, typename To2 = To, typename std::enable_if<
			detail::is_mp_integer<To2>::value && std::is_floating_point<From2>::value,int>::type = 0>
		To2 operator()(const From2 &f) const
		{
			common_float_checks(f);
			return To2{f};
		}
		/// Call operator, integral to piranha::mp_integer and piranha::mp_integer to integral overload.
		/**
		 * The call operator will construct an output type from the input type.
		 *
		 * @param[in] f conversion argument.
		 *
		 * @return a copy of \p f cast safely to \p To.
		 *
		 * @throws unspecified any exception thrown by the conversion operator or the constructor
		 * of piranha::mp_integer.
		 */
		template <typename From2, typename To2 = To, typename std::enable_if<
			(detail::is_mp_integer<To2>::value && std::is_integral<From2>::value) ||
			(detail::is_mp_integer<From2>::value && std::is_integral<To2>::value),int>::type = 0>
		To2 operator()(const From2 &f) const
		{
			return To2(f);
		}
	private:
		static void common_float_checks(const From &f)
		{
			if (unlikely(!std::isfinite(f))) {
				piranha_throw(std::invalid_argument,"invalid safe cast from non-finite floating-point to integral");
			}
			if (std::trunc(f) != f) {
				piranha_throw(std::invalid_argument,"invalid safe cast from floating-point to integral");
			}
		}
};

namespace detail
{

template <typename To, typename From>
using safe_cast_enabler = typename std::enable_if<
	std::is_same<decltype(safe_cast_impl<typename std::decay<To>::type,From>()(std::declval<const From &>())),typename std::decay<To>::type>::value,
	int>::type;

}

/// Safe cast.
/**
 * This function is meant to be used when it is necessary to convert between two types while making
 * sure that the value is preserved after the conversion. For instance, a safe cast between
 * integral types will check that the input value is representable by the return type, otherwise
 * an error will be raised.
 *
 * The actual implementation of this function is in the piranha::safe_cast_impl functor's
 * call operator. The decayed type of \p To is passed as first template parameter of
 * piranha::safe_cast_impl, whereas \p From is passed as-is.
 *
 * Any specialisation of piranha::safe_cast_impl must have a call operator returning
 * an instance of the decayed type of \p To, otherwise this function will be disabled.
 *
 * Any exception thrown by the implementation will be caught and an \p std::invalid_argument exception
 * will be raised instead.
 *
 * @param[in] x argument for the conversion.
 *
 * @return \p x converted to the decay type of \p To.
 *
 * @throws std::invalid_argument if the conversion fails.
 */
template <typename To, typename From, detail::safe_cast_enabler<To,From> = 0>
inline To safe_cast(const From &x)
{
	try {
		return safe_cast_impl<typename std::decay<To>::type,From>()(x);
	} catch (const std::exception &e) {
		piranha_throw(std::invalid_argument,std::string("unsafe conversion, the full error message is:\n") + e.what() + "\n");
	} catch (...) {
		piranha_throw(std::invalid_argument,"unsafe conversion");
	}
}

/// Type trait to detect piranha::safe_cast().
/**
 * The type trait will be \p true if piranha::safe_cast() can be called with the decayed types of \p To and \p From
 * as template arguments, \p false otherwise.
 */
template <typename To, typename From>
class has_safe_cast: detail::sfinae_types
{
		using Tod = typename std::decay<To>::type;
		using Fromd = typename std::decay<From>::type;
		template <typename To1, typename From1, detail::safe_cast_enabler<To1,From1> = 0>
		static yes test(const To1 &, const From1 &);
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test(std::declval<Tod>(),std::declval<Fromd>())),yes>::value;
};

// Static init.
template <typename To, typename From>
const bool has_safe_cast<To,From>::value;

}

#endif

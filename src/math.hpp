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

#ifndef PIRANHA_MATH_HPP
#define PIRANHA_MATH_HPP

#include <algorithm>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/type_traits/is_complex.hpp>
#include <cmath>
#include <cstdarg>
#include <initializer_list>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "detail/base_term_fwd.hpp"
#include "detail/integer_fwd.hpp"
#include "detail/math_tt_fwd.hpp"
#include "detail/sfinae_types.hpp"
#include "exceptions.hpp"
#include "symbol.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

// Implementation of exponentiation with floating-point base.
template <typename T, typename U>
inline auto float_pow_impl(const T &x, const U &n, typename std::enable_if<
	std::is_integral<U>::value>::type * = nullptr) -> decltype(std::pow(x,boost::numeric_cast<int>(n)))
{
	return std::pow(x,boost::numeric_cast<int>(n));
}

template <typename T, typename U>
inline auto float_pow_impl(const T &x, const U &n, typename std::enable_if<
	std::is_same<integer,U>::value>::type * = nullptr) -> decltype(std::pow(x,static_cast<int>(n)))
{
	return std::pow(x,static_cast<int>(n));
}

template <typename T, typename U>
inline auto float_pow_impl(const T &x, const U &y, typename std::enable_if<
	std::is_floating_point<U>::value>::type * = nullptr) -> decltype(std::pow(x,y))
{
	return std::pow(x,y);
}

}

/// Math namespace.
/**
 * Namespace for general-purpose mathematical functions.
 */
namespace math
{

/// Default functor for the implementation of piranha::math::is_zero().
/**
 * This functor should be specialised via the \p std::enable_if mechanism. The default implementation defines a call
 * operator which is enabled only if the argument type is constructible from the C++ \p int type and \p T is equality comparable.
 */
template <typename T, typename = void>
struct is_zero_impl
{
	/// Call operator.
	/**
	 * The operator will compare \p x to an instance of \p U constructed from the literal 0. If \p U cannot
	 * be constructed from the literal 0 or \p U is not equality comparable, the operator will be disabled.
	 * 
	 * @param[in] x argument to be tested.
	 * 
	 * @return \p true if \p x is zero, \p false otherwise.
	 */
	template <typename U>
	bool operator()(const U &x, typename std::enable_if<std::is_constructible<U,int>::value &&
		is_equality_comparable<U>::value>::type * = nullptr) const
	{
		// NOTE: construct instance from integral constant 0.
		// http://groups.google.com/group/comp.lang.c++.moderated/msg/328440a86dae8088?dmode=source
		return x == U(0);
	}
};

/// Specialisation of the piranha::math::is_zero() functor for C++ complex types.
template <typename T>
struct is_zero_impl<T,typename std::enable_if<boost::is_complex<T>::value>::type>
{
	/// Call operator.
	/**
	 * The operator will test separately the real and imaginary parts of the complex argument.
	 * 
	 * @param[in] c argument to be tested.
	 * 
	 * @return \p true if \p c is zero, \p false otherwise.
	 */
	bool operator()(const T &c) const
	{
		return is_zero_impl<typename T::value_type>()(c.real()) &&
			is_zero_impl<typename T::value_type>()(c.imag());
	}
};

/// Zero test.
/**
 * Test if value is zero. The actual implementation of this function is in the piranha::math::is_zero_impl functor's
 * call operator.
 * 
 * @param[in] x value to be tested.
 * 
 * @return \p true if value is zero, \p false otherwise.
 * 
 * @throws unspecified any exception thrown by the call operator of the piranha::math::is_zero_impl functor.
 */
template <typename T>
inline auto is_zero(const T &x) -> decltype(is_zero_impl<T>()(x))
{
	return is_zero_impl<T>()(x);
}

/// Default functor for the implementation of piranha::math::negate().
/**
 * This functor should be specialised via the \p std::enable_if mechanism. Default implementation will
 * assign to the input value its negation.
 */
template <typename T, typename = void>
struct negate_impl
{
	/// Call operator.
	/**
	 * The body of the operator is equivalent to:
	 * @code
	 * return x = -x;
	 * @endcode
	 * 
	 * @param[in,out] x value to be negated.
	 * 
	 * @return the value returned by the assignment operator of \p x.
	 * 
	 * @throws unspecified any exception resulting from the in-place negation or assignment of \p x.
	 */
	template <typename U>
	auto operator()(U &x) const -> decltype(x = -x)
	{
		return x = -x;
	}
};

/// In-place negation.
/**
 * Negate value in-place. The actual implementation of this function is in the piranha::math::negate_impl functor's
 * call operator.
 * 
 * @param[in,out] x value to be negated.
 * 
 * @return the value returned by the call operator of piranha::math::negate_impl.
 * 
 * @throws unspecified any exception thrown by the call operator of piranha::math::negate_impl.
 */
template <typename T>
inline auto negate(T &x) -> decltype(negate_impl<T>()(x))
{
	return negate_impl<T>()(x);
}

/// Default functor for the implementation of piranha::math::multiply_accumulate().
/**
 * This functor should be specialised via the \p std::enable_if mechanism.
 */
template <typename T, typename U, typename V, typename = void>
struct multiply_accumulate_impl
{
	/// Call operator.
	/**
	 * The body of the operator is equivalent to:
	 * @code
	 * x += y * z;
	 * @endcode
	 * where \p y and \p z are perfectly forwarded.
	 * 
	 * @param[in,out] x target value for accumulation.
	 * @param[in] y first argument.
	 * @param[in] z second argument.
	 * 
	 * @return <tt>x += y * z</tt>.
	 * 
	 * @throws unspecified any exception resulting from in-place addition or binary multiplication on the operands.
	 */
	template <typename T2, typename U2, typename V2>
	auto operator()(T2 &x, U2 &&y, V2 &&z) const -> decltype(x += std::forward<U2>(y) * std::forward<V2>(z))
	{
		return x += std::forward<U2>(y) * std::forward<V2>(z);
	}
};

#if defined(FP_FAST_FMA) && defined(FP_FAST_FMAF) && defined(FP_FAST_FMAL)

/// Specialisation of the implementation of piranha::math::multiply_accumulate() for floating-point types.
/**
 * This functor is enabled only if the macros \p FP_FAST_FMA, \p FP_FAST_FMAF and \p FP_FAST_FMAL are defined.
 */
template <typename T>
struct multiply_accumulate_impl<T,T,T,typename std::enable_if<std::is_floating_point<T>::value>::type>
{
	/// Call operator.
	/**
	 * This implementation will use the \p std::fma function.
	 * 
	 * @param[in,out] x target value for accumulation.
	 * @param[in] y first argument.
	 * @param[in] z second argument.
	 * 
	 * @return <tt>x = std::fma(y,z,x)</tt>.
	 */
	template <typename U>
	auto operator()(U &x, const U &y, const U &z) const -> decltype(x = std::fma(y,z,x))
	{
		return x = std::fma(y,z,x);
	}
};

#endif

/// Multiply-accumulate.
/**
 * Will set \p x to <tt>x + y * z</tt>. The actual implementation of this function is in the piranha::math::multiply_accumulate_impl functor's
 * call operator.
 * 
 * @param[in,out] x target value for accumulation.
 * @param[in] y first argument.
 * @param[in] z second argument.
 * 
 * @returns the return value of the call operator of piranha::math::multiply_accumulate_impl.
 * 
 * @throws unspecified any exception thrown by the call operator of piranha::math::multiply_accumulate_impl.
 */
template <typename T, typename U, typename V>
inline auto multiply_accumulate(T &x, U &&y, V &&z) -> decltype(multiply_accumulate_impl<typename std::decay<T>::type,
		typename std::decay<U>::type,typename std::decay<V>::type>()(x,std::forward<U>(y),std::forward<V>(z)))
{
	return multiply_accumulate_impl<typename std::decay<T>::type,
		typename std::decay<U>::type,typename std::decay<V>::type>()(x,std::forward<U>(y),std::forward<V>(z));
}

/// Default functor for the implementation of piranha::math::pow().
/**
 * This functor should be specialised via the \p std::enable_if mechanism. Default implementation will not define
 * the call operator, and will hence result in a compilation error when used.
 */
template <typename T, typename U, typename Enable = void>
struct pow_impl
{};

/// Specialisation of the piranha::math::pow() functor for floating-point bases.
/**
 * This specialisation is activated when \p T is a floating-point type and \p U is either a floating-point type,
 * an integral type or piranha::integer. The result will be computed via the standard <tt>std::pow()</tt> function.
 */
template <typename T, typename U>
struct pow_impl<T,U,typename std::enable_if<std::is_floating_point<T>::value &&
	(std::is_floating_point<U>::value || std::is_integral<U>::value || std::is_same<U,integer>::value)>::type>
{
	/// Call operator.
	/**
	 * The exponentiation will be computed via <tt>std::pow()</tt>. In case \p U2 is an integral type or piranha::integer,
	 * \p y will be converted to \p int via <tt>boost::numeric_cast()</tt> or <tt>static_cast()</tt>.
	 * 
	 * @param[in] x base.
	 * @param[in] y exponent.
	 * 
	 * @return \p x to the power of \p y.
	 * 
	 * @throws unspecified any exception resulting from numerical conversion failures in <tt>boost::numeric_cast()</tt> or <tt>static_cast()</tt>.
	 */
	auto operator()(const T &x, const U &y) const -> decltype(detail::float_pow_impl(x,y))
	{
		return detail::float_pow_impl(x,y);
	}
};

/// Exponentiation.
/**
 * Return \p x to the power of \p y. The actual implementation of this function is in the piranha::math::pow_impl functor's
 * call operator.
 * 
 * @param[in] x base.
 * @param[in] y exponent.
 * 
 * @return \p x to the power of \p y.
 * 
 * @throws unspecified any exception thrown by the call operator of the piranha::math::pow_impl functor.
 */
// NOTE: here the use of trailing decltype gives a nice and compact compilation error in case the specialisation of the functor
// is missing.
template <typename T, typename U>
inline auto pow(T &&x, U &&y) -> decltype(pow_impl<typename std::decay<T>::type,typename std::decay<U>::type>()(std::forward<T>(x),std::forward<U>(y)))
{
	return pow_impl<typename std::decay<T>::type,typename std::decay<U>::type>()(std::forward<T>(x),std::forward<U>(y));
}

/// Default functor for the implementation of piranha::math::cos().
/**
 * This functor should be specialised via the \p std::enable_if mechanism. Default implementation will not define
 * the call operator, and will hence result in a compilation error when used.
 */
template <typename T, typename Enable = void>
struct cos_impl
{};

/// Specialisation of the piranha::math::cos() functor for floating-point types.
/**
 * This specialisation is activated when \p T is a C++ floating-point type.
 * The result will be computed via the standard <tt>std::cos()</tt> function.
 */
template <typename T>
struct cos_impl<T,typename std::enable_if<std::is_floating_point<T>::value>::type>
{
	/// Call operator.
	/**
	 * The cosine will be computed via <tt>std::cos()</tt>.
	 * 
	 * @param[in] x argument.
	 * 
	 * @return cosine of \p x.
	 */
	auto operator()(const T &x) const -> decltype(std::cos(x))
	{
		return std::cos(x);
	}
};

/// Cosine.
/**
 * Returns the cosine of \p x. The actual implementation of this function is in the piranha::math::cos_impl functor's
 * call operator.
 * 
 * @param[in] x cosine argument.
 * 
 * @return cosine of \p x.
 * 
 * @throws unspecified any exception thrown by the call operator of the piranha::math::cos_impl functor.
 */
template <typename T>
inline auto cos(const T &x) -> decltype(cos_impl<T>()(x))
{
	return cos_impl<T>()(x);
}

/// Default functor for the implementation of piranha::math::sin().
/**
 * This functor should be specialised via the \p std::enable_if mechanism. Default implementation will not define
 * the call operator, and will hence result in a compilation error when used.
 */
template <typename T, typename Enable = void>
struct sin_impl
{};

/// Specialisation of the piranha::math::sin() functor for floating-point types.
/**
 * This specialisation is activated when \p T is a C++ floating-point type.
 * The result will be computed via the standard <tt>std::sin()</tt> function.
 */
template <typename T>
struct sin_impl<T,typename std::enable_if<std::is_floating_point<T>::value>::type>
{
	/// Call operator.
	/**
	 * The sine will be computed via <tt>std::sin()</tt>.
	 * 
	 * @param[in] x argument.
	 * 
	 * @return sine of \p x.
	 */
	auto operator()(const T &x) const -> decltype(std::sin(x))
	{
		return std::sin(x);
	}
};

/// Sine.
/**
 * Returns the sine of \p x. The actual implementation of this function is in the piranha::math::sin_impl functor's
 * call operator.
 * 
 * @param[in] x sine argument.
 * 
 * @return sine of \p x.
 * 
 * @throws unspecified any exception thrown by the call operator of the piranha::math::sin_impl functor.
 */
template <typename T>
inline auto sin(const T &x) -> decltype(sin_impl<T>()(x))
{
	return sin_impl<T>()(x);
}

/// Default functor for the implementation of piranha::math::partial().
/**
 * This functor should be specialised via the \p std::enable_if mechanism. Default implementation will not define
 * the call operator, and will hence result in a compilation error when used.
 */
template <typename T, typename Enable = void>
struct partial_impl
{};

/// Specialisation of the piranha::math::partial() functor for arithmetic types.
/**
 * This specialisation is activated when \p T is a C++ arithmetic type.
 * The result will be zero.
 */
template <typename T>
struct partial_impl<T,typename std::enable_if<std::is_arithmetic<T>::value>::type>
{
	/// Call operator.
	/**
	 * @return an instance of \p T constructed from zero.
	 */
	T operator()(const T &, const std::string &) const
	{
		return T(0);
	}
};

/// Partial derivative.
/**
 * Return the partial derivative of \p x with respect to the symbolic quantity named \p str. The actual
 * implementation of this function is in the piranha::math::partial_impl functor.
 * 
 * @param[in] x argument for the partial derivative.
 * @param[in] str name of the symbolic quantity with respect to which the derivative will be computed.
 * 
 * @return partial derivative of \p x with respect to \p str.
 * 
 * @throws unspecified any exception thrown by the call operator of piranha::math::partial_impl.
 */
template <typename T>
inline auto partial(const T &x, const std::string &str) -> decltype(partial_impl<T>()(x,str))
{
	return partial_impl<T>()(x,str);
}

/// Default functor for the implementation of piranha::math::integrate().
/**
 * This functor should be specialised via the \p std::enable_if mechanism. Default implementation will not define
 * the call operator, and will hence result in a compilation error when used.
 */
template <typename T, typename Enable = void>
struct integrate_impl
{};

/// Integration.
/**
 * Return the antiderivative of \p x with respect to the symbolic quantity named \p str. The actual
 * implementation of this function is in the piranha::math::integrate_impl functor.
 * 
 * @param[in] x argument for the integration.
 * @param[in] str name of the symbolic quantity with respect to which the integration will be computed.
 * 
 * @return antiderivative of \p x with respect to \p str.
 * 
 * @throws unspecified any exception thrown by the call operator of piranha::math::integrate_impl.
 */
template <typename T>
inline auto integrate(const T &x, const std::string &str) -> decltype(integrate_impl<T>()(x,str))
{
	return integrate_impl<T>()(x,str);
}

/// Default functor for the implementation of piranha::math::evaluate().
/**
 * This functor should be specialised via the \p std::enable_if mechanism. Default implementation will not define
 * the call operator, and will hence result in a compilation error when used.
 */
template <typename T, typename Enable = void>
struct evaluate_impl
{};

/// Specialisation of the piranha::math::evaluate() functor for arithmetic types.
/**
 * This specialisation is activated when \p T is a C++ arithmetic type.
 * The result will be the input value unchanged.
 */
template <typename T>
struct evaluate_impl<T,typename std::enable_if<std::is_arithmetic<T>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] x evaluation argument.
	 * 
	 * @return copy of \p x.
	 */
	template <typename U>
	T operator()(const T &x, const std::unordered_map<std::string,U> &) const
	{
		return x;
	}
};

/// Evaluation.
/**
 * Evaluation is the simultaneous substitution of all symbolic arguments in an expression. The input dictionary \p dict
 * specifies the quantity (value) that will be susbstituted for each argument (key), here represented as a string.
 * 
 * The actual implementation of this function is in the piranha::math::evaluate_impl functor.
 * 
 * @param[in] x quantity that will be evaluated.
 * @param[in] dict dictionary that will be used to perform the substitution.
 * 
 * @return \p x evaluated according to \p dict.
 * 
 * @throws unspecified any exception thrown by the call operator of piranha::math::evaluate_impl.
 */
template <typename T, typename U>
inline auto evaluate(const T &x, const std::unordered_map<std::string,U> &dict) -> decltype(evaluate_impl<T>()(x,dict))
{
	return evaluate_impl<T>()(x,dict);
}

/// Default functor for the implementation of piranha::math::subs().
/**
 * This functor should be specialised via the \p std::enable_if mechanism. Default implementation will not define
 * the call operator, and will hence result in a compilation error when used.
 */
template <typename T, typename Enable = void>
struct subs_impl
{};

/// Specialisation of the piranha::math::subs() functor for arithmetic types.
/**
 * This specialisation is activated when \p T is a C++ arithmetic type.
 * The result will be the input value unchanged.
 */
template <typename T>
struct subs_impl<T,typename std::enable_if<std::is_arithmetic<T>::value>::type>
{
	/// Call operator.
	/**
	 * @param[in] x substitution argument.
	 * 
	 * @return copy of \p x.
	 */
	template <typename U>
	T operator()(const T &x, const std::string &, const U &) const
	{
		return x;
	}
};

/// Substitution.
/**
 * Substitute a symbolic variable with a generic object.
 * The actual implementation of this function is in the piranha::math::subs_impl functor.
 * 
 * @param[in] x quantity that will be subject to substitution.
 * @param[in] name name of the symbolic variable that will be substituted.
 * @param[in] y object that will substitute the variable.
 * 
 * @return \p x after substitution  of \p name with \p y.
 * 
 * @throws unspecified any exception thrown by the call operator of piranha::math::subs_impl.
 */
template <typename T, typename U>
inline auto subs(const T &x, const std::string &name, const U &y) -> decltype(subs_impl<T>()(x,name,y))
{
	return subs_impl<T>()(x,name,y);
}

/// Default functor for the implementation of piranha::math::t_subs().
/**
 * This functor should be specialised via the \p std::enable_if mechanism. Default implementation will not define
 * the call operator, and will hence result in a compilation error when used.
 */
template <typename T, typename = void>
struct t_subs_impl
{};

/// Trigonometric substitution.
/**
 * Substitute the cosine and sine of a symbolic variable with generic objects.
 * The actual implementation of this function is in the piranha::math::t_subs_impl functor.
 * 
 * @param[in] x quantity that will be subject to substitution.
 * @param[in] name name of the symbolic variable that will be substituted.
 * @param[in] c object that will substitute the cosine of the variable.
 * @param[in] s object that will substitute the sine of the variable.
 * 
 * @return \p x after substitution of cosine and sine of \p name with \p c and \p s.
 * 
 * @throws unspecified any exception thrown by the call operator of piranha::math::t_subs_impl.
 */
template <typename T, typename U, typename V>
inline auto t_subs(const T &x, const std::string &name, const U &c, const V &s) -> decltype(t_subs_impl<T>()(x,name,c,s))
{
	return t_subs_impl<T>()(x,name,c,s);
}

/// Default functor for the implementation of piranha::math::abs().
/**
 * This functor should be specialised via the \p std::enable_if mechanism. Default implementation will not define
 * the call operator, and will hence result in a compilation error when used.
 */
template <typename T, typename Enable = void>
struct abs_impl
{};

/// Specialisation of the piranha::math::abs() functor for signed and unsigned integer types, and floating-point types.
/**
 * This specialisation is activated when \p T is a signed or unsigned integer type, or a floating-point type.
 * The result will be computed via \p std::abs() for floating-point and signed integer types,
 * while for unsigned integer types it will be the input value unchanged.
 */
template <typename T>
struct abs_impl<T,typename std::enable_if<std::is_signed<T>::value || std::is_unsigned<T>::value ||
	std::is_floating_point<T>::value>::type>
{
	private:
		template <typename U>
		static U impl(const U &x, typename std::enable_if<std::is_signed<U>::value ||
			std::is_floating_point<U>::value>::type * = nullptr)
		{
			return std::abs(x);
		}
		template <typename U>
		static U impl(const U &x, typename std::enable_if<std::is_unsigned<U>::value>::type * = nullptr)
		{
			return x;
		}
	public:
		/// Call operator.
		/**
		 * @param[in] x input parameter.
		 * 
		 * @return absolute value of \p x.
		 */
		T operator()(const T &x) const
		{
			return impl(x);
		}
};

/// Absolute value.
/**
 * The actual implementation of this function is in the piranha::math::abs_impl functor.
 * 
 * @param[in] x quantity whose absolute value will be calculated.
 * 
 * @return absolute value of \p x.
 * 
 * @throws unspecified any exception thrown by the call operator of piranha::math::abs_impl.
 */
template <typename T>
inline auto abs(const T &x) -> decltype(abs_impl<T>()(x))
{
	return abs_impl<T>()(x);
}

/// Poisson bracket.
/**
 * The Poisson bracket of \p f and \p g with respect to the list of momenta \p p_list and coordinates \p q_list
 * is defined as:
 * \f[
 * \left\{f,g\right\} = \sum_{i=1}^{N}
 * \left[
 * \frac{\partial f}{\partial q_{i}} \frac{\partial g}{\partial p_{i}} -
 * \frac{\partial f}{\partial p_{i}} \frac{\partial g}{\partial q_{i}}
 * \right],
 * \f]
 * where \f$ p_i \f$ and \f$ q_i \f$ are the elements of \p p_list and \p q_list.
 * 
 * @param[in] f first argument.
 * @param[in] g second argument.
 * @param[in] p_list list of the names of momenta.
 * @param[in] q_list list of the names of coordinates.
 * 
 * @return the poisson bracket of \p f and \p g with respect to \p p_list and \p q_list.
 * 
 * @throws std::invalid_argument if the sizes of \p p_list and \p q_list differ or if
 * \p p_list or \p q_list contain duplicate entries.
 * @throws unspecified any exception thrown by piranha::math::partial() or by the arithmetic operators
 * of \p f and \p g.
 */
template <typename T>
inline auto pbracket(const T &f, const T &g, const std::vector<std::string> &p_list,
	const std::vector<std::string> &q_list) -> decltype(partial(f,q_list[0]) * partial(g,p_list[0]))
{
	typedef decltype(partial(f,q_list[0]) * partial(g,p_list[0])) return_type;
	if (p_list.size() != q_list.size()) {
		piranha_throw(std::invalid_argument,"the number of coordinates is different from the number of momenta");
	}
	if (std::unordered_set<std::string>(p_list.begin(),p_list.end()).size() != p_list.size()) {
		piranha_throw(std::invalid_argument,"the list of momenta contains duplicate entries");
	}
	if (std::unordered_set<std::string>(q_list.begin(),q_list.end()).size() != q_list.size()) {
		piranha_throw(std::invalid_argument,"the list of coordinates contains duplicate entries");
	}
	return_type retval = return_type();
	for (decltype(p_list.size()) i = 0u; i < p_list.size(); ++i) {
		// NOTE: could use multadd/sub here, if we implement it for series.
		retval += partial(f,q_list[i]) * partial(g,p_list[i]);
		retval -= partial(f,p_list[i]) * partial(g,q_list[i]);
	}
	return retval;
}

}

namespace detail
{

template <typename T>
static inline bool is_canonical_impl(const std::vector<T const *> &new_p, const std::vector<T const *> &new_q,
	const std::vector<std::string> &p_list, const std::vector<std::string> &q_list)
{
	typedef decltype(math::pbracket(*new_q[0],*new_p[0],p_list,q_list)) p_type;
	if (p_list.size() != q_list.size()) {
		piranha_throw(std::invalid_argument,"the number of coordinates is different from the number of momenta");
	}
	if (new_p.size() != new_q.size()) {
		piranha_throw(std::invalid_argument,"the number of new coordinates is different from the number of new momenta");
	}
	if (p_list.size() != new_p.size()) {
		piranha_throw(std::invalid_argument,"the number of new momenta is different from the number of momenta");
	}
	if (std::unordered_set<std::string>(p_list.begin(),p_list.end()).size() != p_list.size()) {
		piranha_throw(std::invalid_argument,"the list of momenta contains duplicate entries");
	}
	if (std::unordered_set<std::string>(q_list.begin(),q_list.end()).size() != q_list.size()) {
		piranha_throw(std::invalid_argument,"the list of coordinates contains duplicate entries");
	}
	const auto size = new_p.size();
	for (decltype(new_p.size()) i = 0u; i < size; ++i) {
		for (decltype(new_p.size()) j = 0u; j < size; ++j) {
			// NOTE: no need for actually doing computations when i == j.
			if (i != j && !math::is_zero(math::pbracket(*new_p[i],*new_p[j],p_list,q_list))) {
				return false;
			}
			if (i != j && !math::is_zero(math::pbracket(*new_q[i],*new_q[j],p_list,q_list))) {
				return false;
			}
			// Poisson bracket needs to be zero for i != j, one for i == j.
			// NOTE: cast from bool to int is always 0 or 1.
			if (math::pbracket(*new_q[i],*new_p[j],p_list,q_list) != p_type(static_cast<int>(i == j))) {
				return false;
			}
		}
	}
	return true;
}

}

namespace math
{

/// Check if a transformation is canonical.
/**
 * This function will check if a transformation of Hamiltonian momenta and coordinates is canonical using the Poisson bracket test.
 * The transformation is expressed as two separate collections of objects, \p new_p and \p new_q, representing the new momenta
 * and coordinates as functions of the old momenta \p p_list and \p q_list.
 * 
 * This function requires type \p T to be suitable for use in in piranha::math::pbracket() and piranha::math::is_zero(), and to be constructible
 * from \p int and equality comparable.
 * 
 * @param[in] new_p list of objects representing the new momenta.
 * @param[in] new_q list of objects representing the new coordinates.
 * @param[in] p_list list of names of the old momenta.
 * @param[in] q_list list of names of the old coordinates.
 * 
 * @return \p true if the transformation is canonical, \p false otherwise.
 * 
 * @throws std::invalid_argument if the sizes of the four input arguments are not the same or if either \p p_list or \p q_list
 * contain duplicate entries.
 * @throws unspecified any exception thrown by:
 * - piranha::math::pbracket(),
 * - construction and comparison of objects of the type returned by piranha::math::pbracket(),
 * - piranha::math::is_zero(),
 * - memory errors in standard containers.
 */
template <typename T>
inline bool transformation_is_canonical(const std::vector<T> &new_p, const std::vector<T> &new_q,
	const std::vector<std::string> &p_list, const std::vector<std::string> &q_list)
{
	std::vector<T const *> pv, qv;
	std::transform(new_p.begin(),new_p.end(),std::back_inserter(pv),[](const T &p) {return &p;});
	std::transform(new_q.begin(),new_q.end(),std::back_inserter(qv),[](const T &q) {return &q;});
	return detail::is_canonical_impl(pv,qv,p_list,q_list);
}

/// Check if a transformation is canonical (alternative overload).
template <typename T>
inline bool transformation_is_canonical(std::initializer_list<T> new_p, std::initializer_list<T> new_q,
	const std::vector<std::string> &p_list, const std::vector<std::string> &q_list)
{
	std::vector<T const *> pv, qv;
	std::transform(new_p.begin(),new_p.end(),std::back_inserter(pv),[](const T &p) {return &p;});
	std::transform(new_q.begin(),new_q.end(),std::back_inserter(qv),[](const T &q) {return &q;});
	return detail::is_canonical_impl(pv,qv,p_list,q_list);
}

/// Default functor for the implementation of piranha::math::t_degree().
/**
 * This functor should be specialised via the \p std::enable_if mechanism. Default implementation will not define
 * the call operator, and will hence result in a compilation error when used.
 * 
 * Note that the implementation of this functor requires two overloaded call operators, one for the unary form
 * of piranha::math::t_degree() (the total trigonometric degree), the other for the binary form of piranha::math::t_degree()
 * (the partial trigonometric degree).
 */
template <typename T, typename Enable = void>
struct t_degree_impl
{};

/// Total trigonometric degree.
/**
 * A type exposing a trigonometric degree property, in analogy with the concept of polynomial degree,
 * should be a linear combination of real or complex trigonometric functions. For instance, the Poisson series
 * \f[
 * 2\cos\left(3x+y\right) + 3\cos\left(2x-y\right)
 * \f]
 * has a trigonometric degree of 3+1=4.
 * 
 * The actual implementation of this function is in the piranha::math::t_degree_impl functor.
 * 
 * @param[in] x object whose trigonometric degree will be computed.
 * 
 * @return total trigonometric degree.
 * 
 * @throws unspecified any exception thrown by the call operator of piranha::math::t_degree_impl.
 */
template <typename T>
inline auto t_degree(const T &x) -> decltype(t_degree_impl<T>()(x))
{
	return t_degree_impl<T>()(x);
}

/// Partial trigonometric degree.
/**
 * The partial trigonometric degree is the trigonometric degree when only certain variables are considered in
 * the computation.
 * 
 * The actual implementation of this function is in the piranha::math::t_degree_impl functor.
 * 
 * @param[in] x object whose trigonometric degree will be computed.
 * @param[in] names names of the variables that will be considered in the computation of the degree.
 * 
 * @return partial trigonometric degree.
 * 
 * @throws unspecified any exception thrown by the call operator of piranha::math::t_degree_impl.
 * 
 * @see piranha::math::t_degree().
 */
template <typename T>
inline auto t_degree(const T &x, const std::set<std::string> &names) -> decltype(t_degree_impl<T>()(x,names))
{
	return t_degree_impl<T>()(x,names);
}

/// Default functor for the implementation of piranha::math::t_ldegree().
/**
 * This functor should be specialised via the \p std::enable_if mechanism. Default implementation will not define
 * the call operator, and will hence result in a compilation error when used.
 * 
 * Note that the implementation of this functor requires two overloaded call operators, one for the unary form
 * of piranha::math::t_ldegree() (the total trigonometric low degree), the other for the binary form of piranha::math::t_ldegree()
 * (the partial trigonometric low degree).
 */
template <typename T, typename Enable = void>
struct t_ldegree_impl
{};

/// Total trigonometric low degree.
/**
 * A type exposing a trigonometric low degree property, in analogy with the concept of polynomial low degree,
 * should be a linear combination of real or complex trigonometric functions. For instance, the Poisson series
 * \f[
 * 2\cos\left(3x+y\right) + 3\cos\left(2x-y\right)
 * \f]
 * has a trigonometric low degree of 2-1=1.
 * 
 * The actual implementation of this function is in the piranha::math::t_ldegree_impl functor.
 * 
 * @param[in] x object whose trigonometric low degree will be computed.
 * 
 * @return total trigonometric low degree.
 * 
 * @throws unspecified any exception thrown by the call operator of piranha::math::t_ldegree_impl.
 */
template <typename T>
inline auto t_ldegree(const T &x) -> decltype(t_ldegree_impl<T>()(x))
{
	return t_ldegree_impl<T>()(x);
}

/// Partial trigonometric low degree.
/**
 * The partial trigonometric low degree is the trigonometric low degree when only certain variables are considered in
 * the computation.
 * 
 * The actual implementation of this function is in the piranha::math::t_ldegree_impl functor.
 * 
 * @param[in] x object whose trigonometric low degree will be computed.
 * @param[in] names names of the variables that will be considered in the computation of the degree.
 * 
 * @return partial trigonometric low degree.
 * 
 * @throws unspecified any exception thrown by the call operator of piranha::math::t_ldegree_impl.
 * 
 * @see piranha::math::t_ldegree().
 */
template <typename T>
inline auto t_ldegree(const T &x, const std::set<std::string> &names) -> decltype(t_ldegree_impl<T>()(x,names))
{
	return t_ldegree_impl<T>()(x,names);
}

/// Default functor for the implementation of piranha::math::t_order().
/**
 * This functor should be specialised via the \p std::enable_if mechanism. Default implementation will not define
 * the call operator, and will hence result in a compilation error when used.
 * 
 * Note that the implementation of this functor requires two overloaded call operators, one for the unary form
 * of piranha::math::t_order() (the total trigonometric order), the other for the binary form of piranha::math::t_order()
 * (the partial trigonometric order).
 */
template <typename T, typename Enable = void>
struct t_order_impl
{};

/// Total trigonometric order.
/**
 * A type exposing a trigonometric order property should be a linear combination of real or complex trigonometric functions.
 * The order is computed in a way similar to the trigonometric degree, with the key difference that the absolute values of
 * the trigonometric degrees of each variable are considered in the computation. For instance, the Poisson series
 * \f[
 * 2\cos\left(3x+y\right) + 3\cos\left(2x-y\right)
 * \f]
 * has a trigonometric order of abs(3)+abs(1)=4.
 * 
 * The actual implementation of this function is in the piranha::math::t_order_impl functor.
 * 
 * @param[in] x object whose trigonometric order will be computed.
 * 
 * @return total trigonometric order.
 * 
 * @throws unspecified any exception thrown by the call operator of piranha::math::t_order_impl.
 */
template <typename T>
inline auto t_order(const T &x) -> decltype(t_order_impl<T>()(x))
{
	return t_order_impl<T>()(x);
}

/// Partial trigonometric order.
/**
 * The partial trigonometric order is the trigonometric order when only certain variables are considered in
 * the computation.
 * 
 * The actual implementation of this function is in the piranha::math::t_order_impl functor.
 * 
 * @param[in] x object whose trigonometric order will be computed.
 * @param[in] names names of the variables that will be considered in the computation of the order.
 * 
 * @return partial trigonometric order.
 * 
 * @throws unspecified any exception thrown by the call operator of piranha::math::t_order_impl.
 * 
 * @see piranha::math::t_order().
 */
template <typename T>
inline auto t_order(const T &x, const std::set<std::string> &names) -> decltype(t_order_impl<T>()(x,names))
{
	return t_order_impl<T>()(x,names);
}

/// Default functor for the implementation of piranha::math::t_lorder().
/**
 * This functor should be specialised via the \p std::enable_if mechanism. Default implementation will not define
 * the call operator, and will hence result in a compilation error when used.
 * 
 * Note that the implementation of this functor requires two overloaded call operators, one for the unary form
 * of piranha::math::t_lorder() (the total trigonometric low order), the other for the binary form of piranha::math::t_lorder()
 * (the partial trigonometric low order).
 */
template <typename T, typename Enable = void>
struct t_lorder_impl
{};

/// Total trigonometric low order.
/**
 * A type exposing a trigonometric low order property should be a linear combination of real or complex trigonometric functions.
 * The low order is computed in a way similar to the trigonometric low degree, with the key difference that the absolute values of
 * the trigonometric degrees of each variable are considered in the computation. For instance, the Poisson series
 * \f[
 * 2\cos\left(3x+y\right) + 3\cos\left(2x-y\right)
 * \f]
 * has a trigonometric low order of abs(2)+abs(1)=3.
 * 
 * The actual implementation of this function is in the piranha::math::t_lorder_impl functor.
 * 
 * @param[in] x object whose trigonometric low order will be computed.
 * 
 * @return total trigonometric low order.
 * 
 * @throws unspecified any exception thrown by the call operator of piranha::math::t_lorder_impl.
 */
template <typename T>
inline auto t_lorder(const T &x) -> decltype(t_lorder_impl<T>()(x))
{
	return t_lorder_impl<T>()(x);
}

/// Partial trigonometric low order.
/**
 * The partial trigonometric low order is the trigonometric low order when only certain variables are considered in
 * the computation.
 * 
 * The actual implementation of this function is in the piranha::math::t_lorder_impl functor.
 * 
 * @param[in] x object whose trigonometric low order will be computed.
 * @param[in] names names of the variables that will be considered in the computation of the order.
 * 
 * @return partial trigonometric low order.
 * 
 * @throws unspecified any exception thrown by the call operator of piranha::math::t_lorder_impl.
 * 
 * @see piranha::math::t_lorder().
 */
template <typename T>
inline auto t_lorder(const T &x, const std::set<std::string> &names) -> decltype(t_lorder_impl<T>()(x,names))
{
	return t_lorder_impl<T>()(x,names);
}

}

namespace detail
{

// Generic binomial implementation.
template <typename T>
static inline bool generic_binomial_check_k(const T &, const T &,
	typename std::enable_if<std::is_unsigned<T>::value>::type * = nullptr)
{
	return false;
}

template <typename T>
static inline bool generic_binomial_check_k(const T &k, const T &zero,
	typename std::enable_if<!std::is_unsigned<T>::value>::type * = nullptr)
{
	return k < zero;
}

template <typename T, typename U>
inline T generic_binomial(const T &x, const U &k)
{
	static_assert(std::is_integral<U>::value || std::is_same<integer,U>::value,"Invalid type.");
	const U zero(0), one(1);
	if (generic_binomial_check_k(k,zero)) {
		piranha_throw(std::invalid_argument,"negative k value in binomial coefficient");
	}
	// Zero at bottom results always in 1.
	if (k == zero) {
		return T(1);
	}
	T tmp(x), retval = x / k;
	--tmp;
	for (U i(k - one); i >= one; --i, --tmp) {
		retval *= tmp;
		retval /= i;
	}
	return retval;
}

}

namespace math
{

/// Default functor for the implementation of piranha::math::binomial().
/**
 * This functor should be specialised via the \p std::enable_if mechanism. Default implementation will not define
 * the call operator, and will hence result in a compilation error when used.
 */
template <typename T, typename U, typename = void>
struct binomial_impl
{};

/// Specialisation of the piranha::math::binomial() functor for floating-point top arguments.
/**
 * This specialisation is activated when \p T is a floating-point type and \p U an integral type or piranha::integer.
 */
template <typename T, typename U>
struct binomial_impl<T,U,typename std::enable_if<std::is_floating_point<T>::value &&
	(std::is_integral<U>::value || std::is_same<integer,U>::value)
	>::type>
{
	/// Call operator.
	/**
	 * @param[in] x top argument.
	 * @param[in] k bottom argument.
	 * 
	 * @return \p x choose \p k.
	 * 
	 * @throws std::invalid_argument if \p k is negative.
	 * @throws unspecified any exception resulting from arithmetic operations involving piranha::integer.
	 */
	T operator()(const T &x, const U &k) const
	{
		return detail::generic_binomial(x,k);
	}
};

/// Generalised binomial coefficient.
/**
 * Will return the generalised binomial coefficient:
 * \f[
 * {x \choose k} = \frac{x^{\underline k}}{k!} = \frac{x(x-1)(x-2)\cdots(x-k+1)}{k(k-1)(k-2)\cdots 1}.
 * \f]
 * 
 * The actual implementation of this function is in the piranha::math::binomial_impl functor.
 * 
 * @param[in] x top number.
 * @param[in] k bottom number.
 * 
 * @return \p x choose \p k.
 * 
 * @throws unspecified any exception thrown by the call operator of piranha::math::binomial_impl.
 */
template <typename T, typename U>
inline auto binomial(const T &x, const U &k) -> decltype(binomial_impl<T,U>()(x,k))
{
	return binomial_impl<T,U>()(x,k);
}

}

/// Type trait for differentiable types.
/**
 * The type trait will be \p true if piranha::math::partial() can be successfully called on instances of
 * type \p T, \p false otherwise.
 */
template <typename T>
class is_differentiable: detail::sfinae_types
{
		template <typename U>
		static auto test(const U &u) -> decltype(math::partial(u,""),void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test(std::declval<T>())),yes>::value;
};

// Static init.
template <typename T>
const bool is_differentiable<T>::value;

/// Type trait for differentiable term types.
/**
 * A term is differentiable if it is provided with a const <tt>partial()</tt> method accepting a const
 * reference to piranha::symbol and a const reference to piranha::symbol_set as arguments and returning
 * an <tt>std::vector</tt> of \p T as result.
 * 
 * \p T must satisfy piranha::is_term.
 */
template <typename T>
class term_is_differentiable: detail::sfinae_types
{
		PIRANHA_TT_CHECK(is_term,T);
		template <typename U>
		static auto test(const U &u) -> decltype(u.partial(std::declval<const symbol &>(),
			std::declval<const symbol_set &>()));
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test(std::declval<T>())),std::vector<T>>::value;
};

template <typename T>
const bool term_is_differentiable<T>::value;

/// Type trait for integrable types.
/**
 * The type trait will be \p true if piranha::math::integrate() can be successfully called on instances of
 * type \p T, \p false otherwise.
 */
template <typename T>
class is_integrable: detail::sfinae_types
{
		template <typename U>
		static auto test(const U &u) -> decltype(math::integrate(u,""),void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test(std::declval<T>())),yes>::value;
};

// Static init.
template <typename T>
const bool is_integrable<T>::value;

/// Type trait for exponentiable types.
/**
 * The type trait will be \p true if piranha::math::pow() can be successfully called with base \p T and
 * exponent \p U.
 * 
 * The call to piranha::math::pow() will be tested with const reference arguments.
 */
template <typename T, typename U>
class is_exponentiable: detail::sfinae_types
{
		template <typename Base, typename Expo>
		static auto test(const Base &b, const Expo &e) -> decltype(math::pow(b,e),void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test(std::declval<T>(),std::declval<U>())),yes>::value;
};

// Static init.
template <typename T, typename U>
const bool is_exponentiable<T,U>::value;

/// Type trait to detect if type has a trigonometric degree property.
/**
 * The type trait will be true if instances of type \p T can be used as arguments of piranha::math::t_degree()
 * (both in the unary and binary version of the function).
 */
template <typename T>
class has_t_degree: detail::sfinae_types
{
		template <typename U>
		static auto test1(const U &u) -> decltype(math::t_degree(u),void(),yes());
		static no test1(...);
		template <typename U>
		static auto test2(const U &u) -> decltype(math::t_degree(u,std::declval<const std::set<std::string> &>()),void(),yes());
		static no test2(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test1(std::declval<T>())),yes>::value &&
					  std::is_same<decltype(test2(std::declval<T>())),yes>::value;
};

// Static init.
template <typename T>
const bool has_t_degree<T>::value;

/// Type trait to detect if type has a trigonometric low degree property.
/**
 * The type trait will be true if instances of type \p T can be used as arguments of piranha::math::t_ldegree()
 * (both in the unary and binary version of the function).
 */
template <typename T>
class has_t_ldegree: detail::sfinae_types
{
		template <typename U>
		static auto test1(const U &u) -> decltype(math::t_ldegree(u),void(),yes());
		static no test1(...);
		template <typename U>
		static auto test2(const U &u) -> decltype(math::t_ldegree(u,std::declval<const std::set<std::string> &>()),void(),yes());
		static no test2(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test1(std::declval<T>())),yes>::value &&
					  std::is_same<decltype(test2(std::declval<T>())),yes>::value;
};

// Static init.
template <typename T>
const bool has_t_ldegree<T>::value;

/// Type trait to detect if type has a trigonometric order property.
/**
 * The type trait will be true if instances of type \p T can be used as arguments of piranha::math::t_order()
 * (both in the unary and binary version of the function).
 */
template <typename T>
class has_t_order: detail::sfinae_types
{
		template <typename U>
		static auto test1(const U &u) -> decltype(math::t_order(u),void(),yes());
		static no test1(...);
		template <typename U>
		static auto test2(const U &u) -> decltype(math::t_order(u,std::declval<const std::set<std::string> &>()),void(),yes());
		static no test2(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test1(std::declval<T>())),yes>::value &&
					  std::is_same<decltype(test2(std::declval<T>())),yes>::value;
};

// Static init.
template <typename T>
const bool has_t_order<T>::value;

/// Type trait to detect if type has a trigonometric low order property.
/**
 * The type trait will be true if instances of type \p T can be used as arguments of piranha::math::t_lorder()
 * (both in the unary and binary version of the function).
 */
template <typename T>
class has_t_lorder: detail::sfinae_types
{
		template <typename U>
		static auto test1(const U &u) -> decltype(math::t_lorder(u),void(),yes());
		static no test1(...);
		template <typename U>
		static auto test2(const U &u) -> decltype(math::t_lorder(u,std::declval<const std::set<std::string> &>()),void(),yes());
		static no test2(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test1(std::declval<T>())),yes>::value &&
					  std::is_same<decltype(test2(std::declval<T>())),yes>::value;
};

// Static init.
template <typename T>
const bool has_t_lorder<T>::value;

/// Type trait to detect if a key type has a degree property.
/**
 * The type trait has the same meaning as piranha::has_degree, but it's meant for use with key types.
 * It will test the presence of two <tt>degree()</tt> const methods, the first one accepting a const instance of
 * piranha::symbol_set, the second one a const instance of <tt>std::set<std::string></tt> and a const instance of piranha::symbol_set.
 * 
 * \p Key must satisfy piranha::is_key.
 * 
 * \todo check docs once we implement piranha::has_degree.
 */
template <typename Key>
class key_has_degree: detail::sfinae_types
{
		PIRANHA_TT_CHECK(is_key,Key);
		// NOTE: here it works (despite the difference in constness with the use below) because none of the two versions
		// of test1() is an exact match, and the conversion in constness has a higher priority than the ellipsis conversion.
		// For more info:
		// http://cpp0x.centaur.ath.cx/over.ics.rank.html
		template <typename T>
		static auto test1(const T *t) -> decltype(t->degree(std::declval<const symbol_set &>()),void(),yes());
		static no test1(...);
		template <typename T>
		static auto test2(const T *t) -> decltype(t->degree(std::declval<const std::set<std::string> &>(),std::declval<const symbol_set &>()),void(),yes());
		static no test2(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test1((Key *)nullptr)),yes>::value &&
					  std::is_same<decltype(test2((Key *)nullptr)),yes>::value;
};

template <typename Key>
const bool key_has_degree<Key>::value;

/// Type trait to detect if a key type has a low degree property.
/**
 * The type trait has the same meaning as piranha::has_ldegree, but it's meant for use with key types.
 * It will test the presence of two <tt>ldegree()</tt> const methods, the first one accepting a const instance of
 * piranha::symbol_set, the second one a const instance of <tt>std::set<std::string></tt> and a const instance of piranha::symbol_set.
 * 
 * \p Key must satisfy piranha::is_key.
 */
template <typename Key>
class key_has_ldegree: detail::sfinae_types
{
		PIRANHA_TT_CHECK(is_key,Key);
		template <typename T>
		static auto test1(const T *t) -> decltype(t->ldegree(std::declval<const symbol_set &>()),void(),yes());
		static no test1(...);
		template <typename T>
		static auto test2(const T *t) -> decltype(t->ldegree(std::declval<const std::set<std::string> &>(),std::declval<const symbol_set &>()),void(),yes());
		static no test2(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test1((Key *)nullptr)),yes>::value &&
					  std::is_same<decltype(test2((Key *)nullptr)),yes>::value;
};

template <typename Key>
const bool key_has_ldegree<Key>::value;

/// Type trait to detect if a key type has a trigonometric degree property.
/**
 * The type trait has the same meaning as piranha::has_t_degree, but it's meant for use with key types.
 * It will test the presence of two <tt>t_degree()</tt> const methods, the first one accepting a const instance of
 * piranha::symbol_set, the second one a const instance of <tt>std::set<std::string></tt> and a const instance of piranha::symbol_set.
 * 
 * \p Key must satisfy piranha::is_key.
 */
template <typename Key>
class key_has_t_degree: detail::sfinae_types
{
		PIRANHA_TT_CHECK(is_key,Key);
		template <typename T>
		static auto test1(T const *t) -> decltype(t->t_degree(std::declval<const symbol_set &>()),void(),yes());
		static no test1(...);
		template <typename T>
		static auto test2(T const *t) -> decltype(t->t_degree(std::declval<const std::set<std::string> &>(),std::declval<const symbol_set &>()),void(),yes());
		static no test2(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test1((Key const *)nullptr)),yes>::value &&
					  std::is_same<decltype(test2((Key const *)nullptr)),yes>::value;
};

// Static init.
template <typename T>
const bool key_has_t_degree<T>::value;

/// Type trait to detect if a key type has a trigonometric low degree property.
/**
 * The type trait has the same meaning as piranha::has_t_ldegree, but it's meant for use with key types.
 * It will test the presence of two <tt>t_ldegree()</tt> const methods, the first one accepting a const instance of
 * piranha::symbol_set, the second one a const instance of <tt>std::set<std::string></tt> and a const instance of piranha::symbol_set.
 * 
 * \p Key must satisfy piranha::is_key.
 */
template <typename Key>
class key_has_t_ldegree: detail::sfinae_types
{
		PIRANHA_TT_CHECK(is_key,Key);
		template <typename T>
		static auto test1(T const *t) -> decltype(t->t_ldegree(std::declval<const symbol_set &>()),void(),yes());
		static no test1(...);
		template <typename T>
		static auto test2(T const *t) -> decltype(t->t_ldegree(std::declval<const std::set<std::string> &>(),std::declval<const symbol_set &>()),void(),yes());
		static no test2(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test1((Key const *)nullptr)),yes>::value &&
					  std::is_same<decltype(test2((Key const *)nullptr)),yes>::value;
};

// Static init.
template <typename T>
const bool key_has_t_ldegree<T>::value;

/// Type trait to detect if a key type has a trigonometric order property.
/**
 * The type trait has the same meaning as piranha::has_t_order, but it's meant for use with key types.
 * It will test the presence of two <tt>t_order()</tt> const methods, the first one accepting a const instance of
 * piranha::symbol_set, the second one a const instance of <tt>std::set<std::string></tt> and a const instance of piranha::symbol_set.
 * 
 * \p Key must satisfy piranha::is_key.
 */
template <typename Key>
class key_has_t_order: detail::sfinae_types
{
		PIRANHA_TT_CHECK(is_key,Key);
		template <typename T>
		static auto test1(T const *t) -> decltype(t->t_order(std::declval<const symbol_set &>()),void(),yes());
		static no test1(...);
		template <typename T>
		static auto test2(T const *t) -> decltype(t->t_order(std::declval<const std::set<std::string> &>(),std::declval<const symbol_set &>()),void(),yes());
		static no test2(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test1((Key const *)nullptr)),yes>::value &&
					  std::is_same<decltype(test2((Key const *)nullptr)),yes>::value;
};

// Static init.
template <typename T>
const bool key_has_t_order<T>::value;

/// Type trait to detect if a key type has a trigonometric low order property.
/**
 * The type trait has the same meaning as piranha::has_t_lorder, but it's meant for use with key types.
 * It will test the presence of two <tt>t_lorder()</tt> const methods, the first one accepting a const instance of
 * piranha::symbol_set, the second one a const instance of <tt>std::set<std::string></tt> and a const instance of piranha::symbol_set.
 * 
 * \p Key must satisfy piranha::is_key.
 */
template <typename Key>
class key_has_t_lorder: detail::sfinae_types
{
		PIRANHA_TT_CHECK(is_key,Key);
		template <typename T>
		static auto test1(T const *t) -> decltype(t->t_lorder(std::declval<const symbol_set &>()),void(),yes());
		static no test1(...);
		template <typename T>
		static auto test2(T const *t) -> decltype(t->t_lorder(std::declval<const std::set<std::string> &>(),std::declval<const symbol_set &>()),void(),yes());
		static no test2(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test1((Key const *)nullptr)),yes>::value &&
					  std::is_same<decltype(test2((Key const *)nullptr)),yes>::value;
};

// Static init.
template <typename T>
const bool key_has_t_lorder<T>::value;

/// Type trait to detect the presence of the piranha::math::binomial function.
/**
 * The type trait will be \p true if piranha::math::binomial can be successfully called on instances of \p T and \p U respectively,
 * \p false otherwise.
 */
template <typename T, typename U>
class has_binomial: detail::sfinae_types
{
		template <typename T1, typename U1>
		static auto test(T1 const *t, U1 const *u) -> decltype(math::binomial(*t,*u),void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test((T const *)nullptr,(U const *)nullptr)),yes>::value;
};

// Static init.
template <typename T, typename U>
const bool has_binomial<T,U>::value;

/// Type trait to detect the presence of the piranha::math::is_zero function.
/**
 * The type trait will be \p true if piranha::math::is_zero can be successfully called on instances of \p T, returning
 * an instance of a type implicitly convertible \p bool.
 */
template <typename T>
class has_is_zero: detail::sfinae_types
{
		typedef typename std::decay<T>::type Td;
		template <typename T1>
		static auto test(const T1 &t) -> decltype(math::is_zero(t));
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_convertible<decltype(test(std::declval<Td>())),bool>::value;
};

// Static init.
template <typename T>
const bool has_is_zero<T>::value;

/// Type trait to detect the presence of the piranha::math::negate function.
/**
 * The type trait will be \p true if piranha::math::negate can be successfully called on instances of \p T
 * stripped of reference qualifiers, \p false otherwise.
 */
template <typename T>
class has_negate: detail::sfinae_types
{
		typedef typename std::remove_reference<T>::type Td;
		template <typename T1>
		static auto test(T1 &t) -> decltype(math::negate(t),void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test(*(Td *)nullptr)),yes>::value;
};

template <typename T>
const bool has_negate<T>::value;

/// Type trait to detect the presence of the piranha::math::t_subs function.
/**
 * The type trait will be \p true if piranha::math::t_subs can be successfully called on instances
 * of type \p T, with instances of type \p U  and \p V as substitution arguments.
 */
template <typename T, typename U = T, typename V = U>
class has_t_subs: detail::sfinae_types
{
		typedef typename std::decay<T>::type Td;
		typedef typename std::decay<U>::type Ud;
		typedef typename std::decay<V>::type Vd;
		template <typename T1, typename U1, typename V1>
		static auto test(const T1 &t, const U1 &u, const V1 &v) -> decltype(math::t_subs(t,std::declval<std::string>(),u,v),void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test(std::declval<Td>(),std::declval<Ud>(),std::declval<Vd>())),yes>::value;
};

// Static init.
template <typename T, typename U, typename V>
const bool has_t_subs<T,U,V>::value;

/// Type trait to detect the presence of the trigonometric substitution method in keys.
/**
 * This type trait will be \p true if \p Key provides a const method <tt>t_subs()</tt> accepting as const parameters a string,
 * an instance of \p T, an instance of \p U and an instance of piranha::symbol_set. The return value of the method must be an <tt>std::vector</tt>
 * of pairs in which the second type must be \p Key itself. The <tt>t_subs()</tt> represents the substitution of a symbol with its cosine
 * and sine passed as instances of \p T and \p U respectively.
 * 
 * The decay type of \p Key must satisfy piranha::is_key.
 */
template <typename Key, typename T, typename U = T>
class key_has_t_subs: detail::sfinae_types
{
		typedef typename std::decay<Key>::type Keyd;
		typedef typename std::decay<T>::type Td;
		typedef typename std::decay<U>::type Ud;
		PIRANHA_TT_CHECK(is_key,Keyd);
		template <typename Key1, typename T1, typename U1>
		static auto test(const Key1 &k, const T1 &t, const U1 &u) ->
			decltype(k.t_subs(std::declval<const std::string &>(),t,u,std::declval<const symbol_set &>()));
		static no test(...);
		template <typename T1>
		struct check_result_type
		{
			static const bool value = false;
		};
		template <typename Res>
		struct check_result_type<std::vector<std::pair<Res,Keyd>>>
		{
			static const bool value = true;
		};
	public:
		/// Value of the type trait.
		static const bool value = check_result_type<decltype(test(std::declval<Keyd>(),
			std::declval<Td>(),std::declval<Ud>()))>::value;
};

// Static init.
template <typename Key, typename T, typename U>
const bool key_has_t_subs<Key,T,U>::value;

/// Type trait to detect the availability of piranha::math::multiply_accumulate.
/**
 * This type trait will be \p true if piranha::math::multiply_accumulate can be called with arguments of type \p T, \p U and \p V,
 * \p false otherwise.
 */
template <typename T, typename U = T, typename V = U>
class has_multiply_accumulate: detail::sfinae_types
{
		template <typename T2, typename U2, typename V2>
		static auto test(T2 &t, const U2 &u, const V2 &v) -> decltype(math::multiply_accumulate(t,u,v),void(),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = std::is_same<decltype(test(std::declval<T &>(),std::declval<U>(),std::declval<V>())),yes>::value;
};

// Static init.
template <typename T, typename U, typename V>
const bool has_multiply_accumulate<T,U,V>::value;

}

#endif

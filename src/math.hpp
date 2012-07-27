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

#include <boost/numeric/conversion/cast.hpp>
#include <boost/type_traits/is_complex.hpp>
#include <cmath>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "config.hpp"
#include "detail/integer_fwd.hpp"
#include "detail/sfinae_types.hpp"
#include "exceptions.hpp"

namespace piranha
{

namespace detail
{

// Default implementation of math::is_zero.
// NOTE: the technique and its motivations are well-described here:
// http://www.gotw.ca/publications/mill17.htm
template <typename T, typename Enable = void>
struct math_is_zero_impl
{
	static bool run(const T &x)
	{
		// NOTE: construct instance from integral constant 0.
		// http://groups.google.com/group/comp.lang.c++.moderated/msg/328440a86dae8088?dmode=source
		return x == T(0);
	}
};

// Handle std::complex types.
template <typename T>
struct math_is_zero_impl<T,typename std::enable_if<boost::is_complex<T>::value>::type>
{
	static bool run(const T &c)
	{
		return math_is_zero_impl<typename T::value_type>::run(c.real()) &&
			math_is_zero_impl<typename T::value_type>::run(c.imag());
	}
};

// Default implementation of math::negate.
// NOTE: in gcc 4.6 and up here we could use copysign, signbit, etc. to determine whether it's safe to negate floating-point types.
template <typename T, typename Enable = void>
struct math_negate_impl
{
	static void run(T &x)
	{
		x = -x;
	}
};

// Default implementation of multiply-accumulate.
template <typename T, typename U, typename V, typename Enable = void>
struct math_multiply_accumulate_impl
{
	template <typename U2, typename V2>
	static void run(T &x, U2 &&y, V2 &&z)
	{
		x += std::forward<U2>(y) * std::forward<V2>(z);
	}
};

#if defined(FP_FAST_FMA) && defined(FP_FAST_FMAF) && defined(FP_FAST_FMAL)

// Implementation of multiply-accumulate for floating-point types, if fast FMA is available.
template <typename T>
struct math_multiply_accumulate_impl<T,T,T,typename std::enable_if<std::is_floating_point<T>::value>::type>
{
	static void run(T &x, const T &y, const T &z)
	{
		x = std::fma(y,z,x);
	}
};

#endif

// Implementation of exponentiation with floating-point base.
template <typename T, typename U>
inline auto float_pow_impl(const T &x, const U &n, typename std::enable_if<
	std::is_integral<U>::value>::type * = piranha_nullptr) -> decltype(std::pow(x,boost::numeric_cast<int>(n)))
{
	return std::pow(x,boost::numeric_cast<int>(n));
}

template <typename T, typename U>
inline auto float_pow_impl(const T &x, const U &n, typename std::enable_if<
	std::is_same<integer,U>::value>::type * = piranha_nullptr) -> decltype(std::pow(x,static_cast<int>(n)))
{
	return std::pow(x,static_cast<int>(n));
}

template <typename T, typename U>
inline auto float_pow_impl(const T &x, const U &y, typename std::enable_if<
	std::is_floating_point<U>::value>::type * = piranha_nullptr) -> decltype(std::pow(x,y))
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

/// Zero test.
/**
 * Test if value is zero. This function works with all C++ arithmetic types
 * and with piranha's numerical types. For series types, it will return \p true
 * if the series is empty, \p false otherwise. For \p std::complex, the function will
 * return \p true if both the real and imaginary parts are zero, \p false otherwise.
 * 
 * @param[in] x value to be tested.
 * 
 * @return \p true if value is zero, \p false otherwise.
 */
template <typename T>
inline bool is_zero(const T &x)
{
	return piranha::detail::math_is_zero_impl<T>::run(x);
}

/// In-place negation.
/**
 * Negate value in-place. This function works with all C++ arithmetic types,
 * with piranha's numerical types and with series types. For series, piranha::series::negate() is called.
 * 
 * @param[in,out] x value to be negated.
 */
template <typename T>
inline void negate(T &x)
{
	piranha::detail::math_negate_impl<typename std::decay<T>::type>::run(x);
}

/// Multiply-accumulate.
/**
 * Will set \p x to <tt>x + y * z</tt>. Default implementation is equivalent to:
 * @code
 * x += y * z
 * @endcode
 * where \p y and \p z are perfectly forwarded.
 * 
 * In case fast fma functions are available on the host platform, they will be used instead of the above
 * formula for floating-point types.
 * 
 * @param[in,out] x value used for accumulation.
 * @param[in] y first multiplicand.
 * @param[in] z second multiplicand.
 */
template <typename T, typename U, typename V>
inline void multiply_accumulate(T &x, U &&y, V &&z)
{
	piranha::detail::math_multiply_accumulate_impl<typename std::decay<T>::type,
		typename std::decay<U>::type,typename std::decay<V>::type>::run(x,std::forward<U>(y),std::forward<V>(z));
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

/// Type-trait for differentiable types.
/**
 * The type-trait will be \p true if piranha::math::partial() can be successfully called on instances of
 * type \p T, \p false otherwise.
 */
template <typename T>
class is_differentiable: detail::sfinae_types
{
		template <typename U>
		static auto test(U const *t) -> decltype(math::partial(*t,""),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = (sizeof(test((T const *)piranha_nullptr)) == sizeof(yes));
};

/// Type-trait for integrable types.
/**
 * The type-trait will be \p true if piranha::math::integrate() can be successfully called on instances of
 * type \p T, \p false otherwise.
 */
template <typename T>
class is_integrable: detail::sfinae_types
{
		template <typename U>
		static auto test(U const *t) -> decltype(math::integrate(*t,""),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = (sizeof(test((T const *)piranha_nullptr)) == sizeof(yes));
};

/// Type-trait for exponentiable types.
/**
 * The type-trait will be \p true if piranha::math::pow() can be successfully called with base \p T and
 * exponent \p U.
 */
template <typename T, typename U>
class is_exponentiable: detail::sfinae_types
{
		template <typename Base, typename Expo>
		static auto test(Base const *b, Expo const *e) -> decltype(math::pow(*b,*e),yes());
		static no test(...);
	public:
		/// Value of the type trait.
		static const bool value = (sizeof(test((T const *)piranha_nullptr,(U const *)piranha_nullptr)) == sizeof(yes));
};

}

#endif

/***************************************************************************
 *   Copyright (C) 2009-2011 by Francesco Biscani, Hartmuth Gutsche                          *
 *   bluescarni@gmail.com 
 *   hgutsche@xplornet.com
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

#ifndef PIRANHA_RATIONAL_HPP
#define PIRANHA_RATIONAL_HPP

// not implemented yet in gcc 4.5
//#include <regex>

#include <gmp.h>

#include <boost/regex.hpp>

#include "config.hpp"
#include "integer.hpp"

namespace piranha {


class rational {

	private:

	// C++ arithmetic types supported for interaction with rational.
	template <typename T>
	struct is_interop_type
	{
		static const bool value = std::is_same<T, char>::value ||
								  std::is_same<T, bool>::value ||
								  std::is_same<T, float>::value||
								  std::is_same<T, double>::value||
								  std::is_same<T, long double>::value||
								  std::is_same<T, signed char>::value||
								  std::is_same<T, short>::value||
								  std::is_same<T, int>::value||
								  std::is_same<T, long>::value||
								  std::is_same<T, long long>::value||
								  std::is_same<T, unsigned char>::value||
								  std::is_same<T, unsigned short>::value||
								  std::is_same<T, unsigned int>::value||
								  std::is_same<T, unsigned long>::value||
								  std::is_same<T, unsigned long long>::value;
	};

	static const int NUMBER_BASE = 10;
	//Check if string is valid rational number representation
	void validate_string(std::string const & str, std::string const & errorText)
	{
		static const boost::regex rx("(^\\s*([+-]?\\d+)\\s*/\\s*([+]?\\d+)\\s*$)|(^\\s*([+-]?\\d+)\\s*$)");
		boost::smatch matchResult;
		if(!boost::regex_match(str, matchResult, rx))
		{
			piranha_throw(std::invalid_argument, errorText + str);
		}
	}


	void set_from_string(std::string const & str, std::string const & error_text)
	{
		const int retval = ::mpq_set_str(m_value, str.c_str(), NUMBER_BASE);
		if(retval == -1)
		{
			::mpq_clear(m_value);
			piranha_throw(std::invalid_argument, error_text + str);
		}else
		{
			// need to canonicalize
			::mpq_canonicalize(m_value);
		}
	}


	void construct_from_string(std::string const & str)
	{
		static std::string const errorText("invalid string input for rational type: ");

		validate_string(str, errorText);

		::mpq_init(m_value);
		set_from_string(str, errorText);
	}


	void construct_from_pair(integer const & numerator, integer const & denumerator)
	{
		::mpq_init(m_value);
		::mpz_set(mpq_numref(m_value), numerator.m_value);
		::mpz_set(mpq_denref(m_value), denumerator.m_value);
		::mpq_canonicalize(m_value);
	}

//    /////////////////////
//	// Binary operations.
//    /////////////////////
//	// Type trait for allowed arguments in arithmetic binary operations involving rational
//	template <typename T, typename U>
//	struct are_binary_op_types: std::integral_constant<bool,
//		(std::is_same<typename std::decay<T>::type, rational>::value && is_interop_type<typename std::decay<U>::type>::value)||
//		(std::is_same<typename std::decay<U>::type, integer >::value && is_interop_type<typename std::decay<T>::type>::value)||
//		(std::is_same<typename std::decay<T>::type, rational>::value && std::is_same<typename std::decay<U>::type, rational>::value)>
//	{};
//
//
//	//Binary equality
//	static bool binary_equality(const rational &q1, const rational &q2)
//	{
//		return (::mpq_equal(q1.m_value, q2.m_value) != 0);
//	}
//
//
//	//Binary less than
//	static bool binary_less_than (rational const & q1, rational const & q2)
//	{
//		return (::mpq_cmp(q1.m_value, q2.m_value) < 0);
//	}
//
//	template <typename T>
//	static bool binary_less_than (rational const & q1, integer const &n2)
//	{
//		return binary_less_than(q1, rational(n2));
//	}
//
//	template <typename T>
//	static bool binary_less_than(rational const &q1, T const &n2, typename std::enable_if<std::is_integral<T>::value &&
//			std::is_signed<T>::value && !std::is_same<T, long long>::value>::type * = piranha_nullptr)
//	{
//
//			return mpq_cmp_si(q1.m_value, static_cast<long>(n2), 1) < 0;
//	}
//
//	template <typename T>
//	static bool binary_less_than(rational const &q1, T const &n2, typename std::enable_if<std::is_integral<T>::value &&
//			std::is_unsigned<T>::value && !std::is_same<T, unsigned long long>::value>::type * = piranha_nullptr)
//	{
//		return mpq_cmp_ui(q1.m_value, static_cast<unsigned long> (n2), 1) <0 ;
//	}
//
//	template <typename T>
//	static bool binary_less_than(rational const &q1, T const & n2, typename std::enable_if<std::is_same<T, long long>::value ||
//			std::is_same<T, unsigned long long>::value>::type * = piranha_nullptr)
//	{
//		return binary_less_than(q1, rational(n2));
//	}
//	//TODO: for long long
//
//
//	//Binary less equal
//	static bool binary_leq(rational const & q1, rational const & q2)
//	{
//		return (::mpq_cmp(q1.m_value, q2.m_value) <= 0);
//	}

	public:
	/// Default constructor.
	/**
	 * The rational value is initialised to 0/1
	 */
	rational() piranha_noexcept_spec(true)
	{
		::mpq_init(m_value);
	}

	///Copy constructor
	/**
	 *  @param[in] other rational to be deep-copied
	 */
	rational(rational const &other) piranha_noexcept_spec(true)
	{
		::mpq_init(m_value);
		::mpq_set(m_value, other.m_value);
	}

	/// Move constructor
	/**
	 *  @param[in] other rational to be moved
	 */
	rational(rational &&other) piranha_noexcept_spec(true)
	{
		mpq_numref(m_value)->_mp_size  = mpq_numref(other.m_value)->_mp_size;
		mpq_numref(m_value)->_mp_d     = mpq_numref(other.m_value)->_mp_d;
		mpq_numref(m_value)->_mp_alloc = mpq_numref(other.m_value)->_mp_alloc;

		mpq_denref(m_value)->_mp_size  = mpq_denref(other.m_value)->_mp_size;
		mpq_denref(m_value)->_mp_d     = mpq_denref(other.m_value)->_mp_d;
		mpq_denref(m_value)->_mp_alloc = mpq_denref(other.m_value)->_mp_alloc;

		//remove content of other
		mpq_clear(other.m_value);
	}

	/// Construct a rational from an integer object
	/**
	 *  @param[in] other integer object the rational object to be created from
	 */
	rational(integer const & other) piranha_noexcept_spec(true)
	{
		::mpq_init(m_value);
		::mpq_set_z(m_value, other.m_value);
	}

	/// Constructor from arithmetic types
	template <typename T>
	explicit rational(T const & x, typename std::enable_if<is_interop_type<T>::value>::type * = piranha_nullptr)
	{
		//TODO: Improve performance by not going through Integer
		integer temp(x);
		::mpq_init(m_value);
		::mpz_set(mpq_numref(m_value), temp.m_value);
		::mpz_set_ui(mpq_denref(m_value), static_cast<unsigned long>(1));
	}

	/// Constructor from string
	// create a rational object from a string of the form "123/456",
	// or "42". Leading and trailing white spaces in the numerical components are allowed and simply ignored
	// i.e. "123 / 456" is equivalent to "123/456".

	explicit rational(const std::string &str)
	{
		construct_from_string(str);
	}

	/// Constructor from C string
	explicit rational(const char * str)
	{
		construct_from_string(std::string(str));
	}

	/// Constructor from a pair of piranha::integer
	explicit rational(integer const & numerator, integer const & denumerator)
	{
		construct_from_pair(numerator, denumerator);
	}

	/// Construct from a pair of supported C++ type
	template <typename T, typename U>
	explicit rational(T const &n, U const &d, typename std::enable_if<is_interop_type<T>::value>::type * = piranha_nullptr, typename std::enable_if<is_interop_type<U>::value>::type * = piranha_nullptr)
	{
		integer numerator(n);
		integer denumerator(d);
		construct_from_pair(numerator, denumerator);
	}

	/// Move constructor
	/**
	 * @param[in] numerator integer to be moved to the numerator
	 * @param[in] denumerator integer to be moved to the denumerator
	 */
	explicit rational(integer && numerator, integer && denumerator)
	{
		mpq_numref(m_value)->_mp_size  = numerator.m_value->_mp_size;
		mpq_numref(m_value)->_mp_d     = numerator.m_value->_mp_d;
		mpq_numref(m_value)->_mp_alloc = numerator.m_value->_mp_alloc;

		mpq_denref(m_value)->_mp_size  = denumerator.m_value->_mp_size;
		mpq_denref(m_value)->_mp_d     = denumerator.m_value->_mp_d;
		mpq_denref(m_value)->_mp_alloc = denumerator.m_value->_mp_alloc;

		::mpq_canonicalize(m_value);
		mpz_clear(numerator.m_value);
		mpz_clear(denumerator.m_value);


	}

	/// Destructor
	/**
	 *  Clear the internal \p mpq_t type
	 */
	~rational() piranha_noexcept_spec(true)
	{
		// TODO: should we use unlikely like integer. Optimisation
		::mpq_clear(m_value);
	}

	///Move assignment operator
	/**
	 * @param[in] other integer to be moved.
	 *
	 * @return reference to \p this.
	 */

//    rational &operator=(rational &&other) piranha_noexcept_spec(true)
//	{
//    	//TODO:
//	}
//
//    /// Assignment operator
//	/**
//	 * @param[in] other integer to be assigned.
//	 *
//	 * @return reference to \p this.
//	 */
//    rational &operator=(rational const &q) piranha_noexcept_spec(true)
//	{
//    	if (this != boost::addressof(q))
//    	{
//    		::mpq_set(m_value, q.m_value);
//    	}
//
//    	return *this;
//	}
//
//    /// Assignment operator from string
//    rational &operator=(const std::string &str)
//    {
//    	static std::string const errorText("invalid string input for assignment to rational: ");
//    	validate_string(str, errorText);
//    	set_from_string(str, errorText);
//    	return *this;
//    }
//
//
//    /// Assignment operator from C string
//    rational &operator=(char const * str)
//    {
//    	return operator=(std::string(str));
//    }
//
//    /// Assignment from Integer
//    rational &operator=(integer const & n)
//    {
//    	::mpq_set_z(m_value, n.m_value);
//    	return *this;
//
//    }
//
//    /// Generic assignment from arithmetic types
////    rational &operator=()
////	{
////    	//TODO implement all the template selection
////	}
//
//    /// Swap
//	/**
//	 * Swap the content of \p this and \p r.
//	 *
//	 * @param[in] r swap argument.
//	 */
//
//    void swap(rational &r) piranha_noexcept_spec(true)
//	{
//    	::mpq_swap(m_value, r.m_value);
//	}
//
//    /// Conversion operator to arithmetic type
//
//    /// Conversion to Integer
//
//    /// In-place addition
//
//    /// In-place addition with piranha::integer
//
//    /// generic in-place addition with piranha::rational
//
//    /// generic binary addition involving piranha::rational
//
//    /// identity operation
//    /**
//     * @return reference to \p this
//     */
//    rational &operator+()
//    {
//    	return *this;
//    }
//
//    /// identity operation (const version)
//    /**
//     * @return const reference to \p this.
//     */
//    rational const & operator+() const
//    {
//    	return *this;
//    }
//
//    /// prefix increment
//
//    /// suffix increment
//
//    /// In-place subtraction
//
//    /// In-place subtraction by piranha::integer
//
//    /// generic in-place subtraction with piranha::rational
//
//    /// generic binary subtraction involving piranha::rational
//
//    /// In-place negation
//
//    /// Negated copy
//
//    /// prefix decrement
//
//    /// suffix decrement
//
//    /// In-place multiplication
//
//    /// In-place multiplication with piranha::integer
//
//    /// generic in-place multiplication with piranha::rational
//
//    /// generic binary multiplication involving piranha::rational
//
//    /// binary multiplication between piranha::rational and piranha::integer
//
//    /// in-place division
//
//    /// generic in-place division with piranha::rational
//
//    /// generic binary division involving piranha:rational
//
//    /// generic equality operator for piranha::rational
//	/**
//	 * This template operator is activated if either:
//	 *
//	 * - \p T is piranha::rational  and \p U is an arithmetic type or piranha:integer,
//	 * - \p T is a arithmetic type or piranha::integer  and \p U is a piranha rational type,
//	 * - both \p T and \p U are piranha::rational.
//	 *
//	 * If no floating-point types are involved, the exact result of the comparison will be returned.
//	 *
//	 * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
//	 * and compared to \p f to generate the return value.
//	 *
//	 * @param[in] x first argument
//	 * @param[in] y second argument.
//	 *
//	 * @return \p true if <tt>x == y</tt>, \p false otherwise.
//	 */
//     template<typename T, typename U>
//     friend inline typename std::enable_if<are_binary_op_types<T, U>::value, bool>::type operator==(const T &x, const U &y)
//     {
//    	 return binary_equality(x, y);
//     }
//
//     /// Generic inequality operator involving piranha::rational.
//     /**
//      * This template operator is activated if either:
//      *
//      * - \p T is piranha::ratonal and \p U is an arithmetic type or piranha::integer,
//      * - \p U is piranha::rational and \p T is an arithmetic type or piranha::integer,
//      * - both \p T and \p U are piranha::rational.
//      *
//      * If no floating-point types are involved, the exact result of the comparison will be returned.
//      *
//      * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
//      * and compared to \p f to generate the return value.
//      *
//      * @param[in] x first argument
//      * @param[in] y second argument.
//      *
//      * @return \p true if <tt>x != y</tt>, \p false otherwise.
//      */
//     template <typename T, typename U>
//     friend inline typename std::enable_if<are_binary_op_types<T,U>::value, bool>::type operator!=(const T &x, const U &y)
//     {
//    	 return !(x == y);
//     }
//
//     /// Generic less-than operator involving piranha::integer.
//     /**
//      * This template operator is activated if either:
//      *
//      * - \p T is piranha::rational and \p U is an arithmetic type or integer::integer,
//      * - \p U is piranha::rational and \p T is an arithmetic type or piranha::integer,
//      * - both \p T and \p U are piranha::rationalinteger.
//      *
//      * If no floating-point types are involved, the exact result of the comparison will be returned.
//      *
//      * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
//      * and compared to \p f to generate the return value.
//      *
//      * @param[in] x first argument
//      * @param[in] y second argument.
//      *
//      * @return \p true if <tt>x < y</tt>, \p false otherwise.
//      */
//     template <typename T, typename U>
//     friend inline typename std::enable_if<are_binary_op_types<T, U>::value, bool>::type operator<(const T &x, const U &y)
//     {
//    	 return binary_less_than(x, y);
//     }
//
//
//     /// generic less equal than
//
//     /// Generic greater-than operator involving piranha::rational.
//     /**
//      * This template operator is activated if either:
//      *
//      * - \p T is piranha::rational and \p U is an arithmetic type or piranha::integer,
//      * - \p U is piranha::rational and \p T is an arithmetic type or piranha::integer,
//      * - both \p T and \p U are piranha::rational.
//      *
//      * If no floating-point types are involved, the exact result of the comparison will be returned.
//      *
//      * If one of the arguments is a floating-point value \p f of type \p F, the other argument will be converted to an instance of type \p F
//      * and compared to \p f to generate the return value.
//      *
//      * @param[in] x first argument
//      * @param[in] y second argument.
//      *
//      * @return \p true if <tt>x > y</tt>, \p false otherwise.
//      */
//     template <typename T, typename U>
//     friend inline typename std::enable_if<are_binary_op_types<T, U>::value, bool>::type operator>(const T &x, const U &y)
//     {
//    	 return (y < x);
//     }
//
//    /// generic greater equal than
//
//    /// MA instruction
//
//    /// exponentiation
//
//
//    /// output stream operator
//
//    /// input stream operator
//
//    /// numerator
//    integer num() const
//    {
//
//
//    }
//
//    /// denumerator
//    integer den() const
//    {
//
//    }


private:

	mpq_t m_value;
};

}  // namespace piranha
#endif /* PIRANHA_RATIONAL_HPP */

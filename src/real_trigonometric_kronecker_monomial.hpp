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

#ifndef PIRANHA_REAL_TRIGONOMETRIC_KRONECKER_MONOMIAL_HPP
#define PIRANHA_REAL_TRIGONOMETRIC_KRONECKER_MONOMIAL_HPP

#include <algorithm>
#include <boost/concept/assert.hpp>
#include <boost/integer_traits.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_set>

#include "concepts/key.hpp"
#include "config.hpp"
#include "detail/km_commons.hpp"
#include "exceptions.hpp"
#include "kronecker_array.hpp"
#include "static_vector.hpp"
#include "symbol_set.hpp"

namespace piranha
{

/// Real trigonometric Kronecker monomial class.
/**
 * This class represents a multivariate real trigonometric monomial, i.e., functions of the form
 * \f[
 * \begin{array}{c}
 * \sin \tabularnewline
 * \cos
 * \end{array}
 * \left(n_0x_0 + n_1x_1 + \ldots + n_mx_m\right),
 * \f]
 * where the integers \f$ n_i \f$ are called <em>multipliers</em>. The values of the multipliers
 * are packed in a signed integer using Kronecker substitution, using the facilities provided
 * by piranha::kronecker_array. The boolean <em>flavour</em> of the monomial indicates whether it represents
 * a cosine (<tt>true</tt>) or sine (<tt>false</tt>).
 * 
 * Similarly to an ordinary monomial, this class provides methods to query the <em>harmonic</em> (partial) (low) degree, defined
 * as if the multipliers were exponents of a regular monomial (e.g., the total harmonic degree is the sum of the multipliers).
 * 
 * This class is a model of the piranha::concept::Key concept.
 * 
 * \section type_requirements Type requirements
 * 
 * \p T must be suitable for use in piranha::kronecker_array. The default type for \p T is the signed counterpart of \p std::size_t.
 * 
 * \section exception_safety Exception safety guarantee
 * 
 * Unless otherwise specified, this class provides the strong exception safety guarantee for all operations.
 * 
 * \section move_semantics Move semantics
 * 
 * The move semantics of this class are equivalent to the move semantics of C++ signed integral types.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
template <typename T = std::make_signed<std::size_t>::type>
class real_trigonometric_kronecker_monomial
{
	public:
		/// Alias for \p T.
		typedef T value_type;
	private:
		typedef kronecker_array<value_type> ka;
	public:
		/// Size type.
		/**
		 * Used to represent the number of variables in the monomial. Equivalent to the size type of
		 * piranha::kronecker_array.
		 */
		typedef typename ka::size_type size_type;
		/// Maximum monomial size.
		static const size_type max_size = 255u;
	private:
		static_assert(max_size <= boost::integer_traits<static_vector<int,1u>::size_type>::const_max,"Invalid max size.");
	public:
		/// Vector type used for temporary packing/unpacking.
		typedef static_vector<value_type,max_size> v_type;
		/// Default constructor.
		/**
		 * After construction all multipliers in the monomial will be zero, and the flavour will be set to \p true.
		 */
		real_trigonometric_kronecker_monomial():m_value(0),m_flavour(true) {}
		/// Defaulted copy constructor.
		real_trigonometric_kronecker_monomial(const real_trigonometric_kronecker_monomial &) = default;
		/// Defaulted move constructor.
		real_trigonometric_kronecker_monomial(real_trigonometric_kronecker_monomial &&) = default;
		/// Constructor from initalizer list.
		/**
		 * The values in the initializer list are intended to represent the multipliers of the monomial:
		 * they will be converted to type \p T (if \p T and \p U are not the same type),
		 * encoded using piranha::kronecker_array::encode() and the result assigned to the internal integer instance.
		 * The flavour will be set to \p true.
		 * 
		 * @param[in] list initializer list representing the multipliers.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::kronecker_array::encode(),
		 * - \p boost::numeric_cast (in case \p U is not the same as \p T),
		 * - piranha::static_vector::push_back().
		 */
		template <typename U>
		explicit real_trigonometric_kronecker_monomial(std::initializer_list<U> list):m_value(0),m_flavour(true)
		{
			v_type tmp;
			for (auto it = list.begin(); it != list.end(); ++it) {
				tmp.push_back(boost::numeric_cast<value_type>(*it));
			}
			m_value = ka::encode(tmp);
		}
		/// Constructor from range.
		/**
		 * Will build internally a vector of values from the input iterators, encode it and assign the result
		 * to the internal integer instance. The value type of the iterator is converted to \p T using
		 * \p boost::numeric_cast. The flavour will be set to \p true.
		 * 
		 * @param[in] start beginning of the range.
		 * @param[in] end end of the range.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::kronecker_array::encode(),
		 * - \p boost::numeric_cast (in case the value type of \p Iterator is not the same as \p T),
		 * - piranha::static_vector::push_back().
		 */
		template <typename Iterator>
		explicit real_trigonometric_kronecker_monomial(const Iterator &start, const Iterator &end):m_value(0),m_flavour(true)
		{
			typedef typename std::iterator_traits<Iterator>::value_type it_v_type;
			v_type tmp;
			std::transform(start,end,std::back_inserter(tmp),[](const it_v_type &v) {return boost::numeric_cast<value_type>(v);});
			m_value = ka::encode(tmp);
		}
		/// Constructor from set of symbols.
		/**
		 * After construction all multipliers will be zero and the flavour will be set to \p true.
		 * 
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @throws unspecified any exception thrown by:
		 * - piranha::kronecker_array::encode(),
		 * - piranha::static_vector::push_back().
		 */
		explicit real_trigonometric_kronecker_monomial(const symbol_set &args):m_flavour(true)
		{
			v_type tmp;
			for (auto it = args.begin(); it != args.end(); ++it) {
				tmp.push_back(value_type(0));
			}
			m_value = ka::encode(tmp);
		}
		/// Constructor from \p value_type and flavour.
		/**
		 * This constructor will initialise the internal integer instance
		 * to \p n and the flavour to \p f.
		 * 
		 * @param[in] n initializer for the internal integer instance.
		 * @param[in] f desired flavour.
		 */
		explicit real_trigonometric_kronecker_monomial(const value_type &n, bool f):m_value(n),m_flavour(f) {}
		/// Converting constructor.
		/**
		 * This constructor is for use when converting from one term type to another in piranha::series. It will
		 * set the internal integer instance and flavour to the same value of \p other, after having checked that
		 * \p other is compatible with \p args.
		 * 
		 * @param[in] other construction argument.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @throws std::invalid_argument if \p other is not compatible with \p args.
		 */
		explicit real_trigonometric_kronecker_monomial(const real_trigonometric_kronecker_monomial &other, const symbol_set &args):
			m_value(other.m_value),m_flavour(other.m_flavour)
		{
			if (unlikely(!other.is_compatible(args))) {
				piranha_throw(std::invalid_argument,"incompatible arguments");
			}
		}
		/// Trivial destructor.
		~real_trigonometric_kronecker_monomial() piranha_noexcept_spec(true)
		{
			BOOST_CONCEPT_ASSERT((concept::Key<real_trigonometric_kronecker_monomial>));
		}
		/// Defaulted copy assignment operator.
		real_trigonometric_kronecker_monomial &operator=(const real_trigonometric_kronecker_monomial &) = default;
		/// Trivial move assignment operator.
		/**
		 * @param[in] other monomial to be assigned to this.
		 * 
		 * @return reference to \p this.
		 */
		real_trigonometric_kronecker_monomial &operator=(real_trigonometric_kronecker_monomial &&other) piranha_noexcept_spec(true)
		{
			m_value = std::move(other.m_value);
			m_flavour = std::move(other.m_flavour);
			return *this;
		}
		/// Set the internal integer instance.
		/**
		 * @param[in] n value to which the internal integer instance will be set.
		 */
		void set_int(const value_type &n)
		{
			m_value = n;
		}
		/// Get internal instance.
		/**
		 * @return value of the internal integer instance.
		 */
		value_type get_int() const
		{
			return m_value;
		}
		/// Get flavour.
		/**
		 * @return flavour of the monomial. 
		 */
		bool get_flavour() const
		{
			return m_flavour;
		}
		/// Set flavour.
		/**
		 * @param[in] f value to which the flavour will be set. 
		 */
		void set_flavour(bool f)
		{
			m_flavour = f;
		}
		/// Compatibility check.
		/**
		 * Monomial is considered incompatible if any of these conditions holds:
		 * 
		 * - the size of \p args is zero and the internal integer is not zero,
		 * - the size of \p args is equal to or larger than the size of the output of piranha::kronecker_array::get_limits(),
		 * - the internal integer is not within the limits reported by piranha::kronecker_array::get_limits(),
		 * - the first element of the vector of multipliers represented by the internal integer is negative.
		 * 
		 * Otherwise, the monomial is considered to be compatible for insertion.
		 * 
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return compatibility flag for the monomial.
		 */
		bool is_compatible(const symbol_set &args) const piranha_noexcept_spec(true)
		{
			const auto s = args.size();
			// No args means the value must also be zero.
			if (s == 0u) {
				return !m_value;
			}
			const auto &limits = ka::get_limits();
			// If we overflow the maximum size available, we cannot use this object as key in series.
			if (s >= limits.size()) {
				return false;
			}
			const auto &l = limits[s];
			// Value is compatible if it is within the bounds for the given size.
			if (m_value < std::get<1u>(l) || m_value > std::get<2u>(l)) {
				return false;
			}
			// Now check for the first multiplier.
			// NOTE: here we have already checked all the conditions that could lead to unpack() throwing, so
			// we do not need to put @throw specifications in the doc.
			const auto unpacked = unpack(args);
			// We know that s != 0.
			piranha_assert(unpacked.size() > 0u);
			if (unpacked[0u] < value_type(0)) {
				return false;
			}
			return true;
		}
		/// Ignorability check.
		/**
		 * A monomial is considered ignorable if all multipliers are zero and the flavour is \p false.
		 * 
		 * @return ignorability flag.
		 */
		bool is_ignorable(const symbol_set &) const piranha_noexcept_spec(true)
		{
			if (m_value == value_type(0) && !m_flavour) {
				return true;
			}
			return false;
		}
		/// Merge arguments.
		/**
		 * Merge the new arguments set \p new_args into \p this, given the current reference arguments set
		 * \p orig_args.
		 * 
		 * @param[in] orig_args original arguments set.
		 * @param[in] new_args new arguments set.
		 * 
		 * @return monomial with merged arguments.
		 * 
		 * @throws std::invalid_argument if at least one of these conditions is true:
		 * - the size of \p new_args is not greater than the size of \p orig_args,
		 * - not all elements of \p orig_args are included in \p new_args.
		 * @throws unspecified any exception thrown by:
		 * - piranha::kronecker_array::encode(),
		 * - piranha::static_vector::push_back(),
		 * - unpack().
		 */
		real_trigonometric_kronecker_monomial merge_args(const symbol_set &orig_args, const symbol_set &new_args) const
		{
			return real_trigonometric_kronecker_monomial(
				detail::km_merge_args<v_type,ka>(orig_args,new_args,m_value),m_flavour);
		}
		/// Check if monomial is unitary.
		/**
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return \p true if all the multipliers are zero and the flavour is \p true, \p false otherwise.
		 * 
		 * @throws std::invalid_argument if \p this is not compatible with \p args.
		 */
		bool is_unitary(const symbol_set &args) const
		{
			if (unlikely(!is_compatible(args))) {
				piranha_throw(std::invalid_argument,"invalid symbol set");
			}
			return (!m_value && m_flavour);
		}
		/// Harmonic degree.
		/**
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return harmonic degree of the monomial.
		 * 
		 * @throws std::overflow_error if the computation of the degree overflows type \p value_type.
		 * @throws unspecified any exception thrown by unpack().
		 */
		value_type h_degree(const symbol_set &args) const
		{
			const auto tmp = unpack(args);
			return std::accumulate(tmp.begin(),tmp.end(),value_type(0),detail::km_safe_adder<value_type>);
		}
		/// Low harmonic degree.
		/**
		 * Equivalent to the harmonic degree.
		 * 
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return low harmonic degree of the monomial.
		 * 
		 * @throws unspecified any exception thrown by h_degree().
		 */
		value_type h_ldegree(const symbol_set &args) const
		{
			return h_degree(args);
		}
		/// Partial harmonic degree.
		/**
		 * Partial harmonic degree of the monomial: only the symbols with names in \p active_args are considered during the computation
		 * of the degree. Symbols in \p active_args not appearing in \p args are not considered.
		 * 
		 * @param[in] active_args names of the symbols that will be considered in the computation of the partial degree of the monomial.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return the summation of all the multipliers of the monomial corresponding to the symbols in
		 * \p active_args, or <tt>value_type(0)</tt> if no symbols in \p active_args appear in \p args.
		 * 
		 * @throws std::overflow_error if the computation of the degree overflows type \p value_type.
		 * @throws unspecified any exception thrown by unpack().
		 */
		value_type h_degree(const std::set<std::string> &active_args, const symbol_set &args) const
		{
			return detail::km_partial_degree<v_type,ka>(active_args,args,m_value);
		}
		/// Partial low harmonic degree.
		/**
		 * Equivalent to the partial harmonic degree.
		 * 
		 * @param[in] active_args names of the symbols that will be considered in the computation of the partial low harmonic degree of the monomial.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return the partial low harmonic degree.
		 * 
		 * @throws unspecified any exception thrown by h_degree().
		 */
		value_type h_ldegree(const std::set<std::string> &active_args, const symbol_set &args) const
		{
			return h_degree(active_args,args);
		}
		/// Multiply monomial.
		/**
		 * The two monomials resulting from multiplying \p this by \p other
		 * are computed by adding and subtracting the multipliers of \p this to the multipliers of \p other
		 * according to the prosthaphaeresis formulas. After the multiplication, \p retval_plus will contain the monomial resulting
		 * from the addition of the multipliers, \p retval_minus the monomial resulting from the subtraction of the mulipliers.
		 * The flavours of the two output monomials will be \p true if the flavours of the operands are equal, \p false otherwise.
		 * 
		 * @param[out] retval_plus monomial containing the sum of the multipliers of \p this and \p other.
		 * @param[out] retval_minus monomial containing the difference of the multipliers of \p this and \p other.
		 * @param[in] other multiplicand.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @throws std::overflow_error if the computation of the result overflows type \p value_type.
		 * @throws unspecified any exception thrown by:
		 * - piranha::kronecker_array::encode(),
		 * - unpack(),
		 * - piranha::static_vector::push_back().
		 */
		void multiply(real_trigonometric_kronecker_monomial &retval_plus, real_trigonometric_kronecker_monomial &retval_minus,
			const real_trigonometric_kronecker_monomial &other, const symbol_set &args) const
		{
			const auto size = args.size();
			const auto tmp1 = unpack(args), tmp2 = other.unpack(args);
			v_type result_plus, result_minus;
			for (decltype(args.size()) i = 0u; i < size; ++i) {
				result_plus.push_back(detail::km_safe_adder(tmp1[i],tmp2[i]));
				// NOTE: it is safe here to take the negative because in kronecker_array we are guaranteed
				// that the range of each element is symmetric, so if tmp2[i] is representable also -tmp2[i] is.
				// NOTE: the static cast here is because if value_type is narrower than int, the unary minus will promote
				// to int and safe_adder won't work as it expects identical types.
				result_minus.push_back(detail::km_safe_adder(tmp1[i],static_cast<value_type>(-tmp2[i])));
			}
			// Compute them before assigning, so in case of exceptions we do not touch the return values.
			const auto re_plus = ka::encode(result_plus), re_minus = ka::encode(result_minus);
			retval_plus.m_value = re_plus;
			retval_minus.m_value = re_minus;
			const bool f = (get_flavour() == other.get_flavour());
			retval_plus.m_flavour = f;
			retval_minus.m_flavour = f;
		}
		/// Hash value.
		/**
		 * @return the internal integer instance, cast to \p std::size_t.
		 */
		std::size_t hash() const
		{
			return static_cast<std::size_t>(m_value);
		}
		/// Equality operator.
		/**
		 * @param[in] other comparison argument.
		 * 
		 * @return \p true if the internal integral instance and the flavour of \p this are the same of \p other,
		 * \p false otherwise.
		 */
		bool operator==(const real_trigonometric_kronecker_monomial &other) const
		{
			return (m_value == other.m_value && m_flavour == other.m_flavour);
		}
		/// Inequality operator.
		/**
		 * @param[in] other comparison argument.
		 * 
		 * @return the opposite of operator==().
		 */
		bool operator!=(const real_trigonometric_kronecker_monomial &other) const
		{
			return (m_value != other.m_value || m_flavour != other.m_flavour);
		}
		/// Unpack internal integer instance.
		/**
		 * Will decode the internal integral instance into a piranha::static_vector of size equal to the size of \p args.
		 * 
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @return piranha::static_vector containing the result of decoding the internal integral instance via
		 * piranha::kronecker_array.
		 * 
		 * @throws std::invalid_argument if the size of \p args is larger than the maximum size of piranha::static_vector.
		 * @throws unspecified any exception thrown by piranha::kronecker_array::decode().
		 */
		v_type unpack(const symbol_set &args) const
		{
			return detail::km_unpack<v_type,ka>(args,m_value);
		}
		/// Print.
		/**
		 * Will print to stream a human-readable representation of the monomial.
		 * 
		 * @param[in] os target stream.
		 * @param[in] args reference set of piranha::symbol.
		 * 
		 * @throws unspecified any exception thrown by unpack() or by streaming instances of \p value_type.
		 */
		void print(std::ostream &os, const symbol_set &args) const
		{
			// Don't print anything in case all multipliers are zero.
			if (m_value == value_type(0)) {
				return;
			}
			if (m_flavour) {
				os << "cos(";
			} else {
				os << "sin(";
			}
			const auto tmp = unpack(args);
			piranha_assert(tmp.size() == args.size());
			const value_type zero(0), one(1), m_one(-1);
			for (decltype(tmp.size()) i = 0u; i < tmp.size(); ++i) {
				if (tmp[i] != zero) {
					// A positive multiplier not at the beginning must always be preceded
					// by a "+" sign.
					if (i > 0u && tmp[i] > zero) {
						os << "+";
					}
					// Print the multiplier, unless it's "-1": in that case, just print the minus sign.
					if (tmp[i] == m_one) {
						os << "-";
					} else if (tmp[i] != one) {
						os << tmp[i];
					}
					// Finally, print name of variable.
					os << args[i].get_name();
				}
			}
			os << ")";
		}
	private:
		value_type	m_value;
		bool		m_flavour;
};

}

namespace std
{

/// Specialisation of \p std::hash for piranha::real_trigonometric_kronecker_monomial.
template <typename T>
struct hash<piranha::real_trigonometric_kronecker_monomial<T>>
{
	/// Result type.
	typedef size_t result_type;
	/// Argument type.
	typedef piranha::real_trigonometric_kronecker_monomial<T> argument_type;
	/// Hash operator.
	/**
	 * @param[in] a argument whose hash value will be computed.
	 * 
	 * @return hash value of \p a computed via piranha::real_trigonometric_kronecker_monomial::hash().
	 */
	result_type operator()(const argument_type &a) const
	{
		return a.hash();
	}
};

}

#endif

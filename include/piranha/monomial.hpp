/* Copyright 2009-2017 Francesco Biscani (bluescarni@gmail.com)

This file is part of the Piranha library.

The Piranha library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The Piranha library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the Piranha library.  If not,
see https://www.gnu.org/licenses/. */

#ifndef PIRANHA_MONOMIAL_HPP
#define PIRANHA_MONOMIAL_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <mp++/rational.hpp>

#include <piranha/array_key.hpp>
#include <piranha/config.hpp>
#include <piranha/detail/cf_mult_impl.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/detail/monomial_common.hpp>
#include <piranha/detail/prepare_for_print.hpp>
#include <piranha/detail/safe_integral_arith.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/forwarding.hpp>
#include <piranha/integer.hpp>
#include <piranha/is_cf.hpp>
#include <piranha/is_key.hpp>
#include <piranha/key/key_degree.hpp>
#include <piranha/key/key_is_one.hpp>
#include <piranha/key/key_ldegree.hpp>
#include <piranha/math.hpp>
#include <piranha/math/is_one.hpp>
#include <piranha/math/is_zero.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/rational.hpp>
#include <piranha/s11n.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/term.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

/// Monomial class.
/**
 * This class extends piranha::array_key to define a series key type representing monomials, that is, objects of the
 * form:
 * \f[
 * x_0^{y_0} \cdot x_1^{y_1} \cdot \ldots \cdot x_n^{y_n}.
 * \f]
 * The type \p T represents the type of the exponents \f$ y_i \f$, which are stored in a flat array.
 *
 * This class satisfies the piranha::is_key type trait.
 *
 * ## Type requirements ##
 *
 * \p T and \p S must be suitable for use as first and third template arguments in piranha::array_key. Additionally,
 * \p T must be copy-assignable and it must satisfy the following type-traits:
 * - piranha::is_is_one_type,
 * - piranha::is_ostreamable,
 * - piranha::has_negate.
 *
 * ## Exception safety guarantee ##
 *
 * Unless noted otherwise, this class provides the same exception safety guarantee as piranha::array_key.
 *
 * ## Move semantics ##
 *
 * Move semantics is equivalent to piranha::array_key's move semantics.
 */
// NOTEs:
// - need to fix iterators-related TMP, as done in kronecker_array.
template <typename T, typename S = std::integral_constant<std::size_t, 0u>>
class monomial : public array_key<T, monomial<T, S>, S>
{
    PIRANHA_TT_CHECK(is_is_one_type, const T &);
    PIRANHA_TT_CHECK(is_ostreamable, T);
    PIRANHA_TT_CHECK(has_negate, T);
    PIRANHA_TT_CHECK(std::is_copy_assignable, T);
    using base = array_key<T, monomial<T, S>, S>;
    // Less-than operator.
    template <typename U>
    using comparison_enabler = typename std::enable_if<is_less_than_comparable<U>::value, int>::type;

public:
    /// Arity of the multiply() method.
    static const std::size_t multiply_arity = 1u;
    /// Defaulted default constructor.
    monomial() = default;
    /// Defaulted copy constructor.
    monomial(const monomial &) = default;
    /// Defaulted move constructor.
    monomial(monomial &&) = default;

private:
    // Enabler for ctor from init list.
    template <typename U>
    using init_list_enabler = enable_if_t<std::is_constructible<base, std::initializer_list<U>>::value, int>;

public:
    /// Constructor from initializer list.
    /**
     * \note
     * This constructor is enabled only if the corresponding constructor in piranha::array_key is enabled.
     *
     * @param list initializer list.
     *
     * @see piranha::array_key's constructor from initializer list.
     */
    template <typename U, init_list_enabler<U> = 0>
    explicit monomial(std::initializer_list<U> list) : base(list)
    {
    }

private:
    // Enabler for ctor from range.
    template <typename Iterator>
    using it_ctor_enabler = enable_if_t<
        conjunction<is_input_iterator<Iterator>,
                    is_safely_castable<const typename std::iterator_traits<Iterator>::value_type &, T>>::value,
        int>;

public:
    /// Constructor from range.
    /**
     * \note
     * This constructor is enabled only if \p Iterator is an input iterator whose value type
     * can be cast safely to \p T.
     *
     * This constructor will copy the elements from the range defined by \p begin and \p end into \p this,
     * using piranha::safe_cast() for any necessary type conversion.
     *
     * @param begin beginning of the range.
     * @param end end of the range.
     *
     * @throws unspecified any exception thrown by:
     * - piranha::safe_cast(),
     * - push_back().
     */
    template <typename Iterator, it_ctor_enabler<Iterator> = 0>
    explicit monomial(Iterator begin, Iterator end)
    {
        for (; begin != end; ++begin) {
            this->push_back(piranha::safe_cast<T>(*begin));
        }
    }
    /// Constructor from range and symbol set.
    /**
     * \note
     * This constructor is enabled only if \p Iterator is an input iterator whose value type
     * can be cast safely to \p T.
     *
     * This constructor will copy the elements from the range defined by \p begin and \p end into \p this,
     * using piranha::safe_cast() for any necessary type conversion. If the final size of \p this is different
     * from the size of \p s, a runtime error will be produced. This constructor is used by
     * piranha::polynomial::find_cf().
     *
     * @param begin beginning of the range.
     * @param end end of the range.
     * @param s reference symbol set.
     *
     * @throws std::invalid_argument if the final size of \p this and the size of \p s differ.
     * @throws unspecified any exception thrown by:
     * - piranha::safe_cast(),
     * - push_back().
     */
    template <typename Iterator, it_ctor_enabler<Iterator> = 0>
    explicit monomial(Iterator begin, Iterator end, const symbol_fset &s) : monomial(begin, end)
    {
        if (unlikely(this->size() != s.size())) {
            piranha_throw(std::invalid_argument, "the monomial constructor from range and symbol set "
                                                 "yielded an invalid monomial: the final size is "
                                                     + std::to_string(this->size())
                                                     + ", while the size of the symbol set is "
                                                     + std::to_string(s.size()));
        }
    }
    PIRANHA_FORWARDING_CTOR(monomial, base)
    /// Destructor.
    ~monomial()
    {
        PIRANHA_TT_CHECK(is_key, monomial);
    }
    /// Copy assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     *
     * @throws unspecified any exception thrown by the assignment operator of the base class.
     */
    monomial &operator=(const monomial &other) = default;
    /// Defaulted move assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     */
    monomial &operator=(monomial &&other) = default;
    /// Compatibility check.
    /**
     * A monomial and a set of arguments are compatible if their sizes coincide.
     *
     * @param args reference piranha::symbol_fset.
     *
     * @return ``true`` if the size of ``this`` is equal to the size of ``args``, ``false``
     * otherwise.
     */
    bool is_compatible(const symbol_fset &args) const
    {
        return this->size() == args.size();
    }
    /// Detect linear monomial.
    /**
     * If the monomial is linear in a variable (i.e., all exponents are zero apart from a single unitary
     * exponent), then this method will return a pair formed by the ``true`` value and the position,
     * in ``args``, of the linear variable. Otherwise, the returned value will be a pair formed by the
     * ``false`` value and an unspecified position value.
     *
     * @param args the reference piranha::symbol_fset.
     *
     * @return a pair indicating if the monomial is linear.
     *
     * @throws std::invalid_argument if the sizes of ``this`` and ``args`` differ.
     * @throws unspecified any exception thrown by piranha::is_zero() or piranha::is_one().
     */
    std::pair<bool, symbol_idx> is_linear(const symbol_fset &args) const
    {
        auto sbe = this->size_begin_end();
        if (unlikely(args.size() != std::get<0>(sbe))) {
            piranha_throw(std::invalid_argument, "invalid symbol set for the identification of a linear "
                                                 "monomial: the size of the symbol set ("
                                                     + std::to_string(args.size())
                                                     + ") differs from the size of the monomial ("
                                                     + std::to_string(std::get<0>(sbe)) + ")");
        }
        using size_type = typename base::size_type;
        size_type n_linear = 0u, candidate = 0u;
        for (size_type i = 0u; i < std::get<0>(sbe); ++i, ++std::get<1>(sbe)) {
            // NOTE: is_zero()'s availability is guaranteed by array_key's reqs,
            // is_one() is required by the monomial reqs.
            if (piranha::is_zero(*std::get<1>(sbe))) {
                continue;
            }
            if (!piranha::is_one(*std::get<1>(sbe))) {
                return std::make_pair(false, symbol_idx{0});
            }
            candidate = i;
            ++n_linear;
        }
        piranha_assert(std::get<1>(sbe) == std::get<2>(sbe));
        if (n_linear != 1u) {
            return std::make_pair(false, symbol_idx{0});
        }
        return std::make_pair(true, symbol_idx{candidate});
    }

private:
    // Enabler for pow.
    template <typename U>
    using pow_enabler = monomial_pow_enabler<T, U>;

public:
    /// Monomial exponentiation.
    /**
     * This method will return a monomial corresponding to \p this raised to the ``x``-th power. The exponentiation
     * is computed via the multiplication of the exponents by \p x. The multiplication is performed in different
     * ways, depending on the type ``U``:
     * - if ``T`` and ``U`` are C++ integral types, then the multiplication is checked for overflow; otherwise,
     * - if ``T`` and ``U`` are the same type, and they support math::mul3(), then math::mul3() is used
     *   to compute the product; otherwise,
     * - if the multiplication of ``T`` by ``U`` produces a result of type ``T``, then the binary multiplication
     *   operator is used to compute the product; otherwise,
     * - if the multiplication of ``T`` by ``U`` results in another type ``T2``, then the multiplication
     *   is performed via the binary multiplication operator and the result is cast back to ``T`` via
     *   piranha::safe_cast().
     *
     * If the type ``U`` cannot be used as indicated above, then the method will be disabled.
     *
     * @param x the exponent.
     * @param args the reference piranha::symbol_fset.
     *
     * @return \p this to the power of \p x.
     *
     * @throws std::invalid_argument if the sizes of \p args and \p this differ.
     * @throws std::overflow_error if both ``T`` and ``U`` are integral types and the exponentiation
     * results in overflow.
     * @throws unspecified any exception thrown by:
     * - the construction of a piranha::monomial,
     * - math::mul3(),
     * - the multiplication of the monomial's exponents by ``x``,
     * - piranha::safe_cast().
     */
    template <typename U, pow_enabler<U> = 0>
    monomial pow(const U &x, const symbol_fset &args) const
    {
        auto sbe = this->size_begin_end();
        if (unlikely(args.size() != std::get<0>(sbe))) {
            piranha_throw(std::invalid_argument, "invalid symbol set for the exponentiation of a "
                                                 "monomial: the size of the symbol set ("
                                                     + std::to_string(args.size())
                                                     + ") differs from the size of the monomial ("
                                                     + std::to_string(std::get<0>(sbe)) + ")");
        }
        // Init with zeroes.
        monomial retval(args);
        for (decltype(retval.size()) i = 0u; i < std::get<0>(sbe); ++i, ++std::get<1>(sbe)) {
            monomial_pow_mult_exp(retval[i], *std::get<1>(sbe), x, monomial_pow_dispatcher<T, U>{});
        }
        return retval;
    }

private:
    // Partial utils.
    // Detect decrement operator.
    template <typename U>
    using dec_t = decltype(--std::declval<U &>());
    // Main dispatcher.
    template <typename U>
    using monomial_partial_dispatcher = disjunction_idx<
        // Case 0: U is integral.
        std::is_integral<U>,
        // Case 1: U supports the decrement operator.
        is_detected<dec_t, U>>;
    // In-place decrement by one, checked for integral types.
    template <typename U>
    static void ip_dec(U &x, const std::integral_constant<std::size_t, 0u> &)
    {
        // NOTE: maybe replace with the safe integral subber, eventually.
        if (unlikely(x == std::numeric_limits<U>::min())) {
            piranha_throw(std::overflow_error, "negative overflow error in the calculation of the "
                                               "partial derivative of a monomial");
        }
        x = static_cast<U>(x - U(1));
    }
    template <typename U>
    static void ip_dec(U &x, const std::integral_constant<std::size_t, 1u> &)
    {
        --x;
    }
    // The enabler.
    template <typename U>
    using partial_enabler = enable_if_t<(monomial_partial_dispatcher<U>::value < 2u), int>;

public:
    /// Partial derivative.
    /**
     * \note
     * This method is enabled only if the exponent type supports the pre-decrement operator.
     *
     * This method will return the partial derivative of \p this with respect to the symbol at the position indicated by
     * \p p. The result is a pair consisting of the exponent associated to \p p before differentiation and the monomial
     * itself after differentiation. If \p p is not smaller than the size of \p args or if its corresponding exponent is
     * zero, the returned pair will be <tt>(0,monomial{args})</tt>.
     *
     * If the exponent type is an integral type, then the decrement-by-one operation on the affected exponent is checked
     * for negative overflow.
     *
     * @param p the position of the symbol with respect to which the differentiation will be calculated.
     * @param args the reference piranha::symbol_fset.
     *
     * @return the result of the differentiation.
     *
     * @throws std::invalid_argument if the sizes of \p args and \p this differ.
     * @throws std::overflow_error if the decrement of the affected exponent results in overflow.
     * @throws unspecified any exception thrown by:
     * - the construction of the return value,
     * - the exponent type's pre-decrement operator,
     * - piranha::is_zero().
     */
    template <typename U = T, partial_enabler<U> = 0>
    std::pair<T, monomial> partial(const symbol_idx &p, const symbol_fset &args) const
    {
        auto sbe = this->size_begin_end();
        if (unlikely(args.size() != std::get<0>(sbe))) {
            piranha_throw(std::invalid_argument,
                          "invalid symbol set for the computation of the partial derivative of a "
                          "monomial: the size of the symbol set ("
                              + std::to_string(args.size()) + ") differs from the size of the monomial ("
                              + std::to_string(std::get<0>(sbe)) + ")");
        }
        if (p >= std::get<0>(sbe) || piranha::is_zero(std::get<1>(sbe)[p])) {
            // Derivative wrt a variable not in the monomial: position is outside the bounds, or it refers to a
            // variable with zero exponent.
            return std::make_pair(T(0), monomial{args});
        }
        // Copy the original exponent.
        T expo(std::get<1>(sbe)[p]);
        // Copy the original monomial.
        monomial m(*this);
        // Decrement the affected exponent in m.
        ip_dec(m[static_cast<typename base::size_type>(p)], monomial_partial_dispatcher<U>{});
        // Return the result.
        return std::make_pair(std::move(expo), std::move(m));
    }

private:
    // Integrate utils.
    template <typename U>
    using monomial_int_dispatcher = disjunction_idx<
        // Case 0: U is integral.
        std::is_integral<U>,
        // Case 1: U supports the preincrement operator.
        is_preincrementable<addlref_t<U>>>;
    // In-place increment by one, checked for integral types.
    template <typename U>
    static void ip_inc(U &x, const std::integral_constant<std::size_t, 0u> &)
    {
        // NOTE: maybe replace with the safe integral adder, eventually.
        if (unlikely(x == std::numeric_limits<U>::max())) {
            piranha_throw(std::overflow_error, "positive overflow error in the calculation of the "
                                               "antiderivative of a monomial");
        }
        x = static_cast<U>(x + U(1));
    }
    template <typename U>
    static void ip_inc(U &x, const std::integral_constant<std::size_t, 1u> &)
    {
        ++x;
    }
    // The enabler.
    template <typename U>
    using integrate_enabler = enable_if_t<(monomial_int_dispatcher<U>::value < 2u), int>;

public:
    /// Integration.
    /**
     * \note
     * This method is enabled only if the exponent type supports the pre-increment operator.
     *
     * This method will return the antiderivative of \p this with respect to the symbol \p s. The result is a pair
     * consisting of the exponent associated to \p s increased by one and the monomial itself
     * after integration. If \p s is not in \p args, the returned monomial will have an extra exponent
     * set to 1 in the same position \p s would have if it were added to \p args.
     * If the exponent corresponding to \p s is -1, an error will be produced.
     *
     * If the exponent type is an integral type, then the increment-by-one operation on the affected exponent is checked
     * for negative overflow.
     *
     * @param s the symbol with respect to which the integration will be calculated.
     * @param args the reference piranha::symbol_fset.
     *
     * @return the result of the integration.
     *
     * @throws std::invalid_argument if the sizes of \p args and \p this differ, or
     * if the exponent associated to \p s is -1.
     * @throws std::overflow_error if \p T is a C++ integral type and the integration leads
     * to integer overflow.
     * @throws unspecified any exception thrown by:
     * - piranha::is_zero(),
     * - exponent construction,
     * - push_back(),
     * - the exponent type's pre-increment operator.
     */
    template <typename U = T, integrate_enabler<U> = 0>
    std::pair<T, monomial> integrate(const std::string &s, const symbol_fset &args) const
    {
        auto sbe = this->size_begin_end();
        if (unlikely(args.size() != std::get<0>(sbe))) {
            piranha_throw(std::invalid_argument, "invalid symbol set for the computation of the antiderivative of a "
                                                 "monomial: the size of the symbol set ("
                                                     + std::to_string(args.size())
                                                     + ") differs from the size of the monomial ("
                                                     + std::to_string(std::get<0>(sbe)) + ")");
        }
        monomial retval;
        T expo(0);
        const T one(1);
        auto it_args = args.begin();
        for (decltype(retval.size()) i = 0; std::get<1>(sbe) != std::get<2>(sbe); ++i, ++std::get<1>(sbe), ++it_args) {
            const auto &cur_sym = *it_args;
            if (piranha::is_zero(expo) && s < cur_sym) {
                // If we went past the position of s in args and still we
                // have not performed the integration, it means that we need to add
                // a new exponent.
                retval.push_back(one);
                expo = one;
            }
            retval.push_back(*std::get<1>(sbe));
            if (cur_sym == s) {
                // NOTE: here using i is safe: if retval gained an extra exponent in the condition above,
                // we are never going to land here as cur_sym is at this point never going to be s.
                auto &ref = retval[i];
                // Do the addition and check for zero later, to detect -1 expo.
                ip_inc(ref, monomial_int_dispatcher<U>{});
                if (unlikely(piranha::is_zero(ref))) {
                    piranha_throw(std::invalid_argument,
                                  "unable to perform monomial integration: a negative "
                                  "unitary exponent was encountered in correspondence of the variable '"
                                      + cur_sym + "'");
                }
                expo = ref;
            }
        }
        // If expo is still zero, it means we need to add a new exponent at the end.
        if (piranha::is_zero(expo)) {
            retval.push_back(one);
            expo = one;
        }
        return std::make_pair(std::move(expo), std::move(retval));
    }

private:
    // Let's hard code the custom behaviour for rational exponents for the moment.
    // We can offer a customization point in the future.
    template <typename U, enable_if_t<mppp::is_rational<U>::value, int> = 0>
    static void print_exponent(std::ostream &os, const U &e)
    {
        if (piranha::is_one(e.get_den())) {
            os << e;
        } else {
            os << '(' << e << ')';
        }
    }
    template <typename U, enable_if_t<!mppp::is_rational<U>::value, int> = 0>
    static void print_exponent(std::ostream &os, const U &e)
    {
        os << detail::prepare_for_print(e);
    }

public:
    /// Print.
    /**
     * This method will print to stream a human-readable representation of the monomial.
     *
     * @param os the target stream.
     * @param args the reference piranha::symbol_fset.
     *
     * @throws std::invalid_argument if the sizes of \p args and \p this differ.
     * @throws unspecified any exception resulting from:
     * - printing exponents to stream and the public interface of \p os,
     * - piranha::is_zero(), piranha::is_one().
     */
    void print(std::ostream &os, const symbol_fset &args) const
    {
        auto sbe = this->size_begin_end();
        if (unlikely(args.size() != std::get<0>(sbe))) {
            piranha_throw(std::invalid_argument,
                          "cannot print monomial: the size of the symbol set (" + std::to_string(args.size())
                              + ") differs from the size of the monomial (" + std::to_string(std::get<0>(sbe)) + ")");
        }
        bool empty_output = true;
        auto it_args = args.begin();
        for (decltype(args.size()) i = 0; std::get<1>(sbe) != std::get<2>(sbe); ++i, ++std::get<1>(sbe), ++it_args) {
            if (!piranha::is_zero(*std::get<1>(sbe))) {
                // If we are going to print a symbol, and something has been printed before,
                // then we are going to place the multiplication sign.
                if (!empty_output) {
                    os << '*';
                }
                os << *it_args;
                empty_output = false;
                if (!piranha::is_one(*std::get<1>(sbe))) {
                    os << "**";
                    print_exponent(os, *std::get<1>(sbe));
                }
            }
        }
    }
    /// Print in TeX mode.
    /**
     * This method will print to stream a TeX representation of the monomial.
     *
     * @param os the target stream.
     * @param args the reference piranha::symbol_fset.
     *
     * @throws std::invalid_argument if the sizes of \p args and \p this differ.
     * @throws unspecified any exception resulting from:
     * - construction, comparison and assignment of exponents,
     * - printing exponents to stream and the public interface of \p os,
     * - piranha::math::negate(), piranha::is_zero(), piranha::is_one().
     */
    void print_tex(std::ostream &os, const symbol_fset &args) const
    {
        auto sbe = this->size_begin_end();
        if (unlikely(args.size() != std::get<0>(sbe))) {
            piranha_throw(std::invalid_argument, "cannot print monomial in TeX mode: the size of the symbol set ("
                                                     + std::to_string(args.size())
                                                     + ") differs from the size of the monomial ("
                                                     + std::to_string(std::get<0>(sbe)) + ")");
        }
        std::ostringstream oss_num, oss_den, *cur_oss;
        const T zero(0);
        T cur_value;
        auto it_args = args.begin();
        for (decltype(args.size()) i = 0; std::get<1>(sbe) != std::get<2>(sbe); ++i, ++std::get<1>(sbe), ++it_args) {
            cur_value = *std::get<1>(sbe);
            if (!piranha::is_zero(cur_value)) {
                // NOTE: use this weird form for the test because the presence of operator<()
                // is already guaranteed and thus we don't need additional requirements on T.
                // Maybe in the future we want to do it with a math::sign() function.
                if (zero < cur_value) {
                    cur_oss = std::addressof(oss_num);
                } else {
                    math::negate(cur_value);
                    cur_oss = std::addressof(oss_den);
                }
                *cur_oss << "{" << *it_args << "}";
                if (!piranha::is_one(cur_value)) {
                    *cur_oss << "^{" << detail::prepare_for_print(cur_value) << "}";
                }
            }
        }
        const std::string num_str = oss_num.str(), den_str = oss_den.str();
        if (!num_str.empty() && !den_str.empty()) {
            os << "\\frac{" << num_str << "}{" << den_str << "}";
        } else if (!num_str.empty() && den_str.empty()) {
            os << num_str;
        } else if (num_str.empty() && !den_str.empty()) {
            os << "\\frac{1}{" << den_str << "}";
        }
    }

private:
    // Eval type definition.
    template <typename U>
    using eval_t_ = pow_t<const U &, const T &>;
    template <typename U>
    using eval_type = enable_if_t<conjunction<is_multipliable_in_place<eval_t_<U>>,
                                              std::is_constructible<eval_t_<U>, int>, is_returnable<eval_t_<U>>>::value,
                                  eval_t_<U>>;

public:
    /// Evaluation.
    /**
     * \note
     * This method is available only if \p U satisfies the following requirements:
     * - it can be used in piranha::pow() with the monomial exponents as powers, yielding a type \p eval_type,
     * - \p eval_type is constructible from \p int,
     * - \p eval_type is multipliable in place,
     * - \p eval_type satisfies piranha::is_returnable.
     *
     * The return value will be built by iteratively applying piranha::pow() using the values provided
     * by \p values as bases and the values in the monomial as exponents. If the size of the monomial is zero, 1 will be
     * returned.
     *
     * @param values the values will be used for the evaluation.
     * @param args the reference piranha::symbol_fset.
     *
     * @return the result of evaluating \p this with the values provided in \p values.
     *
     * @throws std::invalid_argument if the sizes of \p values and \p args differ,
     * or if the sizes of \p this and \p args differ.
     * @throws unspecified any exception thrown by:
     * - the construction of the return type,
     * - piranha::pow() or the in-place multiplication operator of the return type.
     */
    template <typename U>
    eval_type<U> evaluate(const std::vector<U> &values, const symbol_fset &args) const
    {
        auto sbe = this->size_begin_end();
        if (unlikely(args.size() != std::get<0>(sbe))) {
            piranha_throw(std::invalid_argument,
                          "cannot evaluate monomial: the size of the symbol set (" + std::to_string(args.size())
                              + ") differs from the size of the monomial (" + std::to_string(std::get<0>(sbe)) + ")");
        }
        if (unlikely(values.size() != std::get<0>(sbe))) {
            piranha_throw(std::invalid_argument,
                          "cannot evaluate monomial: the size of the vector of values (" + std::to_string(values.size())
                              + ") differs from the size of the monomial (" + std::to_string(std::get<0>(sbe)) + ")");
        }
        if (args.size()) {
            eval_type<U> retval(piranha::pow(values[0], *std::get<1>(sbe)++));
            for (decltype(values.size()) i = 1; i < values.size(); ++i, ++std::get<1>(sbe)) {
                // NOTE: here maybe we could use mul3() and pow3() (to be implemented?).
                // NOTE: piranha::pow() for C++ integrals produces an integer result, no need
                // to worry about overflows.
                retval *= piranha::pow(values[i], *std::get<1>(sbe));
            }
            piranha_assert(std::get<1>(sbe) == std::get<2>(sbe));
            return retval;
        }
        return eval_type<U>(1);
    }

private:
    // Subs type is the same as eval type.
    template <typename U>
    using subs_type = eval_type<U>;

public:
    /// Substitution.
    /**
     * \note
     * This method is available only if \p U satisfies the following requirements:
     * - it can be used in piranha::pow() with the monomial exponents as powers, yielding a type \p eval_type,
     * - \p eval_type is constructible from \p int,
     * - \p eval_type is multipliable in place,
     * - \p eval_type satisfies piranha::is_returnable.
     *
     * This method will substitute the symbols at the positions specified in the keys of ``smap`` with the mapped
     * values. The return value is a vector containing one pair in which the first element is the result of the
     * substitution (i.e., the product of the values of ``smap`` raised to the corresponding exponents in the monomial),
     * and the second element is the monomial after the substitution has been performed (i.e., with the exponents
     * at the positions specified by the keys of ``smap`` set to zero). If ``smap`` is empty,
     * the return value will be <tt>(1,this)</tt> (i.e., the monomial is unchanged and the substitution yields 1).
     *
     * For instance, given the monomial ``[2,3,4]``, the reference piranha::symbol_fset ``["x","y","z"]``
     * and the substitution map ``[(0,1),(2,-3)]``, then the return value will be a vector containing
     * the single pair ``(81,[0,3,0])``.
     *
     * @param smap the map relating the positions of the symbols to be substituted to the values
     * they will be substituted with.
     * @param args the reference piranha::symbol_fset.
     *
     * @return the result of the substitution.
     *
     * @throws std::invalid_argument if the last element of the substitution map is not smaller
     * than the size of ``this``, or if the sizes of ``this`` and ``args`` differ.
     * @throws unspecified any exception thrown by:
     * - the construction of the return value,
     * - the construction of an exponent from zero,
     * - the copy assignment of the exponent type,
     * - piranha::pow() or the in-place multiplication operator of the return type,
     * - memory errors in standard containers.
     */
    template <typename U>
    std::vector<std::pair<subs_type<U>, monomial>> subs(const symbol_idx_fmap<U> &smap, const symbol_fset &args) const
    {
        auto sbe = this->size_begin_end();
        if (unlikely(args.size() != std::get<0>(sbe))) {
            piranha_throw(std::invalid_argument,
                          "cannot perform substitution in a monomial: the size of the symbol set ("
                              + std::to_string(args.size()) + ") differs from the size of the monomial ("
                              + std::to_string(std::get<0>(sbe)) + ")");
        }
        if (unlikely(smap.size() && smap.rbegin()->first >= std::get<0>(sbe))) {
            // The last element of the substitution map must be a valid index.
            piranha_throw(std::invalid_argument,
                          "invalid argument(s) for substitution in a monomial: the last index of the substitution map ("
                              + std::to_string(smap.rbegin()->first) + ") must be smaller than the monomial's size ("
                              + std::to_string(std::get<0>(sbe)) + ")");
        }
        std::vector<std::pair<subs_type<U>, monomial>> retval;
        if (smap.size()) {
            // The substitution map contains something, proceed to the substitution.
            const T zero(0);
            // Init the subs return value from the exponentiation of the first value in the map.
            auto it = smap.begin();
            auto ret(piranha::pow(it->second, std::get<1>(sbe)[it->first]));
            // Init the monomial return value with a copy of this.
            auto mon_ret(*this);
            // Zero out the corresponding exponent.
            mon_ret[static_cast<decltype(mon_ret.size())>(it->first)] = zero;
            //  NOTE: move to the next element in the init statement of the for loop.
            for (++it; it != smap.end(); ++it) {
                ret *= piranha::pow(it->second, std::get<1>(sbe)[it->first]);
                mon_ret[static_cast<decltype(mon_ret.size())>(it->first)] = zero;
            }
            // NOTE: the is_returnable requirement ensures we can emplace back a pair
            // containing the subs type.
            retval.emplace_back(std::move(ret), std::move(mon_ret));
        } else {
            // Otherwise, the substitution yields 1 and the monomial is the original one.
            retval.emplace_back(subs_type<U>(1), *this);
        }
        return retval;
    }

private:
    // ipow subs utilities.
    // Dispatcher for the assignment of an exponent to an integer.
    template <typename U>
    using ipow_subs_d_assign_dispatcher = disjunction_idx<
        // Case 0: U is integer or a C++ integral.
        disjunction<std::is_integral<U>, std::is_same<integer, U>>,
        // Case 1: U supports safe cast to integer.
        is_safely_castable<const U &, integer>>;
    template <typename U>
    static void ipow_subs_d_assign(integer &d, const U &expo, const std::integral_constant<std::size_t, 0> &)
    {
        d = expo;
    }
    template <typename U>
    static void ipow_subs_d_assign(integer &d, const U &expo, const std::integral_constant<std::size_t, 1> &)
    {
        d = piranha::safe_cast<integer>(expo);
    }
    // Dispatcher for the assignment of an integer to an exponent.
    template <typename U>
    using ipow_subs_expo_assign_dispatcher = disjunction_idx<
        // Case 0: U is integer.
        std::is_same<integer, U>,
        // Case 1: integer supports safe cast to U.
        is_safely_castable<const integer &, U>>;
    template <typename U>
    static void ipow_subs_expo_assign(U &expo, const integer &r, const std::integral_constant<std::size_t, 0> &)
    {
        expo = r;
    }
    template <typename U>
    static void ipow_subs_expo_assign(U &expo, const integer &r, const std::integral_constant<std::size_t, 1> &)
    {
        expo = piranha::safe_cast<U>(r);
    }
    // Definition of the return type.
    template <typename U>
    using ipow_t_ = pow_t<const U &, const integer &>;
    template <typename U>
    using ipow_subs_type
        = enable_if_t<conjunction<std::is_constructible<ipow_t_<U>, int>, is_returnable<ipow_t_<U>>>::value
                          && (ipow_subs_d_assign_dispatcher<T>::value < 2u)
                          && (ipow_subs_expo_assign_dispatcher<T>::value < 2u),
                      ipow_t_<U>>;

public:
    /// Substitution of integral power.
    /**
     * \note
     * This method is enabled only if:
     * - \p U can be raised to a piranha::integer power, yielding a type \p subs_type,
     * - \p subs_type is constructible from \p int,
     * - \p subs_type satisfies piranha::is_returnable,
     * - the exponent type can be safely cast to piranha::integer, and piranha::integer can be safely
     *   cast to the exponent type.
     *
     * This method will substitute the <tt>n</tt>-th power of the symbol at the position \p p with the quantity \p x.
     * The return value is a vector containing a single pair in which the first element is the result of the
     * substitution, and the second element the monomial after the substitution has been performed.
     * If \p p is not less than the size of \p args, the return value will be <tt>(1,this)</tt> (i.e., the monomial is
     * unchanged and the substitution yields 1).
     *
     * The method will substitute also powers higher than \p n in absolute value.
     * For instance, the substitution of <tt>y**2</tt> with \p a in the monomial <tt>y**7</tt> will produce
     * <tt>a**3 * y</tt>, and the substitution of <tt>y**-2</tt> with \p a in the monomial <tt>y**-7</tt> will produce
     * <tt>a**3 * y**-1</tt>.
     *
     * @param p the position of the symbol that will be substituted.
     * @param n the integral power that will be substituted.
     * @param x the quantity that will be substituted.
     * @param args the reference piranha::symbol_fset.
     *
     * @return the result of substituting \p x for the <tt>n</tt>-th power of the symbol at the position \p p.
     *
     * @throws std::invalid_argument is \p n is zero, or if the sizes of \p args and \p this differ.
     * @throws unspecified any exception thrown by:
     * - the construction of the return value,
     * - piranha::safe_cast(),
     * - piranha::pow(),
     * - arithmetics on piranha::integer,
     * - the copy assignment operator of the exponent type,
     * - memory errors in standard containers.
     */
    template <typename U>
    std::vector<std::pair<ipow_subs_type<U>, monomial>> ipow_subs(const symbol_idx &p, const integer &n, const U &x,
                                                                  const symbol_fset &args) const
    {
        if (unlikely(this->size() != args.size())) {
            piranha_throw(std::invalid_argument,
                          "cannot perform integral power substitution in a monomial: the size of the symbol set ("
                              + std::to_string(args.size()) + ") differs from the size of the monomial ("
                              + std::to_string(this->size()) + ")");
        }
        if (unlikely(!n.sgn())) {
            piranha_throw(std::invalid_argument,
                          "invalid integral power for ipow_subs() in a monomial: the power must be nonzero");
        }
        // Initialise the return value.
        std::vector<std::pair<ipow_subs_type<U>, monomial>> retval;
        auto mon(*this);
        if (p < args.size()) {
            auto &expo = mon[static_cast<decltype(mon.size())>(p)];
            PIRANHA_MAYBE_TLS integer q, r, d;
            // Assign expo to d, possibly safely converting it.
            ipow_subs_d_assign(d, expo, ipow_subs_d_assign_dispatcher<T>{});
            // NOTE: regarding the sign of r: tdiv_qr() sets the sign of r to the sign of q.
            // The only two cases we are interested in here are where d and n have the same sign
            // (otherwise q will have negative sign and we never enter the 'if' below). With
            // d and n positive, everything is straightforward (r's sign will be positive).
            // If d and n are both negative, r will have negative sign, and it will satisfy:
            // q*n + r == d (with d < 0 and d < q*n)
            // This is the result we want: r is the number of steps towards -inf that q*n
            // must take to reach d.
            tdiv_qr(q, r, d, n);
            if (q.sgn() > 0) {
                // Assign back the remainder r to expo, possibly with a safe
                // conversion involved.
                ipow_subs_expo_assign(expo, r, ipow_subs_expo_assign_dispatcher<T>{});
                retval.emplace_back(piranha::pow(x, q), std::move(mon));
                return retval;
            }
        }
        // Otherwise, the substitution yields 1 and the monomial is the original one.
        retval.emplace_back(ipow_subs_type<U>(1), std::move(mon));
        return retval;
    }

private:
    // Multiplication.
    template <typename Cf>
    using multiply_enabler = enable_if_t<conjunction<has_add3<T>, has_mul3<Cf>>::value, int>;

public:
    /// Multiply terms with a monomial key.
    /**
     * \note
     * This method is enabled only if the following conditions hold:
     * - \p T satisfies piranha::has_add3,
     * - \p Cf satisfies piranha::has_mul3.
     *
     * Multiply \p t1 by \p t2, storing the result in the only element of \p res. If \p Cf is an mp++
     * rational, then only the numerators of the coefficients will be multiplied.
     *
     * This method offers the basic exception safety guarantee.
     *
     * @param res the return value.
     * @param t1 the first argument.
     * @param t2 the second argument.
     * @param args the reference piranha::symbol_fset.
     *
     * @throws std::invalid_argument if the size of \p t1 differs from the size of \p args.
     * @throws unspecified any exception thrown by piranha::array_key::vector_add(), or by piranha::math::mul3().
     */
    // NOTE: it should be ok to use this method (and the one below) with overlapping arguments, as this is allowed
    // in small_vector::add().
    template <typename Cf, multiply_enabler<Cf> = 0>
    static void multiply(std::array<term<Cf, monomial>, multiply_arity> &res, const term<Cf, monomial> &t1,
                         const term<Cf, monomial> &t2, const symbol_fset &args)
    {
        term<Cf, monomial> &t = res[0u];
        // NOTE: the check on the monomials' size is in vector_add().
        if (unlikely(t1.m_key.size() != args.size())) {
            piranha_throw(std::invalid_argument,
                          "cannot multiply terms with monomial keys: the size of the symbol set ("
                              + std::to_string(args.size()) + ") differs from the size of the first monomial ("
                              + std::to_string(t1.m_key.size()) + ")");
        }
        // Coefficient.
        cf_mult_impl(t.m_cf, t1.m_cf, t2.m_cf);
        // Now deal with the key.
        t1.m_key.vector_add(t.m_key, t2.m_key);
    }
    /// Comparison operator.
    /**
     * The two monomials will be compared lexicographically.
     *
     * @param other comparison argument.
     *
     * @return \p true if \p this is lexicographically less than \p other, \p false otherwise.
     *
     * @throws std::invalid_argument if the sizes of \p this and \p other differ.
     * @throws unspecified any exception thrown by <tt>std::lexicographical_compare()</tt>.
     */
    bool operator<(const monomial &other) const
    {
        const auto sbe1 = this->size_begin_end();
        const auto sbe2 = other.size_begin_end();
        if (unlikely(std::get<0u>(sbe1) != std::get<0u>(sbe2))) {
            piranha_throw(std::invalid_argument,
                          "mismatched sizes in a monomial comparison: the first monomial has a size of "
                              + std::to_string(std::get<0u>(sbe1)) + ", the second monomial has a size of "
                              + std::to_string(std::get<0u>(sbe2)));
        }
        return std::lexicographical_compare(std::get<1u>(sbe1), std::get<2u>(sbe1), std::get<1u>(sbe2),
                                            std::get<2u>(sbe2));
    }

private:
#if defined(PIRANHA_WITH_BOOST_S11N)
    // Make friend with the s11n functions.
    template <typename Archive, typename T1, typename S1>
    friend void
    boost::serialization::save(Archive &, const piranha::boost_s11n_key_wrapper<piranha::monomial<T1, S1>> &, unsigned);
    template <typename Archive, typename T1, typename S1>
    friend void boost::serialization::load(Archive &, piranha::boost_s11n_key_wrapper<piranha::monomial<T1, S1>> &,
                                           unsigned);
#endif

#if defined(PIRANHA_WITH_MSGPACK)
    // Enablers for msgpack serialization.
    template <typename Stream>
    using msgpack_pack_enabler = enable_if_t<
        conjunction<is_msgpack_stream<Stream>, has_msgpack_pack<Stream, typename base::container_type>>::value, int>;
    template <typename U>
    using msgpack_convert_enabler = enable_if_t<has_msgpack_convert<typename U::container_type>::value, int>;

public:
    /// Serialize in msgpack format.
    /**
     * \note
     * This method is enabled only if \p Stream satisfies piranha::is_msgpack_stream and the internal container type
     * satisfies piranha::has_msgpack_pack.
     *
     * This method will pack \p this into \p packer using the format \p f.
     *
     * @param packer the target packer.
     * @param f the serialization format.
     * @param s the reference piranha::symbol_fset.
     *
     * @throws std::invalid_argument if the sizes of \p s and \p this differ.
     * @throws unspecified any exception thrown by piranha::msgpack_pack().
     */
    template <typename Stream, msgpack_pack_enabler<Stream> = 0>
    void msgpack_pack(msgpack::packer<Stream> &packer, msgpack_format f, const symbol_fset &s) const
    {
        if (unlikely(this->size() != s.size())) {
            piranha_throw(std::invalid_argument, "incompatible symbol set in monomial serialization: the reference "
                                                 "symbol set has a size of "
                                                     + std::to_string(s.size())
                                                     + ", while the monomial being serialized has a size of "
                                                     + std::to_string(this->size()));
        }
        piranha::msgpack_pack(packer, this->m_container, f);
    }
    /// Deserialize from msgpack object.
    /**
     * \note
     * This method is activated only if the internal container type satisfies piranha::has_msgpack_convert.
     *
     * This method will deserialize \p o into \p this using the format \p f.
     * This method provides the basic exception safety guarantee.
     *
     * @param o msgpack object that will be deserialized.
     * @param f serialization format.
     * @param s the reference piranha::symbol_fset.
     *
     * @throws std::invalid_argument if the size of the deserialized array differs from the size of \p s.
     * @throws unspecified any exception thrown by piranha::msgpack_convert().
     */
    template <typename U = monomial, msgpack_convert_enabler<U> = 0>
    void msgpack_convert(const msgpack::object &o, msgpack_format f, const symbol_fset &s)
    {
        piranha::msgpack_convert(this->m_container, o, f);
        if (unlikely(this->size() != s.size())) {
            piranha_throw(std::invalid_argument, "incompatible symbol set in monomial serialization: the reference "
                                                 "symbol set has a size of "
                                                     + std::to_string(s.size())
                                                     + ", while the monomial being deserialized has a size of "
                                                     + std::to_string(this->size()));
        }
    }
#endif
};

template <typename T, typename S>
const std::size_t monomial<T, S>::multiply_arity;

// Implementation of piranha::key_is_one() for monomial.
template <typename T, typename S>
class key_is_one_impl<monomial<T, S>>
{
public:
    bool operator()(const monomial<T, S> &m, const symbol_fset &s) const
    {
        const auto sbe = m.size_begin_end();
        if (unlikely(s.size() != std::get<0>(sbe))) {
            piranha_throw(std::invalid_argument,
                          "invalid sizes in the invocation of key_is_one() for a monomial: the monomial has a size of "
                              + std::to_string(std::get<0>(sbe)) + ", while the reference symbol set has a size of "
                              + std::to_string(s.size()));
        }
        return std::all_of(std::get<1>(sbe), std::get<2>(sbe),
                           [](const T &element) { return piranha::is_zero(element); });
    }
};

// Implementation of piranha::key_degree() for monomial.
template <typename T, typename S>
class key_degree_impl<monomial<T, S>>
{
    // Machinery to determine the degree type.
    template <typename U>
    using degree_type_ = add_t<addlref_t<const U>, addlref_t<const U>>;
    template <typename U>
    using degree_type
        = enable_if_t<conjunction<std::is_constructible<degree_type_<U>, int>, is_addable_in_place<degree_type_<U>, U>,
                                  is_returnable<degree_type_<U>>>::value,
                      degree_type_<U>>;
    // Helpers to add exponents in the degree computation.
    template <typename U>
    static void expo_add(degree_type<U> &retval, const U &n, const std::true_type &)
    {
        retval = safe_int_add(retval, static_cast<degree_type<U>>(n));
    }
    template <typename U>
    static void expo_add(degree_type<U> &retval, const U &x, const std::false_type &)
    {
        retval += x;
    }

public:
    /// Degree.
    /**
     * \note
     * This method is enabled only if:
     * - \p T is addable, yielding a type ``degree_type``,
     * - ``degree_type`` is constructible from \p int,
     * - monomial::value_type can be added in-place to ``degree_type``,
     * - ``degree_type`` satisfies piranha::is_returnable.
     *
     * This method will return the degree of the monomial, computed via the summation of the exponents of the monomial.
     * If \p T is a C++ integral type, the addition of the exponents will be checked for overflow.
     *
     * @param args reference piranha::symbol_fset.
     *
     * @return the degree of the monomial.
     *
     * @throws std::invalid_argument if the sizes of \p args and \p this differ.
     * @throws std::overflow_error if the exponent type is a C++ integral type and the computation
     * of the degree overflows.
     * @throws unspecified any exception thrown by the invoked constructor or arithmetic operators.
     */
    template <typename U>
    degree_type<U> operator()(const monomial<U, S> &m, const symbol_fset &args) const
    {
        auto sbe = m.size_begin_end();
        if (unlikely(args.size() != std::get<0>(sbe))) {
            piranha_throw(
                std::invalid_argument,
                "invalid symbol set for the computation of the degree of a monomial: the size of the symbol set ("
                    + std::to_string(args.size()) + ") differs from the size of the monomial ("
                    + std::to_string(std::get<0>(sbe)) + ")");
        }
        degree_type<U> retval(0);
        for (; std::get<1>(sbe) != std::get<2>(sbe); ++std::get<1>(sbe)) {
            expo_add(retval, *std::get<1>(sbe), std::is_integral<U>{});
        }
        return retval;
    }
    /// Partial degree.
    /**
     * \note
     * This method is enabled only if:
     * - \p T is addable, yielding a type ``degree_type``,
     * - ``degree_type`` is constructible from \p int,
     * - monomial::value_type can be added in-place to ``degree_type``,
     * - ``degree_type`` satisfies piranha::is_returnable.
     *
     * This method will return the partial degree of the monomial, computed via the summation of the exponents of the
     * monomial. If \p T is a C++ integral type, the addition of the exponents will be checked for overflow.
     *
     * The \p p argument is used to indicate the positions of the exponents to be taken into account when computing the
     * partial degree. Exponents at positions not present in \p p will be discarded during the computation of the
     * partial degree.
     *
     * @param p positions of the symbols to be considered.
     * @param args reference piranha::symbol_fset.
     *
     * @return the partial degree of the monomial.
     *
     * @throws std::invalid_argument if the sizes of \p args and \p this differ, or if the largest value in \p p is
     * not less than the size of the monomial.
     * @throws std::overflow_error if the exponent type is a C++ integral type and the computation
     * of the degree overflows.
     * @throws unspecified any exception thrown by the invoked constructor or arithmetic operators.
     */
    template <typename U>
    degree_type<U> operator()(const monomial<U, S> &m, const symbol_idx_fset &p, const symbol_fset &args) const
    {
        auto sbe = m.size_begin_end();
        if (unlikely(args.size() != std::get<0>(sbe))) {
            piranha_throw(std::invalid_argument, "invalid symbol set for the computation of the partial degree of a "
                                                 "monomial: the size of the symbol set ("
                                                     + std::to_string(args.size())
                                                     + ") differs from the size of the monomial ("
                                                     + std::to_string(std::get<0>(sbe)) + ")");
        }
        if (unlikely(p.size() && *p.rbegin() >= args.size())) {
            piranha_throw(std::invalid_argument, "the largest value in the positions set for the computation of the "
                                                 "partial degree of a monomial is "
                                                     + std::to_string(*p.rbegin())
                                                     + ", but the monomial has a size of only "
                                                     + std::to_string(args.size()));
        }
        degree_type<U> retval(0);
        for (const auto &i : p) {
            expo_add(retval, std::get<1>(sbe)[i], std::is_integral<U>{});
        }
        return retval;
    }
};

// Implementation of piranha::key_ldegree() for monomial.
template <typename T, typename S>
class key_ldegree_impl<monomial<T, S>> : public key_degree_impl<monomial<T, S>>
{
};
} // namespace piranha

#if defined(PIRANHA_WITH_BOOST_S11N)

// Implementation of the Boost s11n api.
namespace boost
{
namespace serialization
{

template <typename Archive, typename T, typename S>
inline void save(Archive &ar, const piranha::boost_s11n_key_wrapper<piranha::monomial<T, S>> &k, unsigned)
{
    if (unlikely(k.key().size() != k.ss().size())) {
        piranha_throw(std::invalid_argument, "incompatible symbol set in monomial serialization: the reference "
                                             "symbol set has a size of "
                                                 + std::to_string(k.ss().size())
                                                 + ", while the monomial being serialized has a size of "
                                                 + std::to_string(k.key().size()));
    }
    piranha::boost_save(ar, k.key().m_container);
}

template <typename Archive, typename T, typename S>
inline void load(Archive &ar, piranha::boost_s11n_key_wrapper<piranha::monomial<T, S>> &k, unsigned)
{
    piranha::boost_load(ar, k.key().m_container);
    if (unlikely(k.key().size() != k.ss().size())) {
        piranha_throw(std::invalid_argument, "incompatible symbol set in monomial serialization: the reference "
                                             "symbol set has a size of "
                                                 + std::to_string(k.ss().size())
                                                 + ", while the monomial being deserialized has a size of "
                                                 + std::to_string(k.key().size()));
    }
}

template <typename Archive, typename T, typename S>
inline void serialize(Archive &ar, piranha::boost_s11n_key_wrapper<piranha::monomial<T, S>> &k, unsigned version)
{
    split_free(ar, k, version);
}
} // namespace serialization
} // namespace boost

namespace piranha
{

inline namespace impl
{

// Enablers for the boost s11n methods.
template <typename Archive, typename T, typename S>
using monomial_boost_save_enabler
    = enable_if_t<has_boost_save<Archive, typename monomial<T, S>::container_type>::value>;

template <typename Archive, typename T, typename S>
using monomial_boost_load_enabler
    = enable_if_t<has_boost_load<Archive, typename monomial<T, S>::container_type>::value>;
} // namespace impl

/// Specialisation of piranha::boost_save() for piranha::monomial.
/**
 * \note
 * This specialisation is enabled only if piranha::monomial::container_type satisfies piranha::has_boost_save.
 *
 * @throws std::invalid_argument if the size of the monomial differs from the size of the piranha::symbol_fset.
 * @throws unspecified any exception thrown by piranha::bost_save() or by the public interface of
 * piranha::boost_s11n_key_wrapper.
 */
template <typename Archive, typename T, typename S>
struct boost_save_impl<Archive, boost_s11n_key_wrapper<monomial<T, S>>, monomial_boost_save_enabler<Archive, T, S>>
    : boost_save_via_boost_api<Archive, boost_s11n_key_wrapper<monomial<T, S>>> {
};

/// Specialisation of piranha::boost_load() for piranha::monomial.
/**
 * \note
 * This specialisation is enabled only if piranha::monomial::container_type satisfies piranha::has_boost_load.
 *
 * @throws std::invalid_argument if the size of the deserialized monomial differs from the size of
 * the piranha::symbol_fset.
 * @throws unspecified any exception thrown by piranha::bost_load() or by the public interface of
 * piranha::boost_s11n_key_wrapper.
 */
template <typename Archive, typename T, typename S>
struct boost_load_impl<Archive, boost_s11n_key_wrapper<monomial<T, S>>, monomial_boost_load_enabler<Archive, T, S>>
    : boost_load_via_boost_api<Archive, boost_s11n_key_wrapper<monomial<T, S>>> {
};
} // namespace piranha

#endif

namespace std
{

/// Specialisation of \p std::hash for piranha::monomial.
/**
 * Functionally equivalent to the \p std::hash specialisation for piranha::array_key.
 */
template <typename T, typename S>
struct hash<piranha::monomial<T, S>> : public hash<piranha::array_key<T, piranha::monomial<T, S>, S>> {
};
} // namespace std

#endif

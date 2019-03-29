/* Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)

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

#ifndef PIRANHA_POLYNOMIAL_HPP
#define PIRANHA_POLYNOMIAL_HPP

#include <algorithm>
#include <boost/numeric/conversion/cast.hpp>
#include <cmath> // For std::ceil.
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <map>
#include <mutex>
#include <numeric>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "base_series_multiplier.hpp"
#include "config.hpp"
#include "debug_access.hpp"
#include "detail/atomic_flag_array.hpp"
#include "detail/cf_mult_impl.hpp"
#include "detail/divisor_series_fwd.hpp"
#include "detail/parallel_vector_transform.hpp"
#include "detail/poisson_series_fwd.hpp"
#include "detail/polynomial_fwd.hpp"
#include "detail/safe_integral_adder.hpp"
#include "detail/sfinae_types.hpp"
#include "exceptions.hpp"
#include "forwarding.hpp"
#include "ipow_substitutable_series.hpp"
#include "is_cf.hpp"
#include "key_is_multipliable.hpp"
#include "kronecker_array.hpp"
#include "kronecker_monomial.hpp"
#include "math.hpp"
#include "monomial.hpp"
#include "mp_integer.hpp"
#include "pow.hpp"
#include "power_series.hpp"
#include "safe_cast.hpp"
#include "series.hpp"
#include "series_multiplier.hpp"
#include "settings.hpp"
#include "substitutable_series.hpp"
#include "symbol.hpp"
#include "symbol_set.hpp"
#include "t_substitutable_series.hpp"
#include "thread_pool.hpp"
#include "trigonometric_series.hpp"
#include "tuning.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

// Tag for polynomial instances.
struct polynomial_tag {
};

// Type trait to check the key type in polynomial.
template <typename T>
struct is_polynomial_key {
    static const bool value = false;
};

template <typename T>
struct is_polynomial_key<kronecker_monomial<T>> {
    static const bool value = true;
};

template <typename T, typename U>
struct is_polynomial_key<monomial<T, U>> {
    static const bool value = true;
};

// Implementation detail to check if the monomial key supports the linear_argument() method.
template <typename Key>
struct key_has_linarg : detail::sfinae_types {
    template <typename U>
    static auto test(const U &u) -> decltype(u.linear_argument(std::declval<const symbol_set &>()));
    static no test(...);
    static const bool value = std::is_same<std::string, decltype(test(std::declval<Key>()))>::value;
};

// Division utilities.
// Return iterator to the leading term of a polynomial, as defined by the operator<()
// of its key type. This is always available for any monomial type.
template <typename PType>
inline auto poly_lterm(const PType &p) -> decltype(p._container().begin())
{
    piranha_assert(!p.empty());
    using term_type = typename PType::term_type;
    return std::max_element(p._container().begin(), p._container().end(),
                            [](const term_type &t1, const term_type &t2) { return t1.m_key < t2.m_key; });
}

// Check that there are no negative exponents
// in a polynomial.
template <typename PType>
inline void poly_expo_checker(const PType &p)
{
    using term_type = typename PType::term_type;
    using expo_type = typename term_type::key_type::value_type;
    // Don't run any check if the exponent is unsigned.
    if (std::is_integral<expo_type>::value && std::is_unsigned<expo_type>::value) {
        return;
    }
    const auto &args = p.get_symbol_set();
    if (std::any_of(p._container().begin(), p._container().end(),
                    [&args](const term_type &t) { return t.m_key.has_negative_exponent(args); })) {
        piranha_throw(std::invalid_argument, "negative exponents are not allowed");
    }
}

// Multiply polynomial by non-zero cf in place. Preconditions:
// - a is not zero.
// Type requirements:
// - cf type supports in-place multiplication.
template <typename PType>
inline void poly_cf_mult(const typename PType::term_type::cf_type &a, PType &p)
{
    piranha_assert(!math::is_zero(a));
    const auto it_f = p._container().end();
    for (auto it = p._container().begin(); it != it_f; ++it) {
        it->m_cf *= a;
        piranha_assert(!math::is_zero(it->m_cf));
    }
}

// Divide polynomial by non-zero cf in place. Preconditions:
// - a is not zero.
// Type requirements:
// - cf type supports divexact().
// NOTE: this is always used in exact divisions, we could wrap mp_integer::_divexact()
// to improve performance.
template <typename PType>
inline void poly_exact_cf_div(PType &p, const typename PType::term_type::cf_type &a)
{
    piranha_assert(!math::is_zero(a));
    const auto it_f = p._container().end();
    for (auto it = p._container().begin(); it != it_f; ++it) {
        math::divexact(it->m_cf, it->m_cf, a);
        piranha_assert(!math::is_zero(it->m_cf));
    }
}

// Univariate polynomial GCD via PRS-SR.
// Implementation based on subresultant polynomial remainder sequence:
// https://en.wikipedia.org/wiki/Polynomial_greatest_common_divisor
// See also Zippel, 8.6. This implementation is actually based on
// Winkler, chapter 4.
// NOTE: here we do not need the full sequence of remainders, consider
// removing the vector of polys once this is debugged and tested.
// Preconditions:
// - univariate polynomials on the same variable.
// Type requirements:
// - cf type supports exponentiation to unsigned, yielding the same type,
// - cf type * cf type is still cf type, and multipliable in-place,
// - cf type has divexact.
template <typename PType>
inline PType gcd_prs_sr(PType a, PType b)
{
    using term_type = typename PType::term_type;
    using cf_type = typename term_type::cf_type;
    using key_type = typename term_type::key_type;
    // Only univariate polynomials are allowed.
    piranha_assert(a.get_symbol_set().size() == 1u && a.get_symbol_set() == b.get_symbol_set());
    // If one of the two is zero, the gcd is the other.
    if (math::is_zero(a) && !math::is_zero(b)) {
        return b;
    }
    if (!math::is_zero(a) && math::is_zero(b)) {
        return a;
    }
    // If both are zero, return zero.
    if (math::is_zero(a) && math::is_zero(b)) {
        PType retval;
        retval.set_symbol_set(a.get_symbol_set());
        return retval;
    }
    // Check exponents.
    poly_expo_checker(a);
    poly_expo_checker(b);
    // Order so that deg(a) >= deg(b).
    if (poly_lterm(a)->m_key < poly_lterm(b)->m_key) {
        std::swap(a, b);
    }
    // Cache the arguments set. Make a copy as "a" will be moved below.
    const auto args = a.get_symbol_set();
    // NOTE: coefficients are alway ctible from ints.
    cf_type h(1), g(1);
    // Store the content of a and b for later use.
    auto a_cont = a.content(), b_cont = b.content();
    // NOTE: it seems like removing the content from the inputs
    // can help performance. Let's revisit this after we implement
    // the sorted data structure for polys.
    // poly_exact_cf_div(a,a_cont);
    // poly_exact_cf_div(b,b_cont);
    std::vector<PType> F;
    using size_type = typename std::vector<PType>::size_type;
    F.push_back(std::move(a));
    F.push_back(std::move(b));
    PType fprime(F.back());
    size_type i = 2u;
    // A key representing the constant univariate monomial.
    // NOTE: all monomials support construction from init list.
    key_type zero_key{typename key_type::value_type(0)};
    while (fprime.size() != 0u && poly_lterm(fprime)->m_key != zero_key) {
        auto l2 = poly_lterm(F[static_cast<size_type>(i - 2u)]), l1 = poly_lterm(F[static_cast<size_type>(i - 1u)]);
        // NOTE: we are using the degree here in order to maintain compatibility with
        // all monomials. There is some small overhead in this, but it should not
        // matter too much.
        // NOTE: this will be used only on monomials with integral exponents, so it is always valid.
        integer delta(l2->m_key.degree(args)), l1d(l1->m_key.degree(args));
        piranha_assert(delta >= l1d);
        // NOTE: this is a pseudo-remainder operation here, we don't use the poly method as we need the delta
        // information below.
        delta -= l1d;
        poly_cf_mult(math::pow(l1->m_cf, static_cast<unsigned>(delta + 1)), F[static_cast<size_type>(i - 2u)]);
        fprime = PType::udivrem(F[static_cast<size_type>(i - 2u)], F[static_cast<size_type>(i - 1u)]).second;
        if (fprime.size() != 0u) {
            const cf_type h_delta = math::pow(h, static_cast<unsigned>(delta));
            auto tmp(fprime);
            poly_exact_cf_div(tmp, g * h_delta);
            g = l1->m_cf;
            h = (math::pow(g, static_cast<unsigned>(delta)) * h);
            math::divexact(h, h, h_delta);
            F.push_back(std::move(tmp));
            safe_integral_adder(i, size_type(1u));
        }
    }
    auto retval = std::move(F.back());
    auto content = retval.content();
    poly_exact_cf_div(retval, content);
    poly_cf_mult(math::gcd(a_cont, b_cont), retval);
    return retval;
}

// Establish the limits of the exponents of two polynomials. Will throw if a negative exponent is encountered.
// Preconditions:
// - equal symbol sets.
// - non-empty polys.
template <typename PType>
inline std::vector<std::pair<typename PType::term_type::key_type::value_type,
                             typename PType::term_type::key_type::value_type>>
poly_establish_limits(const PType &n, const PType &d)
{
    using expo_type = typename PType::term_type::key_type::value_type;
    using v_type = std::vector<expo_type>;
    using vp_type = std::vector<std::pair<expo_type, expo_type>>;
    using vp_size_type = typename vp_type::size_type;
    piranha_assert(n.get_symbol_set() == d.get_symbol_set());
    piranha_assert(n.size() != 0u && d.size() != 0u);
    // Return value, init with the first element from the numerator.
    vp_type retval;
    v_type tmp;
    auto it = n._container().begin(), it_f = n._container().end();
    it->m_key.extract_exponents(tmp, n.get_symbol_set());
    std::transform(tmp.begin(), tmp.end(), std::back_inserter(retval),
                   [](const expo_type &e) { return std::make_pair(e, e); });
    for (; it != it_f; ++it) {
        it->m_key.extract_exponents(tmp, n.get_symbol_set());
        for (decltype(tmp.size()) i = 0u; i < tmp.size(); ++i) {
            // NOTE: greater-than comparability is ensured for all exponent types on which this
            // method will be used.
            if (tmp[i] < retval[static_cast<vp_size_type>(i)].first) {
                retval[static_cast<vp_size_type>(i)].first = tmp[i];
            } else if (tmp[i] > retval[static_cast<vp_size_type>(i)].second) {
                retval[static_cast<vp_size_type>(i)].second = tmp[i];
            }
        }
    }
    // Denominator.
    it_f = d._container().end();
    for (it = d._container().begin(); it != it_f; ++it) {
        it->m_key.extract_exponents(tmp, n.get_symbol_set());
        for (decltype(tmp.size()) i = 0u; i < tmp.size(); ++i) {
            if (tmp[i] < retval[static_cast<vp_size_type>(i)].first) {
                retval[static_cast<vp_size_type>(i)].first = tmp[i];
            } else if (tmp[i] > retval[static_cast<vp_size_type>(i)].second) {
                retval[static_cast<vp_size_type>(i)].second = tmp[i];
            }
        }
    }
    // Check for negative expos.
    if (std::any_of(retval.begin(), retval.end(),
                    [](const std::pair<expo_type, expo_type> &p) { return p.first < expo_type(0); })) {
        piranha_throw(std::invalid_argument, "negative exponents are not allowed");
    }
    return retval;
}

// Simple dot product.
template <typename It1, typename It2>
inline auto poly_dot_product(It1 begin1, It1 end1, It2 begin2) -> decltype((*begin1) * (*begin2))
{
    using ret_type = decltype((*begin1) * (*begin2));
    return std::inner_product(begin1, end1, begin2, ret_type(0));
}

// Convert multivariate polynomial to univariate. Preconditions:
// - at least 1 symbolic argument,
// - identical symbol set for n and d,
// - non-null input polys.
template <typename Cf, typename Key>
inline std::tuple<polynomial<Cf, k_monomial>, polynomial<Cf, k_monomial>, std::vector<typename k_monomial::value_type>>
poly_to_univariate(const polynomial<Cf, Key> &n, const polynomial<Cf, Key> &d)
{
    piranha_assert(n.get_symbol_set().size() > 0u);
    piranha_assert(n.get_symbol_set() == d.get_symbol_set());
    piranha_assert(n.size() != 0u && d.size() != 0u);
    using k_expo_type = typename k_monomial::value_type;
    using expo_type = typename Key::value_type;
    // Cache the args.
    const auto &args = n.get_symbol_set();
    // Establish the limits of the two polynomials.
    auto limits = poly_establish_limits(n, d);
    // Extract just the vector of max exponents.
    std::vector<expo_type> max_expos;
    std::transform(limits.begin(), limits.end(), std::back_inserter(max_expos),
                   [](const std::pair<expo_type, expo_type> &p) { return p.second; });
    using me_size_type = decltype(max_expos.size());
    // Next we need to check the limits of the codification. We do everything in multiprecision and
    // then we will attempt to cast back.
    std::vector<integer> mp_cv;
    mp_cv.push_back(integer(1));
    for (decltype(args.size()) i = 0u; i < args.size(); ++i) {
        mp_cv.push_back(mp_cv.back() * (integer(1) + max_expos[static_cast<me_size_type>(i)]));
    }
    // Determine the max univariate expo.
    integer mp_kM = poly_dot_product(max_expos.begin(), max_expos.end(), mp_cv.begin());
    // Attempt casting everything down.
    auto kM = static_cast<k_expo_type>(mp_kM);
    (void)kM;
    std::vector<k_expo_type> cv;
    std::transform(mp_cv.begin(), mp_cv.end(), std::back_inserter(cv),
                   [](const integer &z) { return static_cast<k_expo_type>(z); });
    // Proceed to the codification.
    auto encode_poly = [&args, &cv](const polynomial<Cf, Key> &p) -> polynomial<Cf, k_monomial> {
        using term_type = typename polynomial<Cf, k_monomial>::term_type;
        polynomial<Cf, k_monomial> retval;
        // The only symbol will be the first symbol from the input polys.
        symbol_set ss;
        ss.add(*args.begin());
        retval.set_symbol_set(ss);
        // Prepare rehashed.
        retval._container().rehash(p._container().bucket_count());
        // Go with the codification.
        std::vector<expo_type> tmp_expos;
        for (const auto &t : p._container()) {
            // Extract the exponents.
            t.m_key.extract_exponents(tmp_expos, args);
            // NOTE: unique insertion?
            retval.insert(term_type{t.m_cf, k_monomial(static_cast<k_expo_type>(
                                                poly_dot_product(tmp_expos.begin(), tmp_expos.end(), cv.begin())))});
        }
        return retval;
    };
    return std::make_tuple(encode_poly(n), encode_poly(d), cv);
}

// From univariate to multivariate. Preconditions:
// - n has only 1 symbol,
// - coding vector and args are consistent,
// - non-empty poly.
template <typename Key, typename Cf>
inline polynomial<Cf, Key> poly_from_univariate(const polynomial<Cf, k_monomial> &n,
                                                const std::vector<typename k_monomial::value_type> &cv,
                                                const symbol_set &args)
{
    piranha_assert(n.get_symbol_set().size() == 1u);
    piranha_assert(cv.size() > 1u);
    piranha_assert(args.size() == cv.size() - 1u);
    piranha_assert(*n.get_symbol_set().begin() == *args.begin());
    piranha_assert(n.size() != 0u);
    using expo_type = typename Key::value_type;
    using cv_size_type = decltype(cv.size());
    auto decode_poly = [&args, &cv](const polynomial<Cf, k_monomial> &p) -> polynomial<Cf, Key> {
        using term_type = typename polynomial<Cf, Key>::term_type;
        // Init the return value.
        polynomial<Cf, Key> retval;
        retval.set_symbol_set(args);
        retval._container().rehash(p._container().bucket_count());
        // Go with the decodification.
        std::vector<expo_type> tmp_expos;
        using v_size_type = typename std::vector<expo_type>::size_type;
        tmp_expos.resize(safe_cast<v_size_type>(args.size()));
        for (const auto &t : p._container()) {
            for (v_size_type i = 0u; i < tmp_expos.size(); ++i) {
                // NOTE: use construction here rather than static_cast as expo_type could be integer.
                tmp_expos[i] = expo_type((t.m_key.get_int() % cv[static_cast<cv_size_type>(i + 1u)])
                                         / cv[static_cast<cv_size_type>(i)]);
            }
            retval.insert(term_type{t.m_cf, Key(tmp_expos.begin(), tmp_expos.end())});
        }
        return retval;
    };
    return decode_poly(n);
}

// Exception to signal that heuristic GCD failed.
struct gcdheu_failure final : std::runtime_error {
    explicit gcdheu_failure() : std::runtime_error("")
    {
    }
};

// This implementation is taken straight from the Geddes book. We used to have an implementation based on the paper
// from Liao & Fateman, which supposedly is (or was) implemented in Maple, but that implementation had a bug which
// showed
// up very rarely. Strangely enough, the sympy implementation of heugcd (also based on the Liao paper) has exactly the
// same
// issue. Thus the motivation to use the implementation from Geddes.
// This function returns:
// - a status flag, to signal if the heuristic failed (true if failure occurred),
// - the GCD and the 2 cofactors (a / GCD and b / GCD).
// The function can also fail via throwing gcdheu_failure, which is a different type of failure from the status flag
// (the
// gcdheu_failure signals that too large integers are being generated, the status flag signals that too many iterations
// have been run). This difference is of no consequence for the user, as the exception is caught in try_gcdheu.
template <typename Poly>
std::pair<bool, std::tuple<Poly, Poly, Poly>> gcdheu_geddes(const Poly &a, const Poly &b,
                                                            symbol_set::size_type s_index = 0u)
{
    static_assert(is_mp_integer<typename Poly::term_type::cf_type>::value, "Invalid type.");
    // Require identical symbol sets.
    piranha_assert(a.get_symbol_set() == b.get_symbol_set());
    // Helper function to compute the symmetric version of the modulo operation.
    // See Geddes 4.2 and:
    // http://www.maplesoft.com/support/help/maple/view.aspx?path=mod
    auto mod_s = [](const integer &n, const integer &m) -> integer {
        // NOTE: require that the denominator is strictly positive.
        piranha_assert(m > 0);
        auto retval = n % m;
        if (retval < -((m - 1) / 2)) {
            retval += m;
        } else if (retval > m / 2) {
            retval -= m;
        }
        return retval;
    };
    // Apply mod_s to all coefficients in a poly.
    auto mod_s_poly = [mod_s](const Poly &p, const integer &m) -> Poly {
        Poly retval(p);
        auto it = retval._container().begin();
        for (; it != retval._container().end();) {
            it->m_cf = mod_s(it->m_cf, m);
            if (math::is_zero(it->m_cf)) {
                it = retval._container().erase(it);
            } else {
                ++it;
            }
        }
        return retval;
    };
    // Cache it.
    const auto &args = a.get_symbol_set();
    // The contents of input polys.
    const integer a_cont = a.content(), b_cont = b.content();
    // Handle empty polys.
    if (a.size() == 0u && b.size() == 0u) {
        Poly retval;
        retval.set_symbol_set(args);
        return std::make_pair(false, std::make_tuple(retval, retval, retval));
    }
    if (a.size() == 0u) {
        Poly retval;
        retval.set_symbol_set(args);
        // NOTE: 'retval + 1' could be improved performance-wise.
        return std::make_pair(false, std::make_tuple(b, retval, retval + 1));
    }
    if (b.size() == 0u) {
        Poly retval;
        retval.set_symbol_set(args);
        // NOTE: 'retval + 1' could be improved performance-wise.
        return std::make_pair(false, std::make_tuple(a, retval + 1, retval));
    }
    // If we are at the first recursion, check the exponents.
    if (s_index == 0u) {
        poly_expo_checker(a);
        poly_expo_checker(b);
    }
    // This is the case in which the polynomials are reduced to two integers (essentially, the zerovariate case).
    if (a.is_single_coefficient() && b.is_single_coefficient()) {
        piranha_assert(a.size() == 1u && b.size() == 1u);
        Poly retval;
        retval.set_symbol_set(args);
        auto gab = math::gcd(a_cont, b_cont);
        // NOTE: the creation of the gcd retval and of the cofactors can be improved performance-wise
        // (e.g., exact division of coefficients).
        return std::make_pair(false, std::make_tuple(retval + gab, a / gab, b / gab));
    }
    // s_index is the recursion index for the gcdheu() call.
    piranha_assert(s_index < args.size());
    // We need to work on primitive polys.
    // NOTE: performance improvements.
    const auto ap = a / a_cont;
    const auto bp = b / b_cont;
    // The current variable.
    const std::string var = args[s_index].get_name();
    integer xi{2 * std::min(ap.height(), bp.height()) + 2};
    // Functor to update xi at each iteration.
    auto update_xi = [&xi]() {
        // NOTE: this is just a way of making xi bigger without too much correlation
        // to its previous value.
        xi = (xi * 73794) / 27011;
    };
    // NOTE: the values of 6 iterations is taken straight from the original implementation of the algorithm.
    // The bits limit is larger than the original implementation, on the grounds that GMP should be rather efficient
    // at bignum. It might as well be that other tuning params are better, but let's keep them hardcoded for now.
    for (int i = 0; i < 6; ++i) {
        if (integer(xi.bits_size()) * std::max(ap.degree({var}), bp.degree({var})) > 100000) {
            piranha_throw(gcdheu_failure, );
        }
        auto res = gcdheu_geddes(ap.subs(var, xi), bp.subs(var, xi), static_cast<symbol_set::size_type>(s_index + 1u));
        if (!res.first) {
            Poly &gamma = std::get<0u>(res.second);
            Poly G;
            G.set_symbol_set(args);
            unsigned j = 0;
            while (gamma.size() != 0u) {
                Poly g(mod_s_poly(gamma, xi));
                // NOTE: term mult here could be useful, but probably it makes more
                // sense to just improve the series multiplication routine.
                G += g * math::pow(Poly{var}, j);
                gamma -= g;
                poly_exact_cf_div(gamma, xi);
                safe_integral_adder(j, 1u);
            }
            // NOTE: I don't know if this is possible at all, but it's better to be safe than
            // sorry. This prevents divisions by zero below.
            if (G.size() == 0u) {
                update_xi();
                continue;
            }
            try {
                // For the division test, we need to operate on the primitive
                // part of the candidate GCD.
                poly_exact_cf_div(G, G.content());
                // The division test.
                auto cf_a = ap / G;
                auto cf_b = bp / G;
                // Need to multiply by the GCD of the contents of a and b in order to produce
                // the GCD with the largest coefficients.
                const auto F = math::gcd(a_cont, b_cont);
                poly_cf_mult(F, G);
                // The cofactors above have been computed with the primitive parts of G and a,b.
                // We need to rescale them in order to find the true cofactors, that is, a / G
                // and b / G.
                poly_cf_mult(a_cont, cf_a);
                poly_exact_cf_div(cf_a, F);
                poly_cf_mult(b_cont, cf_b);
                poly_exact_cf_div(cf_b, F);
                return std::make_pair(false, std::make_tuple(std::move(G), std::move(cf_a), std::move(cf_b)));
            } catch (const math::inexact_division &) {
                // Continue in case the division check fails.
            }
        }
        update_xi();
    }
    return std::make_pair(true, std::make_tuple(Poly{}, Poly{}, Poly{}));
}

// Namespace for generic polynomial division enabler, used to stuff
// in handy aliases. We put it here in order to share it with the enabler
// for the divexact specialisation.
namespace ptd
{

template <typename T>
using cf_t = typename T::term_type::cf_type;

template <typename T>
using expo_t = typename T::term_type::key_type::value_type;

template <typename T, typename U>
using enabler = typename std::
    enable_if<std::is_base_of<polynomial_tag, T>::value && std::is_same<T, U>::value
                  && is_multipliable_in_place<cf_t<T>>::value
                  && std::is_same<decltype(std::declval<const cf_t<T> &>() * std::declval<const cf_t<T> &>()),
                                  cf_t<T>>::value
                  && has_exact_division<cf_t<T>>::value && has_exact_ring_operations<cf_t<T>>::value
                  && is_subtractable_in_place<T>::value
                  && (std::is_integral<expo_t<T>>::value || is_mp_integer<expo_t<T>>::value),
              int>::type;
}
}

/// Polynomial GCD algorithms.
enum class polynomial_gcd_algorithm {
    /// Automatic selection.
    automatic,
    /// Subresultant PRS.
    prs_sr,
    /// Heuristic GCD.
    heuristic
};

/// Polynomial class.
/**
 * This class represents multivariate polynomials as collections of multivariate polynomial terms.
 * \p Cf represents the ring over which the polynomial is defined, while \p Key represents the monomial type.
 *
 * Polynomials support an automatic degree-based truncation mechanism, disabled by default, which comes into play during
 * polynomial multiplication. It allows to discard automatically all those terms, generated during series
 * multiplication, whose total or partial degree is greater than a specified limit. This mechanism can be configured via
 * a set of thread-safe static methods, and it is enabled if:
 * - the total and partial degree of the series are represented by the same type \p D,
 * - all the truncation-related requirements in piranha::power_series are satsified,
 * - the type \p D is equality-comparable, subtractable and the type resulting from the subtraction is still \p D.
 *
 * This class satisfies the piranha::is_series and piranha::is_cf type traits.
 *
 * \warning
 * The division and GCD operations are known to have poor performance, especially with large operands. Performance
 * will be improved in future versions.
 *
 * ## Type requirements ##
 *
 * \p Cf must be suitable for use in piranha::series as first template argument,
 * \p Key must be an instance of either piranha::monomial or piranha::kronecker_monomial.
 *
 * ## Exception safety guarantee ##
 *
 * This class provides the same guarantee as the base series type it derives from.
 *
 * ## Move semantics ##
 *
 * Move semantics is equivalent to the move semantics of the base series type it derives from.
 */
template <typename Cf, typename Key>
class polynomial
    : public power_series<trigonometric_series<ipow_substitutable_series<substitutable_series<t_substitutable_series<series<Cf,
                                                                                                                            Key,
                                                                                                                            polynomial<Cf,
                                                                                                                                       Key>>,
                                                                                                                     polynomial<Cf,
                                                                                                                                Key>>,
                                                                                              polynomial<Cf, Key>>,
                                                                         polynomial<Cf, Key>>>,
                          polynomial<Cf, Key>>,
      detail::polynomial_tag
{
    // Check the key.
    PIRANHA_TT_CHECK(detail::is_polynomial_key, Key);
    // Make friend with debug class.
    template <typename>
    friend class debug_access;
    // Make friend with all poly types.
    template <typename, typename>
    friend class polynomial;
    // Make friend with Poisson series.
    template <typename>
    friend class poisson_series;
    // Make friend with divisor series.
    template <typename, typename>
    friend class divisor_series;
    // The base class.
    using base
        = power_series<trigonometric_series<ipow_substitutable_series<substitutable_series<t_substitutable_series<series<Cf,
                                                                                                                         Key,
                                                                                                                         polynomial<Cf,
                                                                                                                                    Key>>,
                                                                                                                  polynomial<Cf,
                                                                                                                             Key>>,
                                                                                           polynomial<Cf, Key>>,
                                                                      polynomial<Cf, Key>>>,
                       polynomial<Cf, Key>>;
    // A couple of helpers from C++14.
    template <typename T>
    using decay_t = typename std::decay<T>::type;
    template <bool B, typename T = void>
    using enable_if_t = typename std::enable_if<B, T>::type;
    // String constructor.
    template <typename Str>
    void construct_from_string(Str &&str)
    {
        typedef typename base::term_type term_type;
        // Insert the symbol.
        this->m_symbol_set.add(symbol(std::forward<Str>(str)));
        // Construct and insert the term.
        this->insert(term_type(Cf(1), typename term_type::key_type{1}));
    }
    template <typename T = Key,
              typename std::enable_if<detail::key_has_linarg<T>::value && has_safe_cast<integer, Cf>::value, int>::type
              = 0>
    std::map<std::string, integer> integral_combination() const
    {
        try {
            std::map<std::string, integer> retval;
            for (auto it = this->m_container.begin(); it != this->m_container.end(); ++it) {
                const std::string lin_arg = it->m_key.linear_argument(this->m_symbol_set);
                piranha_assert(retval.find(lin_arg) == retval.end());
                retval[lin_arg] = safe_cast<integer>(it->m_cf);
            }
            return retval;
        } catch (const std::invalid_argument &) {
            // NOTE: this currently catches failures both in lin_arg and safe_cast, as safe_cast_failure
            // inherits from std::invalid_argument.
            piranha_throw(std::invalid_argument, "polynomial is not an integral linear combination");
        }
    }
    template <
        typename T = Key,
        typename std::enable_if<!detail::key_has_linarg<T>::value || !has_safe_cast<integer, Cf>::value, int>::type = 0>
    std::map<std::string, integer> integral_combination() const
    {
        piranha_throw(std::invalid_argument,
                      "the polynomial type does not support the extraction of a linear combination");
    }
    // Integration utils.
    // Empty for SFINAE.
    template <typename T, typename = void>
    struct integrate_type_ {
    };
    // The type resulting from the integration of the key of series T.
    template <typename T>
    using key_integrate_type
        = decltype(std::declval<const typename T::term_type::key_type &>()
                       .integrate(std::declval<const symbol &>(), std::declval<const symbol_set &>())
                       .first);
    // Basic integration requirements for series T, to be satisfied both when the coefficient is integrable
    // and when it is not. ResT is the type of the result of the integration.
    template <typename T, typename ResT>
    using basic_integrate_requirements = typename std::enable_if<
        // Coefficient differentiable, and can call is_zero on the result.
        has_is_zero<decltype(math::partial(std::declval<const typename T::term_type::cf_type &>(),
                                           std::declval<const std::string &>()))>::value
        &&
        // The key is integrable.
        detail::true_tt<key_integrate_type<T>>::value &&
        // The result needs to be addable in-place.
        is_addable_in_place<ResT>::value &&
        // It also needs to be ctible from zero.
        std::is_constructible<ResT, int>::value>::type;
    // Non-integrable coefficient.
    template <typename T>
    using nic_res_type = decltype((std::declval<const T &>() * std::declval<const typename T::term_type::cf_type &>())
                                  / std::declval<const key_integrate_type<T> &>());
    template <typename T>
    struct integrate_type_<T, typename std::
                                  enable_if<!is_integrable<typename T::term_type::cf_type>::value
                                            && detail::true_tt<basic_integrate_requirements<T, nic_res_type<T>>>::
                                                   value>::type> {
        using type = nic_res_type<T>;
    };
    // Integrable coefficient.
    // The type resulting from the differentiation of the key of series T.
    template <typename T>
    using key_partial_type
        = decltype(std::declval<const typename T::term_type::key_type &>()
                       .partial(std::declval<const symbol_set::positions &>(), std::declval<const symbol_set &>())
                       .first);
    // Type resulting from the integration of the coefficient.
    template <typename T>
    using i_cf_type = decltype(
        math::integrate(std::declval<const typename T::term_type::cf_type &>(), std::declval<const std::string &>()));
    // Type above, multiplied by the type coming out of the derivative of the key.
    template <typename T>
    using i_cf_type_p = decltype(std::declval<const i_cf_type<T> &>() * std::declval<const key_partial_type<T> &>());
    // Final series type.
    template <typename T>
    using ic_res_type = decltype(std::declval<const i_cf_type_p<T> &>() * std::declval<const T &>());
    template <typename T>
    struct integrate_type_<T,
                           typename std::
                               enable_if<is_integrable<typename T::term_type::cf_type>::value
                                         && detail::true_tt<basic_integrate_requirements<T, ic_res_type<T>>>::value &&
                                         // We need to be able to add the non-integrable type.
                                         is_addable_in_place<ic_res_type<T>, nic_res_type<T>>::value &&
                                         // We need to be able to compute the partial degree and cast it to integer.
                                         has_safe_cast<integer,
                                                       decltype(
                                                           std::declval<const typename T::term_type::key_type &>()
                                                               .degree(std::declval<const symbol_set::positions &>(),
                                                                       std::declval<const symbol_set &>()))>::value
                                         &&
                                         // This is required in the initialisation of the return value.
                                         std::is_constructible<i_cf_type_p<T>, i_cf_type<T>>::value &&
                                         // We need to be able to assign the integrated coefficient times key partial.
                                         std::is_assignable<i_cf_type_p<T> &, i_cf_type_p<T>>::value &&
                                         // Needs math::negate().
                                         has_negate<i_cf_type_p<T>>::value>::type> {
        using type = ic_res_type<T>;
    };
    // Final typedef.
    template <typename T>
    using integrate_type = typename std::enable_if<is_returnable<typename integrate_type_<T>::type>::value,
                                                   typename integrate_type_<T>::type>::type;
    // Integration with integrable coefficient.
    template <typename T = polynomial>
    integrate_type<T> integrate_impl(const symbol &s, const typename base::term_type &term,
                                     const std::true_type &) const
    {
        typedef typename base::term_type term_type;
        typedef typename term_type::cf_type cf_type;
        typedef typename term_type::key_type key_type;
        // Get the partial degree of the monomial in integral form.
        integer degree;
        const symbol_set::positions pos(this->m_symbol_set, symbol_set{s});
        try {
            degree = safe_cast<integer>(term.m_key.degree(pos, this->m_symbol_set));
        } catch (const safe_cast_failure &) {
            piranha_throw(std::invalid_argument,
                          "unable to perform polynomial integration: cannot extract the integral form of an exponent");
        }
        // If the degree is negative, integration by parts won't terminate.
        if (degree.sign() < 0) {
            piranha_throw(std::invalid_argument,
                          "unable to perform polynomial integration: negative integral exponent");
        }
        polynomial tmp;
        tmp.set_symbol_set(this->m_symbol_set);
        key_type tmp_key = term.m_key;
        tmp.insert(term_type(cf_type(1), tmp_key));
        i_cf_type_p<T> i_cf(math::integrate(term.m_cf, s.get_name()));
        integrate_type<T> retval(i_cf * tmp);
        for (integer i(1); i <= degree; ++i) {
            // Update coefficient and key. These variables are persistent across loop iterations.
            auto partial_key = tmp_key.partial(pos, this->m_symbol_set);
            i_cf = math::integrate(i_cf, s.get_name()) * std::move(partial_key.first);
            // Account for (-1)**i.
            math::negate(i_cf);
            // Build the other factor from the derivative of the monomial.
            tmp = polynomial{};
            tmp.set_symbol_set(this->m_symbol_set);
            tmp_key = std::move(partial_key.second);
            // NOTE: don't move tmp_key, as it needs to hold a valid value
            // for the next loop iteration.
            tmp.insert(term_type(cf_type(1), tmp_key));
            retval += i_cf * tmp;
        }
        return retval;
    }
    // Integration with non-integrable coefficient.
    template <typename T = polynomial>
    integrate_type<T> integrate_impl(const symbol &, const typename base::term_type &, const std::false_type &) const
    {
        piranha_throw(std::invalid_argument,
                      "unable to perform polynomial integration: coefficient type is not integrable");
    }
    // Template alias for use in pow() overload. Will check via SFINAE that the base pow() method can be called with
    // argument T and that exponentiation of key type is legal.
    template <typename T>
    using key_pow_t
        = decltype(std::declval<Key const &>().pow(std::declval<const T &>(), std::declval<const symbol_set &>()));
    template <typename T>
    using pow_ret_type = enable_if_t<is_detected<key_pow_t, T>::value,
                                     decltype(std::declval<series<Cf, Key, polynomial<Cf, Key>> const &>().pow(
                                         std::declval<const T &>()))>;
    // Invert utils.
    template <typename Series>
    using inverse_type = decltype(std::declval<const Series &>().pow(-1));
    // Auto-truncation machinery.
    // The degree and partial degree types, detected via math::degree().
    template <typename T>
    using degree_type = decltype(math::degree(std::declval<const T &>()));
    template <typename T>
    using pdegree_type
        = decltype(math::degree(std::declval<const T &>(), std::declval<const std::vector<std::string> &>()));
    // Enablers for auto-truncation: degree and partial degree must be the same, series must support
    // math::truncate_degree(), degree type must be subtractable and yield the same type.
    template <typename T>
    using at_degree_enabler =
        typename std::enable_if<std::is_same<degree_type<T>, pdegree_type<T>>::value
                                    && has_truncate_degree<T, degree_type<T>>::value
                                    && std::is_same<decltype(std::declval<const degree_type<T> &>()
                                                             - std::declval<const degree_type<T> &>()),
                                                    degree_type<T>>::value
                                    && is_equality_comparable<degree_type<T>>::value,
                                int>::type;
    // For the setter, we need the above plus we need to be able to convert safely U to the degree type.
    template <typename T, typename U>
    using at_degree_set_enabler =
        typename std::enable_if<detail::true_tt<at_degree_enabler<T>>::value && has_safe_cast<degree_type<T>, U>::value,
                                int>::type;
    // This needs to be separate from the other static inits because we don't have anything to init
    // if the series does not support degree computation.
    // NOTE: here the important thing is that this method does not return the same object for different series types,
    // as the intent of the truncation mechanism is that each polynomial type has its own settings.
    // We need to keep this in mind if we need static resources that must be unique for the series type, sometimes
    // adding the Derived series as template argument in a toolbox might actually be necessary because of this. Note
    // that, contrary to the, e.g., custom derivatives map in series.hpp here we don't care about the type of T - we
    // just need to be able to extract the term type from it.
    template <typename T = polynomial>
    static degree_type<T> &get_at_degree_max()
    {
        // Init to zero for peace of mind - though this should never be accessed
        // if the auto-truncation is not used.
        static degree_type<T> at_degree_max(0);
        return at_degree_max;
    }
    // Enabler for string construction.
    template <typename Str>
    using str_enabler =
        typename std::enable_if<std::is_same<typename std::decay<Str>::type, std::string>::value
                                    || std::is_same<typename std::decay<Str>::type, char *>::value
                                    || std::is_same<typename std::decay<Str>::type, const char *>::value,
                                int>::type;
    // Implementation of find_cf().
    template <typename T>
    using find_cf_enabler =
        typename std::enable_if<std::is_constructible<
                                    typename base::term_type::key_type, decltype(std::begin(std::declval<const T &>())),
                                    decltype(std::end(std::declval<const T &>())), const symbol_set &>::value
                                    && has_begin_end<const T>::value,
                                int>::type;
    template <typename T>
    using find_cf_init_list_enabler = find_cf_enabler<std::initializer_list<T>>;
    template <typename Iterator>
    Cf find_cf_impl(Iterator begin, Iterator end) const
    {
        typename base::term_type tmp_term{Cf(0), Key(begin, end, this->m_symbol_set)};
        auto it = this->m_container.find(tmp_term);
        if (it == this->m_container.end()) {
            return Cf(0);
        }
        return it->m_cf;
    }
    // Division utilities.
    // Couple of handy aliases.
    template <typename T>
    using cf_t = typename T::term_type::cf_type;
    template <typename T>
    using key_t = typename T::term_type::key_type;
    template <typename T>
    using expo_t = typename T::term_type::key_type::value_type;
    // Multiply polynomial by a term. Preconditions:
    // - p is not zero,
    // - the coefficient of the term is not zero.
    // Type requirements:
    // - cf type supports multiplication,
    // - key type supports multiply.
    template <typename PType>
    static PType term_mult(const typename PType::term_type &t, const PType &p)
    {
        using term_type = typename PType::term_type;
        using key_type = typename term_type::key_type;
        piranha_assert(p.size() != 0u);
        piranha_assert(!math::is_zero(t.m_cf));
        // Initialise the return value.
        PType retval;
        retval.set_symbol_set(p.get_symbol_set());
        // Prepare an adequate number of buckets (same as input poly).
        retval._container().rehash(p._container().bucket_count());
        // Go with the loop.
        key_type tmp_key;
        const auto it_f = p._container().end();
        for (auto it = p._container().begin(); it != it_f; ++it) {
            key_type::multiply(tmp_key, t.m_key, it->m_key, p.get_symbol_set());
            piranha_assert(!math::is_zero(t.m_cf * it->m_cf));
            // NOTE: here we could use the unique_insert machinery
            // to improve performance.
            retval.insert(term_type{t.m_cf * it->m_cf, tmp_key});
        }
        return retval;
    }
    // Univariate euclidean division.
    template <bool CheckExpos, typename T>
    static std::pair<polynomial, polynomial> udivrem_impl(const T &n, const T &d)
    {
        using term_type = typename base::term_type;
        using cf_type = typename term_type::cf_type;
        using key_type = typename term_type::key_type;
        // Only univariate polynomials are allowed.
        if (unlikely(n.get_symbol_set().size() != 1u || n.get_symbol_set() != d.get_symbol_set())) {
            piranha_throw(std::invalid_argument, "only univariate polynomials in the same variable "
                                                 "are allowed in the polynomial division with remainder routine");
        }
        if (unlikely(d.size() == 0u)) {
            piranha_throw(zero_division_error, "univariate polynomial division by zero");
        }
        // Cache the symbol set for brevity.
        const auto &args = n.get_symbol_set();
        // If the numerator is zero, just return two empty polys.
        if (n.size() == 0u) {
            polynomial q, r;
            q.set_symbol_set(args);
            r.set_symbol_set(args);
            return std::make_pair(std::move(q), std::move(r));
        }
        // Check negative exponents, if requested.
        if (CheckExpos) {
            // Check if there are negative exponents.
            detail::poly_expo_checker(n);
            detail::poly_expo_checker(d);
        }
        // Initialisation: quotient is empty, remainder is the numerator.
        polynomial q, r(n);
        q.set_symbol_set(args);
        // Leading term of the denominator, always the same.
        const auto lden = detail::poly_lterm(d);
        piranha_assert(!math::is_zero(lden->m_cf));
        // Temp cf and key used for computations in the loop.
        cf_type tmp_cf;
        key_type tmp_key;
        while (true) {
            if (r.size() == 0u) {
                break;
            }
            // Leading term of the remainder.
            const auto lr = poly_lterm(r);
            if (lr->m_key < lden->m_key) {
                break;
            }
            // NOTE: we want to check that the division is exact here,
            // and throw if this is not the case.
            // NOTE: this should be made configurable to optimise the case in which we
            // know the division will be exact (e.g., in GCD).
            math::divexact(tmp_cf, lr->m_cf, lden->m_cf);
            key_type::divide(tmp_key, lr->m_key, lden->m_key, args);
            term_type t{tmp_cf, tmp_key};
            // NOTE: here we are basically progressively removing terms from r until
            // it gets to zero. This sounds exactly like the kind of situation in which
            // the iteration over the hash table could become really slow. This might
            // matter when we need to re-determine the leading term of r above.
            // We should consider adding a re-hashing every time we do a +/- operation on
            // a series and the load factor gets below a certain threshold. We should also
            // review uses of the insert() method of series to spot other possible
            // places where this could be a problem (although once we have a good behaviour
            // for +/- we should be mostly good - multiplication should already be ok).
            r -= term_mult(t, d);
            q.insert(std::move(t));
        }
        return std::make_pair(std::move(q), std::move(r));
    }
    // Multivariate exact division.
    template <typename T>
    static T division_impl(const T &n, const T &d)
    {
        static_assert(std::is_same<T, polynomial>::value, "Invalid type.");
        using term_type = typename polynomial::term_type;
        using expo_type = typename term_type::key_type::value_type;
        // Cache it.
        const auto &args = n.get_symbol_set();
        // Univariate case.
        if (args.size() == 1u) {
            // NOTE: udivrem contains the check for negative exponents, zero n/d, etc.
            auto res = polynomial::udivrem(n, d);
            if (res.second.size() != 0u) {
                piranha_throw(math::inexact_division, );
            }
            return std::move(res.first);
        }
        // Multivariate or zerovariate case.
        // NOTE: here we need to check a few things as the mapping to univariate has certain prereqs.
        // First we check if n or d are zero.
        if (d.size() == 0u) {
            piranha_throw(zero_division_error, "polynomial division by zero");
        }
        if (n.size() == 0u) {
            polynomial retval;
            retval.set_symbol_set(args);
            return retval;
        }
        // Deal with the case in which the number of arguments is zero. We need to do it here as the mapping
        // requires at least 1 var.
        if (args.size() == 0u) {
            piranha_assert(n.size() == 1u && d.size() == 1u);
            piranha_assert(n.is_single_coefficient() && d.is_single_coefficient());
            polynomial retval;
            retval.set_symbol_set(args);
            Cf tmp_cf;
            math::divexact(tmp_cf, n._container().begin()->m_cf, d._container().begin()->m_cf);
            retval.insert(term_type{std::move(tmp_cf), Key(args)});
            return retval;
        }
        // Map to univariate.
        auto umap = detail::poly_to_univariate(n, d);
        // Do the univariate division.
        // NOTE: need to call udivrem_impl from the mapped univariate type.
        // NOTE: the check for negative exponents is done in the mapping.
        auto ures = std::get<0u>(umap).template udivrem_impl<false>(std::get<0u>(umap), std::get<1u>(umap));
        // Check if the division was exact.
        if (ures.second.size() != 0u) {
            piranha_throw(math::inexact_division, );
        }
        // Map back to multivariate.
        auto retval = detail::poly_from_univariate<Key>(ures.first, std::get<2u>(umap), args);
        // Final check: for each variable, the degree of retval + degree of d must be equal to the degree of n.
        auto degree_vector_extractor = [&args](const polynomial &p) -> std::vector<expo_type> {
            std::vector<expo_type> ret, tmp;
            // Init with the first term of the polynomial.
            piranha_assert(p.size() != 0u);
            auto it = p._container().begin();
            it->m_key.extract_exponents(ret, args);
            ++it;
            for (; it != p._container().end(); ++it) {
                it->m_key.extract_exponents(tmp, args);
                for (decltype(tmp.size()) i = 0u; i < tmp.size(); ++i) {
                    if (tmp[i] > ret[i]) {
                        ret[i] = tmp[i];
                    }
                }
            }
            return ret;
        };
        const auto n_dv = degree_vector_extractor(n), d_dv = degree_vector_extractor(d),
                   r_dv = degree_vector_extractor(retval);
        for (decltype(n_dv.size()) i = 0u; i < n_dv.size(); ++i) {
            if (integer(n_dv[i]) != integer(d_dv[i]) + integer(r_dv[i])) {
                piranha_throw(math::inexact_division, );
            }
        }
        return retval;
    }
    // Enabler for exact poly division.
    // NOTE: the further constraining here that T must be polynomial is to work around a GCC < 5 (?) bug,
    // in which template operators from other template instantiations of polynomial<> are considered. We can
    // remove this once we bump the minimum version of GCC required.
    template <typename T, typename U = T>
    using poly_div_enabler
        = enable_if_t<std::is_same<decay_t<T>, polynomial>::value, detail::ptd::enabler<decay_t<T>, decay_t<U>>>;
    // Enabler for in-place division.
    template <typename T, typename U>
    using poly_in_place_div_enabler
        = enable_if_t<detail::true_tt<poly_div_enabler<T, U>>::value && !std::is_const<T>::value, int>;
    // Join enabler.
    template <typename T>
    using join_enabler =
        typename std::enable_if<std::is_base_of<detail::polynomial_tag, cf_t<T>>::value
                                    && std::is_same<Key, key_t<T>>::value
                                    && detail::true_tt<decltype(std::declval<cf_t<T> &>()
                                                                += std::declval<const cf_t<T> &>()
                                                                   * std::declval<const cf_t<T> &>())>::value,
                                int>::type;
    // Content enabler.
    template <typename T>
    using content_enabler = typename std::enable_if<has_gcd3<cf_t<T>>::value, int>::type;
    // Primitive part enabler.
    template <typename T>
    using pp_enabler =
        typename std::enable_if<detail::true_tt<content_enabler<T>>::value && has_exact_division<cf_t<T>>::value,
                                int>::type;
    // Prem enabler.
    template <typename T>
    using uprem_enabler = typename std::
        enable_if<detail::true_tt<poly_div_enabler<T>>::value
                      && std::is_same<decltype(math::pow(std::declval<const cf_t<T> &>(), 0u)), cf_t<T>>::value,
                  int>::type;
    // Enabler for GCD.
    template <typename T>
    using gcd_enabler = typename std::
        enable_if<has_gcd<cf_t<T>>::value && has_gcd3<cf_t<T>>::value
                      && std::is_same<decltype(math::pow(std::declval<const cf_t<T> &>(), 0u)), cf_t<T>>::value
                      && is_multipliable_in_place<cf_t<T>>::value
                      && std::is_same<decltype(std::declval<const cf_t<T> &>() * std::declval<const cf_t<T> &>()),
                                      cf_t<T>>::value
                      && has_exact_division<cf_t<T>>::value && std::is_copy_assignable<cf_t<T>>::value
                      && detail::true_tt<content_enabler<T>>::value && detail::true_tt<poly_div_enabler<T>>::value,
                  int>::type;
    // Height.
    template <typename T>
    using height_type_ = decltype(math::abs(std::declval<const cf_t<T> &>()));
    template <typename T>
    using height_type = typename std::enable_if<std::is_constructible<height_type_<T>, int>::value
                                                    && is_less_than_comparable<height_type_<T>>::value
                                                    && std::is_move_assignable<height_type_<T>>::value
                                                    && (std::is_copy_constructible<height_type_<T>>::value
                                                        || std::is_move_constructible<height_type_<T>>::value),
                                                height_type_<T>>::type;
    // Wrapper around heuristic GCD. It will return false,tuple if the calculation went well,
    // true,tuple otherwise. If run on polynomials with non-integer coefficients, it will throw
    // if the requested algorithm is specifically the heuristic one.
    template <typename T, typename std::enable_if<detail::is_mp_integer<cf_t<T>>::value, int>::type = 0>
    static std::pair<bool, std::tuple<T, T, T>> try_gcdheu(const T &a, const T &b, polynomial_gcd_algorithm)
    {
        try {
            return detail::gcdheu_geddes(a, b);
        } catch (const detail::gcdheu_failure &) {
        }
        return std::make_pair(true, std::tuple<T, T, T>{});
    }
    template <typename T, typename std::enable_if<!detail::is_mp_integer<cf_t<T>>::value, int>::type = 0>
    static std::pair<bool, std::tuple<T, T, T>> try_gcdheu(const T &, const T &, polynomial_gcd_algorithm algo)
    {
        // The idea here is that this is a different kind of failure from the one we throw in the main gcd()
        // function, and we want to be able to discriminate the two.
        if (algo == polynomial_gcd_algorithm::heuristic) {
            piranha_throw(std::runtime_error, "the heuristic polynomial GCD algorithm was explicitly selected, "
                                              "but it cannot be applied to non-integral coefficients");
        }
        return std::make_pair(true, std::tuple<T, T, T>{});
    }
    // This is a wrapper to compute and return the cofactors, together with the GCD, when the PRS algorithm
    // is used (the gcdheu algorithm already computes the cofactors).
    template <typename T, typename U>
    static std::tuple<T, T, T> wrap_gcd_cofactors(U &&gcd, const T &a, const T &b, bool with_cofactors)
    {
        if (with_cofactors) {
            if (math::is_zero(gcd)) {
                // In this case, a tuple of zeroes will be returned.
                return std::make_tuple(std::forward<U>(gcd), T{}, T{});
            } else {
                return std::make_tuple(std::forward<U>(gcd), a / gcd, b / gcd);
            }
        } else {
            return std::make_tuple(std::forward<U>(gcd), T{}, T{});
        }
    }
    // Enabler for untruncated multiplication.
    template <typename T>
    using um_enabler =
        typename std::enable_if<std::is_same<T, decltype(std::declval<const T &>() * std::declval<const T &>())>::value,
                                int>::type;
    // Enabler for truncated multiplication.
    template <typename T, typename U>
    using tm_enabler =
        typename std::enable_if<std::is_same<T, decltype(std::declval<const T &>() * std::declval<const T &>())>::value
                                    && has_safe_cast<degree_type<T>, U>::value
                                    && detail::true_tt<at_degree_enabler<T>>::value,
                                int>::type;
    // Common bits for truncated/untruncated multiplication. Will do the usual merging of the symbol sets
    // before calling the runner functor, which performs the actual multiplication.
    template <typename Functor>
    static polynomial um_tm_implementation(const polynomial &p1, const polynomial &p2, const Functor &runner)
    {
        const auto &ss1 = p1.get_symbol_set(), &ss2 = p2.get_symbol_set();
        if (ss1 == ss2) {
            return runner(p1, p2);
        }
        // If the symbol sets are not the same, we need to merge them and make
        // copies of the original operands as needed.
        auto merge = ss1.merge(ss2);
        const bool need_copy_1 = (merge != ss1), need_copy_2 = (merge != ss2);
        if (need_copy_1) {
            polynomial copy_1(p1.extend_symbol_set(merge));
            if (need_copy_2) {
                polynomial copy_2(p2.extend_symbol_set(merge));
                return runner(copy_1, copy_2);
            }
            return runner(copy_1, p2);
        } else {
            polynomial copy_2(p2.extend_symbol_set(merge));
            return runner(p1, copy_2);
        }
    }
    // Helper function to clear the pow cache when a new auto truncation limit is set.
    template <typename T>
    static void truncation_clear_pow_cache(int mode, const T &max_degree, const std::vector<std::string> &names)
    {
        // The pow cache is cleared only if we are actually changing the truncation settings.
        if (s_at_degree_mode != mode || get_at_degree_max() != max_degree || names != s_at_degree_names) {
            polynomial::clear_pow_cache();
        }
    }

public:
    /// Series rebind alias.
    template <typename Cf2>
    using rebind = polynomial<Cf2, Key>;
    /// Defaulted default constructor.
    /**
     * Will construct a polynomial with zero terms.
     */
    polynomial() = default;
    /// Defaulted copy constructor.
    polynomial(const polynomial &) = default;
    /// Defaulted move constructor.
    polynomial(polynomial &&) = default;
    /// Constructor from symbol name.
    /**
     * \note
     * This template constructor is enabled only if the decay type of \p Str is a C or C++ string.
     *
     * Will construct a univariate polynomial made of a single term with unitary coefficient and exponent, representing
     * the symbolic variable \p name. The type of \p name must be a string type (either C or C++).
     *
     * @param name name of the symbolic variable that the polynomial will represent.
     *
     * @throws unspecified any exception thrown by:
     * - piranha::symbol_set::add(),
     * - the constructor of piranha::symbol from string,
     * - the invoked constructor of the coefficient type,
     * - the invoked constructor of the key type,
     * - the constructor of the term type from coefficient and key,
     * - piranha::series::insert().
     */
    template <typename Str, str_enabler<Str> = 0>
    explicit polynomial(Str &&name) : base()
    {
        construct_from_string(std::forward<Str>(name));
    }
    PIRANHA_FORWARDING_CTOR(polynomial, base)
    /// Trivial destructor.
    ~polynomial()
    {
        PIRANHA_TT_CHECK(is_cf, polynomial);
        PIRANHA_TT_CHECK(is_series, polynomial);
    }
    /// Copy assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     *
     * @throws unspecified any exception thrown by the assignment operator of the base class.
     */
    polynomial &operator=(const polynomial &other) = default;
    /// Move assignment operator.
    /**
     * @param other the assignment argument.
     *
     * @return a reference to \p this.
     */
    polynomial &operator=(polynomial &&other) = default;
    PIRANHA_FORWARDING_ASSIGNMENT(polynomial, base)
    /// Override default exponentiation method.
    /**
     * \note
     * This method is enabled only if piranha::series::pow() can be called with exponent \p x
     * and the key type can be raised to the power of \p x via its exponentiation method.
     *
     * This exponentiation override will check if the polynomial consists of a single-term with non-unitary
     * key. In that case, the return polynomial will consist of a single term with coefficient computed via
     * piranha::math::pow() and key computed via the monomial exponentiation method. Otherwise, the base
     * (i.e., default) exponentiation method will be used.
     *
     * @param x exponent.
     *
     * @return \p this to the power of \p x.
     *
     * @throws unspecified any exception thrown by:
     * - the <tt>is_unitary()</tt> and exponentiation methods of the key type,
     * - piranha::math::pow(),
     * - construction of coefficient, key and term,
     * - the copy assignment operator of piranha::symbol_set,
     * - piranha::series::insert() and piranha::series::pow().
     */
    template <typename T>
    pow_ret_type<T> pow(const T &x) const
    {
        using ret_type = pow_ret_type<T>;
        typedef typename ret_type::term_type term_type;
        typedef typename term_type::cf_type cf_type;
        typedef typename term_type::key_type key_type;
        if (this->size() == 1u && !this->m_container.begin()->m_key.is_unitary(this->m_symbol_set)) {
            cf_type cf(math::pow(this->m_container.begin()->m_cf, x));
            key_type key(this->m_container.begin()->m_key.pow(x, this->m_symbol_set));
            ret_type retval;
            retval.set_symbol_set(this->m_symbol_set);
            retval.insert(term_type(std::move(cf), std::move(key)));
            return retval;
        }
        return static_cast<series<Cf, Key, polynomial<Cf, Key>> const *>(this)->pow(x);
    }
    /// Inversion.
    /**
     * \note
     * This method is enabled only if <tt>piranha::polynomial::pow(-1)</tt> is a well-formed
     * expression.
     *
     * @return the calling polynomial raised to -1 using piranha::polynomial::pow().
     *
     * @throws unspecified any exception thrown by piranha::polynomial::pow().
     */
    template <typename Series = polynomial>
    inverse_type<Series> invert() const
    {
        return this->pow(-1);
    }
    /// Integration.
    /**
     * \note
     * This method is enabled only if the algorithm described below is supported by all the involved types.
     *
     * This method will attempt to compute the antiderivative of the polynomial term by term. If the term's coefficient
     * does not depend on
     * the integration variable, the result will be calculated via the integration of the corresponding monomial.
     * Integration with respect to a variable appearing to the power of -1 will fail.
     *
     * Otherwise, a strategy of integration by parts is attempted, its success depending on the integrability
     * of the coefficient and on the value of the exponent of the integration variable. The integration will
     * fail if the exponent is negative or non-integral.
     *
     * @param name integration variable.
     *
     * @return the antiderivative of \p this with respect to \p name.
     *
     * @throws std::invalid_argument if the integration procedure fails.
     * @throws unspecified any exception thrown by:
     * - piranha::symbol construction,
     * - piranha::math::partial(), piranha::math::is_zero(), piranha::math::integrate(), piranha::safe_cast()
     *   and piranha::math::negate(),
     * - piranha::symbol_set::add(),
     * - term construction,
     * - coefficient construction, assignment and arithmetics,
     * - integration, construction, assignment, differentiation and degree querying methods of the key type,
     * - insert(),
     * - series arithmetics.
     */
    template <typename T = polynomial>
    integrate_type<T> integrate(const std::string &name) const
    {
        typedef typename base::term_type term_type;
        typedef typename term_type::cf_type cf_type;
        // Turn name into symbol.
        const symbol s(name);
        integrate_type<T> retval(0);
        const auto it_f = this->m_container.end();
        for (auto it = this->m_container.begin(); it != it_f; ++it) {
            // If the derivative of the coefficient is null, we just need to deal with
            // the integration of the key.
            if (math::is_zero(math::partial(it->m_cf, name))) {
                polynomial tmp;
                symbol_set sset = this->m_symbol_set;
                // If the variable does not exist in the arguments set, add it.
                if (!std::binary_search(sset.begin(), sset.end(), s)) {
                    sset.add(s);
                }
                tmp.set_symbol_set(sset);
                auto key_int = it->m_key.integrate(s, this->m_symbol_set);
                tmp.insert(term_type(cf_type(1), std::move(key_int.second)));
                retval += (tmp * it->m_cf) / key_int.first;
            } else {
                retval += integrate_impl(s, *it, std::integral_constant<bool, is_integrable<cf_type>::value>());
            }
        }
        return retval;
    }
    /// Set total-degree-based auto-truncation.
    /**
     * \note
     * This method is available only if the requisites outlined in piranha::polynomial are satisfied
     * and if \p U can be safely cast to the degree type.
     *
     * Setup the degree-based auto-truncation mechanism to truncate according to the total maximum degree.
     * If the new auto truncation settings are different from the currently active ones, the natural power cache
     * defined in piranha::series will be cleared.
     *
     * @param max_degree maximum total degree that will be retained during automatic truncation.
     *
     * @throws unspecified any exception thrown by:
     * - threading primitives,
     * - piranha::safe_cast(),
     * - the constructor of the degree type.
     */
    template <typename U, typename T = polynomial, at_degree_set_enabler<T, U> = 0>
    static void set_auto_truncate_degree(const U &max_degree)
    {
        // Init out for exception safety.
        auto new_degree = safe_cast<degree_type<T>>(max_degree);
        // Initialisation of function-level statics is thread-safe, no need to lock. We get
        // a ref before the lock because the initialisation of the static could throw in principle,
        // and we want the section after the lock to be exception-free.
        auto &at_dm = get_at_degree_max();
        std::lock_guard<std::mutex> lock(s_at_degree_mutex);
        // NOTE: here in principle there could be an exception thrown as a consequence of the degree comparison.
        // This is not a problem as at this stage no setting has been modified.
        truncation_clear_pow_cache(1, new_degree, {});
        s_at_degree_mode = 1;
        // NOTE: the degree type of polys satisfies is_container_element, so move assignment is noexcept.
        at_dm = std::move(new_degree);
        // This should not throw (a vector of strings, destructors and deallocation should be noexcept).
        s_at_degree_names.clear();
    }
    /// Set partial-degree-based auto-truncation.
    /**
     * \note
     * This method is available only if the requisites outlined in piranha::polynomial are satisfied
     * and if \p U can be safely cast to the degree type.
     *
     * Setup the degree-based auto-truncation mechanism to truncate according to the partial degree.
     * If the new auto truncation settings are different from the currently active ones, the natural power cache
     * defined in piranha::series will be cleared.
     *
     * @param max_degree maximum partial degree that will be retained during automatic truncation.
     * @param names names of the variables that will be considered during the computation of the
     * partial degree.
     *
     * @throws unspecified any exception thrown by:
     * - threading primitives,
     * - piranha::safe_cast(),
     * - the constructor of the degree type,
     * - memory allocation errors in standard containers.
     */
    template <typename U, typename T = polynomial, at_degree_set_enabler<T, U> = 0>
    static void set_auto_truncate_degree(const U &max_degree, const std::vector<std::string> &names)
    {
        // Copy+move for exception safety.
        auto new_degree = safe_cast<degree_type<T>>(max_degree);
        auto new_names = names;
        auto &at_dm = get_at_degree_max();
        std::lock_guard<std::mutex> lock(s_at_degree_mutex);
        truncation_clear_pow_cache(2, new_degree, new_names);
        s_at_degree_mode = 2;
        at_dm = std::move(new_degree);
        s_at_degree_names = std::move(new_names);
    }
    /// Disable degree-based auto-truncation.
    /**
     * \note
     * This method is available only if the requisites outlined in piranha::polynomial are satisfied.
     *
     * Disable the degree-based auto-truncation mechanism.
     *
     * @throws unspecified any exception thrown by:
     * - threading primitives,
     * - the constructor of the degree type,
     * - memory allocation errors in standard containers.
     */
    template <typename T = polynomial, at_degree_enabler<T> = 0>
    static void unset_auto_truncate_degree()
    {
        degree_type<T> new_degree(0);
        auto &at_dm = get_at_degree_max();
        std::lock_guard<std::mutex> lock(s_at_degree_mutex);
        s_at_degree_mode = 0;
        at_dm = std::move(new_degree);
        s_at_degree_names.clear();
    }
    /// Query the status of the degree-based auto-truncation mechanism.
    /**
     * \note
     * This method is available only if the requisites outlined in piranha::polynomial are satisfied.
     *
     * This method will return a tuple of three elements describing the status of the degree-based auto-truncation
     * mechanism.
     * The elements of the tuple have the following meaning:
     * - truncation mode (0 if disabled, 1 for total-degree truncation and 2 for partial-degree truncation),
     * - the maximum degree allowed,
     * - the list of names to be considered for partial truncation.
     *
     * @return a tuple representing the status of the degree-based auto-truncation mechanism.
     *
     * @throws unspecified any exception thrown by threading primitives or by the involved constructors.
     */
    template <typename T = polynomial, at_degree_enabler<T> = 0>
    static std::tuple<int, degree_type<T>, std::vector<std::string>> get_auto_truncate_degree()
    {
        std::lock_guard<std::mutex> lock(s_at_degree_mutex);
        return std::make_tuple(s_at_degree_mode, get_at_degree_max(), s_at_degree_names);
    }
    /// Find coefficient.
    /**
     * \note
     * This method is enabled only if:
     * - \p T satisfies piranha::has_begin_end,
     * - \p Key can be constructed from the begin/end iterators of \p c and a piranha::symbol_set.
     *
     * This method will first construct a term with zero coefficient and key initialised from the begin/end iterators
     * of \p c and the symbol set of \p this, and it will then try to locate the term inside \p this.
     * If the term is found, its coefficient will be returned. Otherwise, a coefficient initialised
     * from 0 will be returned.
     *
     * @param c the container that will be used to construct the \p Key to be located.
     *
     * @returns the coefficient of the term whose \p Key corresponds to \p c if such term exists,
     * zero otherwise.
     *
     * @throws unspecified any exception thrown by:
     * - term, coefficient and key construction,
     * - piranha::hash_set::find().
     */
    template <typename T, find_cf_enabler<T> = 0>
    Cf find_cf(const T &c) const
    {
        return find_cf_impl(std::begin(c), std::end(c));
    }
    /// Find coefficient.
    /**
     * \note
     * This method is enabled only if \p Key can be constructed from the begin/end iterators of \p l and a
     * piranha::symbol_set.
     *
     * This method is identical to the other overload with the same name, and it is provided for convenience.
     *
     * @param l the list that will be used to construct the \p Key to be located.
     *
     * @returns the coefficient of the term whose \p Key corresponds to \p l if such term exists,
     * zero otherwise.
     *
     * @throws unspecified any exception thrown by the other overload.
     */
    template <typename T, find_cf_init_list_enabler<T> = 0>
    Cf find_cf(std::initializer_list<T> l) const
    {
        return find_cf_impl(std::begin(l), std::end(l));
    }
    /// Split polynomial.
    /**
     * This method will split the first symbolic argument of the polynomial. That is, it will return a univariate
     * representation of \p this in which the only symbolic argument is the first symbol in the symbol set of \p this,
     * and the coefficients are multivariate polynomials in the remaining variables.
     *
     * @return a representation of \p this as a univariate polynomial with multivariate polynomial coefficients.
     *
     * @throws std::invalid_argument if \p this does not have at least 2 symbolic arguments.
     * @throws unspecified: any exception thrown by:
     * - the <tt>split()</tt> method of the monomial,
     * - the construction of piranha::symbol_set,
     * - piranha::series::set_symbol_set(),
     * - the construction of terms, coefficients and keys,
     * - piranha::series::insert().
     */
    polynomial<polynomial<Cf, Key>, Key> split() const
    {
        if (unlikely(this->get_symbol_set().size() < 2u)) {
            piranha_throw(std::invalid_argument, "a polynomial needs at least 2 arguments in order to be split");
        }
        using r_term_type = typename polynomial<polynomial<Cf, Key>, Key>::term_type;
        using term_type = typename base::term_type;
        polynomial<polynomial<Cf, Key>, Key> retval;
        retval.set_symbol_set(symbol_set(this->get_symbol_set().begin(), this->get_symbol_set().begin() + 1));
        symbol_set tmp_ss(this->get_symbol_set().begin() + 1, this->get_symbol_set().end());
        for (const auto &t : this->_container()) {
            auto tmp_s = t.m_key.split(this->get_symbol_set());
            polynomial tmp_p;
            tmp_p.set_symbol_set(tmp_ss);
            tmp_p.insert(term_type{t.m_cf, std::move(tmp_s.first)});
            retval.insert(r_term_type{std::move(tmp_p), std::move(tmp_s.second)});
        }
        return retval;
    }
    /// Join polynomial.
    /**
     * \note
     * This method is enabled only if the coefficient type \p Cf is a polynomial whose key type is \p Key, and which
     * supports
     * the expression <tt>a += b * c</tt> where \p a, \p b and \p c are all instances of \p Cf).
     *
     * This method is the opposite of piranha::polynomial::split(): given a polynomial with polynomial coefficients,
     * it will produce a multivariate polynomial in which the arguments of \p this and the arguments of the coefficients
     * have been joined.
     *
     * @return the joined counterpart of \p this.
     *
     * @throws unspecified any exception thrown by:
     * - piranha::series::set_symbol_set() and piranha::series::insert(),
     * - term construction,
     * - arithmetic operations on the coefficient type.
     */
    template <typename T = polynomial, join_enabler<T> = 0>
    Cf join() const
    {
        // NOTE: here we can improve performance by using
        // lower level primitives rather than arithmetic operators.
        using cf_term_type = typename Cf::term_type;
        Cf retval;
        for (const auto &t : this->_container()) {
            Cf tmp;
            tmp.set_symbol_set(this->get_symbol_set());
            tmp.insert(cf_term_type{1, t.m_key});
            retval += tmp * t.m_cf;
        }
        return retval;
    }
    /// Content.
    /**
     * \note
     * This method is enabled only if the coefficient type satisfies piranha::has_gcd3.
     *
     * This method will return the GCD of the polynomial's coefficients. If the polynomial
     * is empty, zero will be returned.
     *
     * @return the content of \p this.
     *
     * @throws unspecified any exception thrown by the polynomial constructor from \p int
     * or by piranha::math::gcd3().
     */
    template <typename T = polynomial, content_enabler<T> = 0>
    Cf content() const
    {
        Cf retval(0);
        for (const auto &t : this->_container()) {
            math::gcd3(retval, retval, t.m_cf);
        }
        return retval;
    }
    /// Primitive part.
    /**
     * \note
     * This method is enabled only if:
     * - the method piranha::polynomial::content() is enabled,
     * - the polynomial coefficient supports math::divexact().
     *
     * This method will return \p this divided by its content.
     *
     * @return the primitive part of \p this.
     *
     * @throws piranha::zero_division_error if the content is zero.
     * @throws unspecified any exception thrown by the division operation or by piranha::polynomial::content().
     */
    template <typename T = polynomial, pp_enabler<T> = 0>
    polynomial primitive_part() const
    {
        polynomial retval(*this);
        auto c = content();
        if (unlikely(math::is_zero(c))) {
            piranha_throw(zero_division_error, "the content of the polynomial is zero");
        }
        detail::poly_exact_cf_div(retval, c);
        return retval;
    }
    /// Univariate polynomial division with remainder.
    /**
     * \note
     * This static method is enabled only if:
     * - the polynomial coefficient type \p Cf is multipliable yielding the same type \p Cf, it is multipliable in-place
     * and
     *   divisible exactly, and it has exact ring operations,
     * - the polynomial type is subtractable in-place,
     * - the exponent type of the monomial is a C++ integral type or an instance of piranha::mp_integer.
     *
     * This method will return a pair representing the quotient and the remainder of the division of the univariate
     * polynomials
     * \p n and \p d. The input polynomials must be univariate in the same variable.
     *
     * @param n the numerator.
     * @param d the denominator.
     *
     * @return the quotient and remainder of the division of \p n by \p d, represented as a standard pair.
     *
     * @throws std::invalid_argument if:
     * - the input polynomials are not univariate or univariate in different variables, or
     * - the low degree of the input polynomials is less than zero.
     * @throws piranha::zero_division_error if \p d is empty.
     * @throws unspecified any exception thrown by:
     * - the public interface of piranha::hash_set and piranha::series,
     * - the monomial multiplication and division methods,
     * - arithmetic operations on the coefficients and on polynomials,
     * - construction of coefficients, monomials, terms and polynomials.
     */
    template <typename T = polynomial, poly_div_enabler<T> = 0>
    static std::pair<polynomial, polynomial> udivrem(const polynomial &n, const polynomial &d)
    {
        return udivrem_impl<true>(n, d);
    }
    /// Univariate pseudo-remainder.
    /**
     * \note
     * This static method is enabled only if polynomial division is enabled, and the polynomial
     * coefficient type is exponentiable to \p unsigned, yielding the coefficient type as a result.
     *
     * The pseudo-remainder is defined as:
     * \f[
     * \mathrm{rem}\left(\mathrm{lc}\left( d \right)^{a-b+1} n,d \right),
     * \f]
     * where \f$\mathrm{lc}\f$ denotes the leading coefficient (that is, the coefficient of the term with the
     * highest univariate degree), and \f$a\f$ and \f$b\f$ are the degrees of \p n and \p d respectively.
     *
     * This method works only on univariate polyomials in the same variable and it requires the univariate degree of \p
     * n to be not less than
     * the univariate degree of \p d. If \p n is zero, an empty polynomial will be returned.
     *
     * @param n the numerator.
     * @param d the denominator.
     *
     * @return the pseudo-remainder of <tt>n / d</tt>.
     *
     * @throws std::invalid_argument if the polynomials are not univariate in the same variable, or if the univariate
     * degree of \p d is greater than the univariate degree of \p n.
     * @throws piranha::zero_division_error if \p d is zero.
     * @throws unspecified any exception thrown by:
     * - piranha::polynomial::udivrem(),
     * - the public interface of piranha::series(),
     * - copy construction of polynomials,
     * - exponentiation and multiplication of coefficients,
     * - the conversion of piranha::integer to \p unsigned.
     */
    template <typename T = polynomial, uprem_enabler<T> = 0>
    static polynomial uprem(const polynomial &n, const polynomial &d)
    {
        // Only univariate polynomials are allowed.
        if (unlikely(n.get_symbol_set().size() != 1u || n.get_symbol_set() != d.get_symbol_set())) {
            piranha_throw(std::invalid_argument, "only univariate polynomials in the same variable "
                                                 "are allowed in the polynomial uprem routine");
        }
        const auto &args = n.get_symbol_set();
        // NOTE: this check is repeated in divrem, we do it here as we want to get the leading term in d.
        if (unlikely(d.size() == 0u)) {
            piranha_throw(zero_division_error, "univariate polynomial division by zero");
        }
        // Same as above, we need the leading term.
        if (unlikely(n.size() == 0u)) {
            polynomial retval;
            retval.set_symbol_set(args);
            return retval;
        }
        auto ld = detail::poly_lterm(d);
        integer dn(detail::poly_lterm(n)->m_key.degree(args)), dd(ld->m_key.degree(args));
        if (dd > dn) {
            piranha_throw(std::invalid_argument, "the degree of the denominator is greater than "
                                                 "the degree of the numerator in the polynomial uprem routine");
        }
        // NOTE: negative degrees will be caught by udivrem.
        auto n_copy(n);
        detail::poly_cf_mult(math::pow(ld->m_cf, static_cast<unsigned>(dn - dd + 1)), n_copy);
        // NOTE: here we can force exact division in udivrem, when we implement it.
        return udivrem(n_copy, d).second;
    }
    /// Exact polynomial division.
    /**
     * \note
     * This operator is enabled only if the decay types of \p T and \p U are the same type \p Td
     * and polynomial::udivrem() is enabled for \p Td.
     *
     * This operator will compute the exact result of <tt>n / d</tt>. If \p d does not divide \p n exactly, an error
     * will be produced.
     *
     * @param n the numerator.
     * @param d the denominator.
     *
     * @return the quotient <tt>n / d</tt>.
     *
     * @throws piranha::zero_division_error if \p d is zero.
     * @throws piranha::math::inexact_division if the division is not exact.
     * @throws std::invalid_argument if a negative exponent is encountered.
     * @throws unspecified any exception thrown by:
     * - piranha::polynomial::udivrem(),
     * - the public interface of piranha::hash_set, piranha::series, piranha::symbol_set,
     * - the monomial's <tt>extract_exponents()</tt> methods,
     * - construction of coefficients, monomials, terms and polynomials,
     * - arithmetic operations on the coefficients and on polynomials,
     * - memory errors in standard containers,
     * - conversion of piranha::integer to C++ integral types,
     * - piranha::safe_cast().
     */
    template <typename T, typename U, poly_div_enabler<T, U> = 0>
    friend polynomial operator/(T &&n, U &&d)
    {
        // Then we need to deal with different symbol sets.
        polynomial merged_n, merged_d;
        polynomial const *real_n(&n), *real_d(&d);
        if (n.get_symbol_set() != d.get_symbol_set()) {
            auto merge = n.get_symbol_set().merge(d.get_symbol_set());
            if (merge != n.get_symbol_set()) {
                merged_n = n.extend_symbol_set(merge);
                real_n = &merged_n;
            }
            if (merge != d.get_symbol_set()) {
                merged_d = d.extend_symbol_set(merge);
                real_d = &merged_d;
            }
        }
        return division_impl(*real_n, *real_d);
    }
    /// Exact in-place polynomial division.
    /**
     * \note
     * This operator is enabled only if operator/() is enabled for the input types and \p T
     * is not const.
     *
     * This operator is equivalent to:
     * @code
     * return n = n / d;
     * @endcode
     *
     * @param n the numerator.
     * @param d the denominator.
     *
     * @return a reference to \p n.
     *
     * @throws unspecified any exception thrown by the binary operator.
     */
    template <typename T, typename U, poly_in_place_div_enabler<T, U> = 0>
    friend polynomial &operator/=(T &n, U &&d)
    {
        return n = n / d;
    }
    /// Get the default algorithm to be used for GCD computations.
    /**
     * The default value initialised on startup is polynomial_gcd_algorithm::automatic.
     * This method is thread-safe.
     *
     * @return the current default algorithm to be used for GCD computations.
     */
    static polynomial_gcd_algorithm get_default_gcd_algorithm()
    {
        return s_def_gcd_algo.load();
    }
    /// Set the default algorithm to be used for GCD computations.
    /**
     * This method is thread-safe.
     *
     * @param algo the desired default algorithm to be used for GCD computations.
     */
    static void set_default_gcd_algorithm(polynomial_gcd_algorithm algo)
    {
        s_def_gcd_algo.store(algo);
    }
    /// Reset the default algorithm to be used for GCD computations.
    /**
     * This method will set the default algorithm to be used for GCD computations to
     * polynomial_gcd_algorithm::automatic.
     * This method is thread-safe.
     */
    static void reset_default_gcd_algorithm()
    {
        s_def_gcd_algo.store(polynomial_gcd_algorithm::automatic);
    }
    /// Polynomial GCD.
    /**
     * \note
     * This static method is enabled only if the following conditions apply:
     * - the coefficient type has math::gcd() and math::gcd3(), it is exponentiable to \p unsigned yielding
     *   the coefficient type, it is multipliable in-place and in binary form, yielding the coefficient type,
     *   it has math::divexact(), and it is copy-assignable,
     * - the polynomial type has piranha::polynomial::content() and piranha::polynomial::udivrem().
     *
     * This method will compute the GCD of polynomials \p a and \p b. The algorithm that will be employed
     * for the computation is selected by the \p algo flag. If \p algo is set to polynomial_gcd_algorithm::automatic
     * the heuristic GCD algorithm will be tried first, followed by the PRS SR algorithm in case of failures.
     * If \p algo is set to any other value, the selected algorithm will be used. The heuristic GCD algorithm can be
     * used only when the ceofficient type is an instance of piranha::mp_integer. The default value for \p algo
     * is the one returned by get_default_gcd_algorithm().
     *
     * The \p with_cofactors flag signals whether the cofactors should be returned together with the GCD or not.
     *
     * @param a first argument.
     * @param b second argument.
     * @param with_cofactors flag to signal that cofactors must be returned as well.
     * @param algo the GCD algorithm.
     *
     * @return a tuple containing the GCD \p g of \p a and \p b, and the cofactors (that is, <tt>a / g</tt> and <tt>b /
     * g</tt>),
     * if requested. If the cofactors are not requested, the content of the last two elements of the tuple is
     * unspecified. If both input
     * arguments are zero, the cofactors will be zero as well.
     *
     * @throws std::invalid_argument if a negative exponent is encountered in \p a or \p b.
     * @throws std::overflow_error in case of (unlikely) integral overflow errors.
     * @throws std::runtime_error if \p algo is polynomial_gcd_algorithm::heuristic and the execution
     * of the algorithm fails or it the coefficient type is not an instance of piranha::mp_integer.
     * @throws unspecified any exception thrown by:
     * - the public interface of piranha::series, piranha::symbol_set, piranha::hash_set,
     * - construction of keys,
     * - construction, arithmetics and other operations of coefficients and polynomials,
     * - math::gcd(), math::gcd3(), math::divexact(), math::pow(), math::subs(),
     * - piranha::polynomial::udivrem(), piranha::polynomial::content(),
     * - memory errors in standard containers,
     * - the conversion of piranha::integer to \p unsigned.
     */
    template <typename T = polynomial, gcd_enabler<T> = 0>
    static std::tuple<polynomial, polynomial, polynomial>
    gcd(const polynomial &a, const polynomial &b, bool with_cofactors = false,
        polynomial_gcd_algorithm algo = get_default_gcd_algorithm())
    {
        // Deal with different symbol sets.
        polynomial merged_a, merged_b;
        polynomial const *real_a(&a), *real_b(&b);
        if (a.get_symbol_set() != b.get_symbol_set()) {
            auto merge = a.get_symbol_set().merge(b.get_symbol_set());
            if (merge != a.get_symbol_set()) {
                merged_a = a.extend_symbol_set(merge);
                real_a = &merged_a;
            }
            if (merge != b.get_symbol_set()) {
                merged_b = b.extend_symbol_set(merge);
                real_b = &merged_b;
            }
        }
        if (algo == polynomial_gcd_algorithm::automatic || algo == polynomial_gcd_algorithm::heuristic) {
            auto heu_res = try_gcdheu(*real_a, *real_b, algo);
            if (heu_res.first && algo == polynomial_gcd_algorithm::heuristic) {
                // This can happen only if the heuristic fails due to the number of iterations. Calling the heuristic
                // with non-integral coefficients already throws from the try_gcdheu implementation.
                piranha_throw(std::runtime_error, "the heuristic polynomial GCD algorithm was explicitly selected, "
                                                  "but its execution failed due to too many iterations");
            }
            if (!heu_res.first) {
                return std::move(heu_res.second);
            }
        }
        // Cache it.
        const auto &args = real_a->get_symbol_set();
        // Proceed with the univariate case.
        if (args.size() == 1u) {
            return wrap_gcd_cofactors(detail::gcd_prs_sr(*real_a, *real_b), *real_a, *real_b, with_cofactors);
        }
        // Zerovariate case. We need to handle this separately as the use of split() below
        // requires a nonzero number of arguments.
        if (args.size() == 0u) {
            if (real_a->size() == 0u && real_b->size() == 0u) {
                return wrap_gcd_cofactors(polynomial{}, *real_a, *real_b, with_cofactors);
            }
            if (real_a->size() == 0u) {
                return wrap_gcd_cofactors(*real_b, *real_a, *real_b, with_cofactors);
            }
            if (real_b->size() == 0u) {
                return wrap_gcd_cofactors(*real_a, *real_a, *real_b, with_cofactors);
            }
            Cf g(0);
            math::gcd3(g, real_a->_container().begin()->m_cf, real_b->_container().begin()->m_cf);
            return wrap_gcd_cofactors(polynomial(std::move(g)), *real_a, *real_b, with_cofactors);
        }
        // The general multivariate case.
        return wrap_gcd_cofactors(detail::gcd_prs_sr(real_a->split(), real_b->split()).join(), *real_a, *real_b,
                                  with_cofactors);
    }
    /// Height.
    /**
     * \note
     * This method is enabled only if the following conditions hold:
     * - the coefficient type supports piranha::math::abs(), yielding the type <tt>height_type<T></tt>,
     * - <tt>height_type<T></tt> is constructible from \p int, less-than comparable, move-assignable and copy or move
     * constructible.
     *
     * The height of a polynomial is defined as the maximum of the absolute values of the coefficients. If the
     * polynomial
     * is empty, zero will be returned.
     *
     * @return the height of \p this.
     *
     * @throws unspecified any exception thrown by the construction, assignment, comparison, and absolute value
     * calculation
     * of <tt>height_type<T></tt>.
     */
    template <typename T = polynomial>
    height_type<T> height() const
    {
        height_type<T> retval(0);
        for (const auto &t : this->_container()) {
            auto tmp(math::abs(t.m_cf));
            if (!(tmp < retval)) {
                retval = std::move(tmp);
            }
        }
        return retval;
    }
    /// Untruncated multiplication.
    /**
     * \note
     * This function template is enabled only if the calling piranha::polynomial satisfies piranha::is_multipliable,
     * returning the calling piranha::polynomial as return type.
     *
     * This function will return the product of \p p1 and \p p2, computed without truncation (regardless
     * of the current automatic truncation settings). Note that this function is
     * available only if the operands are of the same type and no type promotions affect the coefficient types
     * during multiplication.
     *
     * @param p1 the first operand.
     * @param p2 the second operand.
     *
     * @return the product of \p p1 and \p p2.
     *
     * @throws unspecified any exception thrown by:
     * - the public interface of the specialisation of piranha::series_multiplier for piranha::polynomial,
     * - the public interface of piranha::symbol_set,
     * - the public interface of piranha::series.
     */
    template <typename T = polynomial, um_enabler<T> = 0>
    static polynomial untruncated_multiplication(const polynomial &p1, const polynomial &p2)
    {
        auto runner = [](const polynomial &a, const polynomial &b) {
            return series_multiplier<polynomial>(a, b)._untruncated_multiplication();
        };
        return um_tm_implementation(p1, p2, runner);
    }
    /// Truncated multiplication (total degree).
    /**
     * \note
     * This function template is enabled only if the following conditions hold:
     * - the calling piranha::polynomial satisfies piranha::is_multipliable, returning the calling piranha::polynomial
     *   as return type,
     * - the requirements for truncated multiplication outlined in piranha::polynomial are satisfied,
     * - \p U can be safely cast to the degree type of the calling piranha::polynomial.
     *
     * This function will return the product of \p p1 and \p p2, truncated to the maximum total degree
     * of \p max_degree (regardless of the current automatic truncation settings).
     * Note that this function is
     * available only if the operands are of the same type and no type promotions affect the coefficient types
     * during multiplication.
     *
     * @param p1 the first operand.
     * @param p2 the second operand.
     * @param max_degree the maximum total degree in the result.
     *
     * @return the truncated product of \p p1 and \p p2.
     *
     * @throws unspecified any exception thrown by:
     * - the public interface of the specialisation of piranha::series_multiplier for piranha::polynomial,
     * - the public interface of piranha::symbol_set,
     * - the public interface of piranha::series,
     * - piranha::safe_cast().
     */
    template <typename U, typename T = polynomial, tm_enabler<T, U> = 0>
    static polynomial truncated_multiplication(const polynomial &p1, const polynomial &p2, const U &max_degree)
    {
        // NOTE: these 2 implementations may be rolled into one once we can safely capture variadic arguments
        // in lambdas.
        auto runner = [&max_degree](const polynomial &a, const polynomial &b) {
            return series_multiplier<polynomial>(a, b)._truncated_multiplication(safe_cast<degree_type<T>>(max_degree));
        };
        return um_tm_implementation(p1, p2, runner);
    }
    /// Truncated multiplication (partial degree).
    /**
     * \note
     * This function template is enabled only if the following conditions hold:
     * - the calling piranha::polynomial satisfies piranha::is_multipliable, returning the calling piranha::polynomial
     *   as return type,
     * - the requirements for truncated multiplication outlined in piranha::polynomial are satisfied,
     * - \p U can be safely cast to the degree type of the calling piranha::polynomial.
     *
     * This function will return the product of \p p1 and \p p2, truncated to the maximum partial degree
     * of \p max_degree (regardless of the current automatic truncation settings).
     * Note that this function is
     * available only if the operands are of the same type and no type promotions affect the coefficient types
     * during multiplication.
     *
     * @param p1 the first operand.
     * @param p2 the second operand.
     * @param max_degree the maximum total degree in the result.
     * @param names names of the variables that will be considered in the computation of the degree.
     *
     * @return the truncated product of \p p1 and \p p2.
     *
     * @throws unspecified any exception thrown by:
     * - the public interface of the specialisation of piranha::series_multiplier for piranha::polynomial,
     * - the public interface of piranha::symbol_set,
     * - the public interface of piranha::series,
     * - piranha::safe_cast().
     */
    template <typename U, typename T = polynomial, tm_enabler<T, U> = 0>
    static polynomial truncated_multiplication(const polynomial &p1, const polynomial &p2, const U &max_degree,
                                               const std::vector<std::string> &names)
    {
        // NOTE: total and partial degree must be the same.
        auto runner = [&max_degree, &names](const polynomial &a, const polynomial &b) -> polynomial {
            const symbol_set::positions pos(a.get_symbol_set(), symbol_set(names.begin(), names.end()));
            return series_multiplier<polynomial>(a, b)._truncated_multiplication(safe_cast<degree_type<T>>(max_degree),
                                                                                 names, pos);
        };
        return um_tm_implementation(p1, p2, runner);
    }

private:
    // Static data for auto_truncate_degree.
    static std::mutex s_at_degree_mutex;
    static int s_at_degree_mode;
    static std::vector<std::string> s_at_degree_names;
    // Static data for default GCD algorithm selection.
    static std::atomic<polynomial_gcd_algorithm> s_def_gcd_algo;
};

// Static inits.
template <typename Cf, typename Key>
std::mutex polynomial<Cf, Key>::s_at_degree_mutex;

template <typename Cf, typename Key>
int polynomial<Cf, Key>::s_at_degree_mode = 0;

template <typename Cf, typename Key>
std::vector<std::string> polynomial<Cf, Key>::s_at_degree_names;

template <typename Cf, typename Key>
std::atomic<polynomial_gcd_algorithm> polynomial<Cf, Key>::s_def_gcd_algo(polynomial_gcd_algorithm::automatic);

namespace detail
{

template <unsigned N, typename Cf, typename Key, typename = void>
struct r_poly_impl {
    static_assert(N > 1u, "Invalid recursion index.");
    using type = polynomial<typename r_poly_impl<N - 1u, Cf, Key>::type, Key>;
};

template <unsigned N, typename Cf, typename Key>
struct r_poly_impl<N, Cf, Key, typename std::enable_if<N == 1u>::type> {
    using type = polynomial<Cf, Key>;
};
}

/// Recursive polynomial.
/**
 * This is a convenience alias that can be used to define multivariate polynomials
 * as univariate polynomials with univariate polynomials as coefficients.
 *
 * For instance, the type
 * @code
 * r_polynomial<1,double,k_monomial>;
 * @endcode
 * is exactly equivalent to
 * @code
 * polynomial<double,k_monomial>;
 * @endcode
 * The type
 * @code
 * r_polynomial<2,double,k_monomial>;
 * @endcode
 * is exactly equivalent to
 * @code
 * polynomial<polynomial<double,k_monomial>,k_monomial>;
 * @endcode
 * And so on for increasing values of \p N. \p N must be nonzero, or a compile-time error will be
 * generated.
 */
template <unsigned N, typename Cf, typename Key>
using r_polynomial = typename detail::r_poly_impl<N, Cf, Key>::type;

namespace detail
{

// Identification of key types for dispatching in the multiplier.
template <typename T>
struct is_kronecker_monomial {
    static const bool value = false;
};

template <typename T>
struct is_kronecker_monomial<kronecker_monomial<T>> {
    static const bool value = true;
};

template <typename T>
struct is_monomial {
    static const bool value = false;
};

template <typename T, typename S>
struct is_monomial<monomial<T, S>> {
    static const bool value = true;
};

// Identify the presence of auto-truncation methods in the poly multiplier.
template <typename S, typename T>
class has_set_auto_truncate_degree : sfinae_types
{
    // NOTE: if we have total degree auto truncation, we also have partial degree truncation.
    template <typename S1, typename T1>
    static auto test(const S1 &, const T1 &t) -> decltype(S1::set_auto_truncate_degree(t), void(), yes());
    static no test(...);

public:
    static const bool value = std::is_same<yes, decltype(test(std::declval<S>(), std::declval<T>()))>::value;
};

template <typename S, typename T>
const bool has_set_auto_truncate_degree<S, T>::value;

template <typename S>
class has_get_auto_truncate_degree : sfinae_types
{
    template <typename S1>
    static auto test(const S1 &) -> decltype(S1::get_auto_truncate_degree(), void(), yes());
    static no test(...);

public:
    static const bool value = std::is_same<yes, decltype(test(std::declval<S>()))>::value;
};

template <typename S>
const bool has_get_auto_truncate_degree<S>::value;

// Global enabler for the polynomial multiplier.
template <typename Series>
using poly_multiplier_enabler = typename std::enable_if<std::is_base_of<detail::polynomial_tag, Series>::value>::type;

// Enabler for divexact.
template <typename T>
using poly_divexact_enabler = typename std::enable_if<true_tt<ptd::enabler<T, T>>::value>::type;

// Enabler for exact ring operations.
template <typename T>
using poly_ero_enabler =
    typename std::enable_if<std::is_base_of<detail::polynomial_tag, typename std::decay<T>::type>::value
                            && has_exact_ring_operations<typename std::decay<T>::type::term_type::cf_type>::value
                            && (std::is_integral<typename std::decay<T>::type::term_type::key_type::value_type>::value
                                || has_exact_ring_operations<
                                       typename std::decay<T>::type::term_type::key_type::value_type>::value)>::type;

// Enabler for GCD.
template <typename T>
using poly_gcd_enabler = typename std::
    enable_if<std::is_base_of<detail::polynomial_tag, T>::value
              && true_tt<decltype(T::gcd(std::declval<const T &>(), std::declval<const T &>()))>::value>::type;
}

namespace math
{

/// Implementation of piranha::math::divexact() for piranha::polynomial.
/**
 * This specialisation is enabled if \p T is an instance of piranha::polynomial which supports
 * exact polynomial division.
 */
template <typename T>
struct divexact_impl<T, detail::poly_divexact_enabler<T>> {
    /// Call operator.
    /**
     * This operator will use the division operator of the polynomial type.
     *
     * @param out return value.
     * @param n the numerator.
     * @param d the denominator.
     *
     * @return a reference to \p out.
     *
     * @throws unspecified any exception thrown by the polynomial division operator.
     */
    T &operator()(T &out, const T &n, const T &d) const
    {
        return out = n / d;
    }
};

/// Implementation of piranha::math::gcd() for piranha::polynomial.
/**
 * This specialisation is enabled if \p T is an instance of piranha::polynomial that supports
 * piranha::polynomial::gcd(). The piranha::polynomial_gcd_algorithm::automatic algorithm
 * will be used for the computation.
 */
template <typename T>
struct gcd_impl<T, T, detail::poly_gcd_enabler<T>> {
    /// Call operator.
    /**
     * @param a first argument.
     * @param b second argument.
     *
     * @return the first element of the result of <tt>piranha::polynomial::gcd(a,b)</tt>.
     *
     * @throws unspecified any exception thrown by piranha::polynomial::gcd().
     */
    T operator()(const T &a, const T &b) const
    {
        return std::get<0u>(T::gcd(a, b));
    }
};
}

/// Exact ring operations specialisation for piranha::polynomial.
/**
 * This specialisation is enabled if the decay type of \p T is an instance of piranha::polynomial
 * whose coefficient type satisfies piranha::has_exact_ring_operations and whose exponent type is either
 * a C++ integral type or another type which satisfies piranha::has_exact_ring_operations.
 */
template <typename T>
struct has_exact_ring_operations<T, detail::poly_ero_enabler<T>> {
    /// Value of the type trait.
    static const bool value = true;
};

template <typename T>
const bool has_exact_ring_operations<T, detail::poly_ero_enabler<T>>::value;

/// Specialisation of piranha::series_multiplier for piranha::polynomial.
/**
 * This specialisation of piranha::series_multiplier is enabled when \p Series is an instance of
 * piranha::polynomial.
 *
 * ## Type requirements ##
 *
 * \p Series must be suitable for use in piranha::base_series_multiplier.
 *
 * ## Exception safety guarantee ##
 *
 * This class provides the same guarantee as piranha::base_series_multiplier.
 *
 * ## Move semantics ##
 *
 * Move semantics is equivalent to piranha::base_series_multiplier's move semantics.
 */
template <typename Series>
class series_multiplier<Series, detail::poly_multiplier_enabler<Series>> : public base_series_multiplier<Series>
{
    // Base multiplier type.
    using base = base_series_multiplier<Series>;
    // Cf type getter shortcut.
    template <typename T>
    using cf_t = typename T::term_type::cf_type;
    // Key type getter shortcut.
    template <typename T>
    using key_t = typename T::term_type::key_type;
    // Bounds checking.
    // Functor to return un updated copy of p if v is less than p.first or greater than p.second.
    struct update_minmax {
        template <typename T>
        std::pair<T, T> operator()(const std::pair<T, T> &p, const T &v) const
        {
            return std::make_pair(v < p.first ? v : p.first, v > p.second ? v : p.second);
        }
    };
    // No bounds checking if key is a monomial with non-integral exponents.
    template <typename T = Series,
              typename std::enable_if<detail::is_monomial<key_t<T>>::value
                                          && !std::is_integral<typename key_t<T>::value_type>::value,
                                      int>::type
              = 0>
    void check_bounds() const
    {
    }
    // Monomial with integral exponents.
    template <typename T = Series,
              typename std::enable_if<detail::is_monomial<key_t<T>>::value
                                          && std::is_integral<typename key_t<T>::value_type>::value,
                                      int>::type
              = 0>
    void check_bounds() const
    {
        using expo_type = typename key_t<T>::value_type;
        using term_type = typename Series::term_type;
        using mm_vec = std::vector<std::pair<expo_type, expo_type>>;
        using v_ptr = typename base::v_ptr;
        // NOTE: we know that the input series are not null.
        piranha_assert(this->m_v1.size() != 0u && this->m_v2.size() != 0u);
        // Sync mutex, actually used only in mt mode.
        std::mutex mut;
        // Checker for monomial sizes in debug mode.
        auto monomial_checker = [this](const term_type &t) { return t.m_key.size() == this->m_ss.size(); };
        (void)monomial_checker;
        // The function used to determine minmaxs for the two series. This is used both in
        // single-thread and multi-thread mode.
        auto thread_func = [&mut, this, &monomial_checker](unsigned t_idx, const v_ptr *vp, mm_vec *mmv) {
            piranha_assert(t_idx < this->m_n_threads);
            // Establish the block size.
            const auto block_size = vp->size() / this->m_n_threads;
            auto start = vp->data() + t_idx * block_size;
            const auto end
                = vp->data() + ((t_idx == this->m_n_threads - 1u) ? vp->size() : ((t_idx + 1u) * block_size));
            // We need to make sure we have at least 1 element to process. This is guaranteed
            // in the single-threaded implementation but not in multithreading.
            if (start == end) {
                piranha_assert(this->m_n_threads > 1u);
                return;
            }
            piranha_assert(monomial_checker(**start));
            // Local vector that will hold the minmax values for this thread.
            mm_vec minmax_values;
            // Init the mimnax.
            // NOTE: we can use this as we are sure the series has at least one element (start != end).
            std::transform((*start)->m_key.begin(), (*start)->m_key.end(), std::back_inserter(minmax_values),
                           [](const expo_type &v) { return std::make_pair(v, v); });
            // Move to the next element and go with the loop.
            ++start;
            for (; start != end; ++start) {
                piranha_assert(monomial_checker(**start));
                // NOTE: std::transform is allowed to do transformations in-place - i.e., here the output range is
                // the same as the first or second input range:
                // http://stackoverflow.com/questions/19200528/is-it-safe-for-the-input-iterator-and-output-iterator-in-stdtransform-to-be-fr
                // The important part is that the functor *itself* must not mutate the elements.
                std::transform(minmax_values.begin(), minmax_values.end(), (*start)->m_key.begin(),
                               minmax_values.begin(), update_minmax{});
            }
#ifndef PIRANHA_SINGLE_THREAD
            if (this->m_n_threads == 1u) {
#endif
                // In single thread the output mmv should be written only once, after being def-inited.
                piranha_assert(mmv->empty());
                // Just move in the local minmax values in single-threaded mode.
                *mmv = std::move(minmax_values);
#ifndef PIRANHA_SINGLE_THREAD
            } else {
                std::lock_guard<std::mutex> lock(mut);
                if (mmv->empty()) {
                    // If mmv has not been inited yet, just move in the current values.
                    *mmv = std::move(minmax_values);
                } else {
                    piranha_assert(minmax_values.size() == mmv->size());
                    // Otherwise, update the minmaxs.
                    auto updater
                        = [](const std::pair<expo_type, expo_type> &p1, const std::pair<expo_type, expo_type> &p2) {
                              return std::make_pair(p1.first < p2.first ? p1.first : p2.first,
                                                    p1.second > p2.second ? p1.second : p2.second);
                          };
                    std::transform(minmax_values.begin(), minmax_values.end(), mmv->begin(), mmv->begin(), updater);
                }
            }
#endif
        };
        // minmax vectors for the two series.
        mm_vec minmax_values1, minmax_values2;
        check_bounds_impl(minmax_values1, minmax_values2, thread_func);
        // Compute the sum of the two minmaxs, using multiprecision to avoid overflow (this is a simple interval
        // addition).
        std::vector<std::pair<integer, integer>> minmax_values;
        std::transform(
            minmax_values1.begin(), minmax_values1.end(), minmax_values2.begin(), std::back_inserter(minmax_values),
            [](const std::pair<expo_type, expo_type> &p1, const std::pair<expo_type, expo_type> &p2) {
                return std::make_pair(integer(p1.first) + integer(p2.first), integer(p1.second) + integer(p2.second));
            });
        piranha_assert(minmax_values.size() == minmax_values1.size());
        piranha_assert(minmax_values.size() == minmax_values2.size());
        // Now do the checking.
        for (decltype(minmax_values.size()) i = 0u; i < minmax_values.size(); ++i) {
            try {
                (void)static_cast<expo_type>(minmax_values[i].first);
                (void)static_cast<expo_type>(minmax_values[i].second);
            } catch (...) {
                piranha_throw(std::overflow_error, "monomial components are out of bounds");
            }
        }
    }
    template <typename T = Series,
              typename std::enable_if<detail::is_kronecker_monomial<key_t<T>>::value, int>::type = 0>
    void check_bounds() const
    {
        using value_type = typename key_t<Series>::value_type;
        using ka = kronecker_array<value_type>;
        using v_ptr = typename base::v_ptr;
        using mm_vec = std::vector<std::pair<value_type, value_type>>;
        piranha_assert(this->m_v1.size() != 0u && this->m_v2.size() != 0u);
        // NOTE: here we are sure about this since the symbol set in a series should never
        // overflow the size of the limits, as the check for compatibility in Kronecker monomial
        // would kick in.
        piranha_assert(this->m_ss.size() < ka::get_limits().size());
        std::mutex mut;
        auto thread_func = [&mut, this](unsigned t_idx, const v_ptr *vp, mm_vec *mmv) {
            piranha_assert(t_idx < this->m_n_threads);
            const auto block_size = vp->size() / this->m_n_threads;
            auto start = vp->data() + t_idx * block_size;
            const auto end
                = vp->data() + ((t_idx == this->m_n_threads - 1u) ? vp->size() : ((t_idx + 1u) * block_size));
            if (start == end) {
                piranha_assert(this->m_n_threads > 1u);
                return;
            }
            mm_vec minmax_values;
            // Tmp vector for unpacking, inited with the first element in the range.
            // NOTE: we need to check that the exponents of the monomials in the result do not
            // go outside the bounds of the Kronecker codification. We need to unpack all monomials
            // in the operands and examine them, we cannot operate on the codes for this.
            auto tmp_vec = (*start)->m_key.unpack(this->m_ss);
            // Init the mimnax.
            std::transform(tmp_vec.begin(), tmp_vec.end(), std::back_inserter(minmax_values),
                           [](const value_type &v) { return std::make_pair(v, v); });
            ++start;
            for (; start != end; ++start) {
                tmp_vec = (*start)->m_key.unpack(this->m_ss);
                std::transform(minmax_values.begin(), minmax_values.end(), tmp_vec.begin(), minmax_values.begin(),
                               update_minmax{});
            }
#ifndef PIRANHA_SINGLE_THREAD
            if (this->m_n_threads == 1u) {
#endif
                piranha_assert(mmv->empty());
                *mmv = std::move(minmax_values);
#ifndef PIRANHA_SINGLE_THREAD
            } else {
                std::lock_guard<std::mutex> lock(mut);
                if (mmv->empty()) {
                    *mmv = std::move(minmax_values);
                } else {
                    piranha_assert(minmax_values.size() == mmv->size());
                    auto updater
                        = [](const std::pair<value_type, value_type> &p1, const std::pair<value_type, value_type> &p2) {
                              return std::make_pair(p1.first < p2.first ? p1.first : p2.first,
                                                    p1.second > p2.second ? p1.second : p2.second);
                          };
                    std::transform(minmax_values.begin(), minmax_values.end(), mmv->begin(), mmv->begin(), updater);
                }
            }
#endif
        };
        mm_vec minmax_values1, minmax_values2;
        check_bounds_impl(minmax_values1, minmax_values2, thread_func);
        std::vector<std::pair<integer, integer>> minmax_values;
        std::transform(
            minmax_values1.begin(), minmax_values1.end(), minmax_values2.begin(), std::back_inserter(minmax_values),
            [](const std::pair<value_type, value_type> &p1, const std::pair<value_type, value_type> &p2) {
                return std::make_pair(integer(p1.first) + integer(p2.first), integer(p1.second) + integer(p2.second));
            });
        // Bounds of the Kronecker representation for each component.
        const auto &minmax_vec
            = std::get<0u>(ka::get_limits()[static_cast<decltype(ka::get_limits().size())>(this->m_ss.size())]);
        piranha_assert(minmax_values.size() == minmax_vec.size());
        piranha_assert(minmax_values.size() == minmax_values1.size());
        piranha_assert(minmax_values.size() == minmax_values2.size());
        for (decltype(minmax_values.size()) i = 0u; i < minmax_values.size(); ++i) {
            if (unlikely(minmax_values[i].first < -minmax_vec[i] || minmax_values[i].second > minmax_vec[i])) {
                piranha_throw(std::overflow_error, "Kronecker monomial components are out of bounds");
            }
        }
    }
    // Implementation detail of the bound checking logic. This is common enough to be shared.
    template <typename MmVec, typename Func>
    void check_bounds_impl(MmVec &minmax_values1, MmVec &minmax_values2, Func &thread_func) const
    {
#ifndef PIRANHA_SINGLE_THREAD
        if (this->m_n_threads == 1u) {
#endif
            thread_func(0u, &(this->m_v1), &minmax_values1);
            thread_func(0u, &(this->m_v2), &minmax_values2);
#ifndef PIRANHA_SINGLE_THREAD
        } else {
            // Series 1.
            {
                future_list<void> ff_list;
                try {
                    for (unsigned i = 0u; i < this->m_n_threads; ++i) {
                        ff_list.push_back(thread_pool::enqueue(i, thread_func, i, &(this->m_v1), &minmax_values1));
                    }
                    // First let's wait for everything to finish.
                    ff_list.wait_all();
                    // Then, let's handle the exceptions.
                    ff_list.get_all();
                } catch (...) {
                    ff_list.wait_all();
                    throw;
                }
            }
            // Series 2.
            {
                future_list<void> ff_list;
                try {
                    for (unsigned i = 0u; i < this->m_n_threads; ++i) {
                        ff_list.push_back(thread_pool::enqueue(i, thread_func, i, &(this->m_v2), &minmax_values2));
                    }
                    // First let's wait for everything to finish.
                    ff_list.wait_all();
                    // Then, let's handle the exceptions.
                    ff_list.get_all();
                } catch (...) {
                    ff_list.wait_all();
                    throw;
                }
            }
        }
#endif
    }
    // Enabler for the call operator.
    template <typename T>
    using call_enabler =
        typename std::enable_if<key_is_multipliable<cf_t<T>, key_t<T>>::value && has_multiply_accumulate<cf_t<T>>::value
                                    && detail::true_tt<detail::cf_mult_enabler<cf_t<T>>>::value,
                                int>::type;
    // Utility helpers for the subtraction of degree types in the truncation routines. The specialisation
    // for integral types will check the operation for overflow.
    template <typename T, typename std::enable_if<!std::is_integral<T>::value, int>::type = 0>
    static T degree_sub(const T &a, const T &b)
    {
        return a - b;
    }
    template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    static T degree_sub(const T &a, const T &b)
    {
        T retval(a);
        detail::safe_integral_subber(retval, b);
        return retval;
    }
    // Dispatch of untruncated multiplication.
    template <typename T = Series,
              typename std::enable_if<detail::is_kronecker_monomial<typename T::term_type::key_type>::value, int>::type
              = 0>
    Series um_impl() const
    {
        return untruncated_kronecker_mult();
    }
    template <typename T = Series,
              typename std::enable_if<!detail::is_kronecker_monomial<typename T::term_type::key_type>::value, int>::type
              = 0>
    Series um_impl() const
    {
        return this->plain_multiplication();
    }

public:
    /// Constructor.
    /**
     * The constructor will call the base constructor and run these additional checks:
     * - if the key is a piranha::kronecker_monomial, it will be checked that the result of the multiplication does
     *   not overflow the representation limits of piranha::kronecker_monomial;
     * - if the key is a piranha::monomial of a C++ integral type, it will be checked that the result of the
     *   multiplication does not overflow the limits of the integral type.
     *
     * If any check fails, a runtime error will be produced.
     *
     * @param s1 first series operand.
     * @param s2 second series operand.
     *
     * @throws std::overflow_error if a bounds check, as described above, fails.
     * @throws unspecified any exception thrown by:
     * - the base constructor,
     * - standard threading primitives,
     * - memory errors in standard containers,
     * - thread_pool::enqueue(),
     * - future_list::push_back().
     */
    explicit series_multiplier(const Series &s1, const Series &s2) : base(s1, s2)
    {
        // Nothing to do if the series are null or the merged symbol set is empty.
        if (unlikely(this->m_v1.empty() || this->m_v2.empty() || this->m_ss.size() == 0u)) {
            return;
        }
        check_bounds();
    }
    /// Perform multiplication.
    /**
     * \note
     * This template operator is enabled only if:
     * - the coefficient and key type of \p Series satisfy piranha::key_is_multipliable,
     * - the coefficient type of \p Series supports piranha::math::mul3() and piranha::math::multiply_accumulate().
     *
     * This method will perform the multiplication of the series operands passed to the constructor. Depending on
     * the key type of \p Series, the implementation will use either base_series_multiplier::plain_multiplication()
     * with base_series_multiplier::plain_multiplier or a different algorithm.
     *
     * If a polynomial truncation threshold is defined and the degree type of the polynomial is a C++ integral type,
     * the integral arithmetic operations involved in the truncation logic will be checked for overflow.
     *
     * @return the result of the multiplication of the input series operands.
     *
     * @throws std::overflow_error in case of overflow errors.
     * @throws unspecified any exception thrown by:
     * - piranha::base_series_multiplier::plain_multiplication(),
     * - piranha::base_series_multiplier::estimate_final_series_size(),
     * - piranha::base_series_multiplier::sanitise_series(),
     * - piranha::base_series_multiplier::finalise_series(),
     * - <tt>boost::numeric_cast()</tt>,
     * - the public interface of piranha::hash_set,
     * - piranha::safe_cast(),
     * - memory errors in standard containers,
     * - piranha::math::mul3(),
     * - piranha::math::multiply_accumulate(),
     * - thread_pool::enqueue(),
     * - future_list::push_back(),
     * - _truncated_multiplication(),
     * - polynomial::get_auto_truncate_degree().
     */
    template <typename T = Series, call_enabler<T> = 0>
    Series operator()() const
    {
        return execute();
    }
    /** @name Low-level interface
     * Low-level methods, on top of which the call operator is implemented.
     */
    //@{
    /// Untruncated multiplication.
    /**
     * \note
     * This method can be used only if operator()() can be called.
     *
     * This method will return the result of multiplying the two polynomials used as input arguments
     * in the class' constructor. The multiplication will be untruncated, regardless of the current global
     * truncation settings.
     *
     * @return the result of the untruncated multiplication of the two operands used in the construction of \p this.
     *
     * @throws unspecified any exception thrown by:
     * - piranha::base_series_multiplier::plain_multiplication(),
     * - piranha::base_series_multiplier::estimate_final_series_size(),
     * - piranha::base_series_multiplier::sanitise_series(),
     * - piranha::base_series_multiplier::finalise_series(),
     * - <tt>boost::numeric_cast()</tt>,
     * - the public interface of piranha::hash_set,
     * - piranha::safe_cast(),
     * - memory errors in standard containers,
     * - piranha::math::mul3(),
     * - piranha::math::multiply_accumulate(),
     * - thread_pool::enqueue(),
     * - future_list::push_back().
     */
    Series _untruncated_multiplication() const
    {
        return um_impl();
    }
    /// Truncated multiplication.
    /**
     * \note
     * This method can be used only if the following conditions apply:
     * - the conditions for truncated multiplication outlined in piranha::polynomial are satisfied,
     * - the type \p T is the same as the degree type of the polynomial,
     * - the number and types of \p Args is as specified below,
     * - piranha::base_series_multiplier::plain_multiplication() and _get_skip_limits() can be called.
     *
     * This method will perform the truncated multiplication of the series operands passed to the constructor.
     * The truncation degree is set to \p max_degree, and it is either:
     * - the total maximum degree, if the number of \p Args is zero, or
     * - the partial degree, if the number of \p Args is two.
     *
     * In the latter case, the two arguments must be:
     * - an \p std::vector of \p std::string representing the names of the variables which will be taken
     *   into account when computing the partial degree,
     * - a piranha::symbol_set::positions referring to the positions of the variables of the first argument
     *   in the merged symbol set of the two operands.
     *
     * @param max_degree the maximum degree of the result of the multiplication.
     * @param args either an empty argument, or a pair of arguments as described above.
     *
     * @return the result of the truncated multiplication of the operands used for construction.
     *
     * @throws unspecified any exception thrown by:
     * - memory errors in standard containers,
     * - piranha::safe_cast(),
     * - arithmetic and other operations on the degree type,
     * - base_series_multiplier::plain_multiplication(),
     * - _get_skip_limits().
     */
    template <typename T, typename... Args>
    Series _truncated_multiplication(const T &max_degree, const Args &... args) const
    {
        // NOTE: a possible optimisation here is the following: if the sum degrees of the arguments is less than
        // or equal to the max truncation degree, just do the normal multiplication - which can also then take
        // advantage of faster Kronecker multiplication, if the series are suitable.
        using term_type = typename Series::term_type;
        // NOTE: degree type is the same in total and partial.
        using degree_type = decltype(ps_get_degree(term_type{}, this->m_ss));
        using size_type = typename base::size_type;
        namespace sph = std::placeholders;
        static_assert(std::is_same<T, degree_type>::value, "Invalid degree type");
        static_assert(detail::has_get_auto_truncate_degree<Series>::value, "Invalid series type");
        // First let's create two vectors with the degrees of the terms in the two series.
        using d_size_type = typename std::vector<degree_type>::size_type;
        std::vector<degree_type> v_d1(safe_cast<d_size_type>(this->m_v1.size())),
            v_d2(safe_cast<d_size_type>(this->m_v2.size()));
        detail::parallel_vector_transform(
            this->m_n_threads, this->m_v1, v_d1,
            std::bind(term_degree_getter{}, sph::_1, std::cref(this->m_ss), std::cref(args)...));
        detail::parallel_vector_transform(
            this->m_n_threads, this->m_v2, v_d2,
            std::bind(term_degree_getter{}, sph::_1, std::cref(this->m_ss), std::cref(args)...));
        // Next we need to order the terms in the second series, and also the corresponding degree vector.
        // First we create a vector of indices and we fill it.
        std::vector<size_type> idx_vector(safe_cast<typename std::vector<size_type>::size_type>(this->m_v2.size()));
        std::iota(idx_vector.begin(), idx_vector.end(), size_type(0u));
        // Second, we sort the vector of indices according to the degrees in the second series.
        std::stable_sort(idx_vector.begin(), idx_vector.end(), [&v_d2](const size_type &i1, const size_type &i2) {
            return v_d2[static_cast<d_size_type>(i1)] < v_d2[static_cast<d_size_type>(i2)];
        });
        // Finally, we apply the permutation to v_d2 and m_v2.
        decltype(this->m_v2) v2_copy(this->m_v2.size());
        decltype(v_d2) v_d2_copy(v_d2.size());
        std::transform(idx_vector.begin(), idx_vector.end(), v2_copy.begin(),
                       [this](const size_type &i) { return this->m_v2[i]; });
        std::transform(idx_vector.begin(), idx_vector.end(), v_d2_copy.begin(),
                       [&v_d2](const size_type &i) { return v_d2[static_cast<d_size_type>(i)]; });
        this->m_v2 = std::move(v2_copy);
        v_d2 = std::move(v_d2_copy);
        // Now get the skip limits and we build the limits functor.
        const auto sl = _get_skip_limits(v_d1, v_d2, max_degree);
        auto lf = [&sl](const size_type &idx1) {
            return sl[static_cast<typename std::vector<size_type>::size_type>(idx1)];
        };
        return this->plain_multiplication(lf);
    }
    /// Establish skip limits for truncated multiplication.
    /**
     * \note
     * This method can be called only if \p Series supports truncated multiplication (as explained in
     * piranha::polynomial) and
     * \p T is the same type as the degree type.
     *
     * This method assumes that \p v_d1 and \p v_d2 are vectors containing the degrees of each term in the first and
     * second series respectively, and that \p v_d2 is sorted in ascending order.
     * It will return a vector \p v of indices in the second series such that, given an index \p i in the first
     * series, the term of index <tt>v[i]</tt> in the second series is the first term such that the term-by-term
     * multiplication with the <tt>i</tt>-th term in the first series produces a term of degree greater than
     * \p max_degree. That is, terms of index equal to or greater than <tt>v[i]</tt> in the second series
     * will produce terms with degree greater than \p max_degree when multiplied by the <tt>i</tt>-th term in the first
     * series.
     *
     * @param v_d1 a vector containing the degrees of the terms in the first series.
     * @param v_d2 a sorted vector containing the degrees of the terms in the second series.
     * @param max_degree the truncation degree.
     *
     * @return the vector of skip limits, as explained above.
     *
     * @throws unspecified any exception thrown by:
     * - memory errors in standard containers,
     * - piranha::safe_cast(),
     * - operations on the degree type.
     */
    template <typename T>
    std::vector<typename base::size_type> _get_skip_limits(const std::vector<T> &v_d1, const std::vector<T> &v_d2,
                                                           const T &max_degree) const
    {
        // NOTE: this can be parallelised, but we need to check the heuristic
        // for selecting the number of threads as it is pretty fast wrt the multiplication.
        // Check that we are allowed to call this method.
        PIRANHA_TT_CHECK(detail::has_get_auto_truncate_degree, Series);
        PIRANHA_TT_CHECK(std::is_same, T, decltype(math::degree(Series{})));
        using size_type = typename base::size_type;
        using d_size_type = typename std::vector<T>::size_type;
        piranha_assert(std::is_sorted(v_d2.begin(), v_d2.end()));
        // A vector of indices into the second series.
        std::vector<size_type> idx_vector(safe_cast<typename std::vector<size_type>::size_type>(this->m_v2.size()));
        std::iota(idx_vector.begin(), idx_vector.end(), size_type(0u));
        // The return value.
        std::vector<size_type> retval;
        for (const auto &d1 : v_d1) {
            // Here we will find the index of the first term t2 in the second series such that
            // the degree d2 of t2 is > max_degree - d1, that is, d1 + d2 > max_degree.
            // NOTE: we need to use upper_bound, instead of lower_bound, because we need to find the first
            // element which is *strictly* greater than the max degree, as upper bound of a half closed
            // interval.
            // NOTE: the functor of lower_bound works inversely wrt upper_bound. See the notes on the type
            // requirements for the functor here:
            // http://en.cppreference.com/w/cpp/algorithm/upper_bound
            const T comp = degree_sub(max_degree, d1);
            const auto it = std::upper_bound(
                idx_vector.begin(), idx_vector.end(), comp,
                [&v_d2](const T &c, const size_type &idx) { return c < v_d2[static_cast<d_size_type>(idx)]; });
            retval.push_back(it == idx_vector.end() ? static_cast<size_type>(idx_vector.size()) : *it);
        }
        // Check the consistency of the result in debug mode.
        auto retval_checker = [&retval, &v_d1, &v_d2, &max_degree, this]() -> bool {
            for (decltype(retval.size()) i = 0u; i < retval.size(); ++i) {
                // NOTE: this just means that all terms in s2 are within the limit.
                if (retval[i] == v_d2.size()) {
                    continue;
                }
                if (retval[i] > v_d2.size()) {
                    return false;
                }
                if (!(v_d2[static_cast<d_size_type>(retval[i])]
                      > this->degree_sub(max_degree, v_d1[static_cast<d_size_type>(i)]))) {
                    return false;
                }
            }
            return true;
        };
        (void)retval_checker;
        piranha_assert(retval.size() == this->m_v1.size());
        piranha_assert(retval_checker());
        return retval;
    }
    //@}
private:
    // NOTE: wrapper to multadd that treats specially rational coefficients. We need to decide in the future
    // if this stays here or if it is better to generalise it.
    template <typename T, typename std::enable_if<!detail::is_mp_rational<T>::value, int>::type = 0>
    static void fma_wrap(T &a, const T &b, const T &c)
    {
        math::multiply_accumulate(a, b, c);
    }
    template <typename T, typename std::enable_if<detail::is_mp_rational<T>::value, int>::type = 0>
    static void fma_wrap(T &a, const T &b, const T &c)
    {
        math::multiply_accumulate(a._num(), b.num(), c.num());
    }
    // Wrapper for the plain multiplication routine.
    // Case 1: no auto truncation available, just run the plain multiplication.
    template <typename T = Series,
              typename std::enable_if<!detail::has_get_auto_truncate_degree<T>::value, int>::type = 0>
    Series plain_multiplication_wrapper() const
    {
        return this->plain_multiplication();
    }
    // Case 2: auto-truncation available. Check if auto truncation is active.
    template <typename T = Series,
              typename std::enable_if<detail::has_get_auto_truncate_degree<T>::value, int>::type = 0>
    Series plain_multiplication_wrapper() const
    {
        const auto t = T::get_auto_truncate_degree();
        if (std::get<0u>(t) == 0) {
            // No truncation active.
            return this->plain_multiplication();
        }
        // Truncation is active.
        if (std::get<0u>(t) == 1) {
            // Total degree truncation.
            return _truncated_multiplication(std::get<1u>(t));
        }
        piranha_assert(std::get<0u>(t) == 2);
        // Partial degree truncation.
        const symbol_set::positions pos(this->m_ss, symbol_set(std::get<2u>(t).begin(), std::get<2u>(t).end()));
        return _truncated_multiplication(std::get<1u>(t), std::get<2u>(t), pos);
    }
    // NOTE: the existence of these functors is because GCC 4.8 has troubles capturing variadic arguments in lambdas
    // in _truncated_multiplication, and we need to use std::bind instead. Once we switch to 4.9, we can revert
    // to lambdas and drop the <functional> header.
    struct term_degree_sorter {
        using term_type = typename Series::term_type;
        template <typename... Args>
        bool operator()(term_type const *p1, term_type const *p2, const symbol_set &ss, const Args &... args) const
        {
            return ps_get_degree(*p1, args..., ss) < ps_get_degree(*p2, args..., ss);
        }
    };
    struct term_degree_getter {
        using term_type = typename Series::term_type;
        template <typename... Args>
        auto operator()(term_type const *p, const symbol_set &ss, const Args &... args) const
            -> decltype(ps_get_degree(*p, args..., ss))
        {
            return ps_get_degree(*p, args..., ss);
        }
    };
    // execute() is the top level dispatch for the actual multiplication.
    // Case 1: not a Kronecker monomial, do the plain mult.
    template <typename T = Series,
              typename std::enable_if<!detail::is_kronecker_monomial<typename T::term_type::key_type>::value, int>::type
              = 0>
    Series execute() const
    {
        return plain_multiplication_wrapper();
    }
    // Checking for active truncation.
    template <typename T = Series,
              typename std::enable_if<detail::has_get_auto_truncate_degree<T>::value, int>::type = 0>
    bool check_truncation() const
    {
        const auto t = T::get_auto_truncate_degree();
        return std::get<0u>(t) != 0;
    }
    template <typename T = Series,
              typename std::enable_if<!detail::has_get_auto_truncate_degree<T>::value, int>::type = 0>
    bool check_truncation() const
    {
        return false;
    }
    // Case 2: Kronecker mult, do the special multiplication unless a truncation is active. In that case, run the
    // plain mult.
    template <typename T = Series,
              typename std::enable_if<detail::is_kronecker_monomial<typename T::term_type::key_type>::value, int>::type
              = 0>
    Series execute() const
    {
        if (check_truncation()) {
            return plain_multiplication_wrapper();
        }
        return untruncated_kronecker_mult();
    }
    template <typename T = Series,
              typename std::enable_if<detail::is_kronecker_monomial<typename T::term_type::key_type>::value, int>::type
              = 0>
    Series untruncated_kronecker_mult() const
    {
        // Cache the sizes.
        const auto size1 = this->m_v1.size(), size2 = this->m_v2.size();
        // Determine whether we want to estimate or not. We check the threshold, and
        // we force the estimation in multithreaded mode.
        bool estimate = true;
        const auto e_thr = tuning::get_estimate_threshold();
        if (integer(size1) * size2 < integer(e_thr) * e_thr && this->m_n_threads == 1u) {
            estimate = false;
        }
        // If estimation is not worth it, we go with the plain multiplication.
        // NOTE: this is probably not optimal, but we have to do like this as the sparse
        // Kronecker multiplication below requires estimation. Maybe in the future we can
        // have a version without estimation.
        if (!estimate) {
            return this->plain_multiplication();
        }
        // Setup the return value.
        Series retval;
        retval.set_symbol_set(this->m_ss);
        // Do not do anything if one of the two series is empty, just return an empty series.
        if (unlikely(!size1 || !size2)) {
            return retval;
        }
        // Rehash the retun value's container accordingly. Check the tuning flag to see if we want to use
        // multiple threads for initing the return value.
        // NOTE: it is important here that we use the same n_threads for multiplication and memset as
        // we tie together pinned threads with potentially different NUMA regions.
        const unsigned n_threads_rehash = tuning::get_parallel_memory_set() ? this->m_n_threads : 1u;
        // Use the plain functor in normal mode for the estimation.
        const auto est
            = this->template estimate_final_series_size<1u, typename base::template plain_multiplier<false>>();
        // NOTE: if something goes wrong here, no big deal as retval is still empty.
        retval._container().rehash(boost::numeric_cast<typename Series::size_type>(
                                       std::ceil(static_cast<double>(est) / retval._container().max_load_factor())),
                                   n_threads_rehash);
        piranha_assert(retval._container().bucket_count());
        sparse_kronecker_multiplication(retval);
        return retval;
    }
    void sparse_kronecker_multiplication(Series &retval) const
    {
        using bucket_size_type = typename base::bucket_size_type;
        using size_type = typename base::size_type;
        using term_type = typename Series::term_type;
        // Type representing multiplication tasks:
        // - the current term index from s1,
        // - the first term index in s2,
        // - the last term index in s2.
        using task_type = std::tuple<size_type, size_type, size_type>;
        // Cache a few quantities.
        auto &v1 = this->m_v1;
        auto &v2 = this->m_v2;
        const auto size1 = v1.size();
        const auto size2 = v2.size();
        auto &container = retval._container();
        // A convenience functor to compute the destination bucket
        // of a term into retval.
        auto r_bucket = [&container](term_type const *p) { return container._bucket_from_hash(p->hash()); };
        // Sort input terms according to bucket positions in retval.
        auto term_cmp = [&r_bucket](term_type const *p1, term_type const *p2) { return r_bucket(p1) < r_bucket(p2); };
        std::stable_sort(v1.begin(), v1.end(), term_cmp);
        std::stable_sort(v2.begin(), v2.end(), term_cmp);
        // Task comparator. It will compare the bucket index of the terms resulting from
        // the multiplication of the term in the first series by the first term in the block
        // of the second series. This is essentially the first bucket index of retval in which the task
        // will write.
        // NOTE: this is guaranteed not to overflow as the max bucket size in the hash set is 2**(nbits-1),
        // and the max value of bucket_size_type is 2**nbits - 1.
        auto task_cmp = [&r_bucket, &v1, &v2](const task_type &t1, const task_type &t2) {
            return r_bucket(v1[std::get<0u>(t1)]) + r_bucket(v2[std::get<1u>(t1)])
                   < r_bucket(v1[std::get<0u>(t2)]) + r_bucket(v2[std::get<1u>(t2)]);
        };
        // Task block size.
        const size_type block_size = safe_cast<size_type>(tuning::get_multiplication_block_size());
        // Task splitter: split a task in block_size sized tasks and append them to out.
        auto task_split = [block_size](const task_type &t, std::vector<task_type> &out) {
            size_type start = std::get<1u>(t), end = std::get<2u>(t);
            while (static_cast<size_type>(end - start) > block_size) {
                out.emplace_back(std::get<0u>(t), start, static_cast<size_type>(start + block_size));
                start = static_cast<size_type>(start + block_size);
            }
            if (end != start) {
                out.emplace_back(std::get<0u>(t), start, end);
            }
        };
        // End of the container, always the same value.
        const auto it_end = container.end();
        // Function to perform all the term-by-term multiplications in a task, using tmp_term
        // as a temporary value for the computation of the result.
        auto task_consume = [&v1, &v2, &container, it_end, this](const task_type &task, term_type &tmp_term) {
            // Get the term in the first series.
            term_type const *t1 = v1[std::get<0u>(task)];
            // Get pointers to the second series.
            term_type const **start2 = &(v2[std::get<1u>(task)]), **end2 = &(v2[std::get<2u>(task)]);
            // NOTE: these will have to be adapted for kd_monomial.
            using int_type = decltype(t1->m_key.get_int());
            // Get shortcuts to cf and key in t1.
            const auto &cf1 = t1->m_cf;
            const int_type key1 = t1->m_key.get_int();
            // Iterate over the task.
            for (; start2 != end2; ++start2) {
                // Const ref to the current term in the second series.
                const auto &cur = **start2;
                // Add the keys.
                // NOTE: this will have to be adapted for kd_monomial.
                tmp_term.m_key.set_int(static_cast<int_type>(key1 + cur.m_key.get_int()));
                // Try to locate the term into retval.
                auto bucket_idx = container._bucket(tmp_term);
                const auto it = container._find(tmp_term, bucket_idx);
                if (it == it_end) {
                    // NOTE: for coefficient series, we might want to insert with move() below,
                    // as we are not going to re-use the allocated resources in tmp.m_cf.
                    // Take care of multiplying the coefficient.
                    detail::cf_mult_impl(tmp_term.m_cf, cf1, cur.m_cf);
                    container._unique_insert(tmp_term, bucket_idx);
                } else {
                    // NOTE: here we need to decide if we want to give the same treatment to fmp as we did with
                    // cf_mult_impl.
                    // For the moment it is an implementation detail of this class.
                    this->fma_wrap(it->m_cf, cf1, cur.m_cf);
                }
            }
        };
#ifndef PIRANHA_SINGLE_THREAD
        if (this->m_n_threads == 1u) {
#endif
            try {
                // Single threaded case.
                // Create the vector of tasks.
                std::vector<task_type> tasks;
                for (decltype(v1.size()) i = 0u; i < size1; ++i) {
                    task_split(std::make_tuple(i, size_type(0u), size2), tasks);
                }
                // Sort the tasks.
                std::stable_sort(tasks.begin(), tasks.end(), task_cmp);
                // Iterate over the tasks and run the multiplication.
                term_type tmp_term;
                for (const auto &t : tasks) {
                    task_consume(t, tmp_term);
                }
                this->sanitise_series(retval, this->m_n_threads);
                this->finalise_series(retval);
            } catch (...) {
                retval._container().clear();
                throw;
            }
            return;
#ifndef PIRANHA_SINGLE_THREAD
        }
        // Number of buckets in retval.
        const bucket_size_type bucket_count = container.bucket_count();
        // Compute the number of zones in which the output container will be subdivided,
        // a multiple of the number of threads.
        // NOTE: zm is a tuning parameter.
        const unsigned zm = 10u;
        const bucket_size_type n_zones = static_cast<bucket_size_type>(integer(this->m_n_threads) * zm);
        // Number of buckets per zone (can be zero).
        const bucket_size_type bpz = static_cast<bucket_size_type>(bucket_count / n_zones);
        // For each zone, we need to define a vector of tasks that will write only into that zone.
        std::vector<std::vector<task_type>> task_table;
        task_table.resize(safe_cast<decltype(task_table.size())>(n_zones));
        // Lower bound implementation. Adapted from:
        // http://en.cppreference.com/w/cpp/algorithm/lower_bound
        // Given the [first,last[ index range in v2, find the first index idx in the v2 range such that the i-th
        // term in v1 multiplied by the idx-th term in v2 will be written into retval at a bucket index not less than
        // zb.
        auto l_bound = [&v1, &v2, &r_bucket, &task_split](size_type first, size_type last, bucket_size_type zb,
                                                          size_type i) -> size_type {
            piranha_assert(first <= last);
            bucket_size_type ib = r_bucket(v1[i]);
            // Avoid zb - ib below wrapping around.
            if (zb < ib) {
                return 0u;
            }
            const auto cmp = static_cast<bucket_size_type>(zb - ib);
            size_type idx, step, count = static_cast<size_type>(last - first);
            while (count > 0u) {
                idx = first;
                step = static_cast<size_type>(count / 2u);
                idx = static_cast<size_type>(idx + step);
                if (r_bucket(v2[idx]) < cmp) {
                    idx = static_cast<size_type>(idx + 1u);
                    first = idx;
                    if (count <= step + 1u) {
                        break;
                    }
                    count = static_cast<size_type>(count - (step + 1u));
                } else {
                    count = step;
                }
            }
            return first;
        };
        // Fill the task table.
        auto table_filler = [&task_table, bpz, zm, this, bucket_count, size1, size2, &l_bound, &task_split,
                             &task_cmp](const unsigned &thread_idx) {
            for (unsigned n = 0u; n < zm; ++n) {
                std::vector<task_type> cur_tasks;
                // [a,b[ is the container zone.
                bucket_size_type a = static_cast<bucket_size_type>(thread_idx * bpz * zm + n * bpz);
                bucket_size_type b;
                if (n == zm - 1u && thread_idx == this->m_n_threads - 1u) {
                    // Special casing if this is the last zone in the container.
                    b = bucket_count;
                } else {
                    b = static_cast<bucket_size_type>(a + bpz);
                }
                // First batch of tasks.
                for (size_type i = 0u; i < size1; ++i) {
                    auto t = std::make_tuple(i, l_bound(0u, size2, a, i), l_bound(0u, size2, b, i));
                    if (std::get<1u>(t) == 0u && std::get<2u>(t) == 0u) {
                        // This means that all the next tasks we will compute will be empty,
                        // no sense in calculating them.
                        break;
                    }
                    task_split(t, cur_tasks);
                }
                // Second batch of tasks.
                // Note: we can always compute a,b + bucket_count because of the limits on the maximum value of
                // bucket_count.
                for (size_type i = 0u; i < size1; ++i) {
                    auto t = std::make_tuple(i, l_bound(0u, size2, static_cast<bucket_size_type>(a + bucket_count), i),
                                             l_bound(0u, size2, static_cast<bucket_size_type>(b + bucket_count), i));
                    if (std::get<1u>(t) == 0u && std::get<2u>(t) == 0u) {
                        break;
                    }
                    task_split(t, cur_tasks);
                }
                // Sort the task vector.
                std::stable_sort(cur_tasks.begin(), cur_tasks.end(), task_cmp);
                // Move the vector of tasks in the table.
                task_table[static_cast<decltype(task_table.size())>(thread_idx * zm + n)] = std::move(cur_tasks);
            }
        };
        // Go with the threads to fill the task table.
        future_list<decltype(table_filler(0u))> ff_list;
        try {
            for (unsigned i = 0u; i < this->m_n_threads; ++i) {
                ff_list.push_back(thread_pool::enqueue(i, table_filler, i));
            }
            // First let's wait for everything to finish.
            ff_list.wait_all();
            // Then, let's handle the exceptions.
            ff_list.get_all();
        } catch (...) {
            ff_list.wait_all();
            throw;
        }
        // Check the consistency of the table for debug purposes.
        auto table_checker = [&task_table, size1, size2, &r_bucket, bpz, bucket_count, &v1, &v2]() -> bool {
            // Total number of term-by-term multiplications. Needs to be equal
            // to size1 * size2 at the end.
            integer tot_n(0);
            // Tmp term for multiplications.
            term_type tmp_term;
            for (decltype(task_table.size()) i = 0u; i < task_table.size(); ++i) {
                const auto &v = task_table[i];
                // Bucket limits of each zone.
                bucket_size_type a = static_cast<bucket_size_type>(bpz * i), b;
                // Special casing for the last zone in the table.
                if (i == task_table.size() - 1u) {
                    b = bucket_count;
                } else {
                    b = static_cast<bucket_size_type>(a + bpz);
                }
                for (const auto &t : v) {
                    auto idx1 = std::get<0u>(t), start2 = std::get<1u>(t), end2 = std::get<2u>(t);
                    using int_type = decltype(v1[idx1]->m_key.get_int());
                    piranha_assert(start2 <= end2);
                    tot_n += end2 - start2;
                    for (; start2 != end2; ++start2) {
                        tmp_term.m_key.set_int(
                            static_cast<int_type>(v1[idx1]->m_key.get_int() + v2[start2]->m_key.get_int()));
                        auto b_idx = r_bucket(&tmp_term);
                        if (b_idx < a || b_idx >= b) {
                            return false;
                        }
                    }
                }
            }
            return tot_n == integer(size1) * size2;
        };
        (void)table_checker;
        piranha_assert(table_checker());
        // Init the vector of atomic flags.
        detail::atomic_flag_array af(safe_cast<std::size_t>(task_table.size()));
        // Thread functor.
        auto thread_functor = [zm, &task_table, &af, &v1, &v2, &container, &task_consume](const unsigned &thread_idx) {
            using t_size_type = decltype(task_table.size());
            // Temporary term_type for caching.
            term_type tmp_term;
            // The starting index in the task table.
            auto t_idx = static_cast<t_size_type>(t_size_type(thread_idx) * zm);
            const auto start_t_idx = t_idx;
            while (true) {
                // If this returns false, it means that the tasks still need to be consumed;
                if (!af[static_cast<std::size_t>(t_idx)].test_and_set()) {
                    // Current vector of tasks.
                    const auto &cur_tasks = task_table[t_idx];
                    for (const auto &t : cur_tasks) {
                        task_consume(t, tmp_term);
                    }
                }
                // Update the index, wrapping around if necessary.
                t_idx = static_cast<t_size_type>(t_idx + 1u);
                if (t_idx == task_table.size()) {
                    t_idx = 0u;
                }
                // If we got back to the original index, get out.
                if (t_idx == start_t_idx) {
                    break;
                }
            }
        };
        // Go with the multiplication threads.
        future_list<decltype(thread_functor(0u))> ft_list;
        try {
            for (unsigned i = 0u; i < this->m_n_threads; ++i) {
                ft_list.push_back(thread_pool::enqueue(i, thread_functor, i));
            }
            // First let's wait for everything to finish.
            ft_list.wait_all();
            // Then, let's handle the exceptions.
            ft_list.get_all();
            // Finally, fix and finalise the series.
            this->sanitise_series(retval, this->m_n_threads);
            this->finalise_series(retval);
        } catch (...) {
            ft_list.wait_all();
            // Clean up and re-throw.
            retval._container().clear();
            throw;
        }
#endif
    }
};
}

#endif

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

#ifndef PIRANHA_DETAIL_BASE_SERIES_MULTIPLIER_HPP
#define PIRANHA_DETAIL_BASE_SERIES_MULTIPLIER_HPP

#include <algorithm>
#include <array>
#include <boost/numeric/conversion/cast.hpp>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <limits>
#include <mutex>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "config.hpp"
#include "detail/atomic_utils.hpp"
#include "exceptions.hpp"
#include "key_is_multipliable.hpp"
#include "math.hpp"
#include "mp_integer.hpp"
#include "mp_rational.hpp"
#include "safe_cast.hpp"
#include "series.hpp"
#include "settings.hpp"
#include "symbol_set.hpp"
#include "thread_pool.hpp"
#include "tuning.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

template <typename Series, typename Derived, typename = void>
struct base_series_multiplier_impl {
    using term_type = typename Series::term_type;
    using container_type = typename std::decay<decltype(std::declval<Series>()._container())>::type;
    using c_size_type = typename container_type::size_type;
    using v_size_type = typename std::vector<term_type const *>::size_type;
    template <typename Term,
              typename std::enable_if<!is_less_than_comparable<typename Term::key_type>::value, int>::type = 0>
    void fill_term_pointers(const container_type &c1, const container_type &c2, std::vector<Term const *> &v1,
                            std::vector<Term const *> &v2)
    {
        // If the key is not less-than comparable, we can only copy over the pointers as they are.
        std::transform(c1.begin(), c1.end(), std::back_inserter(v1), [](const term_type &t) { return &t; });
        std::transform(c2.begin(), c2.end(), std::back_inserter(v2), [](const term_type &t) { return &t; });
    }
    template <typename Term,
              typename std::enable_if<is_less_than_comparable<typename Term::key_type>::value, int>::type = 0>
    void fill_term_pointers(const container_type &c1, const container_type &c2, std::vector<Term const *> &v1,
                            std::vector<Term const *> &v2)
    {
        // Fetch the number of threads from the derived class.
        const unsigned n_threads = static_cast<Derived *>(this)->m_n_threads;
        piranha_assert(n_threads > 0u);
        // Threading functor.
        auto thread_func
            = [n_threads](unsigned thread_idx, const container_type *c, std::vector<term_type const *> *v) {
                  piranha_assert(thread_idx < n_threads);
                  // Total bucket count.
                  const auto b_count = c->bucket_count();
                  // Buckets per thread.
                  const auto bpt = b_count / n_threads;
                  // End index.
                  const auto end
                      = static_cast<c_size_type>((thread_idx == n_threads - 1u) ? b_count : (bpt * (thread_idx + 1u)));
                  // Sorter.
                  auto sorter = [](term_type const *p1, term_type const *p2) { return p1->m_key < p2->m_key; };
                  v_size_type j = 0u;
                  for (auto start = static_cast<c_size_type>(bpt * thread_idx); start < end; ++start) {
                      const auto &b = c->_get_bucket_list(start);
                      v_size_type tmp = 0u;
                      for (const auto &t : b) {
                          v->push_back(&t);
                          ++tmp;
                      }
                      std::stable_sort(v->data() + j, v->data() + j + tmp, sorter);
                      j += tmp;
                  }
              };
        if (n_threads == 1u) {
            thread_func(0u, &c1, &v1);
            thread_func(0u, &c2, &v2);
            return;
        }
        auto thread_wrapper = [&thread_func, n_threads](const container_type *c, std::vector<term_type const *> *v) {
            // In the multi-threaded case, each thread needs to work on a separate vector.
            // We will merge the vectors later.
            using vv_t = std::vector<std::vector<Term const *>>;
            using vv_size_t = typename vv_t::size_type;
            vv_t vv(safe_cast<vv_size_t>(n_threads));
            // Go with the threads.
            future_list<void> ff_list;
            try {
                for (unsigned i = 0u; i < n_threads; ++i) {
                    ff_list.push_back(thread_pool::enqueue(i, thread_func, i, c, &(vv[static_cast<vv_size_t>(i)])));
                }
                // First let's wait for everything to finish.
                ff_list.wait_all();
                // Then, let's handle the exceptions.
                ff_list.get_all();
            } catch (...) {
                ff_list.wait_all();
                throw;
            }
            // Last, we need to merge everything into v.
            for (const auto &vi : vv) {
                v->insert(v->end(), vi.begin(), vi.end());
            }
        };
        thread_wrapper(&c1, &v1);
        thread_wrapper(&c2, &v2);
    }
};

template <typename Series, typename Derived>
struct base_series_multiplier_impl<Series, Derived, typename std::enable_if<is_mp_rational<
                                                        typename Series::term_type::cf_type>::value>::type> {
    // Useful shortcuts.
    using term_type = typename Series::term_type;
    using rat_type = typename term_type::cf_type;
    using int_type = typename std::decay<decltype(std::declval<rat_type>().num())>::type;
    using container_type = typename std::decay<decltype(std::declval<Series>()._container())>::type;
    void fill_term_pointers(const container_type &c1, const container_type &c2, std::vector<term_type const *> &v1,
                            std::vector<term_type const *> &v2)
    {
        // Compute the least common multiplier.
        m_lcm = 1;
        auto it_f = c1.end();
        int_type g;
        for (auto it = c1.begin(); it != it_f; ++it) {
            math::gcd3(g, m_lcm, it->m_cf.den());
            math::mul3(m_lcm, m_lcm, it->m_cf.den());
            int_type::_divexact(m_lcm, m_lcm, g);
        }
        it_f = c2.end();
        for (auto it = c2.begin(); it != it_f; ++it) {
            math::gcd3(g, m_lcm, it->m_cf.den());
            math::mul3(m_lcm, m_lcm, it->m_cf.den());
            int_type::_divexact(m_lcm, m_lcm, g);
        }
        // All these computations involve only positive numbers,
        // the GCD must always be positive.
        piranha_assert(m_lcm.sign() == 1);
        // Copy over the terms and renormalise to lcm.
        it_f = c1.end();
        for (auto it = c1.begin(); it != it_f; ++it) {
            // NOTE: these divisions are exact, we could take advantage of that.
            m_terms1.push_back(term_type(rat_type(m_lcm / it->m_cf.den() * it->m_cf.num(), int_type(1)), it->m_key));
        }
        it_f = c2.end();
        for (auto it = c2.begin(); it != it_f; ++it) {
            m_terms2.push_back(term_type(rat_type(m_lcm / it->m_cf.den() * it->m_cf.num(), int_type(1)), it->m_key));
        }
        // Copy over the pointers.
        std::transform(m_terms1.begin(), m_terms1.end(), std::back_inserter(v1), [](const term_type &t) { return &t; });
        std::transform(m_terms2.begin(), m_terms2.end(), std::back_inserter(v2), [](const term_type &t) { return &t; });
        piranha_assert(v1.size() == c1.size());
        piranha_assert(v2.size() == c2.size());
    }
    std::vector<term_type> m_terms1;
    std::vector<term_type> m_terms2;
    int_type m_lcm;
};
}

/// Base series multiplier.
/**
 * This is a class that provides functionality useful to define a piranha::series_multiplier specialisation. Note that
 * this class
 * alone does *not* fulfill the requirements of a piranha::series_multiplier - it merely provides a set of utilities
 * that can be
 * useful for the implementation of a piranha::series_multiplier.
 *
 * ## Type requirements ##
 *
 * \p Series must satisfy piranha::is_series.
 *
 * ## Exception safety guarantee ##
 *
 * This class provides the strong exception safety guarantee.
 *
 * ## Move semantics ##
 *
 * Instances of this class cannot be copied, moved or assigned.
 */
// Some performance ideas:
// - optimisation in case one series has 1 term with unitary key and both series same type: multiply directly
// coefficients;
// - optimisation for coefficient series that merges all args, similar to the rational optimisation;
// - optimisation for load balancing similar to the poly multiplier.
template <typename Series>
class base_series_multiplier : private detail::base_series_multiplier_impl<Series, base_series_multiplier<Series>>
{
    PIRANHA_TT_CHECK(is_series, Series);
    // Make friends with the base, so it can access protected/private members of this.
    friend struct detail::base_series_multiplier_impl<Series, base_series_multiplier<Series>>;

public:
    /// Alias for a vector of const pointers to series terms.
    using v_ptr = std::vector<typename Series::term_type const *>;
    /// The size type of base_series_multiplier::v_ptr.
    using size_type = typename v_ptr::size_type;
    /// The size type of \p Series.
    using bucket_size_type = typename Series::size_type;

private:
    // The default limit functor: it will include all terms in the second series.
    struct default_limit_functor {
        default_limit_functor(const base_series_multiplier &m) : m_size2(m.m_v2.size())
        {
        }
        size_type operator()(const size_type &) const
        {
            return m_size2;
        }
        const size_type m_size2;
    };
    // The purpose of this helper is to move in a coefficient series during insertion. For series,
    // we know that moves leave the series in a valid state, and series multiplications do not benefit
    // from an already-constructed destination - hence it is convenient to move them rather than copy.
    template <typename Term, typename std::enable_if<!is_series<typename Term::cf_type>::value, int>::type = 0>
    static Term &term_insertion(Term &t)
    {
        return t;
    }
    template <typename Term, typename std::enable_if<is_series<typename Term::cf_type>::value, int>::type = 0>
    static Term term_insertion(Term &t)
    {
        return Term{std::move(t.m_cf), t.m_key};
    }
    // Implementation of finalise().
    template <typename T,
              typename std::enable_if<detail::is_mp_rational<typename T::term_type::cf_type>::value, int>::type = 0>
    void finalise_impl(T &s) const
    {
        // Nothing to do if the lcm is unitary.
        if (math::is_unitary(this->m_lcm)) {
            return;
        }
        // NOTE: this has to be the square of the lcm, as in addition to uniformising
        // the denominators in each series we are also multiplying the two series.
        const auto l2 = this->m_lcm * this->m_lcm;
        auto &container = s._container();
        // Single thread implementation.
        if (m_n_threads == 1u) {
            for (const auto &t : container) {
                t.m_cf._set_den(l2);
                t.m_cf.canonicalise();
            }
            return;
        }
        // Multi-thread implementation.
        // Buckets per thread.
        const bucket_size_type bpt = static_cast<bucket_size_type>(container.bucket_count() / m_n_threads);
        auto thread_func = [l2, &container, this, bpt](unsigned t_idx) {
            bucket_size_type start_idx = static_cast<bucket_size_type>(t_idx * bpt);
            // Special handling for the last thread.
            const bucket_size_type end_idx = t_idx == (this->m_n_threads - 1u)
                                                 ? container.bucket_count()
                                                 : static_cast<bucket_size_type>((t_idx + 1u) * bpt);
            for (; start_idx != end_idx; ++start_idx) {
                auto &list = container._get_bucket_list(start_idx);
                for (const auto &t : list) {
                    t.m_cf._set_den(l2);
                    t.m_cf.canonicalise();
                }
            }
        };
        // Go with the threads.
        future_list<decltype(thread_func(0u))> ff_list;
        try {
            for (unsigned i = 0u; i < m_n_threads; ++i) {
                ff_list.push_back(thread_pool::enqueue(i, thread_func, i));
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
    template <typename T,
              typename std::enable_if<!detail::is_mp_rational<typename T::term_type::cf_type>::value, int>::type = 0>
    void finalise_impl(T &) const
    {
    }

public:
    /// Constructor.
    /**
     * This constructor will store in the base_series_multiplier::m_v1 and base_series_multiplier::m_v2 protected
     * members
     * references to the terms of the input series \p s1 and \p s2. \p m_v1 will store references to the larger series,
     * \p m_v2 will store references to the smaller serier. The constructor will also store a copy of the symbol set of
     * \p s1 and \p s2 in the protected member base_series_multiplier::m_ss.
     *
     * If the coefficient type of \p Series is an instance of piranha::mp_rational, then the pointers in \p m_v1 and \p
     * m_v2
     * will refer not to the original terms in \p s1 and \p s2 but to *copies* of these terms, in which all coefficients
     * have unitary denominator and the numerators have all been multiplied by the global least common multiplier. This
     * transformation
     * allows to reduce the multiplication of series with rational coefficients to the multiplication of series with
     * integral coefficients.
     *
     * @param[in] s1 first series.
     * @param[in] s2 second series.
     *
     * @throws std::invalid_argument if the symbol sets of \p s1 and \p s2 differ.
     * @throws unspecified any exception thrown by:
     * - thread_pool::use_threads(),
     * - memory allocation errors in standard containers,
     * - the construction of the term type of \p Series.
     */
    explicit base_series_multiplier(const Series &s1, const Series &s2)
        : m_ss(s1.get_symbol_set()),
          m_n_threads((s1.size() && s2.size()) ? thread_pool::use_threads(integer(s1.size()) * s2.size(),
                                                                          integer(settings::get_min_work_per_thread()))
                                               : 1u)
    {
        if (unlikely(s1.get_symbol_set() != s2.get_symbol_set())) {
            piranha_throw(std::invalid_argument, "incompatible arguments sets");
        }
        // The largest series goes first.
        const Series *p1 = &s1, *p2 = &s2;
        if (s1.size() < s2.size()) {
            std::swap(p1, p2);
        }
        // This is just an optimisation, no troubles if there is a truncation due to static_cast.
        m_v1.reserve(static_cast<size_type>(p1->size()));
        m_v2.reserve(static_cast<size_type>(p2->size()));
        // Fill in the vectors of pointers.
        this->fill_term_pointers(p1->_container(), p2->_container(), m_v1, m_v2);
    }
    /// Deleted default constructor.
    base_series_multiplier() = delete;
    /// Deleted copy constructor.
    base_series_multiplier(const base_series_multiplier &) = delete;
    /// Deleted move constructor.
    base_series_multiplier(base_series_multiplier &&) = delete;
    /// Deleted copy assignment operator.
    base_series_multiplier &operator=(const base_series_multiplier &) = delete;
    /// Deleted move assignment operator.
    base_series_multiplier &operator=(base_series_multiplier &&) = delete;

protected:
    /// Blocked multiplication.
    /**
     * \note
     * If \p MultFunctor or \p LimitFunctor do not satisfy the requirements outlined below,
     * a compile-time error will be produced.
     *
     * This method is logically equivalent to the following double loop:
     *
     * @code
     * for (size_type i = start1; i < end1; ++i) {
     *      const size_type limit = std::min(lf(i),m_v2.size());
     *	for (size_type j = 0; j < limit; ++j) {
     *		mf(i,j);
     *	}
     * }
     * @endcode
     *
     * \p mf must be a function object with a call operator accepting two instances of
     * base_series_multiplier::size_type and returning \p void. \p lf must be a function object
     * with a call operator accepting and returning a base_series_multiplier::size_type.
     *
     * Internally, the double loops is decomposed in blocks of size tuning::get_multiplication_block_size() in an
     * attempt to optimise cache memory access patterns.
     *
     * This method is meant to be used for series multiplication. \p mf is intended to be a function object that
     * multiplies the <tt>i</tt>-th term of the first series by the <tt>j</tt>-th term of the second series.
     * \p lf is intended to be a functor that establishes how many terms in the second series have to be multiplied by
     * the <tt>i</tt>-th term of the first series.
     *
     * @param[in] mf the multiplication functor.
     * @param[in] start1 start index in the first series.
     * @param[in] end1 end index in the first series.
     * @param[in] lf the limit functor.
     *
     * @throws std::invalid_argument if \p start1 is greater than \p end1 or greater than the size of
     * base_series_multiplier::m_v1, or if \p end1 is greater than the size of base_series_multiplier::m_v1.
     * @throws unspecified any exception thrown by the call operator of \p mf or \p sf, or piranha::safe_cast.
     */
    template <typename MultFunctor, typename LimitFunctor>
    void blocked_multiplication(const MultFunctor &mf, const size_type &start1, const size_type &end1,
                                const LimitFunctor &lf) const
    {
        PIRANHA_TT_CHECK(is_function_object, MultFunctor, void, const size_type &, const size_type &);
        if (unlikely(start1 > end1 || start1 > m_v1.size() || end1 > m_v1.size())) {
            piranha_throw(std::invalid_argument, "invalid bounds in blocked_multiplication");
        }
        // Block size and number of regular blocks.
        const size_type bsize = safe_cast<size_type>(tuning::get_multiplication_block_size()),
                        nblocks1 = static_cast<size_type>((end1 - start1) / bsize),
                        nblocks2 = static_cast<size_type>(m_v2.size() / bsize);
        // Start and end of last (possibly irregular) blocks.
        const size_type i_ir_start = static_cast<size_type>(nblocks1 * bsize + start1), i_ir_end = end1;
        const size_type j_ir_start = static_cast<size_type>(nblocks2 * bsize), j_ir_end = m_v2.size();
        for (size_type n1 = 0u; n1 < nblocks1; ++n1) {
            const size_type i_start = static_cast<size_type>(n1 * bsize + start1),
                            i_end = static_cast<size_type>(i_start + bsize);
            // regulars1 * regulars2
            for (size_type n2 = 0u; n2 < nblocks2; ++n2) {
                const size_type j_start = static_cast<size_type>(n2 * bsize),
                                j_end = static_cast<size_type>(j_start + bsize);
                for (size_type i = i_start; i < i_end; ++i) {
                    const size_type limit = std::min<size_type>(lf(i), j_end);
                    for (size_type j = j_start; j < limit; ++j) {
                        mf(i, j);
                    }
                }
            }
            // regulars1 * rem2
            for (size_type i = i_start; i < i_end; ++i) {
                const size_type limit = std::min<size_type>(lf(i), j_ir_end);
                for (size_type j = j_ir_start; j < limit; ++j) {
                    mf(i, j);
                }
            }
        }
        // rem1 * regulars2
        for (size_type n2 = 0u; n2 < nblocks2; ++n2) {
            const size_type j_start = static_cast<size_type>(n2 * bsize),
                            j_end = static_cast<size_type>(j_start + bsize);
            for (size_type i = i_ir_start; i < i_ir_end; ++i) {
                const size_type limit = std::min<size_type>(lf(i), j_end);
                for (size_type j = j_start; j < limit; ++j) {
                    mf(i, j);
                }
            }
        }
        // rem1 * rem2.
        for (size_type i = i_ir_start; i < i_ir_end; ++i) {
            const size_type limit = std::min<size_type>(lf(i), j_ir_end);
            for (size_type j = j_ir_start; j < limit; ++j) {
                mf(i, j);
            }
        }
    }
    /// Blocked multiplication (convenience overload).
    /**
     * This method is equivalent to the other overload with a limit functor that will unconditionally
     * always return the size of the second series.
     *
     * @param[in] mf the multiplication functor.
     * @param[in] start1 start index in the first series.
     * @param[in] end1 end index in the first series.
     *
     * @throws unspecified any exception thrown by the other overload.
     */
    template <typename MultFunctor>
    void blocked_multiplication(const MultFunctor &mf, const size_type &start1, const size_type &end1) const
    {
        blocked_multiplication(mf, start1, end1, default_limit_functor{*this});
    }
    /// Estimate size of series multiplication.
    /**
     * \note
     * If \p MultArity, \p MultFunctor or \p LimitFunctor do not satisfy the requirements outlined below,
     * a compile-time error will be produced.
     *
     * This method expects a \p MultFunctor type exposing the same inteface as explained in blocked_multiplication().
     * Additionally, \p MultFunctor must be constructible from a const reference to piranha::base_series_multiplier
     * and a mutable reference to \p Series. \p MultFunctor objects will be constructed internally by this method,
     * passing \p this and a temporary local \p Series object as construction parameters.
     * It will be assumed that a call <tt>mf(i,j)</tt> multiplies the <tt>i</tt>-th
     * term of the first series by the <tt>j</tt>-th term of the second series, accumulating the result into the \p
     * Series
     * passed as second parameter for construction.
     *
     * This method will apply a statistical approach to estimate the final size of the result of the multiplication of
     * the first
     * series by the second. It will perform random term-by-term multiplications
     * and deduce the estimate from the number of term multiplications performed before finding the first duplicate
     * term.
     * The \p MultArity parameter represents the arity of term multiplications - that is, the number of terms generated
     * by a single
     * term-by-term multiplication. It must be strictly positive.
     *
     * The \p lf parameter must be a function object exposing the same inteface as explained in
     * blocked_multiplication().
     * This functor establishes how many terms in the second series must be multiplied by the <tt>i</tt>-th term of the
     * first series.
     *
     * The number returned by this function is always at least 1. Multiple threads might be used by this method: in such
     * a case, different
     * instances of \p MultFunctor are constructed in different threads, but \p lf is shared among all threads.
     *
     * @param[in] lf the limit functor.
     *
     * @return the estimated size of the multiplication of the first series by the second, always at least 1.
     *
     * @throws std::overflow_error in case of (unlikely) overflows in integral arithmetics.
     * @throws unspecified any exception thrown by:
     * - overflow errors in the conversion of piranha::integer to integral types,
     * - the call operator of \p mf or \p lf, and the constructor of \p mf,
     * - memory errors in standard containers,
     * - the conversion operator of piranha::integer,
     * - standard threading primitives,
     * - thread_pool::enqueue(),
     * - future_list::push_back().
     */
    template <std::size_t MultArity, typename MultFunctor, typename LimitFunctor>
    bucket_size_type estimate_final_series_size(const LimitFunctor &lf) const
    {
        PIRANHA_TT_CHECK(is_function_object, MultFunctor, void, const size_type &, const size_type &);
        PIRANHA_TT_CHECK(std::is_constructible, MultFunctor, const base_series_multiplier &, Series &);
        PIRANHA_TT_CHECK(is_function_object, LimitFunctor, size_type, const size_type &);
        // Cache these.
        const size_type size1 = m_v1.size(), size2 = m_v2.size();
        constexpr std::size_t result_size = MultArity;
        // If one of the two series is empty, just return 0.
        if (unlikely(!size1 || !size2)) {
            return 1u;
        }
        // If either series has a size of 1, just return size1 * size2 * result_size.
        if (size1 == 1u || size2 == 1u) {
            return static_cast<bucket_size_type>(integer(size1) * size2 * result_size);
        }
        // NOTE: Hard-coded number of trials.
        // NOTE: here consider that in case of extremely sparse series with few terms this will incur in noticeable
        // overhead, since we will need many term-by-term before encountering the first duplicate.
        const unsigned n_trials = 15u;
        // NOTE: Hard-coded value for the estimation multiplier.
        // NOTE: This value should be tuned for performance/memory usage tradeoffs.
        const unsigned multiplier = 2u;
        // Number of threads to use. If there are more threads than trials, then reduce
        // the number of actual threads to use.
        // NOTE: this is a bit different from usual, where we do not care if the workload per thread is zero.
        // We do like this because n_trials is a small number and there still seems to be benefit in running
        // just 1 trial per thread.
        const unsigned n_threads = (n_trials >= m_n_threads) ? m_n_threads : n_trials;
        piranha_assert(n_threads > 0u);
        // Trials per thread. This will always be at least 1.
        const unsigned tpt = n_trials / n_threads;
        piranha_assert(tpt >= 1u);
        // The cumulative estimate.
        integer c_estimate(0);
        // Sync mutex - actually used only in multithreading.
        std::mutex mut;
        // The estimation functor.
        auto estimator = [&lf, size1, n_threads, multiplier, tpt, n_trials, this, &c_estimate, &mut,
                          result_size](unsigned thread_idx) {
            piranha_assert(thread_idx < n_threads);
            // Vectors of indices into m_v1.
            std::vector<size_type> v_idx1(safe_cast<typename std::vector<size_type>::size_type>(size1));
            std::iota(v_idx1.begin(), v_idx1.end(), size_type(0));
            // Copy in order to reset to initial state later.
            const auto v_idx1_copy = v_idx1;
            // Random number engine.
            std::mt19937 engine;
            // Uniform int distribution.
            using dist_type = std::uniform_int_distribution<size_type>;
            dist_type dist;
            // Init the accumulated estimation for averaging later.
            integer acc(0);
            // Number of trials for this thread - usual special casing for the last thread.
            const unsigned cur_trials = (thread_idx == n_threads - 1u) ? (n_trials - thread_idx * tpt) : tpt;
            // This should always be guaranteed because tpt is never 0.
            piranha_assert(cur_trials > 0u);
            // Create and setup the temp series.
            Series tmp;
            tmp.set_symbol_set(m_ss);
            // Create the multiplier.
            MultFunctor mf(*this, tmp);
            // Go with the trials.
            for (auto n = 0u; n < cur_trials; ++n) {
                // Seed the engine. The seed should be the global trial number, accounting for multiple
                // threads. This way the estimation will not depend on the number of threads.
                engine.seed(static_cast<std::mt19937::result_type>(tpt * thread_idx + n));
                // Reset the indices vector and re-randomise it.
                // NOTE: we need to do this as every run inside this for loop must be completely independent
                // of any previous run, we cannot keep any state.
                v_idx1 = v_idx1_copy;
                std::shuffle(v_idx1.begin(), v_idx1.end(), engine);
                // The counter. This will be increased each time a term-by-term multiplication
                // does not generate a duplicate term.
                size_type count = 0u;
                // This will be used to determine the average number of terms in s2
                // that participate in the multiplication.
                integer acc_s2(0);
                auto it1 = v_idx1.begin();
                for (; it1 != v_idx1.end(); ++it1) {
                    // Get the limit idx in s2.
                    const size_type limit = lf(*it1);
                    // This is the upper limit of an open ended interval, so it needs
                    // to be decreased by one in order to be used in dist. If zero, it means
                    // there are no terms in v2 that can be multiplied by the current term in t1.
                    if (limit == 0u) {
                        continue;
                    }
                    acc_s2 += limit;
                    // Pick a random index in m_v2 within the limit.
                    const size_type idx2
                        = dist(engine, typename dist_type::param_type(static_cast<size_type>(0u),
                                                                      static_cast<size_type>(limit - 1u)));
                    // Perform term multiplication.
                    mf(*it1, idx2);
                    // Check for unlikely overflows when increasing count.
                    if (unlikely(result_size > std::numeric_limits<size_type>::max()
                                 || count > std::numeric_limits<size_type>::max() - result_size)) {
                        piranha_throw(std::overflow_error, "overflow error");
                    }
                    if (tmp.size() != count + result_size) {
                        break;
                    }
                    // Increase cycle variables.
                    count = static_cast<size_type>(count + result_size);
                }
                integer add;
                if (it1 == v_idx1.end()) {
                    // We never found a duplicate. count is now the number of terms in s1
                    // which actually participate in the multiplication, while acc_s2 / count
                    // is the average number of terms in s2 that participate in the multiplication.
                    // The result will be then count * acc_s2 / count = acc_s2.
                    add = acc_s2;
                } else {
                    // If we found a duplicate, we use the heuristic.
                    add = integer(multiplier) * count * count;
                }
                // Fix if zero, so that the average later never results in zero.
                if (add.sign() == 0) {
                    add = 1;
                }
                acc += add;
                // Reset tmp.
                tmp._container().clear();
            }
            // Accumulate in the shared variable.
            if (n_threads == 1u) {
                // No locking needed.
                c_estimate += acc;
            } else {
                std::lock_guard<std::mutex> lock(mut);
                c_estimate += acc;
            }
        };
        // Run the estimation functor.
        if (n_threads == 1u) {
            estimator(0u);
        } else {
            future_list<void> f_list;
            try {
                for (unsigned i = 0u; i < n_threads; ++i) {
                    f_list.push_back(thread_pool::enqueue(i, estimator, i));
                }
                // First let's wait for everything to finish.
                f_list.wait_all();
                // Then, let's handle the exceptions.
                f_list.get_all();
            } catch (...) {
                f_list.wait_all();
                throw;
            }
        }
        piranha_assert(c_estimate >= n_trials);
        // Return the mean.
        return static_cast<bucket_size_type>(c_estimate / n_trials);
    }
    /// Estimate size of series multiplication (convenience overload)
    /**
     * @return the output of the other overload of estimate_final_series_size(), with a limit
     * functor whose call operator will always return the size of the second series unconditionally.
     *
     * @throws unspecified any exception thrown by the other overload of estimate_final_series_size().
     */
    template <std::size_t MultArity, typename MultFunctor>
    bucket_size_type estimate_final_series_size() const
    {
        return estimate_final_series_size<MultArity, MultFunctor>(default_limit_functor{*this});
    }
    /// A plain multiplier functor.
    /**
     * \note
     * If the key and coefficient types of \p Series do not satisfy piranha::key_is_multipliable, a compile-time error
     * will
     * be produced.
     *
     * This is a functor that conforms to the protocol expected by base_series_multiplier::blocked_multiplication() and
     * base_series_multiplier::estimate_final_series_size().
     *
     * This functor requires that the key and coefficient types of \p Series satisfy piranha::key_is_multipliable, and
     * it will
     * use the key's <tt>multiply()</tt> method to perform term-by-term multiplications.
     *
     * If the \p FastMode boolean parameter is \p true, then the call operator will insert terms into the return value
     * series
     * using the low-level interface of piranha::hash_set, otherwise the call operator will use
     * piranha::series::insert() for
     * term insertion.
     */
    template <bool FastMode>
    class plain_multiplier
    {
        using term_type = typename Series::term_type;
        using key_type = typename term_type::key_type;
        PIRANHA_TT_CHECK(key_is_multipliable, typename term_type::cf_type, key_type);
        using it_type = decltype(std::declval<Series &>()._container().end());
        static constexpr std::size_t m_arity = key_type::multiply_arity;

    public:
        /// Constructor.
        /**
         * The constructor will store references to the input arguments internally.
         *
         * @param[in] bsm a const reference to an instance of piranha::base_series_multiplier, from which
         * the vectors of term pointers will be extracted.
         * @param[in] retval the \p Series instance into which terms resulting from multiplications will be inserted.
         */
        explicit plain_multiplier(const base_series_multiplier &bsm, Series &retval)
            : m_v1(bsm.m_v1), m_v2(bsm.m_v2), m_retval(retval), m_c_end(retval._container().end())
        {
        }
        /// Deleted copy constructor.
        plain_multiplier(const plain_multiplier &) = delete;
        /// Deleted move constructor.
        plain_multiplier(plain_multiplier &&) = delete;
        /// Deleted copy assignment operator.
        plain_multiplier &operator=(const plain_multiplier &) = delete;
        /// Deleted move assignment operator.
        plain_multiplier &operator=(plain_multiplier &&) = delete;
        /// Call operator.
        /**
         * The call operator will perform the multiplication of the <tt>i</tt>-th term of the first series by the
         * <tt>j</tt>-th term of the second series, and it will insert the result into the return value.
         *
         * @param[in] i index of a term in the first series.
         * @param[in] j index of a term in the second series.
         *
         * @throws unspecified any exception thrown by:
         * - piranha::series::insert(),
         * - the low-level interface of piranha::hash_set,
         * - the in-place addition operator of the coefficient type,
         * - term construction.
         */
        void operator()(const size_type &i, const size_type &j) const
        {
            // First perform the multiplication.
            key_type::multiply(m_tmp_t, *m_v1[i], *m_v2[j], m_retval.get_symbol_set());
            for (std::size_t n = 0u; n < m_arity; ++n) {
                auto &tmp_term = m_tmp_t[n];
                if (FastMode) {
                    auto &container = m_retval._container();
                    // Try to locate the term into retval.
                    auto bucket_idx = container._bucket(tmp_term);
                    const auto it = container._find(tmp_term, bucket_idx);
                    if (it == m_c_end) {
                        container._unique_insert(term_insertion(tmp_term), bucket_idx);
                    } else {
                        it->m_cf += tmp_term.m_cf;
                    }
                } else {
                    m_retval.insert(term_insertion(tmp_term));
                }
            }
        }

    private:
        mutable std::array<term_type, m_arity> m_tmp_t;
        const std::vector<term_type const *> &m_v1;
        const std::vector<term_type const *> &m_v2;
        Series &m_retval;
        const it_type m_c_end;
    };
    /// Sanitise series.
    /**
     * When using the low-level interface of piranha::hash_set for term insertion, invariants might be violated
     * both in piranha::hash_set and piranha::series. In particular:
     *
     * - terms may not be checked for compatibility or ignorability upon insertion,
     * - the count of elements in piranha::hash_set might not be updated.
     *
     * This method can be used to fix these invariants: each term of \p retval will be checked for ignorability and
     * compatibility,
     * and the total count of terms in the series will be set to the number of non-ignorable terms. Ignorable terms will
     * be erased.
     *
     * Note that in case of exceptions \p retval will likely be left in an inconsistent state which violates internal
     * invariants.
     * Calls to this function should always be wrapped in a try/catch block that makes sure that \p retval is cleared
     * before
     * re-throwing.
     *
     * @param[in] retval the series to be sanitised.
     * @param[in] n_threads the number of threads to be used.
     *
     * @throws std::invalid_argument if \p n_threads is zero, or if one of the terms in \p retval is not compatible
     * with the symbol set of \p retval.
     * @throws std::overflow_error if the number of terms in \p retval overflows the maximum value representable by
     * base_series_multiplier::bucket_size_type.
     * @throws unspecified any exception thrown by:
     * - the cast operator of piranha::integer,
     * - standard threading primitives,
     * - thread_pool::enqueue(),
     * - future_list::push_back().
     */
    static void sanitise_series(Series &retval, unsigned n_threads)
    {
        using term_type = typename Series::term_type;
        if (unlikely(n_threads == 0u)) {
            piranha_throw(std::invalid_argument, "invalid number of threads");
        }
        auto &container = retval._container();
        const auto &args = retval.get_symbol_set();
        // Reset the size to zero before doing anything.
        container._update_size(static_cast<bucket_size_type>(0u));
        // Single-thread implementation.
        if (n_threads == 1u) {
            const auto it_end = container.end();
            for (auto it = container.begin(); it != it_end;) {
                if (unlikely(!it->is_compatible(args))) {
                    piranha_throw(std::invalid_argument, "incompatible term");
                }
                if (unlikely(container.size() == std::numeric_limits<bucket_size_type>::max())) {
                    piranha_throw(std::overflow_error, "overflow error in the number of terms of a series");
                }
                // First update the size, it will be scaled back in the erase() method if necessary.
                container._update_size(static_cast<bucket_size_type>(container.size() + 1u));
                if (unlikely(it->is_ignorable(args))) {
                    it = container.erase(it);
                } else {
                    ++it;
                }
            }
            return;
        }
        // Multi-thread implementation.
        const auto b_count = container.bucket_count();
        std::mutex m;
        integer global_count(0);
        auto eraser = [b_count, &container, &m, &args, &global_count](const bucket_size_type &start,
                                                                      const bucket_size_type &end) {
            piranha_assert(start <= end && end <= b_count);
            (void)b_count;
            bucket_size_type count = 0u;
            std::vector<term_type> term_list;
            // Examine and count the terms bucket-by-bucket.
            for (bucket_size_type i = start; i != end; ++i) {
                term_list.clear();
                const auto &bl = container._get_bucket_list(i);
                const auto it_f = bl.end();
                for (auto it = bl.begin(); it != it_f; ++it) {
                    // Check first for compatibility.
                    if (unlikely(!it->is_compatible(args))) {
                        piranha_throw(std::invalid_argument, "incompatible term");
                    }
                    // Check for ignorability.
                    if (unlikely(it->is_ignorable(args))) {
                        term_list.push_back(*it);
                    }
                    // Update the count of terms.
                    if (unlikely(count == std::numeric_limits<bucket_size_type>::max())) {
                        piranha_throw(std::overflow_error, "overflow error in the number of terms of a series");
                    }
                    count = static_cast<bucket_size_type>(count + 1u);
                }
                for (auto it = term_list.begin(); it != term_list.end(); ++it) {
                    // NOTE: must use _erase to avoid concurrent modifications
                    // to the number of elements in the table.
                    container._erase(container._find(*it, i));
                    // Account for the erased term.
                    piranha_assert(count > 0u);
                    count = static_cast<bucket_size_type>(count - 1u);
                }
            }
            // Update the global count.
            std::lock_guard<std::mutex> lock(m);
            global_count += count;
        };
        future_list<decltype(eraser(bucket_size_type(), bucket_size_type()))> f_list;
        try {
            for (unsigned i = 0u; i < n_threads; ++i) {
                const auto start = static_cast<bucket_size_type>((b_count / n_threads) * i),
                           end = static_cast<bucket_size_type>((i == n_threads - 1u) ? b_count : (b_count / n_threads)
                                                                                                     * (i + 1u));
                f_list.push_back(thread_pool::enqueue(i, eraser, start, end));
            }
            // First let's wait for everything to finish.
            f_list.wait_all();
            // Then, let's handle the exceptions.
            f_list.get_all();
        } catch (...) {
            f_list.wait_all();
            // NOTE: there's not need to clear retval here - it was already in an inconsistent
            // state coming into this method. We rather need to make sure sanitise_series() is always
            // called in a try/catch block that clears retval in case of errors.
            throw;
        }
        // Final update of the total count.
        container._update_size(static_cast<bucket_size_type>(global_count));
    }
    /// A plain series multiplication routine.
    /**
     * \note
     * If the key and coefficient types of \p Series do not satisfy piranha::key_is_multipliable, or \p LimitFunctor
     * does
     * not satisfy the requirements outlined in base_series_multiplier::blocked_multiplication(), a compile-time error
     * will
     * be produced.
     *
     * This method implements a generic series multiplication routine suitable for key types that satisfy
     * piranha::key_is_multipliable.
     * The implementation is either single-threaded or multi-threaded, depending on the sizes of the input series, and
     * it will use
     * either base_series_multiplier::plain_multiplier or a similar thread-safe multiplier for the term-by-term
     * multiplications.
     * The \p lf functor will be forwarded as limit functor to base_series_multiplier::blocked_multiplication()
     * and base_series_multiplier::estimate_final_series_size().
     *
     * Note that, in multithreaded mode, \p lf will be shared among (and called concurrently from) all the threads.
     *
     * @param[in] lf the limit functor (see base_series_multiplier::blocked_multiplication()).
     *
     * @return the series resulting from the multiplication of the two series used to construct \p this.
     *
     * @throws unspecified any exception thrown by:
     * - piranha::safe_cast(),
     * - base_series_multiplier::estimate_final_series_size(),
     * - <tt>boost::numeric_cast()</tt>,
     * - the public interface of piranha::hash_set,
     * - base_series_multiplier::blocked_multiplication(),
     * - base_series_multiplier::sanitise_series(),
     * - the <tt>multiply()</tt> method of the key type of \p Series,
     * - thread_pool::enqueue(),
     * - future_list::push_back(),
     * - the construction of terms,
     * - in-place addition of coefficients.
     */
    template <typename LimitFunctor>
    Series plain_multiplication(const LimitFunctor &lf) const
    {
        // Shortcuts.
        using term_type = typename Series::term_type;
        using cf_type = typename term_type::cf_type;
        using key_type = typename term_type::key_type;
        PIRANHA_TT_CHECK(key_is_multipliable, cf_type, key_type);
        constexpr std::size_t m_arity = key_type::multiply_arity;
        // Setup the return value with the merged symbol set.
        Series retval;
        retval.set_symbol_set(m_ss);
        // Do not do anything if one of the two series is empty.
        if (unlikely(m_v1.empty() || m_v2.empty())) {
            return retval;
        }
        const size_type size1 = m_v1.size(), size2 = m_v2.size();
        (void)size2;
        piranha_assert(size1 && size2);
        // Convert n_threads to size_type for convenience.
        const size_type n_threads = safe_cast<size_type>(m_n_threads);
        piranha_assert(n_threads);
        // Determine if we should estimate the size. We check the threshold, but we always
        // need to estimate in multithreaded mode.
        bool estimate = true;
        const auto e_thr = tuning::get_estimate_threshold();
        if (integer(m_v1.size()) * m_v2.size() < integer(e_thr) * e_thr && n_threads == 1u) {
            estimate = false;
        }
        if (estimate) {
            // Estimate and rehash.
            const auto est = estimate_final_series_size<m_arity, plain_multiplier<false>>(lf);
            // NOTE: use numeric cast here as safe_cast is expensive, going through an integer-double conversion,
            // and in this case the behaviour of numeric_cast is appropriate.
            const auto n_buckets = boost::numeric_cast<bucket_size_type>(
                std::ceil(static_cast<double>(est) / retval._container().max_load_factor()));
            piranha_assert(n_buckets > 0u);
            // Check if we want to use the parallel memory set.
            // NOTE: it is important here that we use the same n_threads for multiplication and memset as
            // we tie together pinned threads with potentially different NUMA regions.
            const unsigned n_threads_rehash = tuning::get_parallel_memory_set() ? static_cast<unsigned>(n_threads) : 1u;
            retval._container().rehash(n_buckets, n_threads_rehash);
        }
        if (n_threads == 1u) {
            try {
                // Single-thread case.
                if (estimate) {
                    blocked_multiplication(plain_multiplier<true>(*this, retval), 0u, size1, lf);
                    // If we estimated beforehand, we need to sanitise the series.
                    sanitise_series(retval, static_cast<unsigned>(n_threads));
                } else {
                    blocked_multiplication(plain_multiplier<false>(*this, retval), 0u, size1, lf);
                }
                finalise_series(retval);
                return retval;
            } catch (...) {
                retval._container().clear();
                throw;
            }
        }
        // Multi-threaded case.
        piranha_assert(estimate);
        // Init the vector of spinlocks.
        detail::atomic_flag_array sl_array(safe_cast<std::size_t>(retval._container().bucket_count()));
        // Init the future list.
        future_list<void> f_list;
        // Thread block size.
        const auto block_size = size1 / n_threads;
        try {
            for (size_type idx = 0u; idx < n_threads; ++idx) {
                // Thread functor.
                auto tf = [idx, this, block_size, n_threads, &sl_array, &retval, &lf]() {
                    // Used to store the result of term multiplication.
                    std::array<term_type, key_type::multiply_arity> tmp_t;
                    // End of retval container (thread-safe).
                    const auto c_end = retval._container().end();
                    // Block functor.
                    // NOTE: this is very similar to the plain functor, but it does the bucket locking
                    // additionally.
                    auto f = [&c_end, &tmp_t, this, &retval, &sl_array](const size_type &i, const size_type &j) {
                        // Run the term multiplication.
                        key_type::multiply(tmp_t, *(this->m_v1[i]), *(this->m_v2[j]), retval.get_symbol_set());
                        for (std::size_t n = 0u; n < key_type::multiply_arity; ++n) {
                            auto &container = retval._container();
                            auto &tmp_term = tmp_t[n];
                            // Try to locate the term into retval.
                            auto bucket_idx = container._bucket(tmp_term);
                            // Lock the bucket.
                            detail::atomic_lock_guard alg(sl_array[static_cast<std::size_t>(bucket_idx)]);
                            const auto it = container._find(tmp_term, bucket_idx);
                            if (it == c_end) {
                                container._unique_insert(term_insertion(tmp_term), bucket_idx);
                            } else {
                                it->m_cf += tmp_term.m_cf;
                            }
                        }
                    };
                    // Thread block limit.
                    const auto e1
                        = (idx == n_threads - 1u) ? this->m_v1.size() : static_cast<size_type>((idx + 1u) * block_size);
                    this->blocked_multiplication(f, static_cast<size_type>(idx * block_size), e1, lf);
                };
                f_list.push_back(thread_pool::enqueue(static_cast<unsigned>(idx), tf));
            }
            f_list.wait_all();
            f_list.get_all();
            sanitise_series(retval, static_cast<unsigned>(n_threads));
            finalise_series(retval);
        } catch (...) {
            f_list.wait_all();
            // Clean up retval as it might be in an inconsistent state.
            retval._container().clear();
            throw;
        }
        return retval;
    }
    /// A plain series multiplication routine (convenience overload).
    /**
     * @return the output of the other overload of plain_multiplication(), with a limit
     * functor whose call operator will always return the size of the second series unconditionally.
     *
     * @throws unspecified any exception thrown by the other overload of plain_multiplication().
     */
    Series plain_multiplication() const
    {
        return plain_multiplication(default_limit_functor{*this});
    }
    /// Finalise series.
    /**
     * This method will finalise the output \p s of a series multiplication undertaken via
     * piranha::base_series_multiplier.
     * Currently, this method will not do anything unless the coefficient type of \p Series is an instance of
     * piranha::mp_rational.
     * In this case, the coefficients of \p s will be normalised with respect to the least common multiplier computed in
     * the
     * constructor of piranha::base_series_multiplier.
     *
     * @param[in,out] s the \p Series to be finalised.
     *
     * @throws unspecified any exception thrown by:
     * - thread_pool::enqueue(),
     * - future_list::push_back().
     */
    void finalise_series(Series &s) const
    {
        finalise_impl(s);
    }

protected:
    /// Vector of const pointers to the terms in the larger series.
    mutable v_ptr m_v1;
    /// Vector of const pointers to the terms in the smaller series.
    mutable v_ptr m_v2;
    /// The symbol set of the series used during construction.
    const symbol_set m_ss;
    /// Number of threads.
    /**
     * This value will be set by the constructor, and it represents the number of threads
     * that will be used by the multiplier. The value is always at least 1 and it is calculated
     * via thread_pool::use_threads().
     */
    const unsigned m_n_threads;
};
}

#endif

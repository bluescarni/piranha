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

#include "../src/base_series_multiplier.hpp"

#define BOOST_TEST_MODULE base_series_multiplier_test
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include "../src/environment.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/polynomial.hpp"
#include "../src/settings.hpp"
#include "../src/symbol.hpp"
#include "../src/symbol_set.hpp"

using namespace piranha;

template <typename Cf>
using p_type = polynomial<Cf,monomial<int>>;

template <typename Series>
struct m_checker: public base_series_multiplier<Series>
{
	using base = base_series_multiplier<Series>;
	using size_type = typename base::size_type;
	explicit m_checker(const Series &s1, const Series &s2):base(s1,s2)
	{
		term_pointers_checker(s1,s2);
	}
	template <typename T, typename std::enable_if<std::is_same<typename T::term_type::cf_type,integer>::value,int>::type = 0>
	void term_pointers_checker(const T &s1_, const T &s2_) const
	{
		// Swap the operands if needed.
		const T &s1 = s1_.size() < s2_.size() ? s2_ : s1_;
		const T &s2 = s1_.size() < s2_.size() ? s1_ : s2_;
		BOOST_CHECK(s1.size() == this->m_v1.size());
		BOOST_CHECK(s2.size() == this->m_v2.size());
		// Create hash sets with the term pointers from the vectors.
		std::unordered_set<const typename T::term_type *> h1, h2;
		std::copy(this->m_v1.begin(),this->m_v1.end(),std::inserter(h1,h1.begin()));
		std::copy(this->m_v2.begin(),this->m_v2.end(),std::inserter(h2,h2.begin()));
		for (const auto &t: s1._container()) {
			BOOST_CHECK(h1.find(&t) != h1.end());
		}
		for (const auto &t: s2._container()) {
			BOOST_CHECK(h2.find(&t) != h2.end());
		}
	}
	template <typename T, typename std::enable_if<std::is_same<typename T::term_type::cf_type,rational>::value,int>::type = 0>
	void term_pointers_checker(const T &s1_, const T &s2_) const
	{
		const T &s1 = s1_.size() < s2_.size() ? s2_ : s1_;
		const T &s2 = s1_.size() < s2_.size() ? s1_ : s2_;
		BOOST_CHECK(s1.size() == this->m_v1.size());
		BOOST_CHECK(s2.size() == this->m_v2.size());
		std::unordered_set<const typename T::term_type *> h1, h2;
		std::transform(s1._container().begin(),s1._container().end(),std::inserter(h1,h1.begin()),[](const typename T::term_type &t){return &t;});
		std::transform(s2._container().begin(),s2._container().end(),std::inserter(h2,h2.begin()),[](const typename T::term_type &t){return &t;});
		for (size_type i = 0u; i != s1.size(); ++i) {
			BOOST_CHECK(h1.find(this->m_v1[i]) == h1.end());
			BOOST_CHECK(this->m_v1[i]->m_cf.den() == 1);
			auto it = s1._container().find(*this->m_v1[i]);
			BOOST_CHECK(it != s1._container().end());
			BOOST_CHECK(this->m_v1[i]->m_cf.num() % it->m_cf.num() == 0);
		}
		for (size_type i = 0u; i != s2.size(); ++i) {
			BOOST_CHECK(h2.find(this->m_v2[i]) == h2.end());
			BOOST_CHECK(this->m_v2[i]->m_cf.den() == 1);
			auto it = s2._container().find(*this->m_v2[i]);
			BOOST_CHECK(it != s2._container().end());
			BOOST_CHECK(this->m_v2[i]->m_cf.num() % it->m_cf.num() == 0);
		}
	}
	// Perfect forwarding of protected members, to make them accessible.
	template <typename ... Args>
	void blocked_multiplication(Args && ... args) const
	{
		base::blocked_multiplication(std::forward<Args>(args)...);
	}
	template <std::size_t N, typename MultFunctor, typename ... Args>
	typename Series::size_type estimate_final_series_size(Args && ... args) const
	{
		return base::template estimate_final_series_size<N,MultFunctor>(std::forward<Args>(args)...);
	}
	template <typename ... Args>
	static void sanitise_series(Args && ... args)
	{
		return base::sanitise_series(std::forward<Args>(args)...);
	}
	template <typename ... Args>
	Series plain_multiplication(Args && ... args) const
	{
		return base::plain_multiplication(std::forward<Args>(args)...);
	}
	template <typename ... Args>
	void finalise_series(Args && ... args) const
	{
		base::finalise_series(std::forward<Args>(args)...);
	}
	unsigned get_n_threads() const
	{
		return this->m_n_threads;
	}
};

BOOST_AUTO_TEST_CASE(base_series_multiplier_finalise_test)
{
	environment env;
	{
	// Test proper handling of rational coefficients.
	using pt = p_type<rational>;
	pt x{"x"}, y{"y"};
	BOOST_CHECK_EQUAL(x*4/3_q*y*5/2_q,10/3_q*x*y);
	BOOST_CHECK_EQUAL((x*4/3_q+y*5/2_q)*(x.pow(2)*4/13_q-y*5/17_q),
		16*x.pow(3)/39+10/13_q*y*x*x-20*x*y/51-25*y*y/34);
	// No finalisation happening with integral coefficients.
	using pt2 = p_type<integer>;
	pt2 x2{"x"}, y2{"y"};
	BOOST_CHECK_EQUAL(x2*y2,y2*x2);
	}
	{
	// Check with multiple threads.
	settings::set_min_work_per_thread(1u);
	using pt = p_type<rational>;
	using mt = m_checker<pt>;
	for (unsigned nt = 1u; nt <= 4u; ++nt) {
		settings::set_n_threads(nt);
		// Setup a multiplier for a polyomial with two variables and lcm 6.
		auto tmp1 = pt{"x"}/3 + pt{"y"}, tmp2 = pt{"y"}/2 + pt{"x"};
		mt m0{tmp1,tmp2};
		// First let's try with an empty retval.
		pt r;
		r.set_symbol_set(symbol_set({symbol{"x"},symbol{"y"}}));
		BOOST_CHECK_NO_THROW(m0.finalise_series(r));
		BOOST_CHECK_EQUAL(r,0);
		// Put in one term.
		r += pt{"x"};
		BOOST_CHECK_NO_THROW(m0.finalise_series(r));
		BOOST_CHECK_EQUAL(r,pt{"x"}/36);
		// Put in another term.
		r += 12*pt{"y"};
		BOOST_CHECK_NO_THROW(m0.finalise_series(r));
		BOOST_CHECK_EQUAL(r,pt{"x"}/36 + pt{"y"}/3);
	}
	}
	{
	// Same as above, but with k monomial.
	using pt = polynomial<rational,k_monomial>;
	using mt = m_checker<pt>;
	for (unsigned nt = 1u; nt <= 4u; ++nt) {
		settings::set_n_threads(nt);
		// Setup a multiplier for a polyomial with two variables and lcm 6.
		auto tmp1 = pt{"x"}/3 + pt{"y"}, tmp2 = pt{"y"}/2 + pt{"x"};
		mt m0{tmp1,tmp2};
		// First let's try with an empty retval.
		pt r;
		r.set_symbol_set(symbol_set({symbol{"x"},symbol{"y"}}));
		BOOST_CHECK_NO_THROW(m0.finalise_series(r));
		BOOST_CHECK_EQUAL(r,0);
		// Put in one term.
		r += pt{"x"};
		BOOST_CHECK_NO_THROW(m0.finalise_series(r));
		BOOST_CHECK_EQUAL(r,pt{"x"}/36);
		// Put in another term.
		r += 12*pt{"y"};
		BOOST_CHECK_NO_THROW(m0.finalise_series(r));
		BOOST_CHECK_EQUAL(r,pt{"x"}/36 + pt{"y"}/3);
	}
	}
	// Reset.
	settings::reset_n_threads();
	settings::reset_min_work_per_thread();
}

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

#include "../src/base_series_multiplier.hpp"

#define BOOST_TEST_MODULE base_series_multiplier_test
#include <boost/test/unit_test.hpp>

#include <set>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "../src/environment.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/polynomial.hpp"
#include "../src/tuning.hpp"

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
		auto it = s1._container().begin();
		for (size_type i = 0u; i != s1.size(); ++i, ++it) {
			BOOST_CHECK(this->m_v1[i] == &*it);
		}
		it = s2._container().begin();
		for (size_type i = 0u; i != s2.size(); ++i, ++it) {
			BOOST_CHECK(this->m_v2[i] == &*it);
		}
	}
	template <typename T, typename std::enable_if<std::is_same<typename T::term_type::cf_type,rational>::value,int>::type = 0>
	void term_pointers_checker(const T &s1_, const T &s2_) const
	{
		const T &s1 = s1_.size() < s2_.size() ? s2_ : s1_;
		const T &s2 = s1_.size() < s2_.size() ? s1_ : s2_;
		BOOST_CHECK(s1.size() == this->m_v1.size());
		BOOST_CHECK(s2.size() == this->m_v2.size());
		auto it = s1._container().begin();
		for (size_type i = 0u; i != s1.size(); ++i, ++it) {
			BOOST_CHECK(this->m_v1[i] != &*it);
			BOOST_CHECK(this->m_v1[i]->m_cf.den() == 1);
			BOOST_CHECK(this->m_v1[i]->m_cf.num() % it->m_cf.num() == 0);
		}
		it = s2._container().begin();
		for (size_type i = 0u; i != s2.size(); ++i, ++it) {
			BOOST_CHECK(this->m_v2[i] != &*it);
			BOOST_CHECK(this->m_v2[i]->m_cf.den() == 1);
			BOOST_CHECK(this->m_v2[i]->m_cf.num() % it->m_cf.num() == 0);
		}
	}
	template <typename ... Args>
	void blocked_multiplication(Args && ... args) const
	{
		base::blocked_multiplication(std::forward<Args>(args)...);
	}
};

BOOST_AUTO_TEST_CASE(base_series_multiplier_constructor_test)
{
	environment env;
	{
	using pt = p_type<rational>;
	pt x{"x"}, y{"y"}, z{"z"};
	auto s1 = (x/2+y/5).pow(5), s2 = (x/3+y/22).pow(6);
	m_checker<pt> m0(s1,s2);
	std::swap(s1,s2);
	m_checker<pt> m1(s1,s2);
	s1 = 0;
	s2 = 0;
	m_checker<pt> m2(s1,s2);
	BOOST_CHECK_THROW(m_checker<pt>(x,z),std::invalid_argument);
	}
	{
	using pt = p_type<integer>;
	pt x{"x"}, y{"y"}, z{"z"};
	auto s1 = (x+y*2).pow(5), s2 = (-x+y).pow(6);
	m_checker<pt> m0(s1,s2);
	std::swap(s1,s2);
	m_checker<pt> m1(s1,s2);
	s1 = 0;
	s2 = 0;
	m_checker<pt> m2(s1,s2);
	BOOST_CHECK_THROW(m_checker<pt>(x,z),std::invalid_argument);
	}
}

struct p_sorter
{
	bool operator()(const std::pair<unsigned,unsigned> &p1, const std::pair<unsigned,unsigned> &p2) const
	{
		if (p1.first < p2.first) {
			return true;
		}
		if (p1.first == p2.first) {
			return p1.second < p2.second;
		}
		return false;
	}
};

struct m_functor_0
{
	template <typename T>
	void operator()(const T &i, const T &j) const
	{
		m_set.emplace(unsigned(i),unsigned(j));
	}
	mutable std::set<std::pair<unsigned,unsigned>,p_sorter> m_set;
};

BOOST_AUTO_TEST_CASE(base_series_multiplier_blocked_multiplication_test)
{
	using pt = p_type<rational>;
	pt x{"x"}, y{"y"};
	auto s1 = (x + y).pow(100);
	// Take out one term in order to make it exactly 100 terms.
	s1 -= 1345860629046814650_z*x.pow(16)*y.pow(84);
	m_checker<pt> m0(s1,s1);
	tuning::set_multiplication_block_size(16u);
	m_functor_0 mf0;
	m0.blocked_multiplication(mf0,0u,100u,0u,100u);
	BOOST_CHECK(mf0.m_set.size() == 100u * 100u);
	for (unsigned i = 0u; i < 100u; ++i) {
		for (unsigned j = 0u; j < 100u; ++j) {
			BOOST_CHECK(mf0.m_set.count(std::make_pair(i,j)) == 1u);
		}
	}
	// Try with commensurable block size.
	tuning::set_multiplication_block_size(25u);
	mf0.m_set.clear();
	m0.blocked_multiplication(mf0,0u,100u,0u,100u);
	BOOST_CHECK(mf0.m_set.size() == 100u * 100u);
	for (unsigned i = 0u; i < 100u; ++i) {
		for (unsigned j = 0u; j < 100u; ++j) {
			BOOST_CHECK(mf0.m_set.count(std::make_pair(i,j)) == 1u);
		}
	}
	// Block size same as series size.
	tuning::set_multiplication_block_size(100u);
	mf0.m_set.clear();
	m0.blocked_multiplication(mf0,0u,100u,0u,100u);
	BOOST_CHECK(mf0.m_set.size() == 100u * 100u);
	for (unsigned i = 0u; i < 100u; ++i) {
		for (unsigned j = 0u; j < 100u; ++j) {
			BOOST_CHECK(mf0.m_set.count(std::make_pair(i,j)) == 1u);
		}
	}
	// Larger size than series size.
	tuning::set_multiplication_block_size(200u);
	mf0.m_set.clear();
	m0.blocked_multiplication(mf0,0u,100u,0u,100u);
	BOOST_CHECK(mf0.m_set.size() == 100u * 100u);
	for (unsigned i = 0u; i < 100u; ++i) {
		for (unsigned j = 0u; j < 100u; ++j) {
			BOOST_CHECK(mf0.m_set.count(std::make_pair(i,j)) == 1u);
		}
	}
	// Only parts of the series.
	tuning::set_multiplication_block_size(23u);
	mf0.m_set.clear();
	m0.blocked_multiplication(mf0,20u,87u,1u,89u);
	BOOST_CHECK(mf0.m_set.size() == (87u - 20u) * (89u - 1u));
	for (unsigned i = 20u; i < 87u; ++i) {
		for (unsigned j = 1u; j < 89u; ++j) {
			BOOST_CHECK(mf0.m_set.count(std::make_pair(i,j)) == 1u);
		}
	}
}

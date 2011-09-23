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

#include "../src/series.hpp"

#define BOOST_TEST_MODULE series_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <stdexcept>
#include <type_traits>

#include "../src/debug_access.hpp"
#include "../src/integer.hpp"
#include "../src/polynomial_term.hpp"
#include "../src/symbol.hpp"
#include "../src/symbol_set.hpp"

using namespace piranha;

typedef boost::mpl::vector<double,integer> cf_types;
typedef boost::mpl::vector<unsigned,integer> expo_types;

template <typename Cf, typename Expo>
class g_series_type: public series<polynomial_term<Cf,Expo>,g_series_type<Cf,Expo>>
{
	public:
		typedef series<polynomial_term<Cf,Expo>,g_series_type<Cf,Expo>> base;
		g_series_type() = default;
		g_series_type(const g_series_type &) = default;
		g_series_type(g_series_type &&) = default;
		g_series_type &operator=(const g_series_type &) = default;
		g_series_type &operator=(g_series_type &&other)
		{
			if (this != &other) {
				base::operator=(std::move(other));
			}
			return *this;
		}
		template <typename T, typename... Args, typename std::enable_if<sizeof...(Args) || !std::is_same<g_series_type,typename std::decay<T>::type>::value>::type*& = enabler>
		explicit g_series_type(T &&arg1, Args && ... argn) : base(std::forward<T>(arg1),std::forward<Args>(argn)...) {}
};

struct construction_tag {};

namespace piranha
{

template <>
struct debug_access<construction_tag>
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef polynomial_term<Cf,Expo> term_type;
			typedef typename term_type::key_type key_type;
			typedef g_series_type<Cf,Expo> series_type;
			symbol_set ed;
			ed.add(symbol("x"));
			// Default constructor.
			BOOST_CHECK(series_type().empty());
			BOOST_CHECK_EQUAL(series_type().size(), (unsigned)0);
			BOOST_CHECK_EQUAL(series_type().get_symbol_set().size(),0u);
			// Copy constructor.
			series_type s;
			s.m_symbol_set = ed;
			s.insert(term_type(Cf(1),key_type{Expo(1)}));
			series_type t(s);
			BOOST_CHECK(*s.m_container.begin() == *t.m_container.begin());
			BOOST_CHECK(s.m_container.begin()->m_cf == t.m_container.begin()->m_cf);
			BOOST_CHECK(s.get_symbol_set() == t.get_symbol_set());
			// Move constructor.
			series_type u{series_type(s)};
			BOOST_CHECK(*u.m_container.begin() == *t.m_container.begin());
			BOOST_CHECK(u.m_container.begin()->m_cf == t.m_container.begin()->m_cf);
			BOOST_CHECK(u.get_symbol_set() == t.get_symbol_set());
			auto s2 = s;
			series_type u2(std::move(s2));
			BOOST_CHECK(s2.empty());
			BOOST_CHECK(s2.get_symbol_set().size() == 0u);
			// Copy assignment.
			u = t;
			BOOST_CHECK(*u.m_container.begin() == *t.m_container.begin());
			BOOST_CHECK(u.m_container.begin()->m_cf == t.m_container.begin()->m_cf);
			BOOST_CHECK(u.get_symbol_set() == t.get_symbol_set());
			// Move assignment.
			u = std::move(t);
			BOOST_CHECK(*u.m_container.begin() == *s.m_container.begin());
			BOOST_CHECK(u.m_container.begin()->m_cf == s.m_container.begin()->m_cf);
			BOOST_CHECK(u.get_symbol_set() == s.get_symbol_set());
			BOOST_CHECK(t.empty());
			BOOST_CHECK_EQUAL(t.get_symbol_set().size(),0u);
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};
}

typedef debug_access<construction_tag> constructor_tester;

BOOST_AUTO_TEST_CASE(series_constructor_test)
{
	boost::mpl::for_each<cf_types>(constructor_tester());
}

struct insertion_tag {};

namespace piranha
{
template <>
class debug_access<insertion_tag>
{
	public:
		template <typename Cf>
		struct runner
		{
			template <typename Expo>
			void operator()(const Expo &)
			{
				typedef polynomial_term<Cf,Expo> term_type;
				typedef typename term_type::key_type key_type;
				typedef g_series_type<Cf,Expo> series_type;
				symbol_set ed;
				ed.add(symbol("x"));
				// Insert well-behaved term.
				series_type s;
				s.m_symbol_set = ed;
				s.insert(term_type(Cf(1),key_type{Expo(1)}));
				BOOST_CHECK(!s.empty());
				BOOST_CHECK_EQUAL(s.size(),unsigned(1));
				// Insert incompatible term.
				BOOST_CHECK_THROW(s.insert(term_type(Cf(1),key_type())),std::invalid_argument);
				BOOST_CHECK_EQUAL(s.size(),unsigned(1));
				// Insert ignorable term.
				s.insert(term_type(Cf(0),key_type{Expo(1)}));
				BOOST_CHECK_EQUAL(s.size(),unsigned(1));
				// Insert another new term.
				s.insert(term_type(Cf(1),key_type{Expo(2)}));
				BOOST_CHECK_EQUAL(s.size(),unsigned(2));
				// Insert equivalent terms.
				s.insert(term_type(Cf(2),key_type{Expo(2)}));
				BOOST_CHECK_EQUAL(s.size(),unsigned(2));
				s.template insert<false>(term_type(Cf(-2),key_type{Expo(2)}));
				BOOST_CHECK_EQUAL(s.size(),unsigned(2));
				// Insert terms that will prompt for erase of a term.
				s.insert(term_type(Cf(-2),key_type{Expo(2)}));
				s.insert(term_type(Cf(-2),key_type{Expo(2)}));
				s.insert(term_type(Cf(-1),key_type{Expo(2)}));
				BOOST_CHECK_EQUAL(s.size(),unsigned(1));
				// Insertion of different term type.
				typedef polynomial_term<float,Expo> other_term_type;
				s.insert(other_term_type(float(1),key_type{Expo(1)}));
				BOOST_CHECK_EQUAL(s.size(),unsigned(1));
				BOOST_CHECK_EQUAL(s.m_container.begin()->m_cf,float(1) + float(1));
			}
		};
		template <typename Cf>
		void operator()(const Cf &)
		{
			boost::mpl::for_each<expo_types>(runner<Cf>());
		}
};
}

typedef debug_access<insertion_tag> insertion_tester;

BOOST_AUTO_TEST_CASE(series_insertion_test)
{
	boost::mpl::for_each<cf_types>(insertion_tester());
}

struct merge_terms_tag {};

namespace piranha
{
template <>
class debug_access<merge_terms_tag>
{
	public:
		template <typename Cf>
		struct runner
		{
			template <typename Expo>
			void operator()(const Expo &)
			{
				typedef polynomial_term<Cf,Expo> term_type;
				typedef Cf value_type;
				typedef typename term_type::key_type key_type;
				typedef g_series_type<Cf,Expo> series_type;
				symbol_set ed;
				ed.add(symbol("x"));
				series_type s1, s2;
				s1.m_symbol_set = ed;
				s2.m_symbol_set = ed;
				s1.insert(term_type(Cf(1),key_type{Expo(1)}));
				s2.insert(term_type(Cf(2),key_type{Expo(2)}));
				// Merge with copy.
				s1.template merge_terms<true>(s2);
				BOOST_CHECK_EQUAL(s1.size(),unsigned(2));
				auto it = s1.m_container.begin();
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
				++it;
				BOOST_CHECK(it->m_cf == 1 || it->m_cf == 2);
				// Merge with move.
				series_type s3;
				s3.m_symbol_set = ed;
				s3.insert(term_type(Cf(3),key_type{Expo(3)}));
				s1.template merge_terms<true>(std::move(s3));
				BOOST_CHECK(s3.empty());
				BOOST_CHECK_EQUAL(s1.size(),unsigned(3));
				// Merge with move + swap.
				series_type s1_copy = s1;
				s3.insert(term_type(Cf(4),key_type{Expo(4)}));
				s3.insert(term_type(Cf(5),key_type{Expo(5)}));
				s3.insert(term_type(Cf(6),key_type{Expo(6)}));
				s3.insert(term_type(Cf(7),key_type{Expo(7)}));
				s1_copy.template merge_terms<true>(std::move(s3));
				BOOST_CHECK_EQUAL(s1_copy.size(),unsigned(7));
				BOOST_CHECK(s3.empty());
				// Negative merge with move + swap.
				s1_copy = s1;
				s3.insert(term_type(Cf(4),key_type{Expo(4)}));
				s3.insert(term_type(Cf(5),key_type{Expo(5)}));
				s3.insert(term_type(Cf(6),key_type{Expo(6)}));
				s3.insert(term_type(Cf(7),key_type{Expo(7)}));
				s1_copy.template merge_terms<false>(std::move(s3));
				BOOST_CHECK_EQUAL(s1_copy.size(),unsigned(7));
				it = s1_copy.m_container.begin();
				auto check_neg_merge = [&it]() -> void {
					BOOST_CHECK(it->m_cf == value_type(1) || it->m_cf == value_type(2) || it->m_cf == value_type(3) ||
						it->m_cf == value_type(-4) || it->m_cf == value_type(-5) ||
						it->m_cf == value_type(-6) || it->m_cf == value_type(-7));
				};
				check_neg_merge();
				++it;
				check_neg_merge();
				++it;
				check_neg_merge();
				++it;
				check_neg_merge();
				++it;
				check_neg_merge();
				++it;
				check_neg_merge();
				++it;
				check_neg_merge();
				// Merge with self.
				s1.template merge_terms<true>(s1);
				BOOST_CHECK_EQUAL(s1.size(),unsigned(3));
				it = s1.m_container.begin();
				BOOST_CHECK(it->m_cf == value_type(1) + value_type(1) ||
					it->m_cf == value_type(2) + value_type(2) || it->m_cf == value_type(3) + value_type(3));
				++it;
				BOOST_CHECK(it->m_cf == value_type(1) + value_type(1) ||
					it->m_cf == value_type(2) + value_type(2) || it->m_cf == value_type(3) + value_type(3));
				++it;
				BOOST_CHECK(it->m_cf == value_type(1) + value_type(1) ||
					it->m_cf == value_type(2) + value_type(2) || it->m_cf == value_type(3) + value_type(3));
				// Merge with self + move.
				s1.template merge_terms<true>(std::move(s1));
				BOOST_CHECK_EQUAL(s1.size(),unsigned(3));
				it = s1.m_container.begin();
				BOOST_CHECK(it->m_cf == value_type(1) + value_type(1) + value_type(1) + value_type(1) ||
					it->m_cf == value_type(2) + value_type(2) + value_type(2) + value_type(2) ||
					it->m_cf == value_type(3) + value_type(3) + value_type(3) + value_type(3));
				++it;
				BOOST_CHECK(it->m_cf == value_type(1) + value_type(1) + value_type(1) + value_type(1) ||
					it->m_cf == value_type(2) + value_type(2) + value_type(2) + value_type(2) ||
					it->m_cf == value_type(3) + value_type(3) + value_type(3) + value_type(3));
				++it;
				BOOST_CHECK(it->m_cf == value_type(1) + value_type(1) + value_type(1) + value_type(1) ||
					it->m_cf == value_type(2) + value_type(2) + value_type(2) + value_type(2) ||
					it->m_cf == value_type(3) + value_type(3) + value_type(3) + value_type(3));
				// Merge with different series type.
				s1.m_container.clear();
				s1.insert(term_type(Cf(1),key_type{Expo(1)}));
				typedef polynomial_term<long,Expo> term_type2;
				typedef typename term_type2::key_type key_type2;
				typedef g_series_type<long,Expo> series_type2;
				symbol_set ed2;
				ed2.add(symbol("x"));
				series_type2 s4;
				s4.m_symbol_set = ed2;
				s4.insert(term_type2(long(1),key_type2{Expo(1)}));
				s1.template merge_terms<true>(s4);
				BOOST_CHECK_EQUAL(s1.size(), unsigned(1));
				it = s1.m_container.begin();
				value_type tmp(1);
				tmp += long(1);
				BOOST_CHECK(it->m_cf == tmp);
				s4.m_container.clear();
				s4.insert(term_type2(long(1),key_type2{Expo(2)}));
				s1.template merge_terms<true>(s4);
				it = s1.m_container.begin();
				BOOST_CHECK_EQUAL(s1.size(), unsigned(2));
				BOOST_CHECK(it->m_cf == tmp || it->m_cf == long(1));
				++it;
				BOOST_CHECK(it->m_cf == tmp || it->m_cf == long(1));
			}
		};
		template <typename Cf>
		void operator()(const Cf &)
		{
			boost::mpl::for_each<expo_types>(runner<Cf>());
		}
};
}

typedef debug_access<merge_terms_tag> merge_terms_tester;

BOOST_AUTO_TEST_CASE(series_merge_terms_test)
{
	boost::mpl::for_each<cf_types>(merge_terms_tester());
}

struct merge_args_tag {};

namespace piranha
{
template <>
class debug_access<merge_args_tag>
{
	public:
		template <typename Cf>
		struct runner
		{
			template <typename Expo>
			void operator()(const Expo &)
			{
				typedef polynomial_term<Cf,Expo> term_type;
				typedef Cf value_type;
				typedef typename term_type::key_type key_type;
				typedef g_series_type<Cf,Expo> series_type;
				series_type s_derived;
				typename series_type::base &s = static_cast<typename series_type::base &>(s_derived);
				symbol_set ed1, ed2;
				s.insert(term_type(Cf(1),key_type()));
				ed2.add(symbol("x"));
				auto merge_out = s.merge_args(ed2);
				BOOST_CHECK_EQUAL(merge_out.size(),unsigned(1));
				BOOST_CHECK(merge_out.m_container.find(term_type(Cf(1),key_type{Expo(0)})) != merge_out.m_container.end());
				auto compat_check = [](const typename series_type::base &series) -> void {
					for (auto it = series.m_container.begin(); it != series.m_container.end(); ++it) {
						BOOST_CHECK(it->is_compatible(series.m_symbol_set));
					}
				};
				compat_check(merge_out);
				s = std::move(merge_out);
				s.insert(term_type(Cf(1),key_type{Expo(1)}));
				s.insert(term_type(Cf(2),key_type{Expo(2)}));
				ed1 = ed2;
				ed2.add(symbol("y"));
				merge_out = s.merge_args(ed2);
				BOOST_CHECK_EQUAL(merge_out.size(),unsigned(3));
				BOOST_CHECK(merge_out.m_container.find(term_type(Cf(1),key_type{Expo(0),Expo(0)})) != merge_out.m_container.end());
				BOOST_CHECK(merge_out.m_container.find(term_type(Cf(1),key_type{Expo(1),Expo(0)})) != merge_out.m_container.end());
				BOOST_CHECK(merge_out.m_container.find(term_type(Cf(2),key_type{Expo(2),Expo(0)})) != merge_out.m_container.end());
				compat_check(merge_out);
			}
		};
		template <typename Cf>
		void operator()(const Cf &)
		{
			boost::mpl::for_each<expo_types>(runner<Cf>());
		}
};
}

typedef debug_access<merge_args_tag> merge_args_tester;

BOOST_AUTO_TEST_CASE(series_merge_args_test)
{
	boost::mpl::for_each<cf_types>(merge_args_tester());
}

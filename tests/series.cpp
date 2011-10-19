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
		explicit g_series_type(const char *name):base()
		{
			typedef typename base::term_type term_type;
			// Insert the symbol.
			this->m_symbol_set.add(name);
			// Construct and insert the term.
			this->insert(term_type(Cf(1),typename term_type::key_type{Expo(1)}));
		}
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
		template <typename T>
		typename std::enable_if<!std::is_same<g_series_type,typename std::decay<T>::type>::value,g_series_type &>::type operator=(T &&x)
		{
			base::operator=(std::forward<T>(x));
			return *this;
		}
};

template <typename Cf, typename Expo>
class g_series_type2: public series<polynomial_term<Cf,Expo>,g_series_type2<Cf,Expo>>
{
	public:
		typedef series<polynomial_term<Cf,Expo>,g_series_type2<Cf,Expo>> base;
		g_series_type2() = default;
		g_series_type2(const g_series_type2 &) = default;
		g_series_type2(g_series_type2 &&) = default;
		explicit g_series_type2(const char *name):base()
		{
			typedef typename base::term_type term_type;
			// Insert the symbol.
			this->m_symbol_set.add(name);
			// Construct and insert the term.
			this->insert(term_type(Cf(1),typename term_type::key_type{Expo(1)}));
		}
		g_series_type2 &operator=(const g_series_type2 &) = default;
		g_series_type2 &operator=(g_series_type2 &&other)
		{
			if (this != &other) {
				base::operator=(std::move(other));
			}
			return *this;
		}
		template <typename T, typename... Args, typename std::enable_if<sizeof...(Args) || !std::is_same<g_series_type2,typename std::decay<T>::type>::value>::type*& = enabler>
		explicit g_series_type2(T &&arg1, Args && ... argn) : base(std::forward<T>(arg1),std::forward<Args>(argn)...) {}
		template <typename T>
		typename std::enable_if<!std::is_same<g_series_type2,typename std::decay<T>::type>::value,g_series_type2 &>::type operator=(T &&x)
		{
			base::operator=(std::forward<T>(x));
			return *this;
		}
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
			// Generic construction.
			typedef polynomial_term<long,Expo> term_type2;
			typedef g_series_type<long,Expo> series_type2;
			series_type2 other1;
			other1.m_symbol_set.add("x");
			other1.insert(term_type2(1,key_type{Expo(1)}));
			// Series, different term type, copy.
			series_type s1(other1);
			BOOST_CHECK(s1.size() == 1u);
			BOOST_CHECK(s1.m_container.begin()->m_cf == Cf(1));
			BOOST_CHECK(s1.m_container.begin()->m_key.size() == 1u);
			BOOST_CHECK(s1.m_container.begin()->m_key[0u] == Expo(1));
			// Series, different term type, move.
			series_type s1a(std::move(other1));
			BOOST_CHECK(s1a.size() == 1u);
			BOOST_CHECK(s1a.m_container.begin()->m_cf == Cf(1));
			BOOST_CHECK(s1a.m_container.begin()->m_key.size() == 1u);
			BOOST_CHECK(s1a.m_container.begin()->m_key[0u] == Expo(1));
			BOOST_CHECK(other1.empty());
			BOOST_CHECK(other1.m_symbol_set.size() == 0u);
			// Series, same term type, copy.
			g_series_type2<Cf,Expo> other2;
			other2.m_symbol_set.add("x");
			other2.insert(term_type(1,key_type{Expo(1)}));
			series_type so2(other2);
			BOOST_CHECK(so2.size() == 1u);
			BOOST_CHECK(so2.m_container.begin()->m_cf == Cf(1));
			BOOST_CHECK(so2.m_container.begin()->m_key.size() == 1u);
			BOOST_CHECK(so2.m_container.begin()->m_key[0u] == Expo(1));
			// Series, same term type, move.
			series_type so2a(std::move(other2));
			BOOST_CHECK(so2a.size() == 1u);
			BOOST_CHECK(so2a.m_container.begin()->m_cf == Cf(1));
			BOOST_CHECK(so2a.m_container.begin()->m_key.size() == 1u);
			BOOST_CHECK(so2a.m_container.begin()->m_key[0u] == Expo(1));
			BOOST_CHECK(other2.empty());
			BOOST_CHECK(other2.m_symbol_set.size() == 0u);
			// Construction from non-series.
			series_type s1b(1);
			BOOST_CHECK(s1b.size() == 1u);
			BOOST_CHECK(s1b.m_container.begin()->m_cf == Cf(1));
			BOOST_CHECK(s1b.m_container.begin()->m_key.size() == 0u);
			BOOST_CHECK(s1b.m_symbol_set.size() == 0u);
			// Construction from coefficient series.
			typedef g_series_type<series_type,Expo> series_type3;
			series_type3 s3o(series_type(5.));
			BOOST_CHECK(s3o.size() == 1u);
			BOOST_CHECK(s3o.m_container.begin()->m_cf.size() == series_type(5.).m_container.size());
			series_type3 s4o(series_type("x"));
			BOOST_CHECK(s4o.m_container.begin()->m_cf.size() == 1u);
			BOOST_CHECK(s4o.size() == 1u);
			BOOST_CHECK(s4o.m_container.begin()->m_cf.m_container.begin()->m_cf == 1);
			// Generic assignment.
			// Series, different term type, copy.
			series_type s1c;
			other1.m_symbol_set.add("x");
			other1.insert(term_type2(1,key_type{Expo(1)}));
			s1c = other1;
			BOOST_CHECK(s1c.size() == 1u);
			BOOST_CHECK(s1c.m_container.begin()->m_cf == Cf(1));
			BOOST_CHECK(s1c.m_container.begin()->m_key.size() == 1u);
			BOOST_CHECK(s1c.m_container.begin()->m_key[0u] == Expo(1));
			// Series, different term type, move.
			s1c = std::move(other1);
			BOOST_CHECK(s1c.size() == 1u);
			BOOST_CHECK(s1c.m_container.begin()->m_cf == Cf(1));
			BOOST_CHECK(s1c.m_container.begin()->m_key.size() == 1u);
			BOOST_CHECK(s1c.m_container.begin()->m_key[0u] == Expo(1));
			BOOST_CHECK(other1.empty());
			BOOST_CHECK(other1.m_symbol_set.size() == 0u);
			// Series, same term type, copy.
			other2.m_symbol_set.add("x");
			other2.insert(term_type(1,key_type{Expo(1)}));
			series_type sp2;
			sp2 = other2;
			BOOST_CHECK(sp2.size() == 1u);
			BOOST_CHECK(sp2.m_container.begin()->m_cf == Cf(1));
			BOOST_CHECK(sp2.m_container.begin()->m_key.size() == 1u);
			BOOST_CHECK(sp2.m_container.begin()->m_key[0u] == Expo(1));
			// Series, same term type, move.
			sp2 = std::move(other2);
			BOOST_CHECK(sp2.size() == 1u);
			BOOST_CHECK(sp2.m_container.begin()->m_cf == Cf(1));
			BOOST_CHECK(sp2.m_container.begin()->m_key.size() == 1u);
			BOOST_CHECK(sp2.m_container.begin()->m_key[0u] == Expo(1));
			BOOST_CHECK(other2.empty());
			BOOST_CHECK(other2.m_symbol_set.size() == 0u);
			// Assignment from non series.
			s1b = 2;
			BOOST_CHECK(s1b.size() == 1u);
			BOOST_CHECK(s1b.m_container.begin()->m_cf == Cf(2));
			BOOST_CHECK(s1b.m_container.begin()->m_key.size() == 0u);
			BOOST_CHECK(s1b.m_symbol_set.size() == 0u);
			// Assignment from coefficient series.
			series_type3 s5o;
			s5o = series_type("x");
			BOOST_CHECK(s5o.size() == 1u);
			BOOST_CHECK(s5o.m_container.begin()->m_cf.size() == 1u);
			BOOST_CHECK(s5o.m_container.begin()->m_cf.m_container.begin()->m_cf == 1);
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

struct arithmetics_tag {};

namespace piranha
{
template <>
class debug_access<arithmetics_tag>
{
	public:
		template <typename Cf>
		struct runner
		{
			template <typename Expo>
			void operator()(const Expo &)
			{
				typedef g_series_type<Cf,Expo> p_type1;
				typedef g_series_type2<Cf,Expo> p_type2;
				// In-place addition.
				p_type1 p1;
				p1 += 1;
				p1 += 1.;
				BOOST_CHECK(!p1.empty());
				BOOST_CHECK(p1.m_container.begin()->m_cf == Cf(1) + Cf(1.));
				p_type2 p2;
				p2 += 1;
				p2 += 1.;
				p1 += p2;
				BOOST_CHECK(!p1.empty());
				BOOST_CHECK(p1.m_container.begin()->m_cf == Cf(1) + Cf(1.) + Cf(1) + Cf(1.));
				p1 -= p1;
				BOOST_CHECK(p1.empty());
				p1 += std::move(p2);
				BOOST_CHECK(!p1.empty());
				BOOST_CHECK(p1.m_container.begin()->m_cf == Cf(1) + Cf(1.));
				BOOST_CHECK(p2.empty());
				p1 = p_type1("x");
				p2 = p_type2("y");
				p1 += p2;
				BOOST_CHECK_EQUAL(p1.size(),2u);
				BOOST_CHECK_EQUAL(p1.m_symbol_set.size(),2u);
				BOOST_CHECK_EQUAL(p1.m_symbol_set[0],symbol("x"));
				BOOST_CHECK_EQUAL(p1.m_symbol_set[1],symbol("y"));
				p1 += p2;
				BOOST_CHECK_EQUAL(p1.size(),2u);
				auto it1 = p1.m_container.begin();
				BOOST_CHECK(it1->m_cf == Cf(1) || it1->m_cf == Cf(2));
				++it1;
				BOOST_CHECK(it1->m_cf == Cf(1) || it1->m_cf == Cf(2));
				p2 += std::move(p1);
				auto it2 = p2.m_container.begin();
				BOOST_CHECK(it2->m_cf == Cf(1) || it2->m_cf == Cf(3));
				++it2;
				BOOST_CHECK(it2->m_cf == Cf(1) || it2->m_cf == Cf(3));
				// Addition with coefficient series.
				typedef g_series_type<p_type1,Expo> p_type11;
				p_type11 p11("x");
				p11 += p_type1(1);
				BOOST_CHECK(p11.size() == 2u);
				p11 += p_type1("y");
				BOOST_CHECK(p11.size() == 2u);
				BOOST_CHECK(p11.m_symbol_set.size() == 1u);
				BOOST_CHECK(p11.m_symbol_set[0] == symbol("x"));
				auto it = p11.m_container.begin();
				BOOST_CHECK(it->m_cf.m_symbol_set.size() == 0u || it->m_cf.m_symbol_set.size() == 1u);
				BOOST_CHECK(it->m_cf.size() == 1u || it->m_cf.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf.m_symbol_set.size() == 0u || it->m_cf.m_symbol_set.size() == 1u);
				BOOST_CHECK(it->m_cf.size() == 1u || it->m_cf.size() == 2u);
				// In-place subtraction.
				p1 = p_type1{};
				p1 -= 1;
				p1 -= 1.;
				BOOST_CHECK(!p1.empty());
				BOOST_CHECK(p1.m_container.begin()->m_cf == Cf(-1) - Cf(1.));
				p2 = p_type2{};
				p2 -= 1;
				p2 -= 1.;
				p1 += p2;
				BOOST_CHECK(!p1.empty());
				BOOST_CHECK(p1.m_container.begin()->m_cf == Cf(-1) - Cf(1.) - Cf(1) - Cf(1.));
				p1 -= p1;
				BOOST_CHECK(p1.empty());
				p1 -= std::move(p2);
				BOOST_CHECK(!p1.empty());
				BOOST_CHECK(p1.m_container.begin()->m_cf == Cf(1) + Cf(1.));
				BOOST_CHECK(p2.empty());
				p1 = p_type1("x");
				p2 = p_type2("y");
				p1 -= p2;
				BOOST_CHECK_EQUAL(p1.size(),2u);
				BOOST_CHECK_EQUAL(p1.m_symbol_set.size(),2u);
				BOOST_CHECK_EQUAL(p1.m_symbol_set[0],symbol("x"));
				BOOST_CHECK_EQUAL(p1.m_symbol_set[1],symbol("y"));
				p1 -= p2;
				BOOST_CHECK_EQUAL(p1.size(),2u);
				it1 = p1.m_container.begin();
				BOOST_CHECK(it1->m_cf == Cf(1) || it1->m_cf == Cf(-2));
				++it1;
				BOOST_CHECK(it1->m_cf == Cf(1) || it1->m_cf == Cf(-2));
				p2 -= std::move(p1);
				it2 = p2.m_container.begin();
				BOOST_CHECK(it2->m_cf == Cf(-1) || it2->m_cf == Cf(3));
				++it2;
				BOOST_CHECK(it2->m_cf == Cf(-1) || it2->m_cf == Cf(3));
				// Subtraction with coefficient series.
				p11 = p_type11("x");
				p11 -= p_type1(1);
				p11 -= p_type1("y");
				p11 += p_type1(1);
				BOOST_CHECK(p11.size() == 2u);
				p11 -= p_type11("x");
				BOOST_CHECK(p11.size() == 1u);
				BOOST_CHECK(p11.m_container.begin()->m_cf.size() == 1u);
				BOOST_CHECK(p11.m_container.begin()->m_cf.m_container.begin()->m_cf == Cf(-1));
				p11 += p_type1("y");
				BOOST_CHECK(p11.empty());
				BOOST_CHECK(p11.m_symbol_set.size() == 1u);
// 				// Multiplication.
// 				p1 = p_type1("x");
// 				p1 *= 0;
// 				BOOST_CHECK(p1.empty());
// 				BOOST_CHECK(p1 == 0);
// 				p1 = p_type1("x") + p_type1("y");
// 				p1 *= 2;
// 				BOOST_CHECK(p1 == 2 * p_type1("x") + 2 * p_type1("y"));
// 				// In-place with series.
// 				p1 = p_type1("x");
// 				p1 *= 2;
// 				p1 *= p_type1("x");
// 				BOOST_CHECK(p1.m_container.begin()->m_cf.get_value() == (2 * typename Cf::type(1)) *  typename Cf::type(1));
// 				BOOST_CHECK(p1.m_container.begin()->m_key.size() == 1u);
// 				BOOST_CHECK(p1.m_container.begin()->m_key[0] == 2);
// 				p1 = p_type1("x");
// 				p1 *= 2;
// 				p1 *= p_type2("x");
// 				BOOST_CHECK(p1.m_container.begin()->m_cf.get_value() == (2 * typename Cf::type(1)) *  typename Cf::type(1));
// 				BOOST_CHECK(p1.m_container.begin()->m_key.size() == 1u);
// 				BOOST_CHECK(p1.m_container.begin()->m_key[0] == 2);
// 				p2 = p_type2("x");
// 				p2 *= 2;
// 				p2 *= p_type1("x");
// 				BOOST_CHECK(p2.m_container.begin()->m_cf.get_value() == (2 * typename Cf::type(1)) *  typename Cf::type(1));
// 				BOOST_CHECK(p2.m_container.begin()->m_key.size() == 1u);
// 				BOOST_CHECK(p2.m_container.begin()->m_key[0] == 2);
// 				p1 = p_type1("x");
// 				p1 *= 2;
// 				p1 *= p_type1("y");
// 				BOOST_CHECK(p1.m_container.begin()->m_cf.get_value() == (2 * typename Cf::type(1)) *  typename Cf::type(1));
// 				BOOST_CHECK(p1.m_container.begin()->m_key.size() == 2u);
// 				BOOST_CHECK(p1.m_container.begin()->m_key[0] == 1);
// 				BOOST_CHECK(p1.m_container.begin()->m_key[1] == 1);
// 				p1 = p_type1("x") + p_type1("y");
// 				p1 *= 2;
// 				p1 *= p_type1("y");
// 				BOOST_CHECK(p1.size() == 2u);
// 				auto it = p1.m_container.begin();
// 				BOOST_CHECK(it->m_cf.get_value() == (2 * typename Cf::type(1)) *  typename Cf::type(1));
// 				BOOST_CHECK(it->m_key.size() == 2u);
// 				BOOST_CHECK((it->m_key[0] == 1 && it->m_key[1] == 1) || (it->m_key[0] == 0 && it->m_key[1] == 2));
// 				++it;
// 				BOOST_CHECK(it->m_cf.get_value() == (2 * typename Cf::type(1)) *  typename Cf::type(1));
// 				BOOST_CHECK(it->m_key.size() == 2u);
// 				BOOST_CHECK((it->m_key[0] == 1 && it->m_key[1] == 1) || (it->m_key[0] == 0 && it->m_key[1] == 2));
// 				p1 = p_type1("y");
// 				p1 *= 2;
// 				p1 *= p_type1("x") + p_type1("y");
// 				BOOST_CHECK(p1.size() == 2u);
// 				it = p1.m_container.begin();
// 				BOOST_CHECK(it->m_cf.get_value() == (2 * typename Cf::type(1)) *  typename Cf::type(1));
// 				BOOST_CHECK(it->m_key.size() == 2u);
// 				BOOST_CHECK((it->m_key[0] == 1 && it->m_key[1] == 1) || (it->m_key[0] == 0 && it->m_key[1] == 2));
// 				++it;
// 				BOOST_CHECK(it->m_cf.get_value() == (2 * typename Cf::type(1)) *  typename Cf::type(1));
// 				BOOST_CHECK(it->m_key.size() == 2u);
// 				BOOST_CHECK((it->m_key[0] == 1 && it->m_key[1] == 1) || (it->m_key[0] == 0 && it->m_key[1] == 2));
			}
		};
		template <typename Cf>
		void operator()(const Cf &)
		{
			boost::mpl::for_each<expo_types>(runner<Cf>());
		}
};
}

typedef debug_access<arithmetics_tag> arithmetics_tester;

BOOST_AUTO_TEST_CASE(series_arithmetics_test)
{
	boost::mpl::for_each<cf_types>(arithmetics_tester());
}

struct negate_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef g_series_type<Cf,Expo> p_type;
			p_type p("x");
			p += 1;
			p += p_type("y");
			BOOST_CHECK_EQUAL(p.size(),unsigned(3));
			p_type q1 = p, q2 = p;
			p.negate();
			BOOST_CHECK_EQUAL(p.size(),unsigned(3));
			p += q1;
			BOOST_CHECK(p.empty());
			math::negate(q2);
			q2 += q1;
			BOOST_CHECK(q2.empty());
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(series_negate_test)
{
	boost::mpl::for_each<cf_types>(negate_tester());
}

struct binary_arithmetics_tag {};

namespace piranha
{
template <>
class debug_access<binary_arithmetics_tag>
{
	public:
		template <typename Cf>
		struct runner
		{
			template <typename Expo>
			void operator()(const Expo &)
			{
				typedef g_series_type<Cf,Expo> p_type1;
				typedef g_series_type<p_type1,Expo> p_type11;
				typedef g_series_type2<float,Expo> p_type2;
				// Addition.
				p_type1 x("x"), y("y");
				auto z = 1 + x;
				BOOST_CHECK((std::is_same<decltype(z),p_type1>::value));
				BOOST_CHECK_EQUAL(z.size(),2u);
				auto it = z.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1));
				BOOST_CHECK(it->m_key.size() == 1u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(1));
				BOOST_CHECK(it->m_key.size() == 1u);
				z = x + 1;
				it = z.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1));
				BOOST_CHECK(it->m_key.size() == 1u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(1));
				BOOST_CHECK(it->m_key.size() == 1u);
				z = x + y;
				BOOST_CHECK_EQUAL(z.size(),2u);
				it = z.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(1));
				BOOST_CHECK(it->m_key.size() == 2u);
				p_type2 a("a"), b("b");
				auto c = a + b + x;
				BOOST_CHECK_EQUAL(c.size(),3u);
				BOOST_CHECK_EQUAL(c.m_symbol_set.size(),3u);
				BOOST_CHECK_EQUAL(c.m_symbol_set[0],symbol("a"));
				BOOST_CHECK_EQUAL(c.m_symbol_set[1],symbol("b"));
				BOOST_CHECK_EQUAL(c.m_symbol_set[2],symbol("x"));
				c = x + b + a;
				BOOST_CHECK_EQUAL(c.size(),3u);
				BOOST_CHECK_EQUAL(c.m_symbol_set.size(),3u);
				BOOST_CHECK_EQUAL(c.m_symbol_set[0],symbol("a"));
				BOOST_CHECK_EQUAL(c.m_symbol_set[1],symbol("b"));
				BOOST_CHECK_EQUAL(c.m_symbol_set[2],symbol("x"));
				// Coefficient series.
				p_type11 m("m");
				auto n = m + x;
				BOOST_CHECK((std::is_same<decltype(n),p_type11>::value));
				BOOST_CHECK_EQUAL(n.size(),2u);
				BOOST_CHECK(n.m_symbol_set.size() == 1u);
				BOOST_CHECK(n.m_symbol_set[0] == symbol("m"));
				BOOST_CHECK(n.m_container.begin()->m_cf.m_container.begin()->m_cf == Cf(1));
				auto n2 = x + m;
				BOOST_CHECK((std::is_same<decltype(n2),p_type11>::value));
				BOOST_CHECK_EQUAL(n2.size(),2u);
				BOOST_CHECK(n2.m_symbol_set.size() == 1u);
				BOOST_CHECK(n2.m_symbol_set[0] == symbol("m"));
				BOOST_CHECK(n2.m_container.begin()->m_cf.m_container.begin()->m_cf == Cf(1));
				// Subtraction.
				z = 1 - x;
				BOOST_CHECK((std::is_same<decltype(z),p_type1>::value));
				BOOST_CHECK_EQUAL(z.size(),2u);
				BOOST_CHECK(z.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK((z - 1).size() == 1u);
				BOOST_CHECK((z - 1).m_container.begin()->m_cf == -Cf(1));
				z = x - 1;
				BOOST_CHECK((std::is_same<decltype(z),p_type1>::value));
				BOOST_CHECK_EQUAL(z.size(),2u);
				BOOST_CHECK(z.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK((z + 1).size() == 1u);
				BOOST_CHECK((z + 1).m_container.begin()->m_cf == Cf(1));
				z = x - y;
				BOOST_CHECK_EQUAL(z.size(),2u);
				it = z.m_container.begin();
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf == Cf(1) || it->m_cf == Cf(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				c = a - b - x;
				BOOST_CHECK_EQUAL(c.size(),3u);
				BOOST_CHECK_EQUAL(c.m_symbol_set.size(),3u);
				BOOST_CHECK_EQUAL(c.m_symbol_set[0],symbol("a"));
				BOOST_CHECK_EQUAL(c.m_symbol_set[1],symbol("b"));
				BOOST_CHECK_EQUAL(c.m_symbol_set[2],symbol("x"));
				c = x - b - a;
				BOOST_CHECK_EQUAL(c.size(),3u);
				BOOST_CHECK_EQUAL(c.m_symbol_set.size(),3u);
				BOOST_CHECK_EQUAL(c.m_symbol_set[0],symbol("a"));
				BOOST_CHECK_EQUAL(c.m_symbol_set[1],symbol("b"));
				BOOST_CHECK_EQUAL(c.m_symbol_set[2],symbol("x"));
				c = c - c;
				BOOST_CHECK(c.empty());
				// Coefficient series.
				auto n3 = m - x;
				BOOST_CHECK((std::is_same<decltype(n3),p_type11>::value));
				BOOST_CHECK_EQUAL(n3.size(),2u);
				BOOST_CHECK(n3.m_symbol_set.size() == 1u);
				BOOST_CHECK(n3.m_symbol_set[0] == symbol("m"));
				auto n4 = x - m;
				BOOST_CHECK((std::is_same<decltype(n4),p_type11>::value));
				BOOST_CHECK_EQUAL(n4.size(),2u);
				BOOST_CHECK(n4.m_symbol_set.size() == 1u);
				BOOST_CHECK(n4.m_symbol_set[0] == symbol("m"));
				BOOST_CHECK((n3 + n4).empty());
#if 0
				// Multiplication.
				auto res = x * 0;
				BOOST_CHECK(res.empty());
				res = 0 * x;
				BOOST_CHECK(res.empty());
				res = 2 * x;
				BOOST_CHECK(res.size() == 1u);
				BOOST_CHECK(res.m_container.begin()->m_cf.get_value() == typename Cf::type(1) * typename Cf::type(2));
				BOOST_CHECK(res.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK(res.m_container.begin()->m_key[0] == 1);
				res = x * 2;
				BOOST_CHECK(res.size() == 1u);
				BOOST_CHECK(res.m_container.begin()->m_cf.get_value() == typename Cf::type(1) * typename Cf::type(2));
				BOOST_CHECK(res.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK(res.m_container.begin()->m_key[0] == 1);
				res = x * x;
				BOOST_CHECK(res.size() == 1u);
				BOOST_CHECK(res.m_container.begin()->m_cf.get_value() == typename Cf::type(1) * typename Cf::type(1));
				BOOST_CHECK(res.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK(res.m_container.begin()->m_key[0] == 2);
				res = x * y;
				BOOST_CHECK(res.size() == 1u);
				BOOST_CHECK(res.m_container.begin()->m_cf.get_value() == typename Cf::type(1) * typename Cf::type(1));
				BOOST_CHECK(res.m_container.begin()->m_key.size() == 2u);
				BOOST_CHECK(res.m_container.begin()->m_key[0] == 1);
				BOOST_CHECK(res.m_container.begin()->m_key[1] == 1);
				res = (x + y) * (y * 2);
				BOOST_CHECK(res.size() == 2u);
				it = res.m_container.begin();
				BOOST_CHECK(it->m_cf.get_value() == typename Cf::type(1) * (typename Cf::type(1) * 2));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((it->m_key[0] == 1 && it->m_key[1] == 1) || (it->m_key[0] == 0 && it->m_key[1] == 2));
				++it;
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((it->m_key[0] == 1 && it->m_key[1] == 1) || (it->m_key[0] == 0 && it->m_key[1] == 2));
				res = (y * 2) * (x + y);
				BOOST_CHECK(res.size() == 2u);
				it = res.m_container.begin();
				BOOST_CHECK(it->m_cf.get_value() == typename Cf::type(1) * (typename Cf::type(1) * 2));
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((it->m_key[0] == 1 && it->m_key[1] == 1) || (it->m_key[0] == 0 && it->m_key[1] == 2));
				++it;
				BOOST_CHECK(it->m_key.size() == 2u);
				BOOST_CHECK((it->m_key[0] == 1 && it->m_key[1] == 1) || (it->m_key[0] == 0 && it->m_key[1] == 2));
				auto mix = x * p_type2{"x"};
				BOOST_CHECK(mix.size() == 1u);
				BOOST_CHECK(mix.m_container.begin()->m_cf.get_value() == typename Cf::type(1) * 1.f);
				BOOST_CHECK(mix.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK(mix.m_container.begin()->m_key[0] == 2);
				mix = p_type2{"x"} * x;
				BOOST_CHECK(mix.size() == 1u);
				BOOST_CHECK(mix.m_container.begin()->m_cf.get_value() == typename Cf::type(1) * 1.f);
				BOOST_CHECK(mix.m_container.begin()->m_key.size() == 1u);
				BOOST_CHECK(mix.m_container.begin()->m_key[0] == 2);
#endif
			}
		};
		template <typename Cf>
		void operator()(const Cf &)
		{
			boost::mpl::for_each<expo_types>(runner<Cf>());
		}
};
}

typedef debug_access<binary_arithmetics_tag> binary_arithmetics_tester;

BOOST_AUTO_TEST_CASE(series_binary_arithmetics_test)
{
	boost::mpl::for_each<cf_types>(binary_arithmetics_tester());
}

struct equality_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef g_series_type<Cf,Expo> p_type1;
			typedef g_series_type<p_type1,Expo> p_type11;
			typedef g_series_type2<float,Expo> p_type2;
// 			typedef polynomial<Cf,Expo> p_type1;
// 			typedef polynomial<numerical_coefficient<long>,Expo> p_type2;
// 			typedef polynomial2<Cf,Expo> p_type3;
// 			typedef polynomial2<numerical_coefficient<long>,Expo> p_type4;
			BOOST_CHECK(p_type1{} == 0);
			BOOST_CHECK(0 == p_type1{});
			BOOST_CHECK(!(p_type1{} == 1));
			BOOST_CHECK(!(1 == p_type1{}));
			BOOST_CHECK(p_type1{} == p_type11{});
			BOOST_CHECK(p_type11{} == p_type1{});
#if 0
			BOOST_CHECK(p_type1{1} == 1);
			BOOST_CHECK(1 == p_type1{1});
			BOOST_CHECK(p_type1{1} != 0);
			BOOST_CHECK(0 != p_type1{1});
			BOOST_CHECK(p_type1{"x"} != 1);
			BOOST_CHECK(p_type1{"x"} != 0);
			BOOST_CHECK(1 != p_type1{"x"});
			BOOST_CHECK(0 != p_type1{"x"});
			BOOST_CHECK(p_type1{"x"} == p_type1{"x"});
			BOOST_CHECK(p_type1{"x"} == p_type2{"x"});
			BOOST_CHECK(p_type2{"x"} == p_type1{"x"});
			BOOST_CHECK(p_type1{"x"} != p_type1{"x"} + p_type1{"x"});
			BOOST_CHECK(p_type1{"x"} + p_type1{"x"} != p_type1{"x"});
			BOOST_CHECK(p_type1{"x"} != p_type1{"x"} + p_type1{"x"});
			BOOST_CHECK(p_type1{"x"} + p_type1{"x"} != p_type1{"x"});
			BOOST_CHECK(p_type2{"x"} != p_type1{"x"} + p_type1{"x"});
			BOOST_CHECK(p_type2{"x"} + p_type1{"x"} != p_type1{"x"});
			BOOST_CHECK(p_type1{"x"} != p_type2{"x"} + p_type1{"x"});
			BOOST_CHECK(p_type1{"x"} + p_type1{"x"} != p_type2{"x"});
			BOOST_CHECK(0 == p_type1{"x"} - p_type2{"x"});
			BOOST_CHECK(p_type1{"x"} - p_type2{"x"} == 0);
			BOOST_CHECK(1 + p_type1{"x"} - p_type2{"x"} == 1);
			BOOST_CHECK(p_type1{} == p_type2{});
			BOOST_CHECK(1 + p_type1{"x"} != 0);
			BOOST_CHECK(1 + p_type1{"x"} != 1);
			BOOST_CHECK(1 + p_type1{"x"} != p_type1{"x"});
			BOOST_CHECK(0 != 1 + p_type1{"x"});
			BOOST_CHECK(1 != 1 + p_type1{"x"});
			BOOST_CHECK(p_type1{"x"} != 1 + p_type1{"x"});
			BOOST_CHECK(p_type1{"x"} != 1 + p_type1{"x"});
			BOOST_CHECK(p_type1{"x"} + p_type1{"y"} != p_type1{"x"});
			BOOST_CHECK(p_type1{"x"} != p_type1{"x"} + p_type1{"y"});
			BOOST_CHECK(p_type2{"x"} + p_type1{"y"} != p_type1{"x"});
			BOOST_CHECK(p_type1{"x"} != p_type2{"x"} + p_type1{"y"});
			BOOST_CHECK(p_type3{"x"} + p_type1{"y"} != p_type1{"x"});
			BOOST_CHECK(p_type3{"x"} != p_type1{"x"} + p_type1{"y"});
			BOOST_CHECK(p_type3{"x"} + p_type1{"y"} != p_type1{"x"});
			BOOST_CHECK(p_type1{"x"} != p_type3{"x"} + p_type1{"y"});
			BOOST_CHECK(p_type4{"x"} + p_type3{"z"} != p_type2{"x"} + p_type1{"y"});
			BOOST_CHECK(p_type4{"x"} + p_type3{"z"} == p_type2{"x"} + p_type1{"y"} - p_type1{"y"} + p_type1{"z"});
#endif
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(series_equality_test)
{
	boost::mpl::for_each<cf_types>(equality_tester());
}

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

#define BOOST_TEST_MODULE series_01_test
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <initializer_list>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

#include "../src/config.hpp"
#include "../src/debug_access.hpp"
#include "../src/environment.hpp"
#include "../src/exceptions.hpp"
#include "../src/forwarding.hpp"
#include "../src/math.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/monomial.hpp"
#include "../src/polynomial.hpp"
#include "../src/pow.hpp"
#include "../src/print_coefficient.hpp"
#include "../src/print_tex_coefficient.hpp"
#include "../src/real.hpp"
#include "../src/safe_cast.hpp"
#include "../src/serialization.hpp"
#include "../src/settings.hpp"
#include "../src/symbol.hpp"
#include "../src/symbol_set.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

typedef boost::mpl::vector<double,integer,rational,real> cf_types;
typedef boost::mpl::vector<unsigned,integer> expo_types;

template <typename Cf, typename Expo>
class g_series_type: public series<Cf,monomial<Expo>,g_series_type<Cf,Expo>>
{
	public:
		template <typename Cf2>
		using rebind = g_series_type<Cf2,Expo>;
		typedef series<Cf,monomial<Expo>,g_series_type<Cf,Expo>> base;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
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
		g_series_type &operator=(g_series_type &&) = default;
		PIRANHA_FORWARDING_CTOR(g_series_type,base)
		PIRANHA_FORWARDING_ASSIGNMENT(g_series_type,base)
		// Provide fake sin/cos methods with wrong sigs.
		g_series_type sin()
		{
			return g_series_type(42);
		}
		int cos() const
		{
			return -42;
		}
};

template <typename Cf, typename Expo>
class g_series_type2: public series<Cf,monomial<Expo>,g_series_type2<Cf,Expo>>
{
	public:
		template <typename Cf2>
		using rebind = g_series_type2<Cf2,Expo>;
		typedef series<Cf,monomial<Expo>,g_series_type2<Cf,Expo>> base;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
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
		g_series_type2 &operator=(g_series_type2 &&) = default;
		PIRANHA_FORWARDING_CTOR(g_series_type2,base)
		PIRANHA_FORWARDING_ASSIGNMENT(g_series_type2,base)
		// Provide fake sin/cos methods to test math overloads.
		g_series_type2 sin() const
		{
			return g_series_type2(42);
		}
		g_series_type2 cos() const
		{
			return g_series_type2(-42);
		}
};

template <typename Cf, typename Expo>
class g_series_type3: public series<Cf,monomial<Expo>,g_series_type3<Cf,Expo>>
{
	public:
		template <typename Cf2>
		using rebind = g_series_type3<Cf2,Expo>;
		typedef series<Cf,monomial<Expo>,g_series_type3<Cf,Expo>> base;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
		g_series_type3() = default;
		g_series_type3(const g_series_type3 &) = default;
		g_series_type3(g_series_type3 &&) = default;
		explicit g_series_type3(const char *name):base()
		{
			typedef typename base::term_type term_type;
			// Insert the symbol.
			this->m_symbol_set.add(name);
			// Construct and insert the term.
			this->insert(term_type(Cf(1),typename term_type::key_type{Expo(1)}));
		}
		g_series_type3 &operator=(const g_series_type3 &) = default;
		g_series_type3 &operator=(g_series_type3 &&) = default;
		PIRANHA_FORWARDING_CTOR(g_series_type3,base)
		PIRANHA_FORWARDING_ASSIGNMENT(g_series_type3,base)
};

template <typename Cf, typename Expo>
class g_series_type4: public series<Cf,monomial<Expo>,g_series_type4<Cf,Expo>>
{
	public:
		template <typename Cf2>
		using rebind = g_series_type4<Cf2,Expo>;
		typedef series<Cf,monomial<Expo>,g_series_type4<Cf,Expo>> base;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
		g_series_type4() = default;
		g_series_type4(const g_series_type4 &) = default;
		g_series_type4(g_series_type4 &&) = default;
		explicit g_series_type4(const char *name):base()
		{
			typedef typename base::term_type term_type;
			// Insert the symbol.
			this->m_symbol_set.add(name);
			// Construct and insert the term.
			this->insert(term_type(Cf(1),typename term_type::key_type{Expo(1)}));
		}
		g_series_type4 &operator=(const g_series_type4 &) = default;
		g_series_type4 &operator=(g_series_type4 &&) = default;
		PIRANHA_FORWARDING_CTOR(g_series_type4,base)
		PIRANHA_FORWARDING_ASSIGNMENT(g_series_type4,base)
		g_series_type4 sin() const;
		g_series_type4 cos() const;
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
			typedef term<Cf,monomial<Expo>> term_type;
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
			typedef term<long,monomial<Expo>> term_type2;
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
			other1 = 0;
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
			// Series, same term type, copy.
			other2 = 0;
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
			// Type traits.
			BOOST_CHECK((std::is_constructible<series_type,series_type>::value));
			BOOST_CHECK((!std::is_constructible<series_type,series_type,int>::value));
// This should be the same problem as in the explicit integer conversion.
#if !defined(PIRANHA_COMPILER_IS_CLANG)
			BOOST_CHECK((std::is_constructible<series_type2,series_type>::value));
#endif
			BOOST_CHECK((std::is_constructible<series_type3,series_type>::value));
			BOOST_CHECK((std::is_assignable<series_type,int>::value));
			BOOST_CHECK((std::is_assignable<series_type,series_type2>::value));
			BOOST_CHECK((!std::is_assignable<series_type,symbol_set>::value));
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
	environment env;
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
				typedef term<Cf,monomial<Expo>> term_type;
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
				typedef term<Cf,monomial<Expo>> term_type;
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
				auto check_neg_merge = [&it]() {
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

struct merge_arguments_tag {};

namespace piranha
{
template <>
class debug_access<merge_arguments_tag>
{
	public:
		template <typename Cf>
		struct runner
		{
			template <typename Expo>
			void operator()(const Expo &)
			{
				typedef term<Cf,monomial<Expo>> term_type;
				typedef typename term_type::key_type key_type;
				typedef g_series_type<Cf,Expo> series_type;
				series_type s_derived;
				typename series_type::base &s = static_cast<typename series_type::base &>(s_derived);
				symbol_set ed1, ed2;
				s.insert(term_type(Cf(1),key_type()));
				ed2.add(symbol("x"));
				auto merge_out = s.merge_arguments(ed2);
				BOOST_CHECK_EQUAL(merge_out.size(),unsigned(1));
				BOOST_CHECK(merge_out.m_container.find(term_type(Cf(1),key_type{Expo(0)})) != merge_out.m_container.end());
				auto compat_check = [](const typename series_type::base &series) {
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
				merge_out = s.merge_arguments(ed2);
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

typedef debug_access<merge_arguments_tag> merge_arguments_tester;

BOOST_AUTO_TEST_CASE(series_merge_arguments_test)
{
	boost::mpl::for_each<cf_types>(merge_arguments_tester());
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

struct identity_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef g_series_type<Cf,Expo> p_type1;
			BOOST_CHECK(+p_type1{} == +p_type1{});
			BOOST_CHECK(+p_type1{} == p_type1{});
			BOOST_CHECK(p_type1{} == +p_type1{});
			BOOST_CHECK(p_type1("x") == +p_type1("x"));
			BOOST_CHECK(+p_type1("x") == p_type1("x"));
			BOOST_CHECK(+p_type1("x") == +p_type1("x"));
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(series_identity_test)
{
	boost::mpl::for_each<cf_types>(identity_tester());
}

struct negation_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef g_series_type<Cf,Expo> p_type1;
			BOOST_CHECK(+p_type1{} == -(-(+p_type1{})));
			BOOST_CHECK(-(-(+p_type1{})) == p_type1{});
			BOOST_CHECK(-p_type1("x") == -(+p_type1("x")));
			BOOST_CHECK(-(+p_type1("x")) == -p_type1("x"));
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(series_negation_test)
{
	boost::mpl::for_each<cf_types>(negation_tester());
}

struct stream_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			// Avoid the stream tests with floating-point and similar, because of messy output.
			if (std::is_same<Cf,double>::value || std::is_same<Cf,real>::value) {
				return;
			}
			typedef g_series_type<Cf,Expo> p_type1;
			typedef g_series_type<p_type1,Expo> p_type11;
			std::ostringstream oss;
			oss << p_type1{};
			BOOST_CHECK(oss.str() == "0");
			oss.str("");
			oss << p_type1{1};
			BOOST_CHECK(oss.str() == "1");
			oss.str("");
			oss << p_type1{-1};
			BOOST_CHECK(oss.str() == "-1");
			oss.str("");
			oss << p_type1{"x"};
			BOOST_CHECK(oss.str() == "x");
			oss.str("");
			oss << (-p_type1{"x"});
			BOOST_CHECK(oss.str() == "-x");
			oss.str("");
			oss << (-p_type1{"x"} * p_type1{"y"});
			BOOST_CHECK(oss.str() == "-x*y");
			oss.str("");
			oss << (-p_type1{"x"} + 1);
			BOOST_CHECK(oss.str() == "1-x" || oss.str() == "-x+1");
			oss.str("");
			oss << p_type11{};
			BOOST_CHECK(oss.str() == "0");
			oss.str("");
			oss << p_type11{"x"};
			BOOST_CHECK(oss.str() == "x");
			oss.str("");
			oss << (-p_type11{"x"});
			BOOST_CHECK(oss.str() == "-x");
			oss.str("");
			oss << (p_type11{1});
			BOOST_CHECK(oss.str() == "1");
			oss.str("");
			oss << (p_type11{-1});
			BOOST_CHECK(oss.str() == "-1");
			oss.str("");
			oss << (p_type11{"x"} * p_type11{"y"});
			BOOST_CHECK(oss.str() == "x*y");
			oss.str("");
			oss << (-p_type11{"x"} * p_type11{"y"});
			BOOST_CHECK(oss.str() == "-x*y");
			oss.str("");
			oss << (-p_type11{"x"} + 1);
			BOOST_CHECK(oss.str() == "1-x" || oss.str() == "-x+1");
			oss.str("");
			oss << (p_type11{"x"} - 1);
			BOOST_CHECK(oss.str() == "x-1" || oss.str() == "-1+x");
			// Test wih less term output.
			typedef polynomial<Cf,monomial<Expo>> poly_type;
			settings::set_max_term_output(3u);
			oss.str("");
			oss << p_type11{};
			BOOST_CHECK(oss.str() == "0");
			oss.str("");
			oss << p_type11{"x"};
			BOOST_CHECK(oss.str() == "x");
			oss.str("");
			oss << (-p_type11{"x"});
			BOOST_CHECK(oss.str() == "-x");
			oss.str("");
			oss << (p_type11{1});
			BOOST_CHECK(oss.str() == "1");
			oss.str("");
			oss << (p_type11{-1});
			BOOST_CHECK(oss.str() == "-1");
			oss.str("");
			oss << (p_type11{"x"} * p_type11{"y"});
			BOOST_CHECK(oss.str() == "x*y");
			oss.str("");
			oss << (-p_type11{"x"} * p_type11{"y"});
			BOOST_CHECK(oss.str() == "-x*y");
			// Test wih small term output.
			settings::set_max_term_output(1u);
			const std::string tmp_out = boost::lexical_cast<std::string>(3 * poly_type{"x"} + 1 + poly_type{"x"} * poly_type{"x"} +
				poly_type{"x"} * poly_type{"x"} * poly_type{"x"}), tmp_cmp = "...";
			BOOST_CHECK(std::equal(tmp_cmp.begin(),tmp_cmp.end(),tmp_out.rbegin()));
			BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(poly_type{}),"0");
			settings::reset_max_term_output();
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(series_stream_test)
{
	boost::mpl::for_each<cf_types>(stream_tester());
}

struct table_info_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef g_series_type<Cf,Expo> p_type1;
			p_type1 p;
			using s_type = decltype(p.table_sparsity());
			BOOST_CHECK(p.table_sparsity() == s_type{});
			BOOST_CHECK(p.table_bucket_count() == 0u);
			BOOST_CHECK(p.table_load_factor() == 0.);
			p_type1 q{"x"};
			BOOST_CHECK((q.table_sparsity() == s_type{{1u,1u}}));
			BOOST_CHECK(q.table_load_factor() != 0.);
			BOOST_CHECK(q.table_bucket_count() != 0u);
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(series_table_info_test)
{
	boost::mpl::for_each<cf_types>(table_info_tester());
}

struct fake_int_01
{
	fake_int_01();
	explicit fake_int_01(int);
	fake_int_01(const fake_int_01 &);
	fake_int_01(fake_int_01 &&) noexcept;
	fake_int_01 &operator=(const fake_int_01 &);
	fake_int_01 &operator=(fake_int_01 &&) noexcept;
	~fake_int_01();
};

struct fake_int_02
{
	fake_int_02();
	explicit fake_int_02(int);
	fake_int_02(const fake_int_02 &);
	fake_int_02(fake_int_02 &&) noexcept;
	fake_int_02 &operator=(const fake_int_02 &);
	fake_int_02 &operator=(fake_int_02 &&) noexcept;
	~fake_int_02();
};

namespace piranha
{
namespace math
{

template <typename T, typename U>
struct pow_impl<T,U,typename std::enable_if<std::is_floating_point<T>::value &&
	std::is_same<U,fake_int_01>::value>::type>
{
	T operator()(const T &, const U &) const;
};

template <typename T>
struct is_zero_impl<T,typename std::enable_if<std::is_same<T,fake_int_01>::value>::type>
{
	bool operator()(const T &) const;
};

template <typename T>
struct is_zero_impl<T,typename std::enable_if<std::is_same<T,fake_int_02>::value>::type>
{
	bool operator()(const T &) const;
};

}

template <typename T>
struct safe_cast_impl<integer,T,typename std::enable_if<std::is_same<T,fake_int_01>::value>::type>
{
	integer operator()(const T &) const;
};

template <typename T>
struct safe_cast_impl<integer,T,typename std::enable_if<std::is_same<T,fake_int_02>::value>::type>
{
	integer operator()(const T &) const;
};

}

struct pow_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef g_series_type<Cf,Expo> p_type1;
			p_type1 p1;
			BOOST_CHECK(p1.pow(0) == Cf(1));
			BOOST_CHECK(p1.pow(1) == Cf(0));
			p1 = 2;
			BOOST_CHECK(math::pow(p1,4) == math::pow(Cf(2),4));
			BOOST_CHECK(math::pow(p1,-4) == math::pow(Cf(2),-4));
			p1 = p_type1("x");
			p1 += 1;
			BOOST_CHECK(math::pow(p1,1) == p1);
			BOOST_CHECK(p1.pow(2u) == p1 * p1);
			BOOST_CHECK(math::pow(p1,integer(3)) == p1 * p1 * p1);
			BOOST_CHECK_THROW(p1.pow(-1),std::invalid_argument);
			// Coefficient series.
			typedef g_series_type<p_type1,Expo> p_type11;
			p_type11 p11;
			BOOST_CHECK(p11.pow(0) == Cf(1));
			BOOST_CHECK(p11.pow(1) == Cf(0));
			p11 = 2;
			BOOST_CHECK(math::pow(p11,4) == math::pow(p_type1(2),4));
			BOOST_CHECK(math::pow(p11,-4) == math::pow(p_type1(2),-4));
			p11 = p_type11("x");
			p11 += 1;
			BOOST_CHECK(math::pow(p11,1) == p11);
			BOOST_CHECK(p11.pow(2u) == p11 * p11);
			BOOST_CHECK(math::pow(p11,integer(3)) == p11 * p11 * p11);
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(series_pow_test)
{
	boost::mpl::for_each<cf_types>(pow_tester());
	typedef g_series_type<double,int> p_type1;
	if (std::numeric_limits<double>::is_iec559) {
		// Test expo with float-float arguments.
		BOOST_CHECK(p_type1{2.}.pow(0.5) == std::pow(2.,0.5));
		BOOST_CHECK(p_type1{3.}.pow(-0.5) == std::pow(3.,-0.5));
		BOOST_CHECK_THROW(math::pow(p_type1{"x"} + 1,0.5),std::invalid_argument);
	}
	// Check division by zero error.
	typedef g_series_type<rational,int> p_type2;
	BOOST_CHECK_THROW(math::pow(p_type2{},-1),zero_division_error);
	// Check the safe_cast mechanism.
	typedef g_series_type<real,int> p_type3;
	auto p = p_type3{"x"} + 1;
	BOOST_CHECK_EQUAL(p.pow(3),p.pow(real{3}));
	BOOST_CHECK_THROW(p.pow(real{-3}),std::invalid_argument);
	BOOST_CHECK_THROW(p.pow(real{"1.5"}),std::invalid_argument);
	if (std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::radix == 2) {
		auto p = p_type1{"x"} + 1;
		BOOST_CHECK_EQUAL(p.pow(3),p.pow(3.));
		BOOST_CHECK_THROW(p.pow(-3.),std::invalid_argument);
		BOOST_CHECK_THROW(p.pow(1.5),std::invalid_argument);
	}
	BOOST_CHECK((is_exponentiable<p_type1,double>::value));
	BOOST_CHECK((is_exponentiable<const p_type1,double>::value));
	BOOST_CHECK((is_exponentiable<p_type1 &,double>::value));
	BOOST_CHECK((is_exponentiable<p_type1 &,double &>::value));
	BOOST_CHECK((is_exponentiable<const p_type1 &,double &>::value));
	BOOST_CHECK((is_exponentiable<p_type1,integer>::value));
	BOOST_CHECK((!is_exponentiable<p_type1,std::string>::value));
	BOOST_CHECK((!is_exponentiable<p_type1 &,std::string>::value));
	BOOST_CHECK((!is_exponentiable<p_type1 &,std::string &>::value));
	BOOST_CHECK((is_exponentiable<p_type1,fake_int_01>::value));
	BOOST_CHECK((!is_exponentiable<p_type1,fake_int_02>::value));
	// These are a couple of checks for the new pow() code, which is now able to deal with
	// exponentiation creating different types of coefficients.
	BOOST_CHECK((is_exponentiable<g_series_type<short,int>,int>::value));
	BOOST_CHECK((is_exponentiable<g_series_type<int,int>,int>::value));
	BOOST_CHECK((std::is_same<decltype(g_series_type<short,int>{}.pow(3)),g_series_type<integer,int>>::value));
	BOOST_CHECK((std::is_same<decltype(g_series_type<int,int>{}.pow(3)),g_series_type<integer,int>>::value));
	BOOST_CHECK_EQUAL((g_series_type<int,int>{"x"}.pow(2)),(g_series_type<integer,int>{"x"} * g_series_type<integer,int>{"x"}));
	BOOST_CHECK((std::is_same<decltype(g_series_type<int,int>{}.pow(3.)),g_series_type<double,int>>::value));
	BOOST_CHECK_EQUAL((g_series_type<int,int>{"x"}.pow(2.)),(g_series_type<double,int>{"x"} * g_series_type<integer,int>{"x"}));
	BOOST_CHECK((std::is_same<decltype(g_series_type<real,int>{}.pow(3.)),g_series_type<real,int>>::value));
	BOOST_CHECK_EQUAL((g_series_type<real,int>{"x"}.pow(2.)),(g_series_type<real,int>{"x"} * g_series_type<real,int>{"x"}));
	BOOST_CHECK((std::is_same<decltype(g_series_type<rational,int>{}.pow(3_z)),g_series_type<rational,int>>::value));
	BOOST_CHECK_EQUAL((g_series_type<rational,int>{"x"}.pow(2_z)),(g_series_type<rational,int>{"x"} * g_series_type<rational,int>{"x"}));
	// Some multi-threaded testing.
	p_type1 ret0, ret1;
	std::thread t0([&ret0]() {
		p_type1 x{"x"};
		auto tmp = x.pow(6);
		// Throw in a cache clear for good measure.
		p_type1::clear_pow_cache();
		ret0 = tmp.pow(8);
		p_type1::clear_pow_cache();
	});
	std::thread t1([&ret1]() {
		p_type1 x{"x"};
		auto tmp = x.pow(5);
		p_type1::clear_pow_cache();
		ret1 = tmp.pow(8);
		p_type1::clear_pow_cache();
	});
	t0.join();
	t1.join();
	BOOST_CHECK_EQUAL(ret0,p_type1{"x"}.pow(6).pow(8));
	BOOST_CHECK_EQUAL(ret1,p_type1{"x"}.pow(5).pow(8));
	// Clear the caches.
	p_type1::clear_pow_cache();
	p_type2::clear_pow_cache();
	p_type3::clear_pow_cache();
}

BOOST_AUTO_TEST_CASE(series_is_single_coefficient_test)
{
	typedef g_series_type<integer,int> p_type;
	BOOST_CHECK(p_type{}.is_single_coefficient());
	BOOST_CHECK(p_type{1}.is_single_coefficient());
	BOOST_CHECK(!p_type{"x"}.is_single_coefficient());
	BOOST_CHECK(!(3 * p_type{"x"}).is_single_coefficient());
	BOOST_CHECK(!(1 + p_type{"x"}).is_single_coefficient());
}

// Mock coefficient.
struct mock_cf
{
	mock_cf();
	mock_cf(const int &);
	mock_cf(const mock_cf &);
	mock_cf(mock_cf &&) noexcept;
	mock_cf &operator=(const mock_cf &);
	mock_cf &operator=(mock_cf &&) noexcept;
	friend std::ostream &operator<<(std::ostream &, const mock_cf &);
	mock_cf operator-() const;
	bool operator==(const mock_cf &) const;
	bool operator!=(const mock_cf &) const;
	mock_cf &operator+=(const mock_cf &);
	mock_cf &operator-=(const mock_cf &);
	mock_cf operator+(const mock_cf &) const;
	mock_cf operator-(const mock_cf &) const;
	mock_cf &operator*=(const mock_cf &);
	mock_cf operator*(const mock_cf &) const;
};

// Another mock cf, with valid sin/cos specialisations which change the type.
struct mock_cf2
{
	mock_cf2();
	mock_cf2(const int &);
	mock_cf2(const mock_cf2 &);
	mock_cf2(mock_cf2 &&) noexcept;
	mock_cf2 &operator=(const mock_cf2 &);
	mock_cf2 &operator=(mock_cf2 &&) noexcept;
	friend std::ostream &operator<<(std::ostream &, const mock_cf2 &);
	mock_cf2 operator-() const;
	bool operator==(const mock_cf2 &) const;
	bool operator!=(const mock_cf2 &) const;
	mock_cf2 &operator+=(const mock_cf2 &);
	mock_cf2 &operator-=(const mock_cf2 &);
	mock_cf2 operator+(const mock_cf2 &) const;
	mock_cf2 operator-(const mock_cf2 &) const;
	mock_cf2 &operator*=(const mock_cf2 &);
	mock_cf2 operator*(const mock_cf2 &) const;
};

namespace piranha { namespace math {

// Provide mock sine/cosine implementation returning unusable return type.
template <typename T>
struct sin_impl<T,typename std::enable_if<std::is_same<T,mock_cf>::value>::type>
{
	std::string operator()(const T &) const;
};

template <typename T>
struct cos_impl<T,typename std::enable_if<std::is_same<T,mock_cf>::value>::type>
{
	std::string operator()(const T &) const;
};

// Sin/cos of mock_cf2 will return mock_cf.
template <typename T>
struct sin_impl<T,typename std::enable_if<std::is_same<T,mock_cf2>::value>::type>
{
	mock_cf operator()(const T &) const;
};

template <typename T>
struct cos_impl<T,typename std::enable_if<std::is_same<T,mock_cf2>::value>::type>
{
	mock_cf operator()(const T &) const;
};

}}

// NOTE: here:
// - g_series_type has a wrong sin() overload but a good cos() one, and g_series_type2 have suitable sin/cos members,
// - g_series_type3 has no members,
// - g_series_type4 has good members.
BOOST_AUTO_TEST_CASE(series_sin_cos_test)
{
	typedef g_series_type<double,int> p_type1;
	// What happens here:
	// - p_type1 has math::sin() via its coefficient type,
	// - g_series_type<mock_cf,int> has no sine because math::sin() on mock_cf is wrong,
	// - math::cos() on p_type1 returns the -42 value from the method.
	BOOST_CHECK(has_sine<p_type1>::value);
	BOOST_CHECK(has_cosine<p_type1>::value);
	BOOST_CHECK((!has_sine<g_series_type<mock_cf,int>>::value));
	BOOST_CHECK((has_cosine<g_series_type<mock_cf,int>>::value));
	BOOST_CHECK_EQUAL(math::sin(p_type1{.5}),math::sin(.5));
	BOOST_CHECK_EQUAL(math::cos(p_type1{.5}),-42);
	BOOST_CHECK_THROW(math::sin(p_type1{"x"}),std::invalid_argument);
	BOOST_CHECK_THROW(math::sin(p_type1{"x"} + 1),std::invalid_argument);
	BOOST_CHECK_EQUAL(math::cos(p_type1{"x"}),-42);
	BOOST_CHECK_EQUAL(math::cos(p_type1{"x"} - 1),-42);
	typedef g_series_type2<double,int> p_type2;
	BOOST_CHECK(has_sine<p_type2>::value);
	BOOST_CHECK(has_cosine<p_type2>::value);
	BOOST_CHECK_EQUAL(math::sin(p_type2{.5}),double(42));
	BOOST_CHECK_EQUAL(math::cos(p_type2{.5}),double(-42));
	typedef g_series_type2<p_type2,int> p_type3;
	BOOST_CHECK(has_sine<p_type3>::value);
	BOOST_CHECK(has_cosine<p_type3>::value);
	BOOST_CHECK_EQUAL(math::sin(p_type3{.5}),double(42));
	BOOST_CHECK_EQUAL(math::cos(p_type3{.5}),double(-42));
	typedef g_series_type<mock_cf2,int> p_type4;
	BOOST_CHECK(has_sine<p_type4>::value);
	BOOST_CHECK(has_cosine<p_type4>::value);
	BOOST_CHECK((std::is_same<decltype(math::sin(p_type4{})),g_series_type<mock_cf,int>>::value));
	BOOST_CHECK((std::is_same<decltype(math::cos(p_type4{})),int>::value));
	typedef g_series_type<mock_cf2,int> p_type4;
	BOOST_CHECK(has_sine<p_type4>::value);
	BOOST_CHECK(has_cosine<p_type4>::value);
	BOOST_CHECK((std::is_same<decltype(math::sin(p_type4{})),g_series_type<mock_cf,int>>::value));
	BOOST_CHECK((std::is_same<decltype(math::cos(p_type4{})),int>::value));
	typedef g_series_type3<mock_cf2,int> p_type5;
	BOOST_CHECK(has_sine<p_type5>::value);
	BOOST_CHECK(has_cosine<p_type5>::value);
	BOOST_CHECK((std::is_same<decltype(math::sin(p_type5{})),g_series_type3<mock_cf,int>>::value));
	BOOST_CHECK((std::is_same<decltype(math::cos(p_type5{})),g_series_type3<mock_cf,int>>::value));
	// Check that casting a series type to its base type and then calling sin/cos still gets out the original
	// type. Test with series with and without members.
	typedef g_series_type3<double,int> p_type6;
	BOOST_CHECK(has_sine<p_type6>::value);
	BOOST_CHECK(has_cosine<p_type6>::value);
	BOOST_CHECK((std::is_same<p_type6,decltype(math::sin(std::declval<const p_type6::base &>()))>::value));
	BOOST_CHECK((std::is_same<p_type6,decltype(math::cos(std::declval<const p_type6::base &>()))>::value));
	typedef g_series_type4<double,int> p_type7;
	BOOST_CHECK(has_sine<p_type7>::value);
	BOOST_CHECK(has_cosine<p_type7>::value);
	BOOST_CHECK((std::is_same<p_type7,decltype(math::sin(std::declval<const p_type7::base &>()))>::value));
	BOOST_CHECK((std::is_same<p_type7,decltype(math::cos(std::declval<const p_type7::base &>()))>::value));
	// Test also with bad members.
	BOOST_CHECK((std::is_same<p_type1,decltype(math::sin(std::declval<const p_type1::base &>()))>::value));
	BOOST_CHECK((std::is_same<p_type1,decltype(math::cos(std::declval<const p_type1::base &>()))>::value));
}

BOOST_AUTO_TEST_CASE(series_iterator_test)
{
	typedef g_series_type<rational,int> p_type1;
	p_type1 empty;
	BOOST_CHECK(empty.begin() == empty.end());
	p_type1 x{"x"};
	typedef std::decay<decltype(*(x.begin()))>::type pair_type;
	BOOST_CHECK((std::is_same<pair_type,std::pair<rational,p_type1>>::value));
	x *= 2;
	auto it = x.begin();
	BOOST_CHECK_EQUAL(it->first,2);
	BOOST_CHECK((std::is_same<typename p_type1::term_type::cf_type,decltype(it->first)>::value));
	BOOST_CHECK_EQUAL(it->second,p_type1{"x"});
	BOOST_CHECK((std::is_same<p_type1,decltype(it->second)>::value));
	++it;
	BOOST_CHECK(it == x.end());
	x /= 2;
	p_type1 p1 = x + p_type1{"y"} + p_type1{"z"};
	p1 *= 3;
	it = p1.begin();
	BOOST_CHECK(it != p1.end());
	BOOST_CHECK_EQUAL(it->first,3);
	++it;
	BOOST_CHECK(it != p1.end());
	BOOST_CHECK_EQUAL(it->first,3);
	++it;
	BOOST_CHECK(it != p1.end());
	BOOST_CHECK_EQUAL(it->first,3);
	++it;
	BOOST_CHECK(it == p1.end());
}

BOOST_AUTO_TEST_CASE(series_filter_test)
{
	typedef g_series_type<rational,int> p_type1;
	p_type1 x{"x"}, y{"y"}, z{"z"};
	typedef std::decay<decltype(*(x.begin()))>::type pair_type;
	BOOST_CHECK_EQUAL(x,x.filter([](const pair_type &) {return true;}));
	BOOST_CHECK(x.filter([](const pair_type &) {return false;}).empty());
	BOOST_CHECK_EQUAL(x,(x + 2 * y).filter([](const pair_type &p) {return p.first < 2;}));
	BOOST_CHECK_EQUAL(x + 2 * y,(x + 2 * y).filter([](const pair_type &p) {return p.second.size();}));
	BOOST_CHECK_EQUAL(0,(x + 2 * y).filter([](const pair_type &p) {return p.second.size() == 0u;}));
	BOOST_CHECK_EQUAL(-y,(x - y + 3).filter([](const pair_type &p) {return p.first < 0;}));
	BOOST_CHECK_EQUAL(-y - 3,(x - y - 3).filter([](const pair_type &p) {return p.first < 0;}));
	BOOST_CHECK_EQUAL(x,(x - y - 3).filter([](const pair_type &p) {return p.first > 0;}));
}

BOOST_AUTO_TEST_CASE(series_transform_test)
{
	typedef g_series_type<rational,int> p_type1;
	p_type1 x{"x"}, y{"y"};
	typedef std::decay<decltype(*(x.begin()))>::type pair_type;
	BOOST_CHECK_EQUAL(x,x.transform([](const pair_type &p) {return p;}));
	BOOST_CHECK_EQUAL(0,x.transform([](const pair_type &) {return pair_type();}));
	BOOST_CHECK_EQUAL(rational(1,2),x.transform([](const pair_type &) {return pair_type(rational(1,2),p_type1(1));}));
	BOOST_CHECK_EQUAL(2 * (x + y),(x + y).transform([](const pair_type &p) {return pair_type(p.first * 2,p.second);}));
	typedef g_series_type<p_type1,int> p_type2;
	p_type2 y2{"y"};
	y2 *= (x + 2);
	y2 += p_type2{"x"};
	typedef std::decay<decltype(*(y2.begin()))>::type pair_type2;
	BOOST_CHECK_EQUAL(y2.transform([](const pair_type2 &p) {
		return std::make_pair(p.first.filter([](const pair_type &q) {return q.first < 2;}),p.second);
	}),p_type2{"y"} * x + p_type2{"x"});
}

struct print_tex_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			// Avoid the stream tests with floating-point and similar, because of messy output.
			if (std::is_same<Cf,double>::value || std::is_same<Cf,real>::value) {
				return;
			}
			typedef g_series_type<Cf,Expo> p_type1;
			typedef g_series_type<p_type1,Expo> p_type11;
			std::ostringstream oss;
			p_type1{}.print_tex(oss);
			BOOST_CHECK(oss.str() == "0");
			oss.str("");
			p_type1{1}.print_tex(oss);
			BOOST_CHECK(oss.str() == "1");
			oss.str("");
			p_type1{-1}.print_tex(oss);
			BOOST_CHECK(oss.str() == "-1");
			oss.str("");
			p_type1{"x"}.print_tex(oss);
			BOOST_CHECK(oss.str() == "{x}");
			oss.str("");
			(-p_type1{"x"}).print_tex(oss);
			BOOST_CHECK(oss.str() == "-{x}");
			oss.str("");
			(-p_type1{"x"} * p_type1{"y"}.pow(2)).print_tex(oss);
			BOOST_CHECK(oss.str() == "-{x}{y}^{2}");
			oss.str("");
			(-p_type1{"x"} + 1).print_tex(oss);
			BOOST_CHECK(oss.str() == "1-{x}" || oss.str() == "-{x}+1");
			oss.str("");
			p_type11{}.print_tex(oss);
			BOOST_CHECK(oss.str() == "0");
			oss.str("");
			p_type11{"x"}.print_tex(oss);
			BOOST_CHECK(oss.str() == "{x}");
			oss.str("");
			(-3 * p_type11{"x"}.pow(2)).print_tex(oss);
			BOOST_CHECK(oss.str() == "-3{x}^{2}");
			oss.str("");
			(p_type11{1}).print_tex(oss);
			BOOST_CHECK(oss.str() == "1");
			oss.str("");
			(p_type11{-1}).print_tex(oss);
			BOOST_CHECK(oss.str() == "-1");
			oss.str("");
			(p_type11{"x"} * p_type11{"y"}).print_tex(oss);
			BOOST_CHECK(oss.str() == "{x}{y}");
			oss.str("");
			(-p_type11{"x"} * p_type11{"y"}).print_tex(oss);
			BOOST_CHECK(oss.str() == "-{x}{y}");
			oss.str("");
			(-p_type11{"x"} + 1).print_tex(oss);
			BOOST_CHECK(oss.str() == "1-{x}" || oss.str() == "-{x}+1");
			oss.str("");
			(p_type11{"x"} - 1).print_tex(oss);
			BOOST_CHECK(oss.str() == "{x}-1" || oss.str() == "-1+{x}");
			// Test wih less term output.
			settings::set_max_term_output(3u);
			oss.str("");
			p_type11{}.print_tex(oss);
			BOOST_CHECK(oss.str() == "0");
			oss.str("");
			p_type11{"x"}.print_tex(oss);
			BOOST_CHECK(oss.str() == "{x}");
			oss.str("");
			(-p_type11{"x"}).print_tex(oss);
			BOOST_CHECK(oss.str() == "-{x}");
			oss.str("");
			(p_type11{1}).print_tex(oss);
			BOOST_CHECK(oss.str() == "1");
			oss.str("");
			(p_type11{-1}).print_tex(oss);
			BOOST_CHECK(oss.str() == "-1");
			oss.str("");
			(p_type11{"x"} * p_type11{"y"}).print_tex(oss);
			BOOST_CHECK(oss.str() == "{x}{y}");
			oss.str("");
			(-p_type11{"x"} * p_type11{"y"}).print_tex(oss);
			BOOST_CHECK(oss.str() == "-{x}{y}");
			// Test wih little term output.
			typedef polynomial<Cf,monomial<Expo>> poly_type;
			settings::set_max_term_output(1u);
			oss.str("");
			(-3 * poly_type{"x"} + 1 + poly_type{"x"} * poly_type{"x"} + poly_type{"x"} * poly_type{"x"} * poly_type{"x"}).print_tex(oss);
			const std::string tmp_out = oss.str(), tmp_cmp = "\\ldots";
			BOOST_CHECK(std::equal(tmp_cmp.rbegin(),tmp_cmp.rend(),tmp_out.rbegin()));
			oss.str("");
			poly_type{}.print_tex(oss);
			BOOST_CHECK_EQUAL(oss.str(),"0");
			settings::reset_max_term_output();
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(series_print_tex_test)
{
	boost::mpl::for_each<cf_types>(print_tex_tester());
}

struct trim_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			if (std::is_same<Cf,double>::value) {
				return;
			}
			typedef g_series_type<Cf,Expo> p_type1;
			typedef g_series_type<p_type1,Expo> p_type11;
			p_type1 x{"x"}, y{"y"};
			BOOST_CHECK_EQUAL((1 + x - x).trim().get_symbol_set().size(),0u);
			BOOST_CHECK_EQUAL((1 + x*y - y*x + x).trim().get_symbol_set().size(),1u);
			BOOST_CHECK_EQUAL((1 + x*y - y*x + x + y).trim().get_symbol_set().size(),2u);
			p_type11 xx(x), yy(y);
			BOOST_CHECK_EQUAL(((1 + xx) - xx).begin()->first.get_symbol_set().size(),1u);
			BOOST_CHECK_EQUAL(((1 + xx) - xx).trim().begin()->first.get_symbol_set().size(),0u);
			BOOST_CHECK_EQUAL(((1 + xx*yy) - xx*yy + xx).trim().begin()->first.get_symbol_set().size(),1u);
			BOOST_CHECK_EQUAL(((1 + xx*yy) - xx*yy + xx + yy).trim().begin()->first.get_symbol_set().size(),2u);
			BOOST_CHECK_EQUAL((1+x*xx + y*yy - x*xx).trim().begin()->first.get_symbol_set().size(),1u);
			BOOST_CHECK_EQUAL((1+x*p_type11{"x"} + y*p_type11{"y"} - x*p_type11{"x"}).trim().get_symbol_set().size(),1u);
			BOOST_CHECK_EQUAL((((1+x).pow(5) + y) - y).trim(),(1+x).pow(5));
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(series_trim_test)
{
	boost::mpl::for_each<cf_types>(trim_tester());
}

struct is_zero_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef g_series_type<Cf,Expo> p_type1;
			typedef g_series_type<p_type1,Expo> p_type11;
			BOOST_CHECK(has_is_zero<p_type1>::value);
			BOOST_CHECK(has_is_zero<p_type11>::value);
			BOOST_CHECK(math::is_zero(p_type1{}));
			BOOST_CHECK(math::is_zero(p_type11{}));
			BOOST_CHECK(math::is_zero(p_type1{0}));
			BOOST_CHECK(math::is_zero(p_type11{0}));
			BOOST_CHECK(!math::is_zero(p_type1{1}));
			BOOST_CHECK(!math::is_zero(p_type11{1}));
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(series_is_zero_test)
{
	boost::mpl::for_each<cf_types>(is_zero_tester());
}

struct type_traits_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef g_series_type<Cf,Expo> p_type1;
			typedef g_series_type<p_type1,Expo> p_type11;
			BOOST_CHECK(is_series<p_type1>::value);
			BOOST_CHECK(is_series<p_type11>::value);
			BOOST_CHECK(!is_series<p_type1 &>::value);
			BOOST_CHECK(!is_series<p_type11 const &>::value);
			BOOST_CHECK(is_equality_comparable<p_type1>::value);
			BOOST_CHECK((is_equality_comparable<p_type1,Cf>::value));
			BOOST_CHECK((is_equality_comparable<Cf,p_type1>::value));
			BOOST_CHECK((!is_equality_comparable<p_type1,std::string>::value));
			BOOST_CHECK(is_equality_comparable<p_type11>::value);
			BOOST_CHECK((is_equality_comparable<p_type11,p_type1>::value));
			BOOST_CHECK((is_equality_comparable<p_type1,p_type11>::value));
			BOOST_CHECK((is_instance_of<p_type1,series>::value));
			BOOST_CHECK((is_instance_of<p_type11,series>::value));
			BOOST_CHECK(is_ostreamable<p_type1>::value);
			BOOST_CHECK(is_ostreamable<p_type11>::value);
			BOOST_CHECK(is_container_element<p_type1>::value);
			BOOST_CHECK(is_container_element<p_type11>::value);
			BOOST_CHECK(!is_less_than_comparable<p_type1>::value);
			BOOST_CHECK((!is_less_than_comparable<p_type1,int>::value));
			BOOST_CHECK(!is_less_than_comparable<p_type11>::value);
			BOOST_CHECK((!is_less_than_comparable<p_type11,int>::value));
			BOOST_CHECK((!is_less_than_comparable<p_type11,p_type1>::value));
			BOOST_CHECK(is_addable<p_type1>::value);
			BOOST_CHECK((is_addable<p_type1,int>::value));
			BOOST_CHECK((is_addable<int,p_type1>::value));
			BOOST_CHECK(is_addable<p_type11>::value);
			BOOST_CHECK((is_addable<p_type11,int>::value));
			BOOST_CHECK((is_addable<int,p_type11>::value));
			BOOST_CHECK((is_addable<p_type11,p_type1>::value));
			BOOST_CHECK(is_addable_in_place<p_type1>::value);
			BOOST_CHECK((is_addable_in_place<p_type1,int>::value));
			BOOST_CHECK(is_addable_in_place<p_type11>::value);
			BOOST_CHECK((is_addable_in_place<p_type11,int>::value));
			BOOST_CHECK((is_addable_in_place<p_type11,p_type1>::value));
			BOOST_CHECK(is_subtractable<p_type1>::value);
			BOOST_CHECK((is_subtractable<p_type1,int>::value));
			BOOST_CHECK((is_subtractable<int,p_type1>::value));
			BOOST_CHECK(is_subtractable<p_type11>::value);
			BOOST_CHECK((is_subtractable<p_type11,int>::value));
			BOOST_CHECK((is_subtractable<int,p_type11>::value));
			BOOST_CHECK((is_subtractable<p_type11,p_type1>::value));
			BOOST_CHECK(is_subtractable_in_place<p_type1>::value);
			BOOST_CHECK((is_subtractable_in_place<p_type1,int>::value));
			BOOST_CHECK(is_subtractable_in_place<p_type11>::value);
			BOOST_CHECK((is_subtractable_in_place<p_type11,int>::value));
			BOOST_CHECK((is_subtractable_in_place<p_type11,p_type1>::value));
			BOOST_CHECK(has_print_coefficient<p_type1>::value);
			BOOST_CHECK(has_print_coefficient<p_type11>::value);
			BOOST_CHECK(has_print_tex_coefficient<p_type1>::value);
			BOOST_CHECK(has_print_tex_coefficient<p_type11>::value);
			BOOST_CHECK((std::is_same<void,decltype(print_coefficient(*(std::ostream *)nullptr,std::declval<p_type1>()))>::value));
			BOOST_CHECK((std::is_same<void,decltype(print_coefficient(*(std::ostream *)nullptr,std::declval<p_type11>()))>::value));
			BOOST_CHECK((std::is_same<void,decltype(print_tex_coefficient(*(std::ostream *)nullptr,std::declval<p_type1>()))>::value));
			BOOST_CHECK((std::is_same<void,decltype(print_tex_coefficient(*(std::ostream *)nullptr,std::declval<p_type11>()))>::value));
			BOOST_CHECK(has_negate<p_type1>::value);
			BOOST_CHECK(has_negate<p_type1 &>::value);
			BOOST_CHECK(!has_negate<const p_type1 &>::value);
			BOOST_CHECK(!has_negate<const p_type1>::value);
			BOOST_CHECK((std::is_same<decltype(math::negate(*(p_type1 *)nullptr)),void>::value));
			BOOST_CHECK(has_negate<p_type11>::value);
			BOOST_CHECK(has_negate<p_type11 &>::value);
			BOOST_CHECK(!has_negate<const p_type11 &>::value);
			BOOST_CHECK(!has_negate<const p_type11>::value);
			BOOST_CHECK((std::is_same<decltype(math::negate(*(p_type11 *)nullptr)),void>::value));
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(series_type_traits_test)
{
	boost::mpl::for_each<cf_types>(type_traits_tester());
	BOOST_CHECK(!is_series<int>::value);
	BOOST_CHECK(!is_series<double>::value);
	BOOST_CHECK(!is_series<void>::value);
}

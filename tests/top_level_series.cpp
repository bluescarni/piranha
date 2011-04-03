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

#include "../src/top_level_series.hpp"

#define BOOST_TEST_MODULE top_level_series_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <string>
#include <tuple>

#include "../src/config.hpp"
#include "../src/debug_access.hpp"
#include "../src/integer.hpp"
#include "../src/numerical_coefficient.hpp"
#include "../src/polynomial_term.hpp"
#include "../src/symbol.hpp"

using namespace piranha;

typedef boost::mpl::vector<numerical_coefficient<double>,numerical_coefficient<integer>> cf_types;
typedef boost::mpl::vector<unsigned,integer> expo_types;

template <typename Cf, typename Expo>
class polynomial:
	public top_level_series<polynomial_term<Cf,Expo>,polynomial<Cf,Expo>>
{
		typedef top_level_series<polynomial_term<Cf,Expo>,polynomial<Cf,Expo>> base;
	public:
		polynomial() = default;
		polynomial(const polynomial &) = default;
		polynomial(polynomial &&) = default;
		explicit polynomial(const std::string &name):base()
		{
			typedef typename base::term_type term_type;
			// Insert the symbol.
			this->m_ed.template add_symbol<term_type>(symbol(name));
			// Construct and insert the term.
			this->insert(term_type(1,typename term_type::key_type{Expo(1)}),this->m_ed);
		}
		~polynomial() = default;
		polynomial &operator=(const polynomial &) = default;
		polynomial &operator=(polynomial &&other) piranha_noexcept_spec(true)
		{
			base::operator=(std::move(other));
			return *this;
		}
};

struct constructor_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef polynomial<Cf,Expo> p_type;
			// Default construction.
			p_type p1;
			BOOST_CHECK_EQUAL(p1.size(),unsigned(0));
			p_type x("x");
			BOOST_CHECK_EQUAL(x.size(),unsigned(1));
			// Copy construction.
			p_type x2 = x;
			BOOST_CHECK_EQUAL(x2.size(),unsigned(1));
			// Move construction.
			p_type x3 = std::move(x2);
			BOOST_CHECK(x2.empty());
			BOOST_CHECK_EQUAL(x3.size(),unsigned(1));
			// Copy assignment.
			x3 = x;
			BOOST_CHECK_EQUAL(x3.size(),unsigned(1));
			// Revive moved-from object.
			x2 = x;
			BOOST_CHECK_EQUAL(x2.size(),unsigned(1));
			// Move assignment.
			x2 = std::move(x);
			BOOST_CHECK_EQUAL(x2.size(),unsigned(1));
			BOOST_CHECK(x.empty());
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(top_level_series_constructors_test)
{
	boost::mpl::for_each<cf_types>(constructor_tester());
}

template <typename Cf, typename Expo>
class polynomial2:
	public top_level_series<polynomial_term<Cf,Expo>,polynomial2<Cf,Expo>>
{
		typedef top_level_series<polynomial_term<Cf,Expo>,polynomial2<Cf,Expo>> base;
	public:
		polynomial2() = default;
		polynomial2(const polynomial2 &) = default;
		polynomial2(polynomial2 &&) = default;
		explicit polynomial2(const std::string &name):base()
		{
			typedef typename base::term_type term_type;
			// Insert the symbol.
			this->m_ed.template add_symbol<term_type>(symbol(name));
			// Construct and insert the term.
			this->insert(term_type(1,typename term_type::key_type{Expo(1)}),this->m_ed);
		}
		~polynomial2() = default;
		polynomial2 &operator=(const polynomial2 &) = default;
		polynomial2 &operator=(polynomial2 &&other) piranha_noexcept_spec(true)
		{
			base::operator=(std::move(other));
			return *this;
		}
};

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
				typedef polynomial<Cf,Expo> p_type1;
				typedef polynomial2<Cf,Expo> p_type2;
				// In-place addition.
				p_type1 p1;
				p1 += 1;
				p1 += 1.;
				BOOST_CHECK(!p1.empty());
				BOOST_CHECK(p1.m_container.begin()->m_cf.get_value() == typename Cf::type(1) + typename Cf::type(1.));
				p_type2 p2;
				p2 += 1;
				p2 += 1.;
				p1 += p2;
				BOOST_CHECK(!p1.empty());
				BOOST_CHECK(p1.m_container.begin()->m_cf.get_value() == typename Cf::type(1) + typename Cf::type(1.) + typename Cf::type(1) + typename Cf::type(1.));
				p1 -= p1;
				BOOST_CHECK(p1.empty());
				p1 += std::move(p2);
				BOOST_CHECK(!p1.empty());
				BOOST_CHECK(p1.m_container.begin()->m_cf.get_value() == typename Cf::type(1) + typename Cf::type(1.));
				BOOST_CHECK(p2.empty());
				p1 = p_type1("x");
				p2 = p_type2("y");
				p1 += p2;
				BOOST_CHECK_EQUAL(p1.size(),2u);
				BOOST_CHECK_EQUAL(std::get<0>(p1.m_ed.get_args_tuple()).size(),2u);
				BOOST_CHECK_EQUAL(std::get<0>(p1.m_ed.get_args_tuple())[0],symbol("x"));
				BOOST_CHECK_EQUAL(std::get<0>(p1.m_ed.get_args_tuple())[1],symbol("y"));
				p1 += p2;
				BOOST_CHECK_EQUAL(p1.size(),2u);
				auto it1 = p1.m_container.begin();
				BOOST_CHECK(it1->m_cf.get_value() == typename Cf::type(1) || it1->m_cf.get_value() == typename Cf::type(2));
				++it1;
				BOOST_CHECK(it1->m_cf.get_value() == typename Cf::type(1) || it1->m_cf.get_value() == typename Cf::type(2));
				p2 += std::move(p1);
				auto it2 = p2.m_container.begin();
				BOOST_CHECK(it2->m_cf.get_value() == typename Cf::type(1) || it2->m_cf.get_value() == typename Cf::type(3));
				++it2;
				BOOST_CHECK(it2->m_cf.get_value() == typename Cf::type(1) || it2->m_cf.get_value() == typename Cf::type(3));
				// In-place subtraction.
				p1 = p_type1{};
				p1 -= 1;
				p1 -= 1.;
				BOOST_CHECK(!p1.empty());
				BOOST_CHECK(p1.m_container.begin()->m_cf.get_value() == typename Cf::type(-1) - typename Cf::type(1.));
				p2 = p_type2{};
				p2 -= 1;
				p2 -= 1.;
				p1 += p2;
				BOOST_CHECK(!p1.empty());
				BOOST_CHECK(p1.m_container.begin()->m_cf.get_value() == typename Cf::type(-1) - typename Cf::type(1.) - typename Cf::type(1) - typename Cf::type(1.));
				p1 -= p1;
				BOOST_CHECK(p1.empty());
				p1 -= std::move(p2);
				BOOST_CHECK(!p1.empty());
				BOOST_CHECK(p1.m_container.begin()->m_cf.get_value() == typename Cf::type(1) + typename Cf::type(1.));
				BOOST_CHECK(p2.empty());
				p1 = p_type1("x");
				p2 = p_type2("y");
				p1 -= p2;
				BOOST_CHECK_EQUAL(p1.size(),2u);
				BOOST_CHECK_EQUAL(std::get<0>(p1.m_ed.get_args_tuple()).size(),2u);
				BOOST_CHECK_EQUAL(std::get<0>(p1.m_ed.get_args_tuple())[0],symbol("x"));
				BOOST_CHECK_EQUAL(std::get<0>(p1.m_ed.get_args_tuple())[1],symbol("y"));
				p1 -= p2;
				BOOST_CHECK_EQUAL(p1.size(),2u);
				it1 = p1.m_container.begin();
				BOOST_CHECK(it1->m_cf.get_value() == typename Cf::type(1) || it1->m_cf.get_value() == typename Cf::type(-2));
				++it1;
				BOOST_CHECK(it1->m_cf.get_value() == typename Cf::type(1) || it1->m_cf.get_value() == typename Cf::type(-2));
				p2 -= std::move(p1);
				it2 = p2.m_container.begin();
				BOOST_CHECK(it2->m_cf.get_value() == typename Cf::type(-1) || it2->m_cf.get_value() == typename Cf::type(3));
				++it2;
				BOOST_CHECK(it2->m_cf.get_value() == typename Cf::type(-1) || it2->m_cf.get_value() == typename Cf::type(3));
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

BOOST_AUTO_TEST_CASE(top_level_series_arithmetics_test)
{
	boost::mpl::for_each<cf_types>(arithmetics_tester());
}

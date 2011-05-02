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
#include <type_traits>

#include "../src/config.hpp"
#include "../src/debug_access.hpp"
#include "../src/integer.hpp"
#include "../src/math.hpp"
#include "../src/numerical_coefficient.hpp"
#include "../src/polynomial_term.hpp"
#include "../src/symbol.hpp"
#include "../src/type_traits.hpp"

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
		explicit polynomial(const char *name):base()
		{
			typedef typename base::term_type term_type;
			// Insert the symbol.
			this->m_ed.template add_symbol<term_type>(symbol(name));
			// Construct and insert the term.
			this->insert(term_type(Cf(1,this->m_ed),typename term_type::key_type{Expo(1)}),this->m_ed);
		}
		template <typename T>
		explicit polynomial(T &&name, typename std::enable_if<std::is_same<std::string,typename strip_cv_ref<T>::type>::value>::type * = piranha_nullptr):base()
		{
			typedef typename base::term_type term_type;
			// Insert the symbol.
			this->m_ed.template add_symbol<term_type>(symbol(name));
			// Construct and insert the term.
			this->insert(term_type(Cf(1,this->m_ed),typename term_type::key_type{Expo(1)}),this->m_ed);
		}
		template <typename... Args>
		explicit polynomial(Args && ... params):base(std::forward<Args>(params)...) {}
		~polynomial() = default;
		polynomial &operator=(const polynomial &) = default;
		polynomial &operator=(polynomial &&other) piranha_noexcept_spec(true)
		{
			base::operator=(std::move(other));
			return *this;
		}
		template <typename T>
		polynomial &operator=(T &&x)
		{
			base::operator=(std::forward<T>(x));
			return *this;
		}
};

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
			this->insert(term_type(Cf(1,this->m_ed),typename term_type::key_type{Expo(1)}),this->m_ed);
		}
		~polynomial2() = default;
		polynomial2 &operator=(const polynomial2 &) = default;
		polynomial2 &operator=(polynomial2 &&other) piranha_noexcept_spec(true)
		{
			base::operator=(std::move(other));
			return *this;
		}
		template <typename T>
		polynomial2 &operator=(T &&x)
		{
			base::operator=(std::forward<T>(x));
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
			p_type x(std::string("x"));
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
			// Generic construction tests.
			p_type x4(0);
			BOOST_CHECK(x4.empty());
			p_type x5(1);
			BOOST_CHECK_EQUAL(x5.size(),1u);
			p_type x6(integer(10));
			BOOST_CHECK_EQUAL(x6.size(),1u);
			x6 -= integer(10);
			BOOST_CHECK(x6.empty());
			// Construction from different series type, but same term_type.
			polynomial2<Cf,Expo> y0;
			y0 += 1;
			p_type x7(y0);
			x7 -= 1;
			BOOST_CHECK(x7.empty());
			p_type x7a(std::move(y0));
			BOOST_CHECK_EQUAL(x7a.size(),1u);
			// Construction from different series type with different term_type.
			polynomial2<numerical_coefficient<float>,Expo> y1;
			y1 += 1;
			p_type x8(y1);
			x8 -=1;
			BOOST_CHECK(x8.empty());
			p_type x8a(std::move(y1));
			BOOST_CHECK_EQUAL(x8a.size(),1u);
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

struct negate_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef polynomial<Cf,Expo> p_type;
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

BOOST_AUTO_TEST_CASE(top_level_series_negate_test)
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
				typedef polynomial<Cf,Expo> p_type1;
				typedef polynomial<numerical_coefficient<float>,Expo> p_type2;
				// Addition.
				p_type1 x("x"), y("y");
				auto z = x + y;
				BOOST_CHECK_EQUAL(z.size(),2u);
				auto it = z.m_container.begin();
				BOOST_CHECK(it->m_cf.get_value() == typename Cf::type(1));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf.get_value() == typename Cf::type(1));
				BOOST_CHECK(it->m_key.size() == 2u);
				z = z + 1.f;
				z = 1.f + z;
				BOOST_CHECK_EQUAL(z.size(),3u);
				p_type2 a("a"), b("b");
				auto c = a + b + x;
				BOOST_CHECK_EQUAL(c.size(),3u);
				BOOST_CHECK_EQUAL(std::get<0>(c.m_ed.get_args_tuple()).size(),3u);
				BOOST_CHECK_EQUAL(std::get<0>(c.m_ed.get_args_tuple())[0],symbol("a"));
				BOOST_CHECK_EQUAL(std::get<0>(c.m_ed.get_args_tuple())[1],symbol("b"));
				BOOST_CHECK_EQUAL(std::get<0>(c.m_ed.get_args_tuple())[2],symbol("x"));
				c = x + b + a;
				BOOST_CHECK_EQUAL(c.size(),3u);
				BOOST_CHECK_EQUAL(std::get<0>(c.m_ed.get_args_tuple()).size(),3u);
				BOOST_CHECK_EQUAL(std::get<0>(c.m_ed.get_args_tuple())[0],symbol("a"));
				BOOST_CHECK_EQUAL(std::get<0>(c.m_ed.get_args_tuple())[1],symbol("b"));
				BOOST_CHECK_EQUAL(std::get<0>(c.m_ed.get_args_tuple())[2],symbol("x"));
				// Subtraction.
				z = x - y;
				BOOST_CHECK_EQUAL(z.size(),2u);
				it = z.m_container.begin();
				BOOST_CHECK(it->m_cf.get_value() == typename Cf::type(1) || it->m_cf.get_value() == typename Cf::type(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				++it;
				BOOST_CHECK(it->m_cf.get_value() == typename Cf::type(1) || it->m_cf.get_value() == typename Cf::type(-1));
				BOOST_CHECK(it->m_key.size() == 2u);
				z = z - 1.f;
				z = 1.f - z;
				BOOST_CHECK_EQUAL(z.size(),3u);
				c = a - b - x;
				BOOST_CHECK_EQUAL(c.size(),3u);
				BOOST_CHECK_EQUAL(std::get<0>(c.m_ed.get_args_tuple()).size(),3u);
				BOOST_CHECK_EQUAL(std::get<0>(c.m_ed.get_args_tuple())[0],symbol("a"));
				BOOST_CHECK_EQUAL(std::get<0>(c.m_ed.get_args_tuple())[1],symbol("b"));
				BOOST_CHECK_EQUAL(std::get<0>(c.m_ed.get_args_tuple())[2],symbol("x"));
				c = x - b - a;
				BOOST_CHECK_EQUAL(c.size(),3u);
				BOOST_CHECK_EQUAL(std::get<0>(c.m_ed.get_args_tuple()).size(),3u);
				BOOST_CHECK_EQUAL(std::get<0>(c.m_ed.get_args_tuple())[0],symbol("a"));
				BOOST_CHECK_EQUAL(std::get<0>(c.m_ed.get_args_tuple())[1],symbol("b"));
				BOOST_CHECK_EQUAL(std::get<0>(c.m_ed.get_args_tuple())[2],symbol("x"));
				c = c - c;
				BOOST_CHECK(c.empty());
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

BOOST_AUTO_TEST_CASE(top_level_series_binary_arithmetics_test)
{
	boost::mpl::for_each<cf_types>(binary_arithmetics_tester());
}

struct generic_assignment_tag {};

namespace piranha
{
template <>
class debug_access<generic_assignment_tag>
{
	public:
		template <typename Cf>
		struct runner
		{
			template <typename Expo>
			void operator()(const Expo &)
			{
				typedef polynomial<Cf,Expo> p_type1;
				typedef polynomial<numerical_coefficient<float>,Expo> p_type2;
				auto checker1 = [](const p_type1 &poly) -> void {
					BOOST_CHECK_EQUAL(poly.size(),1u);
					BOOST_CHECK_EQUAL(poly.m_container.begin()->m_cf.get_value(),Cf(5,poly.m_ed).get_value());
				};
				p_type1 p;
				p = 5;
				checker1(p);
				integer tmp(5);
				p = integer(5);
				checker1(p);
				p = tmp;
				checker1(p);
				p_type1 q;
				q = 6;
				auto checker2 = [](const p_type1 &poly) -> void {
					BOOST_CHECK_EQUAL(poly.size(),1u);
					BOOST_CHECK_EQUAL(poly.m_container.begin()->m_cf.get_value(),Cf(6,poly.m_ed).get_value());
				};
				p = q;
				checker2(p);
				p = std::move(q);
				checker2(p);
				p_type2 r;
				r = 7;
				auto checker3 = [](const p_type1 &poly) -> void {
					BOOST_CHECK_EQUAL(poly.size(),1u);
					BOOST_CHECK_EQUAL(poly.m_container.begin()->m_cf.get_value(),Cf(numerical_coefficient<float>(7,poly.m_ed),poly.m_ed).get_value());
				};
				p = r;
				checker3(p);
				p = std::move(r);
				checker3(p);
			}
		};
		template <typename Cf>
		void operator()(const Cf &)
		{
			boost::mpl::for_each<expo_types>(runner<Cf>());
		}
};
}

typedef debug_access<generic_assignment_tag> generic_assignment_tester;

BOOST_AUTO_TEST_CASE(top_level_series_generic_assignment_test)
{
	boost::mpl::for_each<cf_types>(generic_assignment_tester());
}

struct equality_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef polynomial<Cf,Expo> p_type1;
			typedef polynomial<numerical_coefficient<long>,Expo> p_type2;
			typedef polynomial2<Cf,Expo> p_type3;
			typedef polynomial2<numerical_coefficient<long>,Expo> p_type4;
			BOOST_CHECK(p_type1{} == 0);
			BOOST_CHECK(0 == p_type1{});
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
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(top_level_series_equality_test)
{
	boost::mpl::for_each<cf_types>(equality_tester());
}

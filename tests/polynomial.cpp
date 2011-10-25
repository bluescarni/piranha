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

#include "../src/polynomial.hpp"

#define BOOST_TEST_MODULE polynomial_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <type_traits>

#include "../src/integer.hpp"
#include "../src/polynomial_term.hpp"
#include "../src/series.hpp"
#include "../src/settings.hpp"
#include "../src/symbol.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

typedef boost::mpl::vector<double,integer> cf_types;
typedef boost::mpl::vector<unsigned,integer> expo_types;

template <typename Cf, typename Expo>
class polynomial_alt:
	public series<polynomial_term<Cf,Expo>,polynomial_alt<Cf,Expo>>
{
		typedef series<polynomial_term<Cf,Expo>,polynomial_alt<Cf,Expo>> base;
	public:
		polynomial_alt() = default;
		polynomial_alt(const polynomial_alt &) = default;
		polynomial_alt(polynomial_alt &&) = default;
		explicit polynomial_alt(const char *name):base()
		{
			typedef typename base::term_type term_type;
			// Insert the symbol.
			this->m_symbol_set.add(symbol(name));
			// Construct and insert the term.
			this->insert(term_type(Cf(1),typename term_type::key_type{Expo(1)}));
		}
		template <typename T, typename... Args, typename std::enable_if<sizeof...(Args) || !std::is_same<polynomial_alt,typename std::decay<T>::type>::value>::type*& = enabler>
		explicit polynomial_alt(T &&arg1, Args && ... argn) : base(std::forward<T>(arg1),std::forward<Args>(argn)...) {}
		~polynomial_alt() = default;
		polynomial_alt &operator=(const polynomial_alt &) = default;
		polynomial_alt &operator=(polynomial_alt &&other) piranha_noexcept_spec(true)
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
			BOOST_CHECK(p1 == 0);
			BOOST_CHECK(p1.empty());
			// Construction from symbol name.
			p_type p2{"x"};
			BOOST_CHECK(p2.size() == 1u);
			BOOST_CHECK(p2 == p_type{"x"});
			BOOST_CHECK(p2 != p_type{"y"});
			BOOST_CHECK(p2 == p_type{"x"} + p_type{"y"} - p_type{"y"});
			// Construction from number-like entities.
			p_type p3{3};
			BOOST_CHECK(p3.size() == 1u);
			BOOST_CHECK(p3 == 3);
			BOOST_CHECK(3 == p3);
			BOOST_CHECK(p3 != p2);
			p_type p3a{integer(3)};
			BOOST_CHECK(p3a == p3);
			BOOST_CHECK(p3 == p3a);
			// Construction from polynomial of different type.
			typedef polynomial<long,int> p_type1;
			typedef polynomial<int,short> p_type2;
			p_type1 p4(1);
			p_type2 p5(p4);
			BOOST_CHECK(p4 == p5);
			BOOST_CHECK(p5 == p4);
			p_type1 p6("x");
			p_type2 p7("x");
			p_type2 p8("y");
			BOOST_CHECK(p6 == p7);
			BOOST_CHECK(p7 == p6);
			BOOST_CHECK(p6 != p8);
			BOOST_CHECK(p8 != p6);
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(polynomial_constructors_test)
{
	boost::mpl::for_each<cf_types>(constructor_tester());
}

struct assignment_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef polynomial<Cf,Expo> p_type;
			p_type p1;
			p1 = 1;
			BOOST_CHECK(p1 == 1);
			p1 = integer(10);
			BOOST_CHECK(p1 == integer(10));
			p1 = "x";
			BOOST_CHECK(p1 == p_type("x"));
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(polynomial_assignment_test)
{
	boost::mpl::for_each<cf_types>(assignment_tester());
}

struct multiplication_tester
{
	// NOTE: this test is going to be exact in case of coefficients cancellations with double
	// precision coefficients only if the platform has ieee 754 format (integer exactly representable
	// as doubles up to 2 ** 53).
	template <typename Cf>
	void operator()(const Cf &)
	{
		typedef polynomial<Cf,int> p_type;
		typedef polynomial_alt<Cf,int> p_type_alt;
		p_type x("x"), y("y"), z("z"), t("t"), u("u");
		// Dense case, default setup.
		auto f = 1 + x + y + z + t;
		auto tmp(f);
		for (int i = 1; i < 10; ++i) {
			f *= tmp;
		}
		auto g = f + 1;
		auto retval = f * g;
		BOOST_CHECK_EQUAL(retval.size(),10626u);
		auto retval_alt = p_type_alt(f) * p_type_alt(g);
		BOOST_CHECK(retval == retval_alt);
		// Dense case, force number of threads.
		for (auto i = 1u; i <= 4u; ++i) {
			settings::set_n_threads(i);
			auto tmp = f * g;
			auto tmp_alt = p_type_alt(f) * p_type_alt(g);
			BOOST_CHECK_EQUAL(tmp.size(),10626u);
			BOOST_CHECK(tmp == retval);
			BOOST_CHECK(tmp == tmp_alt);
		}
		settings::reset_n_threads();
		// Dense case with cancellations, default setup.
		auto h = 1 - x + y + z + t;
		tmp = h;
		for (int i = 1; i < 10; ++i) {
			h *= tmp;
		}
		retval = f * h;
		retval_alt = p_type_alt(f) * p_type_alt(h);
		BOOST_CHECK_EQUAL(retval.size(),5786u);
		BOOST_CHECK(retval == retval_alt);
		// Dense case with cancellations, force number of threads.
		for (auto i = 1u; i <= 4u; ++i) {
			settings::set_n_threads(i);
			auto tmp = f * h;
			auto tmp_alt = p_type_alt(f) * p_type_alt(h);
			BOOST_CHECK_EQUAL(tmp.size(),5786u);
			BOOST_CHECK(retval == tmp);
			BOOST_CHECK(tmp_alt == tmp);
		}
		settings::reset_n_threads();
		// Sparse case, default.
		f = (x + y + z*z*2 + t*t*t*3 + u*u*u*u*u*5 + 1);
		auto tmp_f(f);
		g = (u + t + z*z*2 + y*y*y*3 + x*x*x*x*x*5 + 1);
		auto tmp_g(g);
		h = (-u + t + z*z*2 + y*y*y*3 + x*x*x*x*x*5 + 1);
		auto tmp_h(h);
		for (int i = 1; i < 8; ++i) {
			f *= tmp_f;
			g *= tmp_g;
			h *= tmp_h;
		}
		retval = f * g;
		BOOST_CHECK_EQUAL(retval.size(),591235u);
		retval_alt = p_type_alt(f) * p_type_alt(g);
		BOOST_CHECK(retval == retval_alt);
		// Sparse case, force n threads.
		for (auto i = 1u; i <= 4u; ++i) {
			settings::set_n_threads(i);
			auto tmp = f * g;
			auto tmp_alt = p_type_alt(f) * p_type_alt(g);
			BOOST_CHECK_EQUAL(tmp.size(),591235u);
			BOOST_CHECK(retval == tmp);
			BOOST_CHECK(tmp_alt == tmp);
		}
		settings::reset_n_threads();
		// Sparse case with cancellations, default.
		retval = f * h;
		BOOST_CHECK_EQUAL(retval.size(),591184u);
		retval_alt = p_type_alt(f) * p_type_alt(h);
		BOOST_CHECK(retval_alt == retval);
		// Sparse case with cancellations, force number of threads.
		for (auto i = 1u; i <= 4u; ++i) {
			settings::set_n_threads(i);
			auto tmp = f * h;
			auto tmp_alt = p_type_alt(f) * p_type_alt(h);
			BOOST_CHECK_EQUAL(tmp.size(),591184u);
			BOOST_CHECK(tmp == retval);
			BOOST_CHECK(tmp == tmp_alt);
		}
	}
};

BOOST_AUTO_TEST_CASE(polynomial_multiplier_test)
{
	boost::mpl::for_each<cf_types>(multiplication_tester());
}

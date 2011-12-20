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
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include "../src/degree_truncator_settings.hpp"
#include "../src/integer.hpp"
#include "../src/kronecker_array.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/polynomial_term.hpp"
#include "../src/series.hpp"
#include "../src/settings.hpp"
#include "../src/symbol.hpp"
#include "../src/type_traits.hpp"
#include "../src/univariate_monomial.hpp"

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

BOOST_AUTO_TEST_CASE(polynomial_recursive_test)
{
	typedef polynomial<double,univariate_monomial<int>> p_type1;
	typedef polynomial<p_type1,univariate_monomial<int>> p_type11;
	typedef polynomial<p_type11,univariate_monomial<int>> p_type111;
	p_type1 x("x");
	p_type11 y("y");
	p_type111 z("z");
	BOOST_CHECK((std::is_same<decltype(x+y),p_type11>::value));
	BOOST_CHECK((std::is_same<decltype(y+x),p_type11>::value));
	BOOST_CHECK((std::is_same<decltype(z+y),p_type111>::value));
	BOOST_CHECK((std::is_same<decltype(y+z),p_type111>::value));
	BOOST_CHECK((std::is_same<decltype(z+x),p_type111>::value));
	BOOST_CHECK((std::is_same<decltype(x+z),p_type111>::value));
	BOOST_CHECK_THROW(x + p_type1("y"),std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(polynomial_degree_test)
{
	typedef polynomial<double,univariate_monomial<int>> p_type1;
	typedef polynomial<p_type1,univariate_monomial<int>> p_type11;
	typedef polynomial<p_type11,univariate_monomial<int>> p_type111;
	p_type1 x("x");
	BOOST_CHECK(x.degree() == 1);
	BOOST_CHECK(x.ldegree() == 1);
	BOOST_CHECK((x * x).degree() == 2);
	BOOST_CHECK((x * x).ldegree() == 2);
	BOOST_CHECK((x * x).degree({"y","z"}) == 0);
	BOOST_CHECK((x * x).ldegree({"y","z"}) == 0);
	p_type11 y("y");
	p_type111 z("z");
	BOOST_CHECK((x * y* z).degree() == 3);
	BOOST_CHECK((x * y* z).ldegree() == 3);
	BOOST_CHECK((x * y* z).degree({"x"}) == 1);
	BOOST_CHECK((x * y* z).ldegree({"x"}) == 1);
	BOOST_CHECK((x * y* z).degree({"y"}) == 1);
	BOOST_CHECK((x * y* z).ldegree({"y"}) == 1);
	BOOST_CHECK((x * y* z).degree({"z"}) == 1);
	BOOST_CHECK((x * y* z).ldegree({"z"}) == 1);
	BOOST_CHECK((x * y* z).degree({"z","y"}) == 2);
	BOOST_CHECK((x * y* z).ldegree({"z","y"}) == 2);
	BOOST_CHECK((x * y* z).degree({"z","x"}) == 2);
	BOOST_CHECK((x * y* z).ldegree({"z","x"}) == 2);
	BOOST_CHECK((x * y* z).degree({"y","x"}) == 2);
	BOOST_CHECK((x * y* z).ldegree({"y","x"}) == 2);
	BOOST_CHECK((x * y* z).degree({"y","x","z"}) == 3);
	BOOST_CHECK((x * y* z).ldegree({"y","x","z"}) == 3);
	BOOST_CHECK((x + y + z).degree() == 1);
	BOOST_CHECK((x + y + z).ldegree() == 1);
	BOOST_CHECK((x + y + z).degree({"x"}) == 1);
	BOOST_CHECK((x + y + z).ldegree({"x"}) == 0);
	BOOST_CHECK((x + y + z).ldegree({"x","y"}) == 0);
	BOOST_CHECK((x + y + 1).ldegree({"x","y"}) == 0);
	BOOST_CHECK((x + y + 1).ldegree({"x","y","t"}) == 0);
	BOOST_CHECK((x + y + 1).ldegree() == 0);
}

struct truncator_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			// Unary truncator.
			typedef polynomial<Cf,int> p_type;
			typedef typename p_type::term_type term_type;
			typedef typename term_type::key_type key_type;
			p_type p1;
			BOOST_CHECK(truncator_traits<p_type>::is_sorting);
			BOOST_CHECK(!p1.get_truncator().is_active());
			BOOST_CHECK(!p1.get_truncator().filter(term_type{}));
			p1.get_truncator().set(5);
			BOOST_CHECK(p1.get_truncator().is_active());
			BOOST_CHECK(!p1.get_truncator().filter(term_type{Cf(1),key_type()}));
			p1.get_truncator().set("x",5);
			BOOST_CHECK(p1.get_truncator().is_active());
			p1.get_truncator().unset();
			BOOST_CHECK(!p1.get_truncator().is_active());
			p1.get_truncator().set("x",5);
			term_type t1{Cf(1),key_type{Expo(1)}}, t2(t1);
			BOOST_CHECK_THROW(p1.get_truncator().compare_terms(t1,t2),std::invalid_argument);
			BOOST_CHECK_THROW(p1.get_truncator().filter(t1),std::invalid_argument);
			p1 = "x";
			BOOST_CHECK(!p1.get_truncator().compare_terms(t1,t2));
			BOOST_CHECK(!p1.get_truncator().filter(t1));
			term_type t3{Cf(1),key_type{Expo(2)}};
			BOOST_CHECK(p1.get_truncator().compare_terms(t1,t3));
			BOOST_CHECK(!p1.get_truncator().compare_terms(t3,t1));
			BOOST_CHECK(!p1.get_truncator().filter(t3));
			term_type t3a{Cf(1),key_type{Expo(4)}};
			BOOST_CHECK(!p1.get_truncator().filter(t3a));
			term_type t3b{Cf(1),key_type{Expo(5)}};
			BOOST_CHECK(p1.get_truncator().filter(t3b));
			p1.get_truncator().set(5);
			BOOST_CHECK(p1.get_truncator().compare_terms(t1,t3));
			BOOST_CHECK(!p1.get_truncator().compare_terms(t3,t1));
			typedef polynomial<p_type,int> p_type1;
			typedef typename p_type1::term_type term_type1;
			BOOST_CHECK(truncator_traits<p_type1>::is_sorting);
			BOOST_CHECK(truncator_traits<p_type1>::is_filtering);
			p1.get_truncator().set("y",5);
			BOOST_CHECK(!p1.get_truncator().filter(t3b));
			term_type1 tt1{p_type(1),key_type{Expo(1)}}, tt2(tt1);
			p_type1 pp1{"x"};
			pp1.get_truncator().set("x",5);
			BOOST_CHECK(!pp1.get_truncator().compare_terms(tt1,tt2));
			term_type1 tt3{p_type(1),key_type{Expo(2)}};
			BOOST_CHECK(pp1.get_truncator().compare_terms(tt1,tt3));
			BOOST_CHECK(!pp1.get_truncator().compare_terms(tt3,tt1));
			p1.get_truncator().set(5);
			BOOST_CHECK(pp1.get_truncator().compare_terms(tt1,tt3));
			BOOST_CHECK(!pp1.get_truncator().compare_terms(tt3,tt1));
			p1.get_truncator().unset();
			// Binary truncator.
			typedef polynomial<Cf,Expo> p_typea;
			typedef polynomial<Cf,int> p_typeb;
			p_typea x{"x"}, y{"y"};
			p_typeb xb{"x"};
			typedef typename p_typea::term_type::key_type key_typea;
			typedef typename p_typeb::term_type::key_type key_typeb;
			BOOST_CHECK((truncator_traits<p_typea,p_typea>::is_sorting));
			BOOST_CHECK((truncator_traits<p_typea,p_typeb>::is_sorting));
			BOOST_CHECK((truncator_traits<p_typea,p_typea>::is_filtering));
			BOOST_CHECK((truncator_traits<p_typea,p_typeb>::is_filtering));
			BOOST_CHECK((truncator_traits<p_typea,p_typea>::is_skipping));
			BOOST_CHECK((truncator_traits<p_typea,p_typeb>::is_skipping));
			BOOST_CHECK((!truncator<p_typea,p_typea>(p_typea{},p_typea{}).is_active()));
			BOOST_CHECK((!truncator<p_typea,p_typeb>(p_typea{},p_typeb{}).is_active()));
			typedef truncator<p_typea,p_typea> trunc1;
			typedef truncator<p_typea,p_typeb> trunc2;
			trunc1 tr0{p_typea{},p_typea{}};
			BOOST_CHECK_THROW(tr0.compare_terms(typename p_typea::term_type{},typename p_typea::term_type{}),std::invalid_argument);
			BOOST_CHECK(!tr0.skip(typename p_typea::term_type{},typename p_typea::term_type{}));
			BOOST_CHECK(!tr0.filter(typename p_typea::term_type{}));
			p1.get_truncator().set(5);
			p_typea empty;
			trunc1 tr1{empty,empty};
			BOOST_CHECK((truncator<p_typea,p_typea>(p_typea{},p_typea{}).is_active()));
			BOOST_CHECK((truncator<p_typea,p_typeb>(p_typea{},p_typeb{}).is_active()));
			BOOST_CHECK(!tr1.compare_terms(typename p_typea::term_type{},typename p_typea::term_type{}));
			BOOST_CHECK(!tr1.skip(typename p_typea::term_type{},typename p_typea::term_type{}));
			BOOST_CHECK(!tr1.filter(typename p_typea::term_type{}));
			trunc1 tr2{x,x};
			BOOST_CHECK(!tr2.compare_terms(typename p_typea::term_type{1,key_typea{1}},typename p_typea::term_type{1,key_typea{1}}));
			BOOST_CHECK(tr2.compare_terms(typename p_typea::term_type{1,key_typea{1}},typename p_typea::term_type{1,key_typea{2}}));
			BOOST_CHECK(!tr2.compare_terms(typename p_typea::term_type{1,key_typea{2}},typename p_typea::term_type{1,key_typea{1}}));
			BOOST_CHECK(!tr2.skip(typename p_typea::term_type{1,key_typea{1}},typename p_typea::term_type{1,key_typea{1}}));
			BOOST_CHECK(tr2.skip(typename p_typea::term_type{1,key_typea{3}},typename p_typea::term_type{1,key_typea{2}}));
			BOOST_CHECK(tr2.skip(typename p_typea::term_type{1,key_typea{3}},typename p_typea::term_type{1,key_typea{3}}));
			BOOST_CHECK(!tr2.filter(typename p_typea::term_type{1,key_typea{3}}));
			BOOST_CHECK(tr2.filter(typename p_typea::term_type{1,key_typea{5}}));
			BOOST_CHECK(tr2.filter(typename p_typea::term_type{1,key_typea{6}}));
			trunc2 tr3{x,xb};
			BOOST_CHECK(!tr3.skip(typename p_typea::term_type{1,key_typea{1}},typename p_typeb::term_type{1,key_typeb{1}}));
			BOOST_CHECK(tr3.skip(typename p_typea::term_type{1,key_typea{3}},typename p_typeb::term_type{1,key_typeb{2}}));
			BOOST_CHECK(tr3.skip(typename p_typea::term_type{1,key_typea{3}},typename p_typeb::term_type{1,key_typeb{3}}));
			BOOST_CHECK(!tr3.filter(typename p_typea::term_type{1,key_typea{3}}));
			BOOST_CHECK(tr3.filter(typename p_typea::term_type{1,key_typea{5}}));
			BOOST_CHECK(tr3.filter(typename p_typea::term_type{1,key_typea{6}}));
			p1.get_truncator().set("y",5);
			auto xy = x*y;
			trunc1 tr4{xy,xy};
			BOOST_CHECK(!tr4.compare_terms(typename p_typea::term_type{1,key_typea{1,1}},typename p_typea::term_type{1,key_typea{1,1}}));
			BOOST_CHECK(!tr4.compare_terms(typename p_typea::term_type{1,key_typea{10,1}},typename p_typea::term_type{1,key_typea{1,1}}));
			BOOST_CHECK(tr4.compare_terms(typename p_typea::term_type{1,key_typea{10,1}},typename p_typea::term_type{1,key_typea{1,2}}));
			BOOST_CHECK(!tr4.skip(typename p_typea::term_type{1,key_typea{10,1}},typename p_typea::term_type{1,key_typea{1,2}}));
			BOOST_CHECK(tr4.skip(typename p_typea::term_type{1,key_typea{10,3}},typename p_typea::term_type{1,key_typea{1,2}}));
			BOOST_CHECK(tr4.skip(typename p_typea::term_type{1,key_typea{10,3}},typename p_typea::term_type{1,key_typea{1,3}}));
			BOOST_CHECK(!tr4.filter(typename p_typea::term_type{1,key_typea{10,3}}));
			BOOST_CHECK(!tr4.filter(typename p_typea::term_type{1,key_typea{10,4}}));
			BOOST_CHECK(tr4.filter(typename p_typea::term_type{1,key_typea{0,5}}));
			BOOST_CHECK(tr4.filter(typename p_typea::term_type{1,key_typea{0,6}}));
			BOOST_CHECK_THROW((trunc1{xy,decltype(xy){}}),std::invalid_argument);
			p1.get_truncator().unset();
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(polynomial_truncator_test)
{
	boost::mpl::for_each<cf_types>(truncator_tester());
}

struct multiplication_tester
{
	template <typename Cf>
	void operator()(const Cf &)
	{
		// NOTE: this test is going to be exact in case of coefficients cancellations with double
		// precision coefficients only if the platform has ieee 754 format (integer exactly representable
		// as doubles up to 2 ** 53).
		if (std::is_same<Cf,double>::value && (!std::numeric_limits<double>::is_iec559 ||
			std::numeric_limits<double>::digits < 53))
		{
			return;
		}
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

// Tests specific for the polynomial multiplier specialisation for Kronecker monomials.
struct kronecker_multiplication_tester
{
	template <typename Cf>
	void operator()(const Cf &)
	{
		degree_truncator_settings::unset();
		typedef kronecker_array<std::int_least8_t> ka1;
		typedef polynomial<Cf,kronecker_monomial<std::int_least8_t>> p_type1;
		// Test for empty series.
		BOOST_CHECK((p_type1{} * p_type1{}).empty());
		p_type1 tmp = p_type1("x") * p_type1("y"), xy(tmp);
		BOOST_CHECK((p_type1{} * xy).empty());
		BOOST_CHECK((xy * p_type1{}).empty());
		// Check for correct throwing on overflow.
		for (std::int_least8_t i = 2; tmp.degree({"x"}) < std::get<0u>(ka1::get_limits()[2u])[0u]; ++i) {
			tmp *= p_type1("x");
			BOOST_CHECK_EQUAL(i,tmp.degree({"x"}));
			BOOST_CHECK_EQUAL(1,tmp.degree({"y"}));
		}
		BOOST_CHECK_THROW(tmp * xy,std::overflow_error);
		BOOST_CHECK_THROW(xy * tmp,std::overflow_error);
		typedef polynomial<Cf,kronecker_monomial<std::int_least32_t>> p_type2;
		p_type2 y("y"), z("z"), t("t"), u("u");
		auto f = 1 + p_type2("x") + y + z + t;
		auto tmp2 = f;
		for (int i = 1; i < 10; ++i) {
			f *= tmp2;
		}
		auto g = f + 1;
		auto retval = f * g;
		BOOST_CHECK_EQUAL(retval.size(),10626u);
		degree_truncator_settings::set(0);
		f = 1 + p_type2("x") + y + z + t;
		tmp2 = f;
		for (int i = 1; i < 10; ++i) {
			f *= tmp2;
		}
		g = f + 1;
		retval = f * g;
		BOOST_CHECK_EQUAL(retval.size(),0u);
		degree_truncator_settings::set(1);
		f = 1 + p_type2("x") + y + z + t;
		tmp2 = f;
		for (int i = 1; i < 10; ++i) {
			f *= tmp2;
		}
		g = f + 1;
		retval = f * g;
		BOOST_CHECK_EQUAL(retval.size(),1u);
		BOOST_CHECK(retval.degree() == 0);
		degree_truncator_settings::set(10);
		f = 1 + p_type2("x") + y + z + t;
		tmp2 = f;
		for (int i = 1; i < 10; ++i) {
			f *= tmp2;
		}
		g = f + 1;
		retval = f * g;
		BOOST_CHECK(retval.degree() == 9);
		degree_truncator_settings::unset();
		// NOTE: this test is going to be exact in case of coefficients cancellations with double
		// precision coefficients only if the platform has ieee 754 format (integer exactly representable
		// as doubles up to 2 ** 53).
		if (std::is_same<Cf,double>::value && (!std::numeric_limits<double>::is_iec559 ||
			std::numeric_limits<double>::digits < 53))
		{
			return;
		}
		// Dense case with cancellations, default setup.
		auto h = 1 - p_type2("x") + y + z + t;
		f = 1 + p_type2("x") + y + z + t;
		tmp2 = h;
		auto tmp3 = f;
		for (int i = 1; i < 10; ++i) {
			h *= tmp2;
			f *= tmp3;
		}
		retval = f * h;
		BOOST_CHECK_EQUAL(retval.size(),5786u);
	}
};

BOOST_AUTO_TEST_CASE(polynomial_kronecker_multiplier_test)
{
	boost::mpl::for_each<cf_types>(kronecker_multiplication_tester());
}

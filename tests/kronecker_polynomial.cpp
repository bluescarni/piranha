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

#define BOOST_TEST_MODULE kronecker_polynomial_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include "../src/environment.hpp"
#include "../src/math.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/kronecker_array.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/settings.hpp"

using namespace piranha;

typedef boost::mpl::vector<double,integer,rational> cf_types;

// Tests specific for the polynomial multiplier specialisation for Kronecker monomials.
struct multiplication_tester
{
	template <typename Cf>
	void operator()(const Cf &)
	{
		typedef kronecker_array<std::int_least8_t> ka1;
		typedef polynomial<Cf,kronecker_monomial<std::int_least8_t>> p_type1;
		// Test for empty series.
		BOOST_CHECK((p_type1{} * p_type1{}).empty());
		p_type1 tmp = p_type1("x") * p_type1("y"), xy(tmp);
		BOOST_CHECK((p_type1{} * xy).empty());
		BOOST_CHECK((xy * p_type1{}).empty());
		// Check for correct throwing on overflow.
		for (std::int_least8_t i = 2; math::degree(tmp,{"x"}) < std::get<0u>(ka1::get_limits()[2u])[0u]; ++i) {
			tmp *= p_type1("x");
			BOOST_CHECK_EQUAL(i,math::degree(tmp,{"x"}));
			BOOST_CHECK_EQUAL(1,math::degree(tmp,{"y"}));
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
		auto r_no_trunc = retval;
		BOOST_CHECK_EQUAL(retval.size(),10626u);
		// NOTE: this test is going to be exact in case of coefficients cancellations with double
		// precision coefficients only if the platform has ieee 754 format (integer exactly representable
		// as doubles up to 2 ** 53).
		// NOTE: no check on radix needed, 2 is the minimum value.
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

BOOST_AUTO_TEST_CASE(kronecker_polynomial_multiplier_test)
{
	environment env;
	boost::mpl::for_each<cf_types>(multiplication_tester());
	for (unsigned i = 1u; i <= 4u; ++i) {
		settings::set_n_threads(i);
		boost::mpl::for_each<cf_types>(multiplication_tester());
	}
}

struct overflow_tester
{
	template <typename Cf>
	void operator()(const Cf &)
	{
		polynomial<Cf,kronecker_monomial<std::int_least32_t>> x{"x"}, y{"y"}, z{"z"}, t{"t"}, u{"u"};
		auto prod = x * y * z * t * u;
		auto tmp_t(t);
		auto l = std::get<0u>(kronecker_array<std::int_least32_t>::get_limits()[5u])[0u] / 2;
		for (decltype(l) i = 1; i < l; ++i) {
			prod *= t;
			tmp_t *= t;
		}
		tmp_t *= tmp_t;
		BOOST_CHECK_THROW((prod + tmp_t) * prod,std::overflow_error);
	}
};

BOOST_AUTO_TEST_CASE(kronecker_polynomial_overflow_test)
{
	boost::mpl::for_each<cf_types>(overflow_tester());
}

struct st_vs_mt_tester
{
	void operator()()
	{
		// Sparse case.
		// Compute result in st mode.
		// NOTE: do not check for size as via unsigned wrapover some coefficients
		// might go to zero.
		settings::set_n_threads(1u);
		typedef polynomial<std::size_t,kronecker_monomial<>> p_type;
		p_type x("x"), y("y"), z("z"), t("t");
		auto f = 1 + x + y + z + t;
		auto tmp2 = f;
		for (int i = 1; i < 10; ++i) {
			f *= tmp2;
		}
		auto g = f + 1;
		auto st = f * g;
		for (auto i = 2u; i <= 4u; ++i) {
			settings::set_n_threads(i);
			auto mt = f * g;
			BOOST_CHECK(mt == st);
		}
		// Dense case.
		settings::set_n_threads(1u);
		f *= f;
		g *= g;
		st = f * g;
		for (auto i = 2u; i <= 4u; ++i) {
			settings::set_n_threads(i);
			auto mt = f * g;
			BOOST_CHECK(mt == st);
		}
	}
};

BOOST_AUTO_TEST_CASE(kronecker_polynomial_st_vs_mt_test)
{
	st_vs_mt_tester t;
	t();
}

BOOST_AUTO_TEST_CASE(kronecker_polynomial_different_cf_test)
{
	settings::set_n_threads(1u);
	typedef polynomial<std::size_t,kronecker_monomial<>> p_type1;
	typedef polynomial<integer,kronecker_monomial<>> p_type2;
	p_type1 x("x"), y("y"), z("z"), t("t");
	auto f = 1 + x + y + z + t;
	p_type2 tmp2(f);
	for (int i = 1; i < 10; ++i) {
		f *= tmp2;
	}
	auto g = f + 1;
	auto st = f * g;
	BOOST_CHECK_EQUAL(st.size(),10626u);
}

// Test cancellations in sparse multiplication with multiple threads.
BOOST_AUTO_TEST_CASE(kronecker_polynomial_sparse_cancellation_mt_test)
{
	settings::set_n_threads(4);
	typedef polynomial<double,kronecker_monomial<>> p_type;
	// Dense case with cancellations, default setup.
	auto h = 1 - p_type("x") + p_type("y") + p_type("z") + p_type("t");
	auto f = 1 + p_type("x") + p_type("y") + p_type("z") + p_type("t");
	auto tmp2 = h;
	auto tmp3 = f;
	for (int i = 1; i < 10; ++i) {
		h *= tmp2;
		f *= tmp3;
	}
	auto retval = f * h;
	BOOST_CHECK_EQUAL(retval.size(),5786u);
}

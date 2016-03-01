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

#include "../src/polynomial.hpp"

#define BOOST_TEST_MODULE polynomial_03_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <random>
#include <stdexcept>

#include "../src/environment.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/pow.hpp"
#include "../src/symbol.hpp"
#include "../src/symbol_set.hpp"

using namespace piranha;

static std::mt19937 rng;
static const int ntrials = 100;

using cf_types = boost::mpl::vector<integer,rational>;
using key_types = boost::mpl::vector<monomial<int>,k_monomial>;

struct split_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Key>
		void operator()(const Key &)
		{
			using p_type = polynomial<Cf,Key>;
			using pp_type = polynomial<p_type,Key>;
			p_type x{"x"}, y{"y"}, z{"z"};
			pp_type xx{"x"};
			BOOST_CHECK_THROW(p_type{}.split(),std::invalid_argument);
			BOOST_CHECK_THROW(x.split(),std::invalid_argument);
			BOOST_CHECK_EQUAL((2*x+3*y).split(),2*xx+3*y);
			BOOST_CHECK((2*x+3*y).split().get_symbol_set() == symbol_set{symbol{"x"}});
			BOOST_CHECK(((2*x+3*y).split()._container().begin()->m_cf.get_symbol_set() == symbol_set{symbol{"y"}}));
			BOOST_CHECK_EQUAL((2*x*z+3*x*x*y-6*x*y*z).split(),xx*(2*z-6*y*z)+3*xx*xx*y);
			BOOST_CHECK((2*x*z+3*x*x*y-6*x*y*z).split().get_symbol_set() == symbol_set{symbol{"x"}});
			BOOST_CHECK(((2*x*z+3*x*x*y-6*x*y*z).split()._container().begin()->m_cf.get_symbol_set() == symbol_set{symbol{"y"},symbol{"z"}}));
			BOOST_CHECK((std::is_same<decltype(x.split()),pp_type>::value));
			p_type null;
			null.set_symbol_set((x+y).get_symbol_set());
			BOOST_CHECK_EQUAL(null.split(),pp_type{});
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<key_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(polynomial_split_test)
{
	environment env;
	boost::mpl::for_each<cf_types>(split_tester());
}

struct uldiv_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using namespace piranha::detail;
		using piranha::math::pow;
		using p_type = polynomial<rational,Key>;
		p_type x{"x"};
		// A few initial tests that can be checked manually.
		auto res = poly_uldiv(x,x);
		BOOST_CHECK_EQUAL(res.first,1);
		BOOST_CHECK_EQUAL(res.second,0);
		res = poly_uldiv(2*x,x);
		BOOST_CHECK_EQUAL(res.first,2);
		BOOST_CHECK_EQUAL(res.second,0);
		res = poly_uldiv(x,2*x);
		BOOST_CHECK_EQUAL(res.first,1/2_q);
		BOOST_CHECK_EQUAL(res.second,0);
		res = poly_uldiv(pow(x,3)-2*pow(x,2)-4,x-3);
		BOOST_CHECK_EQUAL(res.first,pow(x,2)+x+3);
		BOOST_CHECK_EQUAL(res.second,5);
		res = poly_uldiv(pow(x,8)+pow(x,6)-3*pow(x,4)-3*pow(x,3)+8*pow(x,2)+2*x-5,3*pow(x,6)+5*pow(x,4)-4*x*x-9*x+21);
		BOOST_CHECK_EQUAL(res.first,x*x/3-2/9_q);
		BOOST_CHECK_EQUAL(res.second,-5*x.pow(4)/9+x*x/9-1/3_q);
		res = poly_uldiv(x*x+4*x+1,x+2);
		BOOST_CHECK_EQUAL(res.first,2+x);
		BOOST_CHECK_EQUAL(res.second,-3);
		// With zero numerator.
		res = poly_uldiv(x-x,x+2);
		BOOST_CHECK_EQUAL(res.first,0);
		BOOST_CHECK_EQUAL(res.second,0);
		// With plain numbers
		res = poly_uldiv(x-x-3,x-x+2);
		BOOST_CHECK_EQUAL(res.first,-3/2_q);
		BOOST_CHECK_EQUAL(res.second,0);
		// Randomised testing.
		std::uniform_int_distribution<int> ud(0,9);
		for (auto i = 0; i < ntrials; ++i) {
			// Little hack to make sure that the symbol set contains always x. It could be
			// that in the random loop num/den get inited to a constant.
			p_type num = x - x, den = num;
			const int nterms = ud(rng);
			for (int j = 0; j < nterms; ++j) {
				num += ((ud(rng) < 5 ? 1 : -1) * ud(rng) * x.pow(ud(rng))) / (ud(rng) + 1);
				den += ((ud(rng) < 5 ? 1 : -1) * ud(rng) * x.pow(ud(rng))) / (ud(rng) + 1);
			}
			if (den.size() == 0u) {
				den = x - x + 1;
			}
			auto tmp = poly_uldiv(num,den);
			BOOST_CHECK_EQUAL(num,den*tmp.first + tmp.second);
		}
	}
};

BOOST_AUTO_TEST_CASE(polynomial_uldiv_test)
{
	boost::mpl::for_each<key_types>(uldiv_tester());
}

struct ugcd_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using namespace piranha::detail;
		using piranha::math::pow;
		using p_type = polynomial<integer,Key>;
		using pq_type = polynomial<rational,Key>;
		p_type x{"x"};
		// Some known tests.
		BOOST_CHECK_EQUAL(x,poly_ugcd(x,x));
		BOOST_CHECK_EQUAL(12+12*x,poly_ugcd(pow(x,2)+7*x+6,pow(x,2)-5*x-6));
		BOOST_CHECK_EQUAL(poly_ugcd(pow(x,8)+pow(x,6)-3*pow(x,4)-3*pow(x,3)+8*pow(x,2)+2*x-5,
			3*pow(x,6)+5*pow(x,4)-4*pow(x,2)-9*x+21),260708);
		BOOST_CHECK_EQUAL(poly_ugcd(pow(x,4)-9*pow(x,2)-4*x+12,pow(x,3)+5*pow(x,2)+2*x-8),-28+14*x*x+14*x);
		BOOST_CHECK_EQUAL(1,poly_ugcd(pow(x,4)+pow(x,2)+1,pow(x,2)+1));
		BOOST_CHECK_EQUAL(poly_ugcd(x*x+1,pow(x,5)+pow(x,4)+x+1),8);
		BOOST_CHECK_EQUAL(poly_ugcd(pow(x,6)+pow(x,5)+pow(x,3)+x,pow(x,4)+pow(x,2)+1),1);
		// With zeroes.
		BOOST_CHECK_EQUAL(x+1,poly_ugcd(x-x,x+1));
		BOOST_CHECK_EQUAL(x+1,poly_ugcd(x+1,x-x));
		BOOST_CHECK_EQUAL(0,poly_ugcd(x-x,x-x));
		// Random testing.
		std::uniform_int_distribution<int> ud(0,9);
		for (auto i = 0; i < ntrials; ++i) {
			p_type a = x - x, b = a;
			const int nterms = ud(rng);
			for (int j = 0; j < nterms; ++j) {
				a += ((ud(rng) < 5 ? 1 : -1) * ud(rng) * x.pow(ud(rng))) / (ud(rng) + 1);
				b += ((ud(rng) < 5 ? 1 : -1) * ud(rng) * x.pow(ud(rng))) / (ud(rng) + 1);
			}
			auto tmp = poly_ugcd(a,b);
			if (a.size() != 0u || b.size() != 0u) {
				BOOST_CHECK_EQUAL(poly_uldiv(pq_type(a),pq_type(tmp)).second.size(),0u);
				BOOST_CHECK_EQUAL(poly_uldiv(pq_type(b),pq_type(tmp)).second.size(),0u);
			}
		}
		for (auto i = 0; i < ntrials; ++i) {
			p_type a = x - x, b = a;
			const int nterms = ud(rng);
			for (int j = 0; j < nterms; ++j) {
				a += ((ud(rng) < 5 ? 1 : -1) * ud(rng) * x.pow(ud(rng))) / (ud(rng) + 1);
				b += ((ud(rng) < 5 ? 1 : -1) * ud(rng) * x.pow(ud(rng))) / (ud(rng) + 1);
			}
			auto tmp = poly_ugcd(a*b,b);
			if ((a*b).size() != 0u || b.size() != 0u) {
				BOOST_CHECK_EQUAL(poly_uldiv(pq_type(a*b),pq_type(tmp)).second.size(),0u);
				BOOST_CHECK_EQUAL(poly_uldiv(pq_type(b),pq_type(tmp)).second.size(),0u);
			}
			tmp = poly_ugcd(a*b*b,b*a);
			if ((a*b*b).size() != 0u || (b*a).size() != 0u) {
				BOOST_CHECK_EQUAL(poly_uldiv(pq_type(a*b*b),pq_type(tmp)).second.size(),0u);
				BOOST_CHECK_EQUAL(poly_uldiv(pq_type(b*a),pq_type(tmp)).second.size(),0u);
			}
		}
	}
};

BOOST_AUTO_TEST_CASE(polynomial_ugcd_test)
{
	boost::mpl::for_each<key_types>(ugcd_tester());
}

struct establish_limits_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using namespace piranha::detail;
		using p_type = polynomial<integer,Key>;
		p_type x{"x"}, y{"y"};
		auto lims = poly_establish_limits(x+y,x+y);
		BOOST_CHECK_EQUAL(lims.size(),2u);
		BOOST_CHECK_EQUAL(lims[0].first,0);
		BOOST_CHECK_EQUAL(lims[0].second,1);
		BOOST_CHECK_EQUAL(lims[1].first,0);
		BOOST_CHECK_EQUAL(lims[1].second,1);
		lims = poly_establish_limits(x+y-x,x+y-x+y*y);
		BOOST_CHECK_EQUAL(lims.size(),2u);
		BOOST_CHECK_EQUAL(lims[0].first,0);
		BOOST_CHECK_EQUAL(lims[0].second,0);
		BOOST_CHECK_EQUAL(lims[1].first,1);
		BOOST_CHECK_EQUAL(lims[1].second,2);
		lims = poly_establish_limits(p_type{1},p_type{2});
		BOOST_CHECK_EQUAL(lims.size(),0u);
		lims = poly_establish_limits(x+1-x,x+2-x);
		BOOST_CHECK_EQUAL(lims.size(),1u);
		BOOST_CHECK_EQUAL(lims[0].first,0);
		BOOST_CHECK_EQUAL(lims[0].second,0);
		lims = poly_establish_limits(x+y-y,y+x.pow(4));
		BOOST_CHECK_EQUAL(lims.size(),2u);
		BOOST_CHECK_EQUAL(lims[0].first,0);
		BOOST_CHECK_EQUAL(lims[0].second,4);
		BOOST_CHECK_EQUAL(lims[1].first,0);
		BOOST_CHECK_EQUAL(lims[1].second,1);
		BOOST_CHECK_THROW(poly_establish_limits(x+y.pow(-1),y+x.pow(4)),std::invalid_argument);
		// Try with zero variables.
		p_type a{1}, b{2};
		lims = poly_establish_limits(a,b);
		BOOST_CHECK_EQUAL(lims.size(),0u);
	}
};

BOOST_AUTO_TEST_CASE(polynomial_establish_limits_test)
{
	boost::mpl::for_each<key_types>(establish_limits_tester());
}

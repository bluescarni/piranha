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

#define BOOST_TEST_MODULE polynomial_04_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <random>
#include <stdexcept>

#include "../src/environment.hpp"
#include "../src/exceptions.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/math.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/symbol.hpp"
#include "../src/symbol_set.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

static std::mt19937 rng;
static const int ntrials = 100;

using key_types = boost::mpl::vector<monomial<short>,monomial<integer>,k_monomial>;

template <typename Poly>
inline static Poly rn_poly(const Poly &x, const Poly &y, const Poly &z, std::uniform_int_distribution<int> &dist)
{
	int nterms = dist(rng);
	Poly retval;
	for (int i = 0; i < nterms; ++i) {
		int m = dist(rng);
		retval += m * (m % 2 ? 1 : -1 ) * x.pow(dist(rng)) * y.pow(dist(rng)) * z.pow(dist(rng));
	}
	return retval;
}

template <typename Poly>
inline static Poly rq_poly(const Poly &x, const Poly &y, const Poly &z, std::uniform_int_distribution<int> &dist)
{
	int nterms = dist(rng);
	Poly retval;
	for (int i = 0; i < nterms; ++i) {
		int m = dist(rng) , n = dist(rng);
		retval += (m * (m % 2 ? 1 : -1 ) * x.pow(dist(rng)) * y.pow(dist(rng)) * z.pow(dist(rng))) / (n == 0 ? 1 : n);
	}
	return retval;
}

struct division_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using p_type = polynomial<integer,Key>;
		using pq_type = polynomial<rational,Key>;
		p_type x{"x"}, y{"y"}, z{"z"};
		pq_type xq{"x"}, yq{"y"}, zq{"z"};
		// Zero division.
		BOOST_CHECK_THROW(p_type{1} / p_type{},zero_division_error);
		BOOST_CHECK_THROW(x / p_type{},zero_division_error);
		BOOST_CHECK_THROW(x / (x - x),zero_division_error);
		BOOST_CHECK_THROW((x + y - y) / (x - x + y - y),zero_division_error);
		// Null numerator.
		BOOST_CHECK_EQUAL(p_type{0} / p_type{-2},0);
		auto res = (x-x) / p_type{2};
		BOOST_CHECK_EQUAL(res.size(),0u);
		BOOST_CHECK(res.get_symbol_set() == symbol_set{symbol{"x"}});
		res = (x-x+y-y) / p_type{2};
		BOOST_CHECK_EQUAL(res.size(),0u);
		BOOST_CHECK((res.get_symbol_set() == symbol_set{symbol{"x"},symbol{"y"}}));
		res = (x-x+y-y) / (2+x-x+y-y);
		BOOST_CHECK_EQUAL(res.size(),0u);
		BOOST_CHECK((res.get_symbol_set() == symbol_set{symbol{"x"},symbol{"y"}}));
		// Coefficient polys.
		BOOST_CHECK_EQUAL(p_type{12} / p_type{-4},-3);
		BOOST_CHECK_EQUAL(p_type{24} / p_type{3},8);
		BOOST_CHECK_THROW(p_type{12} / p_type{11},std::invalid_argument);
		BOOST_CHECK_EQUAL(pq_type{12} / pq_type{-11},-12/11_q);
		res = (x-x+y-y+6) / p_type{2};
		BOOST_CHECK_EQUAL(res,3);
		BOOST_CHECK((res.get_symbol_set() == symbol_set{symbol{"x"},symbol{"y"}}));
		// Simple univariate tests.
		BOOST_CHECK_EQUAL(x / x,1);
		x /= x;
		BOOST_CHECK_EQUAL(x,1);
		x = p_type{"x"};
		BOOST_CHECK_EQUAL(x*x / x,x);
		BOOST_CHECK_EQUAL(x*x*x / x,x*x);
		BOOST_CHECK_EQUAL(2*x / p_type{2},x);
		BOOST_CHECK_THROW(x / p_type{2},std::invalid_argument);
		BOOST_CHECK_EQUAL(xq / pq_type{2},xq / 2);
		BOOST_CHECK_EQUAL((x+1)*(x-2)/(x + 1),x-2);
		BOOST_CHECK_EQUAL((x+1)*(x-2)*(x+3)/((x + 1)*(x+3)),x-2);
		BOOST_CHECK_THROW((x+1)*(x-2)/(x + 4),std::invalid_argument);
		// Negative exponents.
		BOOST_CHECK_THROW(x/x.pow(-1),std::invalid_argument);
		BOOST_CHECK_THROW(x.pow(-1)/x,std::invalid_argument);
		BOOST_CHECK_THROW((x.pow(-1) + y*x)/x,std::invalid_argument);
		// Will work with zero numerator.
		BOOST_CHECK_EQUAL((x-x)/(x+y.pow(-1)),0);
		// Simple multivariate tests.
		BOOST_CHECK_EQUAL((2*x*(x - y)) / x,2*x-2*y);
		BOOST_CHECK_EQUAL((2*x*z*(x - y)*(x*x-y)) / (x - y),2*x*z*(x*x-y));
		BOOST_CHECK_EQUAL((2*x*z*(x - y)*(x*x-y)) / (z*(x - y)),2*x*(x*x-y));
		BOOST_CHECK_THROW((2*x*z*(x - y)*(x*x-y)) / (4*z*(x - y)),std::invalid_argument);
		BOOST_CHECK_EQUAL((2*x*z*(x - y)*(x*x-y)) / (2*z*(x - y)),x*(x*x-y));
		BOOST_CHECK_EQUAL((2*xq*zq*(xq - yq)*(xq*xq-yq)) / (4*zq*(xq - yq)),xq*(xq*xq-yq)/2);
		BOOST_CHECK_THROW((2*x*(x - y)) / z,std::invalid_argument);
		// This fails after the mapping back to multivariate.
		BOOST_CHECK_THROW((y*y+x*x*y*y*y) / x,std::invalid_argument);
		// Random testing.
		std::uniform_int_distribution<int> dist(0,9);
		for (int i = 0; i < ntrials; ++i) {
			auto n = rn_poly(x,y,z,dist), m = rn_poly(x,y,z,dist);
			if (m.size() == 0u) {
				BOOST_CHECK_THROW(n / m,zero_division_error);
			} else {
				BOOST_CHECK_EQUAL((n * m) / m, n);
				BOOST_CHECK_THROW((n * m + 1) / m,std::invalid_argument);
				if (n.size() != 0u) {
					BOOST_CHECK_EQUAL((n * m * n) / (m * n), n);
				}
				auto tmp = n * m;
				tmp /= m;
				BOOST_CHECK_EQUAL(tmp,n);
				tmp *= m;
				math::divexact(tmp,tmp,m);
				BOOST_CHECK_EQUAL(tmp,n);
			}
		}
		// With rationals as well.
		for (int i = 0; i < ntrials; ++i) {
			auto n = rq_poly(xq,yq,zq,dist), m = rq_poly(xq,yq,zq,dist);
			if (m.size() == 0u) {
				BOOST_CHECK_THROW(n / m,zero_division_error);
			} else {
				BOOST_CHECK_EQUAL((n * m) / m, n);
				BOOST_CHECK_THROW((n * m + 1) / m,std::invalid_argument);
				if (n.size() != 0u) {
					BOOST_CHECK_EQUAL((n * m * n) / (m * n), n);
				}
				auto tmp = n * m;
				tmp /= m;
				BOOST_CHECK_EQUAL(tmp,n);
				tmp *= m;
				math::divexact(tmp,tmp,m);
				BOOST_CHECK_EQUAL(tmp,n);
			}
		}
		// Check the type traits.
		BOOST_CHECK(is_divisible<p_type>::value);
		BOOST_CHECK(is_divisible<pq_type>::value);
		BOOST_CHECK(is_divisible_in_place<p_type>::value);
		BOOST_CHECK(is_divisible_in_place<pq_type>::value);
		BOOST_CHECK(has_exact_division<p_type>::value);
		BOOST_CHECK(has_exact_division<pq_type>::value);
		BOOST_CHECK(has_exact_ring_operations<p_type>::value);
		BOOST_CHECK(has_exact_ring_operations<pq_type>::value);
	}
};

BOOST_AUTO_TEST_CASE(polynomial_division_test)
{
	environment env;
	boost::mpl::for_each<key_types>(division_tester());
	BOOST_CHECK((!has_exact_ring_operations<polynomial<double,k_monomial>>::value));
	BOOST_CHECK((!has_exact_division<polynomial<double,k_monomial>>::value));
	BOOST_CHECK((!is_divisible<polynomial<double,k_monomial>>::value));
	BOOST_CHECK((!is_divisible_in_place<polynomial<double,k_monomial>>::value));
	BOOST_CHECK((has_exact_ring_operations<polynomial<integer,monomial<rational>>>::value));
	BOOST_CHECK((!has_exact_division<polynomial<integer,monomial<rational>>>::value));
	BOOST_CHECK((!is_divisible<polynomial<integer,monomial<rational>>>::value));
	BOOST_CHECK((!is_divisible_in_place<polynomial<integer,monomial<rational>>>::value));
}

BOOST_AUTO_TEST_CASE(polynomial_division_recursive_test)
{
	using p_type = polynomial<rational,k_monomial>;
	using pp_type = polynomial<p_type,k_monomial>;
	BOOST_CHECK(has_exact_ring_operations<p_type>::value);
	BOOST_CHECK(has_exact_ring_operations<pp_type>::value);
	BOOST_CHECK(is_divisible<p_type>::value);
	BOOST_CHECK(is_divisible<pp_type>::value);
	BOOST_CHECK(has_exact_division<p_type>::value);
	BOOST_CHECK(has_exact_division<pp_type>::value);
	// A couple of simple tests.
	pp_type x{"x"};
	p_type y{"y"}, z{"z"}, t{"t"};
	BOOST_CHECK_EQUAL((x*x*y*z) / x,x*y*z);
	BOOST_CHECK_THROW((x*y*z) / (x*x),std::invalid_argument);
	BOOST_CHECK_EQUAL((x*x*y*z) / y,x*x*z);
	BOOST_CHECK_EQUAL((2*x*z*(x - y)*(x*x-y)) / (z*(x - y)),2*x*(x*x-y));
	BOOST_CHECK_EQUAL(pp_type{} / (x*y*z),0);
	BOOST_CHECK_EQUAL(pp_type{} / (x.pow(-1)*y*z),0);
	BOOST_CHECK_EQUAL(pp_type{} / (x*y*z + z.pow(-1)),0);
	// Random testing.
	std::uniform_int_distribution<int> dist(0,9);
	for (int i = 0; i < ntrials; ++i) {
		auto n_ = rq_poly(y,z,t,dist), m_ = rq_poly(y,z,t,dist);
		auto d = dist(rng);
		auto n = (n_ * dist(rng) * x.pow(dist(rng))) / (d == 0 ? 1 : d) ;
		d = dist(rng);
		auto m = (m_ * dist(rng) * x.pow(dist(rng))) / (d == 0 ? 1 : d);
		if (m.size() == 0u) {
			BOOST_CHECK_THROW(n / m,zero_division_error);
		} else {
			BOOST_CHECK_EQUAL((n * m) / m, n);
			BOOST_CHECK_THROW((n * m + 1) / m,std::invalid_argument);
			if (n.size() != 0u) {
				BOOST_CHECK_EQUAL((n * m * n) / (m * n), n);
			}
			auto tmp = n * m;
			tmp /= m;
			BOOST_CHECK_EQUAL(tmp,n);
			tmp *= m;
			math::divexact(tmp,tmp,m);
			BOOST_CHECK_EQUAL(tmp,n);
		}
	}
}

struct uprem_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using p_type = polynomial<integer,Key>;
		p_type x{"x"}, y{"y"};
		BOOST_CHECK_THROW(p_type::uprem(x+y,x+y),std::invalid_argument);
		BOOST_CHECK_THROW(p_type::uprem(x,y),std::invalid_argument);
		BOOST_CHECK_THROW(p_type::uprem(x,x-x),zero_division_error);
		BOOST_CHECK_EQUAL(p_type::uprem(x-x,x),0);
		BOOST_CHECK_THROW(p_type::uprem(x,x.pow(2)),std::invalid_argument);
		BOOST_CHECK_EQUAL(p_type::uprem(x.pow(2),x),0);
		BOOST_CHECK_EQUAL(p_type::uprem(x,x+1),-1);
		// Random testing.
		std::uniform_int_distribution<int> dist(0,9);
		for (int i = 0; i < ntrials; ++i) {
			p_type n, d;
			auto nterms = dist(rng);
			for (int j = 0; j < nterms; ++j) {
				auto tmp = dist(rng);
				n += x.pow(dist(rng)) * (tmp % 2 ? tmp : -tmp);
			}
			nterms = dist(rng);
			for (int j = 0; j < nterms; ++j) {
				auto tmp = dist(rng);
				d += x.pow(dist(rng)) * (tmp % 2 ? tmp : -tmp);
			}
			if (n.get_symbol_set() != d.get_symbol_set() || n.get_symbol_set().size() != 1u ||
				(n.degree() < d.degree() && n.size() != 0u))
			{
				BOOST_CHECK_THROW(p_type::uprem(n,d),std::invalid_argument);
			} else if (d.size() == 0u) {
				BOOST_CHECK_THROW(p_type::uprem(n,d),zero_division_error);
			} else {
				BOOST_CHECK_NO_THROW(p_type::uprem(n,d));
			}
		}
	}
};

BOOST_AUTO_TEST_CASE(polynomial_uprem_test)
{
	boost::mpl::for_each<key_types>(uprem_tester());
}

struct content_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using p_type = polynomial<integer,Key>;
		p_type x{"x"}, y{"y"};
		BOOST_CHECK_EQUAL(p_type{}.content(),0);
		BOOST_CHECK_EQUAL((x-x).content(),0);
		BOOST_CHECK_EQUAL(p_type{1}.content(),1);
		BOOST_CHECK_EQUAL(p_type{2}.content(),2);
		BOOST_CHECK_EQUAL(x.content(),1);
		BOOST_CHECK_EQUAL((2*x).content(),2);
		BOOST_CHECK_EQUAL((12*x+9*y).content(),3);
		BOOST_CHECK_EQUAL((12*x+8*y+6*x*y).content(),2);
	}
};

BOOST_AUTO_TEST_CASE(polynomial_content_test)
{
	boost::mpl::for_each<key_types>(content_tester());
}

struct pp_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using p_type = polynomial<integer,Key>;
		p_type x{"x"}, y{"y"};
		BOOST_CHECK_THROW(p_type{}.primitive_part(),zero_division_error);
		BOOST_CHECK_THROW((x-x).primitive_part(),zero_division_error);
		BOOST_CHECK_EQUAL(x.primitive_part(),x);
		BOOST_CHECK_EQUAL(p_type{1}.primitive_part(),1);
		BOOST_CHECK_EQUAL(p_type{2}.primitive_part(),1);
		BOOST_CHECK_EQUAL((2*x).primitive_part(),x);
		BOOST_CHECK_EQUAL((12*x+9*y).primitive_part(),4*x+3*y);
		BOOST_CHECK_EQUAL((12*x+8*y+6*x*y).primitive_part(),6*x+4*y+3*x*y);
	}
};

BOOST_AUTO_TEST_CASE(polynomial_pp_test)
{
	boost::mpl::for_each<key_types>(pp_tester());
}

struct gcd_tester
{
	template <typename Key>
	void operator()(const Key &)
	{
		using p_type = polynomial<integer,Key>;
		BOOST_CHECK(has_gcd<p_type>::value);
		BOOST_CHECK(has_gcd3<p_type>::value);
		// Some zero tests.
		BOOST_CHECK_EQUAL(p_type::gcd(p_type{},p_type{}),0);
		p_type x{"x"}, y{"y"}, z{"z"};
		BOOST_CHECK_EQUAL(p_type::gcd(x,p_type{}),x);
		BOOST_CHECK_EQUAL(p_type::gcd(p_type{},x),x);
		BOOST_CHECK_EQUAL(p_type::gcd(x-x,p_type{}),0);
		BOOST_CHECK_EQUAL(p_type::gcd(x-x,y-y),0);
		BOOST_CHECK_EQUAL(p_type::gcd(x-x+y-y,y-y),0);
		// The test from the Geddes book.
		auto a = -30*x.pow(3)*y+90*x*x*y*y+15*x*x-60*x*y+45*y*y;
		auto b = 100*x*x*y-140*x*x-250*x*y*y+350*x*y-150*y*y*y+210*y*y;
		BOOST_CHECK_EQUAL(p_type::gcd(a,b),-15*y+5*x);
		// Random testing.
		std::uniform_int_distribution<int> dist(0,4);
		for (int i = 0; i < ntrials; ++i) {
			auto n = rn_poly(x,y,z,dist), m = rn_poly(x,y,z,dist), r = rn_poly(x,y,z,dist);
			auto g = p_type::gcd(n*r,m*n);
std::cout << "i=" << i << '\n';
std::cout << (n*r) << '\n';
std::cout << (m*n) << '\n';
std::cout << g << '\n';
			if (math::is_zero(m*n)) {
				BOOST_CHECK_EQUAL(g,n*r);
			} else if (math::is_zero(n*r)) {
				BOOST_CHECK_EQUAL(g,m*n);
			} else {
				BOOST_CHECK_NO_THROW((n * r) / g);
				BOOST_CHECK_NO_THROW((m * n) / g);
				BOOST_CHECK_EQUAL(g,p_type::gcd(m*n,n*r));
			}
		}
	}
};

BOOST_AUTO_TEST_CASE(polynomial_gcd_test)
{
	boost::mpl::for_each<key_types>(gcd_tester());
}

//BOOST_AUTO_TEST_CASE(polynomial_gcd_bug_test)
//{
//	using p_type = polynomial<integer,monomial<short>>;
//	using math::pow;
//	p_type x{"x"}, y{"y"}, z{"z"};
//	auto n = 9*x*pow(y,8)+5*x*x*pow(y,9)*pow(z,7)+9*pow(x,3)*pow(z,3)+5*pow(x,6)*pow(y,8)*pow(z,8)-8*pow(x,8)*y*y*pow(z,7)-8*pow(x,7)*pow(y,7)*pow(z,5)-8*pow(x,9)*pow(y,7)*pow(z,5);
//	auto m = 5*pow(x,6)*pow(y,5)*pow(z,6)+9*pow(x,4)*pow(y,3)*pow(z,8)+pow(x,5)*pow(y,5)*pow(z,8)-8*pow(x,9)*y*pow(z,3)-2*pow(x,5)*pow(y,8)*pow(z,5)-2*pow(x,7)*pow(y,9)*pow(z,2)+5*pow(x,9)
//		*pow(y,4)*pow(z,9)-8*pow(x,5)*pow(y,7)*pow(z,5);
//	std::cout << p_type::gcd(n,m) << '\n';
//}

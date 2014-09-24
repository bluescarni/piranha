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

#include <boost/lexical_cast.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>

#include "../src/config.hpp"
#include "../src/debug_access.hpp"
#include "../src/environment.hpp"
#include "../src/forwarding.hpp"
#include "../src/math.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/polynomial_term.hpp"
#include "../src/real.hpp"
#include "../src/series.hpp"
#include "../src/settings.hpp"
#include "../src/symbol.hpp"
#include "../src/univariate_monomial.hpp"

// NOTE: when we specialize for univariate monomials, review the test here and move the unviariate
// specific ones in separate test.

using namespace piranha;

typedef boost::mpl::vector<double,integer,rational> cf_types;
typedef boost::mpl::vector<int,integer> expo_types;

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
		PIRANHA_FORWARDING_CTOR(polynomial_alt,base)
		~polynomial_alt() = default;
		polynomial_alt &operator=(const polynomial_alt &) = default;
		polynomial_alt &operator=(polynomial_alt &&) = default;
		PIRANHA_FORWARDING_ASSIGNMENT(polynomial_alt,base)
};

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
			BOOST_CHECK(p2 != p_type{std::string("y")});
			BOOST_CHECK(p2 == p_type{"x"} + p_type{"y"} - p_type{"y"});
			// Construction from number-like entities.
			p_type p3{3};
			BOOST_CHECK(p3.size() == 1u);
			BOOST_CHECK(p3 == 3);
			BOOST_CHECK(3 == p3);
			BOOST_CHECK(p3 != p2);
// NOTE: same problem as in poisson_series.
#if !defined(PIRANHA_COMPILER_IS_CLANG)
			p_type p3a{integer(3)};
			BOOST_CHECK(p3a == p3);
			BOOST_CHECK(p3 == p3a);
#endif
			// Construction from polynomial of different type.
			typedef polynomial<long,int> p_type1;
			typedef polynomial<int,short> p_type2;
			p_type1 p4(1);
			p_type2 p5(p4);
			BOOST_CHECK(p4 == p5);
			BOOST_CHECK(p5 == p4);
			p_type1 p6("x");
			p_type2 p7(std::string("x"));
			p_type2 p8("y");
			BOOST_CHECK(p6 == p7);
			BOOST_CHECK(p7 == p6);
			BOOST_CHECK(p6 != p8);
			BOOST_CHECK(p8 != p6);
			// Type traits checks.
			BOOST_CHECK((std::is_constructible<p_type,Cf>::value));
			BOOST_CHECK((std::is_constructible<p_type,std::string>::value));
			BOOST_CHECK((std::is_constructible<p_type2,p_type1>::value));
			BOOST_CHECK((!std::is_constructible<p_type,symbol>::value));
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
	environment env;
	boost::mpl::for_each<cf_types>(constructor_tester());
}

struct is_evaluable_tester
{
	template <typename Cf>
	struct runner
	{
		// NOTE: this is temporary, the enable_if has to be removed once we implement evaluation for the univariate monomial.
		template <typename Expo>
		void operator()(const Expo &, typename std::enable_if<!is_instance_of<typename polynomial<Cf,Expo>::term_type::key_type,univariate_monomial>::value>::type * = nullptr)
		{
			typedef polynomial<Cf,Expo> p_type;
			BOOST_CHECK((is_evaluable<p_type,double>::value));
			BOOST_CHECK((is_evaluable<p_type,float>::value));
			BOOST_CHECK((is_evaluable<p_type,integer>::value));
			BOOST_CHECK((is_evaluable<p_type,int>::value));
		}
		template <typename Expo>
		void operator()(const Expo &, typename std::enable_if<is_instance_of<typename polynomial<Cf,Expo>::term_type::key_type,univariate_monomial>::value>::type * = nullptr)
		{}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(polynomial_is_evaluable_test)
{
	boost::mpl::for_each<cf_types>(is_evaluable_tester());
	BOOST_CHECK((!is_evaluable<polynomial<mock_cf,int>,double>::value));
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
#if !defined(PIRANHA_COMPILER_IS_CLANG)
			p1 = integer(10);
			BOOST_CHECK(p1 == integer(10));
#endif
			p1 = "x";
			BOOST_CHECK(p1 == p_type("x"));
			BOOST_CHECK((std::is_assignable<p_type,Cf>::value));
			BOOST_CHECK((std::is_assignable<p_type,std::string>::value));
			BOOST_CHECK((std::is_assignable<p_type,p_type>::value));
			BOOST_CHECK((!std::is_assignable<p_type,symbol>::value));
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
	BOOST_CHECK(has_degree<p_type1>::value);
	BOOST_CHECK(has_ldegree<p_type1>::value);
	BOOST_CHECK(has_degree<p_type11>::value);
	BOOST_CHECK(has_ldegree<p_type11>::value);
	BOOST_CHECK(has_degree<p_type111>::value);
	BOOST_CHECK(has_ldegree<p_type111>::value);
	p_type1 x("x");
	BOOST_CHECK(math::degree(x) == 1);
	BOOST_CHECK(math::ldegree(x) == 1);
	BOOST_CHECK(math::degree(x * x) == 2);
	BOOST_CHECK(math::ldegree(x * x) == 2);
	BOOST_CHECK(math::degree(x * x,{"y","z"}) == 0);
	BOOST_CHECK(math::ldegree(x * x,{"y","z"}) == 0);
	p_type11 y("y");
	p_type111 z("z");
	BOOST_CHECK(math::degree(x * y) == 2);
	BOOST_CHECK(math::degree(x * y* z) == 3);
	BOOST_CHECK(math::ldegree(x * y* z) == 3);
	BOOST_CHECK(math::degree(x * y* z,{"x"}) == 1);
	BOOST_CHECK(math::ldegree(x * y* z,{"x"}) == 1);
	BOOST_CHECK(math::degree(x * y* z,{"y"}) == 1);
	BOOST_CHECK(math::ldegree(x * y* z,{"y"}) == 1);
	BOOST_CHECK(math::degree(x * y* z,{"z"}) == 1);
	BOOST_CHECK(math::ldegree(x * y* z,{"z"}) == 1);
	BOOST_CHECK(math::degree(x * y* z,{"z","y"}) == 2);
	BOOST_CHECK(math::ldegree(x * y* z,{"z","y"}) == 2);
	BOOST_CHECK(math::degree(x * y* z,{"z","x"}) == 2);
	BOOST_CHECK(math::ldegree(x * y* z,{"z","x"}) == 2);
	BOOST_CHECK(math::degree(x * y* z,{"y","x"}) == 2);
	BOOST_CHECK(math::ldegree(x * y* z,{"y","x"}) == 2);
	BOOST_CHECK(math::degree(x * y* z,{"y","x","z"}) == 3);
	BOOST_CHECK(math::ldegree(x * y* z,{"y","x","z"}) == 3);
	BOOST_CHECK(math::degree(x + y + z) == 1);
	BOOST_CHECK(math::ldegree(x + y + z) == 1);
	BOOST_CHECK(math::degree(x + y + z,{"x"}) == 1);
	BOOST_CHECK(math::ldegree(x + y + z,{"x"}) == 0);
	BOOST_CHECK(math::ldegree(x + y + z,{"x","y"}) == 0);
	BOOST_CHECK(math::ldegree(x + y + 1,{"x","y"}) == 0);
	BOOST_CHECK(math::ldegree(x + y + 1,{"x","y","t"}) == 0);
	BOOST_CHECK(math::ldegree(x + y + 1) == 0);
}

struct multiplication_tester
{
	template <typename Cf>
	void operator()(const Cf &)
	{
		// NOTE: this test is going to be exact in case of coefficients cancellations with double
		// precision coefficients only if the platform has ieee 754 format (integer exactly representable
		// as doubles up to 2 ** 53).
		if (std::is_same<Cf,double>::value && (!std::numeric_limits<double>::is_iec559 || std::numeric_limits<double>::radix != 2 ||
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

struct integral_combination_tag {};

namespace piranha
{
template <>
class debug_access<integral_combination_tag>
{
	public:
		template <typename Cf>
		struct runner
		{
			template <typename Expo>
			void operator()(const Expo &)
			{
				// Skip tests for fp values.
				if (std::is_floating_point<Cf>::value) {
					return;
				}
				typedef polynomial<Cf,Expo> p_type;
				typedef std::map<std::string,integer> map_type;
				p_type p1;
				BOOST_CHECK((p1.integral_combination() == map_type{}));
				p1 = "x";
				BOOST_CHECK((p1.integral_combination() == map_type{{"x",integer(1)}}));
				p1 += 2 * p_type{"y"};
				BOOST_CHECK((p1.integral_combination() == map_type{{"y",integer(2)},{"x",integer(1)}}));
				p1 = p_type{"x"} + 1;
				BOOST_CHECK_THROW(p1.integral_combination(),std::invalid_argument);
				p1 = p_type{"x"}.pow(2);
				BOOST_CHECK_THROW(p1.integral_combination(),std::invalid_argument);
				p1 = p_type{"x"} * 2 - p_type{"z"} * 3;
				BOOST_CHECK((p1.integral_combination() == map_type{{"x",integer(2)},{"z",integer(-3)}}));
			}
		};
		template <typename Cf>
		void operator()(const Cf &)
		{
			boost::mpl::for_each<expo_types>(runner<Cf>());
			// Tests specific to rational, double and real.
			typedef polynomial<rational,int> p_type;
			typedef std::map<std::string,integer> map_type;
			p_type p1;
			p1 = p_type{"x"} * rational(4,2) + p_type{"y"} * 4;
			BOOST_CHECK((p1.integral_combination() == map_type{{"x",integer(2)},{"y",integer(4)}}));
			p1 = p_type{"x"} * rational(4,3) + p_type{"y"} * 4;
			BOOST_CHECK_THROW(p1.integral_combination(),std::invalid_argument);
			p1 = 3 * (p_type{"x"} * rational(5,3) - p_type{"y"} * 4);
			BOOST_CHECK((p1.integral_combination() == map_type{{"x",integer(5)},{"y",integer(-12)}}));
			if (std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::radix == 2 && std::numeric_limits<double>::has_infinity &&
				std::numeric_limits<double>::has_quiet_NaN)
			{
				typedef polynomial<double,int> p_type2;
				p_type2 p2;
				p2 = p_type2{"x"} * 2. + p_type2{"y"} * 4.;
				BOOST_CHECK((p2.integral_combination() == map_type{{"x",integer(2)},{"y",integer(4)}}));
				p2 = p_type2{"x"} * 2.5 + p_type2{"y"} * 4.;
				BOOST_CHECK_THROW(p2.integral_combination(),std::invalid_argument);
			}
			typedef polynomial<real,int> p_type3;
			p_type3 p3;
			p3 = p_type3{"x"} * 2 + p_type3{"y"} * 4;
			BOOST_CHECK((p3.integral_combination() == map_type{{"x",integer(2)},{"y",integer(4)}}));
			p3 = p_type3{"x"} * real{"2.5"} + p_type3{"y"} * 4.;
			BOOST_CHECK_THROW(p3.integral_combination(),std::invalid_argument);
		}
};
}

typedef debug_access<integral_combination_tag> ic_tester;

BOOST_AUTO_TEST_CASE(polynomial_integral_combination_test)
{
	boost::mpl::for_each<cf_types>(ic_tester());
}

struct pow_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Expo>
		void operator()(const Expo &)
		{
			typedef polynomial<Cf,Expo> p_type;
			p_type p{"x"};
			BOOST_CHECK_EQUAL((2 * p).pow(4),p_type{math::pow(Cf(1) * 2,4)} * p * p * p * p);
			p *= p_type{"y"}.pow(2);
			BOOST_CHECK_EQUAL((3 * p).pow(4),p_type{math::pow(Cf(1) * 3,4)} * p * p * p * p);
			if (!std::is_unsigned<Expo>::value) {
				BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p.pow(-1)),"x**-1*y**-2");
			}
			BOOST_CHECK_EQUAL(p.pow(0),p_type{math::pow(Cf(1),0)});
			BOOST_CHECK_EQUAL(p_type{3}.pow(4),math::pow(Cf(3),4));
			BOOST_CHECK_THROW((p + p_type{"x"}).pow(-1),std::invalid_argument);
			BOOST_CHECK_EQUAL((p + p_type{"x"}).pow(0),Cf(1));
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<expo_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(polynomial_pow_test)
{
	boost::mpl::for_each<cf_types>(pow_tester());
	typedef polynomial<integer,int> p_type1;
	BOOST_CHECK((is_exponentiable<p_type1,integer>::value));
	BOOST_CHECK((is_exponentiable<const p_type1,integer>::value));
	BOOST_CHECK((is_exponentiable<p_type1 &,integer>::value));
	BOOST_CHECK((is_exponentiable<p_type1 &,integer &>::value));
	BOOST_CHECK((!is_exponentiable<p_type1,std::string>::value));
	BOOST_CHECK((!is_exponentiable<p_type1 &,std::string &>::value));
	BOOST_CHECK((!is_exponentiable<p_type1,double>::value));
	typedef polynomial<real,int> p_type2;
	BOOST_CHECK((is_exponentiable<p_type2,integer>::value));
	BOOST_CHECK((is_exponentiable<p_type2,real>::value));
	BOOST_CHECK((!is_exponentiable<p_type2,std::string>::value));
}

BOOST_AUTO_TEST_CASE(polynomial_partial_test)
{
	using math::partial;
	using math::pow;
	typedef polynomial<rational,short> p_type1;
	p_type1 x{"x"}, y{"y"};
	BOOST_CHECK_EQUAL(partial(x * y,"x"),y);
	BOOST_CHECK_EQUAL(partial(x * y,"y"),x);
	BOOST_CHECK_EQUAL(partial((x * y + x - 3 * pow(y,2)).pow(10),"y"),10 * (x * y + x - 3 * pow(y,2)).pow(9) * (x - 6 * y));
	BOOST_CHECK_EQUAL(partial((x * y + x - 3 * pow(y,2)).pow(10),"z"),0);
	BOOST_CHECK(is_differentiable<p_type1>::value);
	BOOST_CHECK(has_pbracket<p_type1>::value);
	BOOST_CHECK(has_transformation_is_canonical<p_type1>::value);
	BOOST_CHECK((!is_differentiable<polynomial<mock_cf,short>>::value));
	BOOST_CHECK((!has_pbracket<polynomial<mock_cf,short>>::value));
	BOOST_CHECK((!has_transformation_is_canonical<polynomial<mock_cf,short>>::value));
}

BOOST_AUTO_TEST_CASE(polynomial_subs_test)
{
	{
	typedef polynomial<rational,short> p_type1;
	BOOST_CHECK_EQUAL(p_type1{"x"}.subs("x",integer(1)),1);
	BOOST_CHECK_EQUAL(p_type1{"x"}.subs("x",p_type1{"x"}),p_type1{"x"});
	p_type1 x{"x"}, y{"y"}, z{"z"};
	BOOST_CHECK_EQUAL((x.pow(2) + x * y + z).subs("x",integer(3)),9 + 3 * y + z);
	BOOST_CHECK_EQUAL((x.pow(2) + x * y + z).subs("y",rational(3,2)),x * x + x * rational(3,2) + z);
	BOOST_CHECK_EQUAL((x.pow(2) + x * y + z).subs("k",rational(3,2)),x * x + x * y + z);
	BOOST_CHECK_EQUAL(x.pow(-1).subs("x",x.pow(-1)),x);
	BOOST_CHECK_EQUAL((x.pow(2) + x * y + z).subs("x",rational(3,2)).subs("y",rational(4,5)).subs("z",-rational(6,7)),
		(x.pow(2) + x * y + z).evaluate(std::unordered_map<std::string,rational>{{"x",rational(3,2)},
		{"y",rational(4,5)},{"z",-rational(6,7)}}));
	BOOST_CHECK_EQUAL(math::subs(x.pow(2) + x * y + z,"x",rational(3,2)).subs("y",rational(4,5)).subs("z",-rational(6,7)),
		(x.pow(2) + x * y + z).evaluate(std::unordered_map<std::string,rational>{{"x",rational(3,2)},
		{"y",rational(4,5)},{"z",-rational(6,7)}}));
	BOOST_CHECK((std::is_same<decltype(p_type1{"x"}.subs("x",integer(1))),p_type1>::value));
	BOOST_CHECK((std::is_same<decltype(p_type1{"x"}.subs("x",rational(1))),p_type1>::value));
	BOOST_CHECK_EQUAL((x.pow(2) + x * y + z).subs("k",rational(3,2)),x * x + x * y + z);
	BOOST_CHECK_EQUAL(((y + 4 * z).pow(5) * x.pow(-1)).subs("x",rational(3)),((y + 4 * z).pow(5)) / 3);
	}
	{
	typedef polynomial<real,int> p_type2;
	p_type2 x{"x"}, y{"y"};
	BOOST_CHECK_EQUAL((x*x*x + y*y).subs("x",real(1.234)),y*y + math::pow(real(1.234),3));
	BOOST_CHECK_EQUAL((x*x*x + y*y).subs("x",real(1.234)).subs("y",real(-5.678)),math::pow(real(-5.678),2) +
		math::pow(real(1.234),3));
	BOOST_CHECK_EQUAL(math::subs(x*x*x + y*y,"x",real(1.234)).subs("y",real(-5.678)),math::pow(real(-5.678),2) +
		math::pow(real(1.234),3));
	}
	typedef polynomial<integer,long> p_type3;
	p_type3 x{"x"}, y{"y"}, z{"z"};
	BOOST_CHECK_EQUAL((x*x*x + y*y + z*y*x).subs("x",integer(2)).subs("y",integer(-3)).subs("z",integer(4)).subs("k",integer()),
		integer(2).pow(3) + integer(-3).pow(2) + integer(2) * integer(-3) * integer(4));
	BOOST_CHECK_EQUAL(math::subs(x*x*x + y*y + z*y*x,"x",integer(2)).subs("y",integer(-3)).subs("z",integer(4)).subs("k",integer()),
		integer(2).pow(3) + integer(-3).pow(2) + integer(2) * integer(-3) * integer(4));
	BOOST_CHECK_EQUAL((x*x*x + y*y + z*y*x).subs("x",integer(0)).subs("y",integer(0)).subs("z",integer(0)).subs("k",integer()),0);
}

BOOST_AUTO_TEST_CASE(polynomial_integrate_test)
{
	// Simple echelon-1 polynomial.
	typedef polynomial<rational,short> p_type1;
	BOOST_CHECK(is_integrable<p_type1>::value);
	BOOST_CHECK(is_integrable<p_type1 &>::value);
	BOOST_CHECK(is_integrable<const p_type1>::value);
	BOOST_CHECK(is_integrable<p_type1 const &>::value);
	p_type1 x("x"), y("y"), z("z");
	BOOST_CHECK_EQUAL(p_type1{}.integrate("x"),p_type1{});
	BOOST_CHECK_EQUAL(x.integrate("x"),x * x / 2);
	BOOST_CHECK_EQUAL(y.integrate("x"),x * y);
	BOOST_CHECK_EQUAL((x + 3*y*x*x + z*y*x / 4).integrate("x"),x*x/2 + y*x*x*x + z*y*x*x/8);
	BOOST_CHECK_THROW(x.pow(-1).integrate("x"),std::invalid_argument);
	BOOST_CHECK_EQUAL((x + 3*y*x*x + z*y*x / 4).integrate("x").partial("x"),x + 3*y*x*x + z*y*x / 4);
	BOOST_CHECK_EQUAL((x + 3*y*x*x + z*y*x / 4).integrate("y").partial("y"),x + 3*y*x*x + z*y*x / 4);
	BOOST_CHECK_EQUAL((x + 3*y*x*x + z*y*x / 4).integrate("z").partial("z"),x + 3*y*x*x + z*y*x / 4);
	BOOST_CHECK_EQUAL(p_type1{4}.integrate("z"),4 * z);
	BOOST_CHECK_EQUAL((x * y * z).pow(-5).integrate("x"),(y * z).pow(-5) * x.pow(-4) * rational(1,-4));
	// Polynomial with polynomial coefficient, no variable mixing.
	typedef polynomial<p_type1,short> p_type11;
	BOOST_CHECK(is_integrable<p_type11>::value);
	BOOST_CHECK(is_integrable<p_type11 &>::value);
	BOOST_CHECK(is_integrable<const p_type11>::value);
	BOOST_CHECK(is_integrable<p_type11 const &>::value);
	p_type11 a("a"), b("b"), c("c");
	BOOST_CHECK_EQUAL((a*x).integrate("x"),a*x*x/2);
	BOOST_CHECK_EQUAL((a*x).integrate("a"),a*a*x/2);
	BOOST_CHECK_EQUAL((a*x*x + b*x/15 - c * x * y).integrate("x"),a*x*x*x/3 + b*x*x/30 - c*x*x*y/2);
	BOOST_CHECK_EQUAL((a*((x*x).pow(-1)) + b*x/15 - a * y).integrate("x"),-a*(x).pow(-1) + b*x*x/30 - a*x*y);
	BOOST_CHECK_THROW((a*(x).pow(-1) + b*x/15 - a * y).integrate("x"),std::invalid_argument);
	BOOST_CHECK_EQUAL((a*x*x + b*x/15 - a * y).integrate("a"),a*a*x*x/2 + a*b*x/15 - a*a*y/2);
	BOOST_CHECK_EQUAL(math::integrate(a*x*x + b*x/15 - a * y,"a"),a*a*x*x/2 + a*b*x/15 - a*a*y/2);
	BOOST_CHECK_EQUAL((7 * x * a.pow(-2) + b*x/15 - a * y).integrate("a"),-7*x*a.pow(-1) + a*b*x/15 - a*a*y/2);
	BOOST_CHECK_EQUAL((7 * x * a.pow(-2) - a * y + b*x/15).integrate("a"),-7*x*a.pow(-1) + a*b*x/15 - a*a*y/2);
	BOOST_CHECK_EQUAL(math::integrate(x.pow(4) * y * a.pow(4) + x * y * b,"x"),x.pow(5) * y * a.pow(4) / 5 + x * x / 2 * y * b);
	// Variable mixing (integration by parts).
	p_type11 xx("x"), yy("y"), zz("z");
	BOOST_CHECK_EQUAL((x*xx).integrate("x"),x*x * xx / 2 - math::integrate(x*x/2,"x"));
	BOOST_CHECK_EQUAL(((3*x + y)*xx).integrate("x"),(3*x*x + 2*x*y) * xx / 2 - math::integrate((3*x*x + 2*x*y)/2,"x"));
	BOOST_CHECK_EQUAL((x*xx*xx).integrate("x"),x*x * xx * xx / 2 - 2 * xx  * x * x * x / 6 + 2 * x * x * x * x / 24);
	BOOST_CHECK_EQUAL(math::partial((x*xx*xx).integrate("x"),"x"),x*xx*xx);
	BOOST_CHECK_THROW((x.pow(-1)*xx*xx).integrate("x"),std::invalid_argument);
	BOOST_CHECK_THROW((x.pow(-2)*xx*xx).integrate("x"),std::invalid_argument);
	BOOST_CHECK_THROW((x.pow(-3)*xx*xx).integrate("x"),std::invalid_argument);
	BOOST_CHECK_EQUAL((x.pow(-4)*xx*xx).integrate("x"),-x.pow(-3)/3*xx*xx - x.pow(-2) * 2 * xx / 6 - 2 * x.pow(-1) / 6);
	BOOST_CHECK_EQUAL((x.pow(-4)*xx).integrate("x"),-x.pow(-3)/3*xx - x.pow(-2) / 6);
	BOOST_CHECK_EQUAL((y * x.pow(-4)*xx*xx).integrate("x"),y*(-x.pow(-3)/3*xx*xx - x.pow(-2) * 2 * xx / 6 - 2 * x.pow(-1) / 6));
	BOOST_CHECK_EQUAL(((y + z.pow(2) * y) * x.pow(-4)*xx*xx).integrate("x"),(y + z.pow(2) * y) * (-x.pow(-3)/3*xx*xx - x.pow(-2) * 2 * xx / 6 - 2 * x.pow(-1) / 6));
	BOOST_CHECK_EQUAL(((y + z.pow(2) * y) * x.pow(-4)*xx*xx - x.pow(-4)*xx).integrate("x"),
		(y + z.pow(2) * y) * (-x.pow(-3)/3*xx*xx - x.pow(-2) * 2 * xx / 6 - 2 * x.pow(-1) / 6) - (-x.pow(-3)/3*xx - x.pow(-2) / 6));
	// Misc tests.
	BOOST_CHECK_EQUAL(math::partial((x + y + z).pow(10).integrate("x"),"x"),(x + y + z).pow(10));
	BOOST_CHECK_EQUAL(math::partial((x + y + z).pow(10).integrate("y"),"y"),(x + y + z).pow(10));
	BOOST_CHECK_EQUAL(math::partial((x + y + z).pow(10).integrate("z"),"z"),(x + y + z).pow(10));
	BOOST_CHECK_THROW((x*xx.pow(-1)).integrate("x"),std::invalid_argument);
	BOOST_CHECK_EQUAL((x*xx.pow(-1)).integrate("y"),x * xx.pow(-1) * yy);
	BOOST_CHECK_THROW((x*yy.pow(-1)).integrate("y"),std::invalid_argument);
	BOOST_CHECK_EQUAL((x*yy.pow(-2)).integrate("y"),-x*yy.pow(-1));
	// Non-integrable coefficient.
	typedef polynomial<polynomial_alt<rational,int>,int> p_type_alt;
	p_type_alt n("n"), m("m");
	BOOST_CHECK_EQUAL(math::integrate(n * m + m,"n"),n*n*m/2 + m*n);
	BOOST_CHECK_EQUAL(math::integrate(n * m + m,"m"),m*n*m/2 + m*m/2);
	BOOST_CHECK_THROW(math::integrate(p_type_alt{polynomial_alt<rational,int>{"m"}},"m"),std::invalid_argument);
	BOOST_CHECK_EQUAL(math::integrate(p_type_alt{polynomial_alt<rational,int>{"n"}},"m"),(polynomial_alt<rational,int>{"n"} * m));
	BOOST_CHECK_EQUAL(math::integrate(p_type_alt{polynomial_alt<rational,int>{"m"}},"n"),(polynomial_alt<rational,int>{"m"} * n));
}

BOOST_AUTO_TEST_CASE(polynomial_ipow_subs_test)
{
	typedef polynomial<rational,int> p_type1;
	BOOST_CHECK(has_ipow_subs<p_type1>::value);
	BOOST_CHECK((has_ipow_subs<p_type1,integer>::value));
	{
	BOOST_CHECK_EQUAL(p_type1{"x"}.ipow_subs("x",integer(4),integer(1)),p_type1{"x"});
	BOOST_CHECK_EQUAL(p_type1{"x"}.ipow_subs("x",integer(1),p_type1{"x"}),p_type1{"x"});
	p_type1 x{"x"}, y{"y"}, z{"z"};
	BOOST_CHECK_EQUAL((x.pow(2) + x * y + z).ipow_subs("x",integer(2),integer(3)),3 + x * y + z);
	BOOST_CHECK_EQUAL((x.pow(2) + x * y + z).ipow_subs("y",integer(1),rational(3,2)),x * x + x * rational(3,2) + z);
	BOOST_CHECK_EQUAL((x.pow(7) + x.pow(2) * y + z).ipow_subs("x",integer(3),x),x.pow(3) + x.pow(2) * y + z);
	BOOST_CHECK_EQUAL((x.pow(6) + x.pow(2) * y + z).ipow_subs("x",integer(3),p_type1{}),x.pow(2) * y + z);
	BOOST_CHECK_EQUAL((1+3*x.pow(2)-5*y.pow(5)).pow(10).ipow_subs("x",integer(2),p_type1{"x2"})
		.subs("x2",x.pow(2)),(1+3*x.pow(2)-5*y.pow(5)).pow(10));
	}
	{
	typedef polynomial<real,int> p_type2;
	BOOST_CHECK(has_ipow_subs<p_type2>::value);
	BOOST_CHECK((has_ipow_subs<p_type2,integer>::value));
	p_type2 x{"x"}, y{"y"};
	BOOST_CHECK_EQUAL((x*x*x + y*y).ipow_subs("x",integer(1),real(1.234)),y*y + math::pow(real(1.234),3));
	BOOST_CHECK_EQUAL((x*x*x + y*y).ipow_subs("x",integer(3),real(1.234)),y*y + real(1.234));
	BOOST_CHECK_EQUAL((x*x*x + y*y).ipow_subs("x",integer(2),real(1.234)).ipow_subs("y",integer(2),real(-5.678)),real(-5.678) +
		real(1.234) * x);
	BOOST_CHECK_EQUAL(math::ipow_subs(x*x*x + y*y,"x",integer(1),real(1.234)).ipow_subs("y",integer(1),real(-5.678)),math::pow(real(-5.678),2) +
		math::pow(real(1.234),3));
	}
	typedef polynomial<integer,long> p_type3;
	BOOST_CHECK(has_ipow_subs<p_type3>::value);
	BOOST_CHECK((has_ipow_subs<p_type3,integer>::value));
	p_type3 x{"x"}, y{"y"}, z{"z"};
	BOOST_CHECK_EQUAL(math::ipow_subs(x.pow(-7) + y + z,"x",integer(2),y),x.pow(-7) + y + z);
	BOOST_CHECK_EQUAL(math::ipow_subs(x.pow(-7) + y + z,"x",integer(-2),y),x.pow(-1) * y.pow(3) + y + z);
	BOOST_CHECK_EQUAL(math::ipow_subs(x.pow(-7) + y + z,"x",integer(-7),z),y + 2*z);
}

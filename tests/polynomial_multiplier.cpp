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

#define BOOST_TEST_MODULE polynomial_multiplier_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include "../src/environment.hpp"
#include "../src/kronecker_array.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/pow.hpp"

using namespace piranha;

using cf_types = boost::mpl::vector<double,integer,rational>;
using k_types = boost::mpl::vector<monomial<int>,monomial<integer>,monomial<rational>,
	// This should ensure in the overflow tests below we have enough bit width (we
	// are testing with 3 variables).
	kronecker_monomial<std::int_least64_t>>;

struct bounds_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Key, typename std::enable_if<detail::is_monomial<Key>::value && !std::is_integral<typename Key::value_type>::value,int>::type = 0>
		void operator()(const Key &)
		{}
		template <typename Key, typename std::enable_if<detail::is_monomial<Key>::value && std::is_integral<typename Key::value_type>::value,int>::type = 0>
		void operator()(const Key &)
		{
			using pt = polynomial<Cf,Key>;
			using int_type = typename Key::value_type;
			auto x = pt{"x"}, y = pt{"y"};
			BOOST_CHECK_THROW(math::pow(x,std::numeric_limits<int_type>::max()) * x,std::overflow_error);
			BOOST_CHECK_THROW(math::pow(x,std::numeric_limits<int_type>::min()) * x.pow(-1),std::overflow_error);
			BOOST_CHECK_EQUAL(math::pow(x,std::numeric_limits<int_type>::max() - 1) * x,math::pow(x,std::numeric_limits<int_type>::max()));
			BOOST_CHECK_EQUAL(math::pow(x,std::numeric_limits<int_type>::min() + 1) * x.pow(-1),math::pow(x,std::numeric_limits<int_type>::min()));
			// Try also with more than one variable.
			BOOST_CHECK_THROW(x * math::pow(y,std::numeric_limits<int_type>::max()) * y,std::overflow_error);
			BOOST_CHECK_THROW(math::pow(x,std::numeric_limits<int_type>::max()) *
				math::pow(y,std::numeric_limits<int_type>::min()) * y.pow(-1),std::overflow_error);
			BOOST_CHECK_EQUAL(math::pow(y,std::numeric_limits<int_type>::max()) * math::pow(x,std::numeric_limits<int_type>::max() - 1) * x,
				math::pow(y,std::numeric_limits<int_type>::max()) * math::pow(x,std::numeric_limits<int_type>::max()));
			BOOST_CHECK_EQUAL(math::pow(y,std::numeric_limits<int_type>::min()) * math::pow(x,std::numeric_limits<int_type>::min() + 1) * x.pow(-1),
				math::pow(y,std::numeric_limits<int_type>::min()) * math::pow(x,std::numeric_limits<int_type>::min()));
			// Check with empty series.
			BOOST_CHECK_EQUAL(math::pow(y,std::numeric_limits<int_type>::max()) * 0,0);
			BOOST_CHECK_EQUAL(math::pow(y,std::numeric_limits<int_type>::min()) * 0,0);
			BOOST_CHECK_EQUAL(pt(0) * pt(0),0);
		}
		template <typename Key, typename std::enable_if<detail::is_kronecker_monomial<Key>::value,int>::type = 0>
		void operator()(const Key &)
		{
			using pt = polynomial<Cf,Key>;
			using value_type = typename Key::value_type;
			using ka = kronecker_array<value_type>;
			// Use polynomials with 3 variables for testing.
			const auto &limits = std::get<0u>(ka::get_limits()[3u]);
			auto x = pt{"x"}, y = pt{"y"}, z = pt{"z"};
			BOOST_CHECK_THROW(x.pow(limits[0u]) * y.pow(limits[1u]) * z.pow(limits[2u]) * x,
				std::overflow_error);
			BOOST_CHECK_THROW(x.pow(limits[0u]) * y.pow(limits[1u]) * z.pow(limits[2u]) * y,
				std::overflow_error);
			BOOST_CHECK_THROW(x.pow(limits[0u]) * y.pow(limits[1u]) * z.pow(limits[2u]) * z,
				std::overflow_error);
			BOOST_CHECK_THROW(x.pow(-limits[0u]) * y.pow(limits[1u]) * z.pow(limits[2u]) * x.pow(-1),
				std::overflow_error);
			BOOST_CHECK_THROW(x.pow(limits[0u]) * y.pow(-limits[1u]) * z.pow(limits[2u]) * y.pow(-1),
				std::overflow_error);
			BOOST_CHECK_THROW(x.pow(limits[0u]) * y.pow(limits[1u]) * z.pow(-limits[2u]) * z.pow(-1),
				std::overflow_error);
			BOOST_CHECK_EQUAL(x.pow(limits[0u] - 1) * y.pow(limits[1u]) * z.pow(limits[2u]) * x,
				x.pow(limits[0u]) * y.pow(limits[1u]) * z.pow(limits[2u]));
			BOOST_CHECK_EQUAL(x.pow(limits[0u]) * y.pow(limits[1u] - 1) * z.pow(limits[2u]) * y,
				x.pow(limits[0u]) * y.pow(limits[1u]) * z.pow(limits[2u]));
			BOOST_CHECK_EQUAL(x.pow(limits[0u]) * y.pow(limits[1u]) * z.pow(limits[2u] - 1) * z,
				x.pow(limits[0u]) * y.pow(limits[1u]) * z.pow(limits[2u]));
			BOOST_CHECK_EQUAL(x.pow(-limits[0u] + 1) * y.pow(-limits[1u]) * z.pow(-limits[2u]) * x.pow(-1),
				x.pow(-limits[0u]) * y.pow(-limits[1u]) * z.pow(-limits[2u]));
			BOOST_CHECK_EQUAL(x.pow(-limits[0u]) * y.pow(-limits[1u] + 1) * z.pow(-limits[2u]) * y.pow(-1),
				x.pow(-limits[0u]) * y.pow(-limits[1u]) * z.pow(-limits[2u]));
			BOOST_CHECK_EQUAL(x.pow(-limits[0u]) * y.pow(-limits[1u]) * z.pow(-limits[2u] + 1) * z.pow(-1),
				x.pow(-limits[0u]) * y.pow(-limits[1u]) * z.pow(-limits[2u]));
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<k_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(polynomial_multiplier_bounds_test)
{
	environment env;
	boost::mpl::for_each<cf_types>(bounds_tester());
}

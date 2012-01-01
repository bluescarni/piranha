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

#include "fateman2.hpp"
#include "pearce2.hpp"

#define BOOST_TEST_MODULE benchmark_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <iostream>

#include "../src/integer.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/settings.hpp"

#ifdef __GNUC__
// TODO: this is illegal, remove once we have 128bit integer class.
namespace std
{
inline ostream &operator<<(ostream &os, const __uint128_t &)
{
	return os;
}
}

inline std::string cf_name(const __uint128_t &)
{
	return "128-bit integer";
}

#endif

inline std::string cf_name(const std::size_t &)
{
	return "Word-size integer";
}

inline std::string cf_name(const double &)
{
	return "Double-precision (FP)";
}

inline std::string cf_name(const piranha::integer &)
{
	return "GMP integer";
}

using namespace piranha;

typedef boost::mpl::vector<double,std::size_t,
#ifdef __GNUC__
	__uint128_t,
#endif
integer> cf_types;

struct benchmark_runner
{
	template <typename Cf>
	void operator()(const Cf &cf)
	{
		const unsigned n_trials = 10u;
		std::cout << ">>>>>>>>" << cf_name(cf) << '\n';
		settings::reset_n_threads();
		const unsigned def_n_threads = settings::get_n_threads();
		for (auto i = 1u; i <= def_n_threads; ++i) {
			settings::set_n_threads(i);
			std::cout << "Dense," << i << '\n';
			for (auto j = 0u; j < n_trials; ++j) {
				std::cout << "->";
				BOOST_CHECK_EQUAL((fateman2<Cf,kronecker_monomial<>>().size()),635376u);
			}
			std::cout << "Sparse," << i << '\n';
			for (auto j = 0u; j < n_trials; ++j) {
				std::cout << "->";
				BOOST_CHECK_EQUAL((pearce2<Cf,kronecker_monomial<>>().size()),28398035u);
			}
		}
		std::cout << "<<<<<<<<\n";
	}
};

// Composite benchmark, including dense and sparse tests in serial
// and parallel mode.

BOOST_AUTO_TEST_CASE(benchmark_test)
{
	boost::mpl::for_each<cf_types>(benchmark_runner());
}

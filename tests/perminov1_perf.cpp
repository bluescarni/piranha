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

#define BOOST_TEST_MODULE perminov1_test
#include <boost/test/unit_test.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/timer/timer.hpp>

#include "../src/divisor.hpp"
#include "../src/divisor_series.hpp"
#include "../src/environment.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_rational.hpp"
#include "../src/poisson_series.hpp"
#include "../src/polynomial.hpp"
#include "../src/series.hpp"
#include "../src/settings.hpp"

using namespace piranha;
namespace bfs = boost::filesystem;

// Perminov's polynomial multiplication test number 1. Calculate the truncated multiplication:
// f * g
// Where f = sin(2_l1).epst.bz2 and g = sin(l1-l3).epst.bz2 (from the test data dir).

// The root path for the tests directory.
static const bfs::path root_path(PIRANHA_TESTS_DIRECTORY);

BOOST_AUTO_TEST_CASE(perminov1_test)
{
	environment env;
	if (boost::unit_test::framework::master_test_suite().argc > 1) {
		settings::set_n_threads(boost::lexical_cast<unsigned>(boost::unit_test::framework::master_test_suite().argv[1u]));
	}

	using pt = polynomial<rational,monomial<rational>>;
	using epst = poisson_series<divisor_series<pt,divisor<short>>>;

	auto f = epst::load((root_path / "data" / "sin(2_l1).epst.bz2").native(),file_compression::bzip2);
	auto g = epst::load((root_path / "data" / "sin(l1-l3).epst.bz2").native(),file_compression::bzip2);

	pt::set_auto_truncate_degree(2,{"x1","x2","x3","y1","y2","y3","u1","u2","u3","v1","v2","v3"});

	epst res;
	{
	boost::timer::auto_cpu_timer t;
	res = f*g;
	}

	BOOST_CHECK_EQUAL(res.size(),2u);
	auto it = res._container().begin();
	BOOST_CHECK_EQUAL(it->m_cf._container().begin()->m_cf.size(),177152u);
	++it;
	BOOST_CHECK_EQUAL(it->m_cf._container().begin()->m_cf.size(),177152u);
}

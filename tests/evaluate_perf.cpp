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

#define BOOST_TEST_MODULE evaluate_test
#include <boost/test/unit_test.hpp>

#include <boost/timer/timer.hpp>

#include "../src/init.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "pearce1.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(evaluate_test)
{
    init();
    settings::set_thread_binding(true);
    {
        std::cout << "Timing multiplication, integer:\n";
        auto ret1 = pearce1<integer, kronecker_monomial<>>();
        {
            std::cout << "Timing evaluation, integer: ";
            boost::timer::auto_cpu_timer t;
            std::cout << math::evaluate<integer>(ret1, {{"x", 1_z}, {"y", 1_z}, {"z", 1_z}, {"t", 1_z}, {"u", 1_z}})
                      << '\n';
        }
        {
            std::cout << "Timing evaluation, double: ";
            boost::timer::auto_cpu_timer t;
            std::cout << math::evaluate<double>(ret1, {{"x", 1.}, {"y", 1.}, {"z", 1.}, {"t", 1.}, {"u", 1.}}) << '\n';
        }
    }
    {
        std::cout << "Timing multiplication, double:\n";
        auto ret1 = pearce1<double, kronecker_monomial<>>();
        {
            std::cout << "Timing evaluation, double: ";
            boost::timer::auto_cpu_timer t;
            std::cout << math::evaluate<double>(ret1, {{"x", 1.}, {"y", 1.}, {"z", 1.}, {"t", 1.}, {"u", 1.}}) << '\n';
        }
    }
    {
        std::cout << "Timing multiplication, rational:\n";
        auto ret1 = pearce1<rational, kronecker_monomial<>>();
        {
            std::cout << "Timing evaluation, double: ";
            boost::timer::auto_cpu_timer t;
            std::cout << math::evaluate<double>(ret1, {{"x", 1.}, {"y", 1.}, {"z", 1.}, {"t", 1.}, {"u", 1.}}) << '\n';
        }
        {
            std::cout << "Timing evaluation, integer: ";
            boost::timer::auto_cpu_timer t;
            std::cout << math::evaluate<integer>(ret1, {{"x", 1_z}, {"y", 1_z}, {"z", 1_z}, {"t", 1_z}, {"u", 1_z}})
                      << '\n';
        }
        {
            std::cout << "Timing evaluation, rational: ";
            boost::timer::auto_cpu_timer t;
            std::cout << math::evaluate<rational>(ret1, {{"x", 1_q}, {"y", 1_q}, {"z", 1_q}, {"t", 1_q}, {"u", 1_q}})
                      << '\n';
        }
    }
}

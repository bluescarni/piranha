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

#define BOOST_TEST_MODULE evaluate_test
#include <boost/test/unit_test.hpp>

#include <boost/timer/timer.hpp>

#include "../src/environment.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "pearce1.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(evaluate_test)
{
	environment env;
	{
	std::cout << "Timing multiplication, integer:\n";
	auto ret1 = pearce1<integer,kronecker_monomial<>>();
	{
	std::cout << "Timing evaluation, integer: ";
	boost::timer::auto_cpu_timer t;
	std::cout << math::evaluate<integer>(ret1,{{"x",1_z},{"y",1_z},{"z",1_z},{"t",1_z},{"u",1_z}}) << '\n';
	}
	{
	std::cout << "Timing evaluation, double: ";
	boost::timer::auto_cpu_timer t;
	std::cout << math::evaluate<double>(ret1,{{"x",1.},{"y",1.},{"z",1.},{"t",1.},{"u",1.}}) << '\n';
	}
	}
	{
	std::cout << "Timing multiplication, double:\n";
	auto ret1 = pearce1<double,kronecker_monomial<>>();
	{
	std::cout << "Timing evaluation, double: ";
	boost::timer::auto_cpu_timer t;
	std::cout << math::evaluate<double>(ret1,{{"x",1.},{"y",1.},{"z",1.},{"t",1.},{"u",1.}}) << '\n';
	}
	}
	{
	std::cout << "Timing multiplication, rational:\n";
	auto ret1 = pearce1<rational,kronecker_monomial<>>();
	{
	std::cout << "Timing evaluation, double: ";
	boost::timer::auto_cpu_timer t;
	std::cout << math::evaluate<double>(ret1,{{"x",1.},{"y",1.},{"z",1.},{"t",1.},{"u",1.}}) << '\n';
	}
	{
	std::cout << "Timing evaluation, integer: ";
	boost::timer::auto_cpu_timer t;
	std::cout << math::evaluate<integer>(ret1,{{"x",1_z},{"y",1_z},{"z",1_z},{"t",1_z},{"u",1_z}}) << '\n';
	}
	{
	std::cout << "Timing evaluation, rational: ";
	boost::timer::auto_cpu_timer t;
	std::cout << math::evaluate<rational>(ret1,{{"x",1_q},{"y",1_q},{"z",1_q},{"t",1_q},{"u",1_q}}) << '\n';
	}
	}
}

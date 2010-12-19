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

#include "../src/cvector.hpp"

#define BOOST_TEST_MODULE cvector_test
#include <boost/test/unit_test.hpp>

#include <exception>
#include <mutex>
#include <vector>

#include "../src/runtime_info.hpp"
#include "../src/settings.hpp"

struct custom_exception: public std::exception
{
	virtual const char *what() const throw()
	{
		return "Custom exception thrown.";
	}
};

struct trivial
{
	int n;
	double x;
};

struct nontrivial
{
	nontrivial():v(std::vector<double>::size_type(1)) {}
	std::vector<double> v;
};

std::mutex mutex;
unsigned copies = 0;

struct nontrivial_throwing
{
	nontrivial_throwing():v(1) {}
	nontrivial_throwing(const nontrivial_throwing &ntt):v(ntt.v)
	{
		std::lock_guard<std::mutex> lock(mutex);
		++copies;
		if (copies > 10000) {
			throw custom_exception();
		}
	}
	std::vector<double> v;
};

BOOST_AUTO_TEST_CASE(cvector_size_constructor)
{
	piranha::cvector<trivial> t(10000000);
	piranha::cvector<nontrivial> nt(1000000);
	BOOST_CHECK_THROW(piranha::cvector<nontrivial_throwing> ntt(1000000),custom_exception);
}

BOOST_AUTO_TEST_CASE(cvector_copy_constructor)
{
	piranha::cvector<trivial> t(10000000);
	piranha::cvector<trivial> t_copy(t);
	piranha::cvector<nontrivial> nt(1000000);
	piranha::cvector<nontrivial> nt_copy(nt);
}

static inline piranha::cvector<nontrivial> get_cvector()
{
	return piranha::cvector<nontrivial>(1000000);
}

BOOST_AUTO_TEST_CASE(cvector_move_assignment)
{
	piranha::cvector<nontrivial> t;
	t = get_cvector();
	BOOST_CHECK_EQUAL(t.size(),piranha::cvector<nontrivial>::size_type(1000000));
}

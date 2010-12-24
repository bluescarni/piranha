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
	nontrivial(nontrivial &&nt):v(std::move(nt.v)) {}
	nontrivial &operator=(nontrivial &&nt)
	{
		v = std::move(nt.v);
		return *this;
	}
	std::vector<double> v;
};

std::mutex mutex;
unsigned copies = 0;

struct nontrivial_throwing
{
	nontrivial_throwing():v(1)
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (copies > 10000) {
			throw custom_exception();
		}
		++copies;
	}
	nontrivial_throwing(const nontrivial_throwing &ntt):v()
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (copies > 10000) {
			throw custom_exception();
		}
		++copies;
		v = ntt.v;
	}
	nontrivial_throwing(nontrivial_throwing &&ntt):v(std::move(ntt.v)) {}
	nontrivial_throwing &operator=(nontrivial_throwing &&ntt)
	{
		v = std::move(ntt.v);
		return *this;
	}
	std::vector<double> v;
};

const unsigned size = 1000000;

static inline void set_mt()
{
	piranha::settings::set_n_threads((piranha::runtime_info::hardware_concurrency() == 0) ? 1 : piranha::runtime_info::hardware_concurrency());
}

static inline void set_st()
{
	piranha::settings::set_n_threads(1);
}

BOOST_AUTO_TEST_CASE(cvector_size_constructor)
{
	set_mt();
	piranha::cvector<trivial> t(size);
	piranha::cvector<nontrivial> nt(size);
	BOOST_CHECK_THROW(piranha::cvector<nontrivial_throwing> ntt(size),custom_exception);
	set_st();
}

BOOST_AUTO_TEST_CASE(cvector_move_constructor)
{
	set_mt();
	piranha::cvector<trivial> t(size);
	piranha::cvector<trivial> t_move(std::move(t));
	piranha::cvector<nontrivial> nt(size);
	piranha::cvector<nontrivial> nt_move(std::move(nt));
	set_st();
}

BOOST_AUTO_TEST_CASE(cvector_copy_constructor)
{
	set_mt();
	piranha::cvector<trivial> t(size);
	piranha::cvector<trivial> t_copy(t);
	piranha::cvector<nontrivial> nt(size);
	piranha::cvector<nontrivial> nt_copy(nt);
	set_st();
}

static inline piranha::cvector<nontrivial> get_nontrivial()
{
	return piranha::cvector<nontrivial>(size);
}

static inline piranha::cvector<trivial> get_trivial()
{
	return piranha::cvector<trivial>(size);
}

BOOST_AUTO_TEST_CASE(cvector_move_assignment)
{
	set_mt();
	piranha::cvector<nontrivial> nt;
	nt = get_nontrivial();
	BOOST_CHECK_EQUAL(nt.size(),piranha::cvector<nontrivial>::size_type(size));
	piranha::cvector<trivial> t;
	t = get_trivial();
	BOOST_CHECK_EQUAL(t.size(),piranha::cvector<trivial>::size_type(size));
	set_st();
}

BOOST_AUTO_TEST_CASE(cvector_assignment)
{
	set_mt();
	piranha::cvector<trivial> t, u(get_trivial());
	t = u;
	BOOST_CHECK_EQUAL(t.size(),piranha::cvector<trivial>::size_type(size));
	piranha::cvector<nontrivial> nt, nu(get_nontrivial());
	nt = nu;
	BOOST_CHECK_EQUAL(nt.size(),piranha::cvector<nontrivial>::size_type(size));
	set_st();
}

BOOST_AUTO_TEST_CASE(cvector_resize_01)
{
	set_mt();
	piranha::cvector<trivial> t(size);
	t.resize(size + 100);
	BOOST_CHECK_EQUAL(t.size(),piranha::cvector<trivial>::size_type(size + 100));
	piranha::cvector<nontrivial> nt(size);
	nt.resize(size + 100);
	BOOST_CHECK_EQUAL(nt.size(),piranha::cvector<nontrivial>::size_type(size + 100));
	copies = 0;
	piranha::cvector<nontrivial_throwing> ntt(9000);
	BOOST_CHECK_THROW(ntt.resize(10100),custom_exception);
	BOOST_CHECK_EQUAL(ntt.size(),piranha::cvector<nontrivial>::size_type(9000));
	set_st();
}

BOOST_AUTO_TEST_CASE(cvector_resize_02)
{
	set_mt();
	piranha::cvector<trivial> t(size);
	t.resize(size - 100);
	BOOST_CHECK_EQUAL(t.size(),piranha::cvector<trivial>::size_type(size - 100));
	piranha::cvector<nontrivial> nt(size);
	nt.resize(size - 100);
	BOOST_CHECK_EQUAL(nt.size(),piranha::cvector<nontrivial>::size_type(size - 100));
	copies = 0;
	piranha::cvector<nontrivial_throwing> ntt(9000);
	BOOST_CHECK_NO_THROW(ntt.resize(8900));
	BOOST_CHECK_EQUAL(ntt.size(),piranha::cvector<nontrivial>::size_type(8900));
	set_st();
}

BOOST_AUTO_TEST_CASE(cvector_accessors)
{
	set_mt();
	piranha::cvector<int> t(size);
	t[100] = -10;
	BOOST_CHECK_EQUAL(t[0],0);
	BOOST_CHECK_EQUAL(t[100],-10);
	set_st();
}

BOOST_AUTO_TEST_CASE(cvector_mt_destructor)
{
	set_mt();
	piranha::cvector<trivial> t(size);
	piranha::cvector<nontrivial> nt(size);
}

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

#ifndef PIRANHA_MONAGAN_HPP
#define PIRANHA_MONAGAN_HPP

#include <boost/timer/timer.hpp>

#include "../src/polynomial.hpp"

namespace piranha
{

// Various polynomial multiplication tests from:
// https://peerj.com/preprints/504/

template <typename Cf,typename Key>
inline polynomial<Cf,Key> monagan1()
{
	using p_type = polynomial<Cf,Key>;
	p_type x("x"), y("y"), z("z");
	auto f1 = (x + y + z + 1).pow(20) + 1;
	auto g = f1 + 1;
	{
	boost::timer::auto_cpu_timer t;
	return f1 * g;
	}
}

template <typename Cf,typename Key>
inline polynomial<Cf,Key> monagan2()
{
	using p_type = polynomial<Cf,Key>;
	p_type x("x"), y("y"), z("z");
	auto f2 = (x*x + y*y + z*z + 1).pow(20) + 1;
	auto g = f2 + 1;
	{
	boost::timer::auto_cpu_timer t;
	return f2 * g;
	}
}

template <typename Cf,typename Key>
inline polynomial<Cf,Key> monagan3()
{
	using p_type = polynomial<Cf,Key>;
	p_type x("x"), y("y"), z("z");
	auto f3 = (x + y + z + 1).pow(30) + 1;
	auto g = f3 + 1;
	{
	boost::timer::auto_cpu_timer t;
	return f3 * g;
	}
}

template <typename Cf,typename Key>
inline polynomial<Cf,Key> monagan4()
{
	using p_type = polynomial<Cf,Key>;
	p_type x("x"), y("y"), z("z"), t("t");
	auto f4 = (x + y + z + t + 1).pow(20) + 1;
	auto g = f4 + 1;
	{
	boost::timer::auto_cpu_timer t;
	return f4 * g;
	}
}

template <typename Cf,typename Key>
inline polynomial<Cf,Key> monagan5()
{
	using p_type = polynomial<Cf,Key>;
	p_type x("x"), y("y"), u("u"), v("v"), w("w");
	auto f5 = (1 + u*u + v + w*w + x - y).pow(10) + 1;
	auto g5 = (1 + u + v*v + w + x*x + y).pow(10) + 1;
	{
	boost::timer::auto_cpu_timer t;
	return f5 * g5;
	}
}

}

#endif

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

#ifndef PIRANHA_FATEMAN1_HPP
#define PIRANHA_FATEMAN1_HPP

#include <boost/timer/timer.hpp>

#include "../src/polynomial.hpp"

namespace piranha
{

template <typename Cf,typename Key>
inline polynomial<Cf,Key> fateman1()
{
	typedef polynomial<Cf,Key> p_type;
	p_type x("x"), y("y"), z("z"), t("t"), u("u"), v("v"), n("n"), m("m"), o("o"), p("p"), q("q"), r("r");
	auto f = x + y + z + t + u + v + n + m + o + p + q + r + 1;
	auto tmp(f);
	for (auto i = 1; i < 5; ++i) {
		f *= tmp;
	}
	{
	boost::timer::auto_cpu_timer t;
	return f * (f + 1);
	}
}

}

#endif

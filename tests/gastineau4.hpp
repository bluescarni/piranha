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

#ifndef PIRANHA_GASTINEAU4_HPP
#define PIRANHA_GASTINEAU4_HPP

#include <boost/timer/timer.hpp>

#include "../src/polynomial.hpp"

namespace piranha
{

template <typename Cf,typename Key>
inline polynomial<Cf,Key> gastineau4()
{
	typedef polynomial<Cf,Key> p_type;
	p_type z("z"), t("t"), u("u"), x("x"), y("y");

	auto f = (1 + x + y + 2*z*z + 3*t*t*t + 5*u*u*u*u*u);
	auto g = (1 + u + t + 2*z*z + 3*y*y*y + 5*x*x*x*x*x);
	auto tmp_f(f), tmp_g(g);
	for (int i = 1; i < 20; ++i) {
		f *= tmp_f;
		g *= tmp_g;
	}
	{
	boost::timer::auto_cpu_timer t;
	return f * g;
	}
}

}

#endif

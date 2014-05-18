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

#ifndef PIRANHA_GASTINEAU3_HPP
#define PIRANHA_GASTINEAU3_HPP

#include <boost/timer/timer.hpp>

#include "../src/polynomial.hpp"

namespace piranha
{

template <typename Cf,typename Key>
inline polynomial<Cf,Key> gastineau3()
{
	typedef polynomial<Cf,Key> p_type;
	p_type u("u"), v("v"), w("w"), x("x"), y("y");

	auto f = (1 + u*u + v + w*w + x - y*y);
	auto g = (1 + u + v*v + w + x*x + y*y*y);
	auto tmp_f(f), tmp_g(g);
	for (int i = 1; i < 28; ++i) {
		f *= tmp_f;
		g *= tmp_g;
	}
	g += 1;
	{
	boost::timer::auto_cpu_timer t;
	return f * g;
	}
}

}

#endif

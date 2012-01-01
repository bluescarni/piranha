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

#ifndef PIRANHA_PEARCE2_HPP
#define PIRANHA_PEARCE2_HPP

#include "../src/polynomial.hpp"
#include "../src/timeit.hpp"

namespace piranha
{

template <typename Cf,typename Key>
inline polynomial<Cf,Key> pearce2()
{
	typedef polynomial<Cf,Key> p_type;
	p_type x("x"), y("y"), z("z"), t("t"), u("u");

	auto f = (x + y + z*z*2 + t*t*t*3 + u*u*u*u*u*5 + 1);
	auto tmp_f(f);
	auto g = (u + t + z*z*2 + y*y*y*3 + x*x*x*x*x*5 + 1);
	auto tmp_g(g);
	for (int i = 1; i < 16; ++i) {
		f *= tmp_f;
		g *= tmp_g;
	}
	return timeit([&f,&g](){return f * g;});
}

}

#endif

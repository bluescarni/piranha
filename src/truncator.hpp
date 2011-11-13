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

#ifndef PIRANHA_TRUNCATOR_HPP
#define PIRANHA_TRUNCATOR_HPP

#include <boost/concept/assert.hpp>

#include "concepts/series.hpp"

namespace piranha
{

template <typename Series, typename Enable = void>
class truncator
{
		BOOST_CONCEPT_ASSERT((concept::Series<Series>));
	public:
		typedef typename Series::term_type term_type;
		explicit truncator(const Series &s):m_s(s) {}
		truncator(const truncator &) = delete;
		truncator(truncator &&) = delete;
		truncator &operator=(const truncator &) = delete;
		truncator &operator=(truncator &&) = delete;
		bool is_active() const
		{
			return false;
		}
		bool truncate(const term_type &) const
		{
			return false;
		}
	private:
		const Series &m_s;
};

template <typename Truncator>
class truncator_traits
{
	
};

}

#endif

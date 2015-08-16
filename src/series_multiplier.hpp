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

#ifndef PIRANHA_SERIES_MULTIPLIER_HPP
#define PIRANHA_SERIES_MULTIPLIER_HPP

namespace piranha
{

/// Series multiplier.
/**
 * This class is used by the multiplication operators involving two series operands. The class works as follows:
 * 
 * - an instance of series multiplier is created using two series as construction arguments;
 * - when operator()() is called, an instance of \p Series is returned, representing
 *   the result of the multiplication of the two series used for construction.
 * 
 * Any specialisation of this class must respect the protocol described above (i.e., construction from series
 * instances and operator()() to compute the result). Note that this class is guaranteed to be used after the symbolic arguments of the series used for construction
 * have been merged (in other words, the two series have identical symbolic arguments sets).
 *
 * The default implementation of this class does not define any call operator: any use of this class will thus result in
 * a compile-time error.
 */
template <typename Series, typename Enable = void>
class series_multiplier
{};

}

#endif

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

/** \file forwarding.hpp
 * \brief Forwarding macros.
 * 
 * This header contains macros to forward constructors and assignment operators to a base class.
 */

#ifndef PIRANHA_FORWARDING_HPP
#define PIRANHA_FORWARDING_HPP

#include <type_traits>
#include <utility>

/// Constructor-forwarding macro.
/**
 * This macro will declare and define an explicit constructor for class \p Derived that accepts
 * a variadic set of arguments whose size is greater than zero.
 * The constructor will perfectly forward the arguments to the base class
 * \p Base, and it is enabled only if the base class is constructible from the set of arguments.
 * In order to avoid clashes with copy/move constructors, in case the only argument derives from \p Derived
 * the constructor will be disabled.
 */
#define PIRANHA_FORWARDING_CTOR(Derived,Base) \
template <typename T_, typename ... Args_,typename = typename std::enable_if< \
	std::is_constructible<Base,T_ &&,Args_ && ...>::value && \
	(sizeof...(Args_) || !std::is_base_of<Derived,typename std::decay<T_>::type>::value)>::type> \
	explicit Derived(T_ &&arg0, Args_ && ... args):Base(std::forward<T_>(arg0),std::forward<Args_>(args)...) {}

/// Assignment-forwarding macro.
/**
 * This macro will declare and define a generic assignment operator for class \p Derived.
 * The operator will perfectly forward the argument to the base class
 * \p Base, and it is enabled only if the base class is assignable from the generic argument.
 * In order to avoid clashes with copy/move assignment operators, in case the argument derives from \p Derived
 * the operator will be disabled.
 */
#define PIRANHA_FORWARDING_ASSIGNMENT(Derived,Base) \
template <typename T_> \
typename std::enable_if<std::is_assignable<Base &,T_ &&>::value && \
	!std::is_base_of<Derived,typename std::decay<T_>::type>::value, \
	Derived &>::type operator=(T_ &&arg) \
{ \
	Base::operator=(std::forward<T_>(arg)); \
	return *this; \
}

#endif


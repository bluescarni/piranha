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
 
#ifndef PIRANHA_INHERIT_HPP
#define PIRANHA_INHERIT_HPP

#include <type_traits>

#include "../type_traits.hpp"

// Emulation of constructor inheritance. Will define an explicit constructor that accepts
// at least 1 argument (whose type must not be a base of Derived in order to avoid clashes with
// copy/move ctors, which need to be implicit) and whose arguments can be used to construct the base class.
// NOTE: ideally, when inheriting ctor becomes available, we will want to remove this macro and also
// all the defaulted default/copy/move ctors in the derived classes.
#define PIRANHA_USING_CTOR(Derived,Base) \
template <typename T_, typename ... Args_,typename = typename std::enable_if< \
	std::is_constructible<Base,T_,Args_...>::value && \
	(sizeof...(Args_) || !std::is_base_of<typename std::decay<T_>::type,Derived>::value)>::type> \
explicit Derived(T_ &&arg0, Args_ && ... args):Base(std::forward<T_>(arg0),std::forward<Args_>(args)...) {}

// Assignment inheritance, activated if the base class is assignable and the argument is not
// a base of Derived.
#define PIRANHA_USING_ASSIGNMENT(Derived,Base) \
template <typename T_> \
typename std::enable_if<piranha::is_assignable<Base,T_>::value && \
	!std::is_base_of<typename std::decay<T_>::type,Derived>::value, \
Derived &>::type operator=(T_ &&arg) \
{ \
	Base::operator=(std::forward<T_>(arg)); \
	return *this; \
}

#endif

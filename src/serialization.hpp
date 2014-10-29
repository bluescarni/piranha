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

#ifndef PIRANHA_SERIALIZATION_HPP
#define PIRANHA_SERIALIZATION_HPP

// Common headers for serialization.
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>
// Serialization todo:
// - provide alternative overloads of the serialization methods
//   when working with binary archives. This should lead to improved performance
//   at the expense of portability;
// - think about potentially "malicious" archives being loaded. We have some classes that
//   rely on their members to satisfy certain conditions. Examples:
//   - the rational numerator and denominator being coprimes (done),
//   - elements in a symbol_set being ordered (done)
//   - series symbol set inconsistent with contained terms (done),
//   - possibly more...
//   Right now the general policy is to just restore class members whatever their value is.
//   We need to decide ultimately if we want to have a layer of safety in the serialization methods
//   or not. It comes at a price in terms of complexity and performance. A possible middle way would
//   be to enable the safety checks in the text archives but disable them in the binary ones,
//   for performance. The safety layer could be checked by crafting bad text archives and storing
//   them as strings from the tests.
// - Should probably test the exception safety of the serialization routines. Some examples:
//   - serialization of small vector leaves the object in a consistent state if the deserialization
//     of an element fails;
//   - deserialization of a term leaves cf/key in the original state in case of errors;
//   - more...
//   At the moment the routines are coded to provide at least the basic exception safety, but
//   we should test this explicitly eventually.

// Macro for trivial serialization through base class.
#define PIRANHA_SERIALIZE_THROUGH_BASE(base) \
friend class boost::serialization::access; \
template <typename Archive> \
void serialize(Archive &ar, unsigned int) \
{ \
	ar & boost::serialization::base_object<base>(*this); \
}

#endif

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

#ifndef PIRANHA_CONCEPT_COEFFICIENT_HPP
#define PIRANHA_CONCEPT_COEFFICIENT_HPP

#include <boost/concept_check.hpp>
#include <iostream>
#include <type_traits>

#include "../config.hpp"
#include "../detail/base_term_fwd.hpp"
#include "container_element.hpp"

namespace piranha
{

namespace concept
{

/// Concept for series coefficients.
/**
 * The requisites for type \p T are the following:
 * 
 * - must be a model of piranha::concept::ContainerElement,
 * - must not be a pointer,
 * - must be directable to output stream,
 * - must be provided with a const non-throwing \p is_compatible method accepting a generic piranha::echelon_descriptor
 *   as input and returning a boolean value,
 * - must be provided with a const non-throwing \p is_ignorable method accepting a generic piranha::echelon_descriptor
 *   as input and returning a boolean value,
 * - must be provided with a const \p is_equal_to method accepting an instance of \p T and a generic piranha::echelon_descriptor
 *   as input and returning a boolean value,
 * - must be provided with a \p negate method accepting a generic piranha::echelon_descriptor
 *   as input and returning void,
 * - must be provided with an \p add method accepting an instance of \p T and a generic piranha::echelon_descriptor
 *   as input and returning void,
 * - must be provided with a \p subtract method accepting an instance of \p T and a generic piranha::echelon_descriptor
 *   as input and returning void,
 * - must be provided with a \p multiply_by method accepting a generic object and a generic piranha::echelon_descriptor
 *   as input and returning void,
 * - must be provided with a const \p merge_args method accepting two generic piranha::echelon_descriptor of the same type
 *   as input and returning an instance of \p T.
 */
template <typename T>
struct Coefficient:
	ContainerElement<T>
{
	/// Concept usage pattern.
	BOOST_CONCEPT_USAGE(Coefficient)
	{
// 		typedef echelon_descriptor<base_term<int,int,void>> ed_type;
// 		static_assert(!std::is_pointer<T>::value,"Coefficient type cannot be a pointer.");
// 		std::cout << *(static_cast<T *>(piranha_nullptr));
// 		const T inst = T();
// 		static_assert(piranha_noexcept_op(inst.template is_compatible(std::declval<ed_type>())),"is_compatible() must be non-throwing.");
// 		auto tmp1 = inst.template is_compatible(*static_cast<ed_type const *>(piranha_nullptr));
// 		static_assert(std::is_same<decltype(tmp1),bool>::value,"Invalid is_compatible() method signature for coefficient type.");
// 		static_assert(piranha_noexcept_op(inst.template is_ignorable(std::declval<ed_type>())),"is_ignorable() must be non-throwing.");
// 		auto tmp2 = inst.template is_ignorable(*static_cast<ed_type const *>(piranha_nullptr));
// 		static_assert(std::is_same<decltype(tmp2),bool>::value,"Invalid is_ignorable() method signature for coefficient type.");
// 		static_assert(std::is_same<decltype(inst.is_equal_to(inst,std::declval<ed_type>())),bool>::value,"Invalid is_equal_to() method signature for coefficient type.");
// 		T inst_m;
// 		inst_m.negate(*static_cast<ed_type const *>(piranha_nullptr));
// 		static_assert(std::is_same<decltype(inst_m.negate(*static_cast<ed_type const *>(piranha_nullptr))),void>::value,"Invalid negate() method signature for coefficient type.");
// 		inst_m.add(inst,*static_cast<ed_type const *>(piranha_nullptr));
// 		static_assert(std::is_same<decltype(inst_m.add(inst,*static_cast<ed_type const *>(piranha_nullptr))),void>::value,"Invalid add() method signature for coefficient type.");
// 		inst_m.subtract(inst,*static_cast<ed_type const *>(piranha_nullptr));
// 		static_assert(std::is_same<decltype(inst_m.subtract(inst,*static_cast<ed_type const *>(piranha_nullptr))),void>::value,"Invalid add() method signature for coefficient type.");
// 		auto merge_out = inst.merge_args(*static_cast<ed_type const *>(piranha_nullptr),*static_cast<ed_type const *>(piranha_nullptr));
// 		static_assert(std::is_same<decltype(merge_out),T>::value,"Invalid merge_args() method signature for coefficient type.");
	}
};

}
}

#endif

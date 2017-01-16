/* Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)

This file is part of the Piranha library.

The Piranha library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The Piranha library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the Piranha library.  If not,
see https://www.gnu.org/licenses/. */

#ifndef PIRANHA_DETAIL_VECTOR_HASHER_HPP
#define PIRANHA_DETAIL_VECTOR_HASHER_HPP

#include <boost/functional/hash.hpp>
#include <cstddef>
#include <functional>

namespace piranha
{
namespace detail
{

template <typename Vector>
inline std::size_t vector_hasher(const Vector &v)
{
    using value_type = typename Vector::value_type;
    const auto size = v.size();
    switch (size) {
        case 0u:
            return 0u;
        case 1u: {
            std::hash<value_type> hasher;
            return hasher(v[0u]);
        }
    }
    std::hash<value_type> hasher;
    std::size_t retval = hasher(v[0u]);
    for (decltype(v.size()) i = 1u; i < size; ++i) {
        boost::hash_combine(retval, hasher(v[i]));
    }
    return retval;
}
}
}

#endif

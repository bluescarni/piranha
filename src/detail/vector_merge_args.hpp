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

#ifndef PIRANHA_DETAIL_VECTOR_MERGE_ARGS_HPP
#define PIRANHA_DETAIL_VECTOR_MERGE_ARGS_HPP

#include <algorithm>
#include <stdexcept>

#include "../config.hpp"
#include "../exceptions.hpp"
#include "../symbol_set.hpp"

namespace piranha {

namespace detail {

// Merge arguments for a vector-like key. Vector must support a minimal
// std::vector interface,
// and the value type must be constructible from zero.
template <typename Vector>
inline Vector vector_merge_args(const Vector &v, const symbol_set &orig_args,
                                const symbol_set &new_args) {
  using size_type = typename Vector::size_type;
  using value_type = typename Vector::value_type;
  // NOTE: here and elsewhere (i.e., kronecker keys) the check on
  // new_args.size() <= orig_args.size()
  // is not redundant with the std::includes check; indeed it actually checks
  // that the new args are
  // _more_ than the old args (whereas with just the std::includes check
  // identical orig_args and new_args
  // would be allowed).
  if (unlikely(v.size() != orig_args.size() ||
               new_args.size() <= orig_args.size() ||
               !std::includes(new_args.begin(), new_args.end(),
                              orig_args.begin(), orig_args.end()))) {
    piranha_throw(std::invalid_argument,
                  "invalid argument(s) for symbol set merging");
  }
  Vector retval;
  piranha_assert(std::is_sorted(orig_args.begin(), orig_args.end()));
  piranha_assert(std::is_sorted(new_args.begin(), new_args.end()));
  auto it_new = new_args.begin();
  for (size_type i = 0u; i < v.size(); ++i, ++it_new) {
    // NOTE: here the static_cast is fine as i is < v.size() and v.size() ==
    // orig_args.size().
    while (*it_new != orig_args[static_cast<symbol_set::size_type>(i)]) {
      retval.push_back(value_type(0));
      piranha_assert(it_new != new_args.end());
      ++it_new;
      piranha_assert(it_new != new_args.end());
    }
    retval.push_back(v[i]);
  }
  // Fill up arguments at the tail of new_args but not in orig_args.
  for (; it_new != new_args.end(); ++it_new) {
    retval.push_back(value_type(0));
  }
  piranha_assert(retval.size() == new_args.size());
  return retval;
}
}
}

#endif

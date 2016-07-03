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

#ifndef PIRANHA_DETAIL_PARALLEL_VECTOR_TRANSFORM_HPP
#define PIRANHA_DETAIL_PARALLEL_VECTOR_TRANSFORM_HPP

#include <algorithm>
#include <stdexcept>
#include <vector>

#include "../config.hpp"
#include "../exceptions.hpp"
#include "../thread_pool.hpp"

namespace piranha {
namespace detail {

// A parallel version of std::transform that operates on vectors.
template <typename T, typename U, typename Op>
inline void parallel_vector_transform(unsigned n_threads,
                                      const std::vector<T> &ic,
                                      std::vector<U> &oc, Op op) {
  if (unlikely(n_threads == 0u)) {
    piranha_throw(std::invalid_argument, "invalid number of threads");
  }
  if (unlikely(ic.size() != oc.size())) {
    piranha_throw(std::invalid_argument, "mismatched vector sizes");
  }
  if (n_threads == 1u) {
    std::transform(ic.begin(), ic.end(), oc.begin(), op);
    return;
  }
  const auto block_size = ic.size() / n_threads;
  auto local_transform = [&op](T const *b, T const *e, U *o) {
    std::transform(b, e, o, op);
  };
  future_list<decltype(local_transform(ic.data(), ic.data(), oc.data()))>
      ff_list;
  try {
    for (unsigned i = 0u; i < n_threads; ++i) {
      auto b = ic.data() + i * block_size;
      auto e = (i == n_threads - 1u) ? (ic.data() + ic.size())
                                     : (ic.data() + (i + 1u) * block_size);
      auto o = oc.data() + i * block_size;
      ff_list.push_back(thread_pool::enqueue(i, local_transform, b, e, o));
    }
    // First let's wait for everything to finish.
    ff_list.wait_all();
    // Then, let's handle the exceptions.
    ff_list.get_all();
  } catch (...) {
    ff_list.wait_all();
    throw;
  }
}
}
}

#endif

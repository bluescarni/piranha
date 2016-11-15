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

#ifndef PIRANHA_DETAIL_INIT_DATA_HPP
#define PIRANHA_DETAIL_INIT_DATA_HPP

#include <atomic>

namespace piranha
{

inline namespace impl
{

// Global variables for init/shutdown.
template <typename = void>
struct piranha_init_statics {
    static std::atomic_flag s_init_flag;
    static std::atomic<bool> s_shutdown_flag;
    static std::atomic<unsigned> s_failed;
};

// Static init of the global flags.
template <typename T>
std::atomic_flag piranha_init_statics<T>::s_init_flag = ATOMIC_FLAG_INIT;

template <typename T>
std::atomic<bool> piranha_init_statics<T>::s_shutdown_flag(false);

template <typename T>
std::atomic<unsigned> piranha_init_statics<T>::s_failed(0u);

// Query if we are at shutdown.
inline bool shutdown()
{
    return piranha_init_statics<>::s_shutdown_flag.load();
}
}
}

#endif

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

#ifndef PIRANHA_INIT_HPP
#define PIRANHA_INIT_HPP

#include <atomic>
#include <cstdlib>
#include <iostream>

#include "detail/mpfr.hpp"

namespace piranha
{

namespace detail
{

// Global variables for init/shutdown.
template <typename = void>
struct piranha_init_statics
{
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

// The cleanup function.
inline void cleanup_function()
{
    std::cout << "Freeing MPFR caches.\n";
    ::mpfr_free_cache();
    std::cout << "Setting shutdown flag.\n";
    piranha_init_statics<>::s_shutdown_flag.store(true);
}

// Query if we are at shutdown.
inline bool shutdown()
{
    return piranha_init_statics<>::s_shutdown_flag.load();
}

}

/// Main initialisation function.
/**
 * This function should be called before accessing any Piranha functionality.
 * It will register cleanup functions that will be run on program exit (e.g.,
 * the MPFR <tt>mpfr_free_cache()</tt> function).
 *
 * It is allowed to call this function concurrently from multiple threads: after the first
 * invocation, additional invocations will not perform any action.
 */
inline void init()
{
    if (detail::piranha_init_statics<>::s_init_flag.test_and_set()) {
        // If the previous value of the init flag was true, it means we already called
        // init() previously. Increase the failure count and exit.
        ++detail::piranha_init_statics<>::s_failed;
        return;
    }
    if (std::atexit(detail::cleanup_function)) {
        std::cerr << "Unable to register cleanup function with std::atexit().\n";
        std::cerr.flush();
        std::abort();
    }
    if (!::mpfr_buildopt_tls_p()) {
        // NOTE: logging candidate, we probably want to push
        // this to a warning channel.
        std::cerr << "The MPFR library was not built thread-safe.\n";
        std::cerr.flush();
    }
}

}

#endif

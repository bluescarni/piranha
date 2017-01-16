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

#include <cstdlib>
#include <iostream>

#include <piranha/detail/init_data.hpp>
#include <piranha/detail/mpfr.hpp>

namespace piranha
{

inline namespace impl
{

// The cleanup function.
inline void cleanup_function()
{
    std::cout << "Freeing MPFR caches.\n";
    ::mpfr_free_cache();
    std::cout << "Setting shutdown flag.\n";
    piranha_init_statics<>::s_shutdown_flag.store(true);
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
// A few notes about the init function. The basic idea is that we want certain things to happen
// when the program is shutting down but (crucially) *before* destruction of static objects starts.
// The language guarantees that anything registered with std::atexit() runs before the destruction of
// the statics, provided that the registration happens *after* the initialization of static objects.
// The complication here is that, even if we run this function as first thing in main(), we are still
// not 100% sure all statics have been initialised, because of potentially deferred dynamic init:
//
// http://en.cppreference.com/w/cpp/language/initialization#Deferred_dynamic_initialization
//
// The saving grace here seems to be in this quote:
//
// "If the initialization of a variable is deferred to happen after the first statement of main/thread function, it
// happens before the first odr-use of any variable with static/thread storage duration defined in the same translation
// unit as the variable to be initialized."
//
// In the init() function we are using indeed a static variable, the s_init_flag, and its use should guarantee the
// initialisation of all other statics. That's my understanding anyway. I am not 100% sure how this all maps to the
// init of pyranha, let's just keep the eyes open regarding this potential issue.
inline void init()
{
    if (piranha_init_statics<>::s_init_flag.test_and_set()) {
        // If the previous value of the init flag was true, it means we already called
        // init() previously. Increase the failure count and exit.
        ++piranha_init_statics<>::s_failed;
        return;
    }
    std::cout << "Initializing piranha.\n";
    if (std::atexit(cleanup_function)) {
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

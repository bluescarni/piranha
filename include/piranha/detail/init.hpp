/* Copyright 2009-2017 Francesco Biscani (bluescarni@gmail.com)

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

#ifndef PIRANHA_DETAIL_INIT_HPP
#define PIRANHA_DETAIL_INIT_HPP

#include <ios>
#include <iostream>

#include <mp++/config.hpp>

#if defined(MPPP_WITH_MPFR)

#include <mp++/detail/mpfr.hpp>

#endif

#include <piranha/config.hpp>

#if defined(PIRANHA_WITH_BOOST_STACKTRACE) && !defined(_WIN32)

#include <backtrace-supported.h>

#endif

namespace piranha
{

inline namespace impl
{

// LCOV_EXCL_START
struct init_checks {
    init_checks()
    {
#if defined(MPPP_WITH_MPFR)
        if (!::mpfr_buildopt_tls_p()) {
            // Make sure we can access the default C++ streams:
            // http://en.cppreference.com/w/cpp/io/ios_base/Init
            std::ios_base::Init ios_init;
            std::cerr << "=========================\nWARNING: MPFR was not compiled as thread safe, and piranha's "
                         "parallel algorithms may thus fail at runtime in unpredictable ways.\nPlease re-compile "
                         "MPFR with the '--enable-thread-safe' configure option (see the MPFR "
                         "installation instructions at "
                         "http://www.mpfr.org/mpfr-current/mpfr.html#Installing-MPFR)\n=========================\n";
        }
#endif
#if defined(PIRANHA_WITH_BOOST_STACKTRACE) && !defined(_WIN32)
        if (!BACKTRACE_SUPPORTED) {
            std::ios_base::Init ios_init;
            std::cerr << "=========================\nWARNING: the BACKTRACE_SUPPORTED define is set to 0, please "
                         "double check your libbacktrace installation.\n=========================\n";
        }
        if (!BACKTRACE_SUPPORTS_THREADS) {
            std::ios_base::Init ios_init;
            std::cerr << "=========================\nWARNING: it looks like libbacktrace was not compiled as thread "
                         "safe, and the generation of stacktraces from concurrent threads may thus fail at runtime in "
                         "unpredictable ways. Please double check your libbacktrace "
                         "installation.\n=========================\n";
        }
#endif
    }
};
// LCOV_EXCL_STOP

#if PIRANHA_CPLUSPLUS < 201703L

// Inline variable emulation machinery for pre-C++17.
template <typename = void>
struct init_checks_holder {
    static const init_checks s_init_checks;
};

template <typename T>
const init_checks init_checks_holder<T>::s_init_checks;

inline void inst_init_checks()
{
    auto ptr = &init_checks_holder<>::s_init_checks;
    (void)ptr;
}

#else

inline const init_checks init_checks_register;

#endif
}
}

#endif

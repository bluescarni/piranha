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

// NOTE: so far the only init we need is to check the MPFR build, so let's implement
// the following only if mp++ was built with MPFR support.
#if defined(MPPP_WITH_MPFR)

#include <mp++/detail/mpfr.hpp>

#include <piranha/config.hpp>

namespace piranha
{

inline namespace impl
{

struct mpfr_init_checks {
    mpfr_init_checks()
    {
        if (!::mpfr_buildopt_tls_p()) {
            // Make sure we can access the default C++ streams:
            // http://en.cppreference.com/w/cpp/io/ios_base/Init
            std::ios_base::Init ios_init;
            std::cerr << "=========================\nWARNING: MPFR was not compiled as thread safe, and piranha's "
                         "parallel algorithms might thus fail at runtime in unpredictable ways.\n";
            std::cerr << "Please re-compile MPFR with the '--enable-thread-safe' configure option (see the MPFR "
                         "installation instructions at "
                         "http://www.mpfr.org/mpfr-current/mpfr.html#Installing-MPFR)\n=========================\n";
        }
    }
};

#if PIRANHA_CPLUSPLUS < 201703L

// Inline variable emulation machinery for pre-C++17.
template <typename = void>
struct mpfr_init_checks_holder {
    static const mpfr_init_checks s_init_checks;
};

template <typename T>
const mpfr_init_checks mpfr_init_checks_holder<T>::s_init_checks;

inline void inst_mpfr_init_checks()
{
    auto ptr = &mpfr_init_checks_holder<>::s_init_checks;
    (void)ptr;
}

#else

inline const mpfr_init_checks mpfr_init_checks_register;

#endif
}
}

#endif // MPPP_WITH_MPFR

#endif

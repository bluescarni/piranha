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

#ifndef PIRANHA_DETAIL_STACKTRACE_HPP
#define PIRANHA_DETAIL_STACKTRACE_HPP

#include <piranha/config.hpp>

#if defined(PIRANHA_WITH_BOOST_STACKTRACE)

#include <cstdlib>
#include <ios>
#include <iostream>
#include <limits>
#include <string>
#include <type_traits>

#if defined(_WIN32)

// NOTE: setting the backend explicitly is needed for
// proper support for clang-cl.
#define BOOST_STACKTRACE_USE_WINDBG
#include <boost/stacktrace.hpp>
#undef BOOST_STACKTRACE_USE_WINDBG

#else

// On non-windows, we force the use of libbacktrace for the time being.
#define BOOST_STACKTRACE_USE_BACKTRACE
#include <boost/stacktrace.hpp>
#undef BOOST_STACKTRACE_USE_BACKTRACE

#endif

namespace piranha
{
inline namespace impl
{

// Handy alias.
using stacktrace = boost::stacktrace::stacktrace;

// Custom streaming for stacktraces.
inline void stream_stacktrace(std::ostream &os, const stacktrace &st)
{
    os << '\n';
    const auto w = os.width();
    const auto max_idx_width = std::to_string(st.size()).size();
    // NOTE: the overflow check is because we will have to cast to std::streamsize
    // later, due to the iostreams API.
    // LCOV_EXCL_START
    if (unlikely(max_idx_width > static_cast<std::make_unsigned<std::streamsize>::type>(
                                     std::numeric_limits<std::streamsize>::max()))) {
        // Not much left to do here really.
        std::cerr << "Overflow in the size of a stacktrace, aborting now.\n";
        std::abort();
    }
    // LCOV_EXCL_STOP
    // We customize a bit the printing of the stacktrace, and we traverse
    // it backwards.
    auto i = st.size();
    for (auto it = st.rbegin(); it != st.rend(); --i, ++it) {
        os << "#";
        os.width(static_cast<std::streamsize>(max_idx_width));
        os << i;
        os.width(w);
        os << "| " << *it << '\n';
    }
}
}
}

#else

#error The detail/stacktrace.hpp header was included, but piranha was not configured with the PIRANHA_WITH_BOOST_STACKTRACE option.

#endif // PIRANHA_WITH_BOOST_STACKTRACE

#endif // Include guard.

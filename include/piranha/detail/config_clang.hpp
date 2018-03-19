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

#ifndef PIRANHA_CONFIG_CLANG_HPP
#define PIRANHA_CONFIG_CLANG_HPP

#if defined(__apple_build_version__)

// NOTE: Xcode hijacks the clang version macros. It seems like the first
// Xcode version with at least clang 3.1 was 4.3.
// https://gist.github.com/yamaya/2924292
#if __clang_major__ < 4 || (__clang_major__ == 4 && __clang_minor__ < 3)
#error The minimum supported Xcode version is 4.3.
#endif

#else

#if __clang_major__ < 3 || (__clang_major__ == 3 && __clang_minor__ < 1)
#error The minimum supported Clang version is 3.1.
#endif

#endif

#define PIRANHA_COMPILER_IS_CLANG

#if defined(_MSC_VER)

#define PIRANHA_COMPILER_IS_CLANG_CL
#define PIRANHA_PRETTY_FUNCTION __FUNCSIG__

#else

#define PIRANHA_PRETTY_FUNCTION __PRETTY_FUNCTION__

#endif

// NOTE: since clang 3.4, the -Wdeprecated-increment-bool flag is available
// and activated in debug builds. It sometimes generates a warning
// in unevaluated contexts which gets turned into an error by -Werror, so we want
// to be able to disable it selectively.
// NOTE: since clang 3.8, there's also a -Wincrement-bool flag.

#if defined(__apple_build_version__)

// clang 3.4 is available since Xcode 5.1.
#if __clang_major__ > 5 || (__clang_major__ == 5 && __clang_minor__ >= 1)

#define PIRANHA_CLANG_HAS_WDEPRECATED_INCREMENT_BOOL

#endif

// clang 3.8 is available since Xcode 7.3.
// https://en.wikipedia.org/wiki/Xcode#8.x_series
#if __clang_major__ > 7 || (__clang_major__ == 7 && __clang_minor__ >= 3)

#define PIRANHA_CLANG_HAS_WINCREMENT_BOOL

#endif

#else

#if __clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 4)

#define PIRANHA_CLANG_HAS_WDEPRECATED_INCREMENT_BOOL

#endif

#if __clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 8)

#define PIRANHA_CLANG_HAS_WINCREMENT_BOOL

#endif

#endif

#endif

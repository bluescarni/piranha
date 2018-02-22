# Copyright (c) 2016-2017 Francesco Biscani, <bluescarni@gmail.com>

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# ------------------------------------------------------------------------------------------

if(libbacktrace_INCLUDE_DIR AND libbacktrace_LIBRARY)
	# Already in cache, be silent
	set(libbacktrace_FIND_QUIETLY TRUE)
endif()

find_path(libbacktrace_INCLUDE_DIR NAMES backtrace.h)
find_library(libbacktrace_LIBRARY NAMES backtrace)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(libbacktrace DEFAULT_MSG libbacktrace_INCLUDE_DIR libbacktrace_LIBRARY)

mark_as_advanced(libbacktrace_INCLUDE_DIR)
mark_as_advanced(libbacktrace_LIBRARY)

# NOTE: this has been adapted from CMake's FindPNG.cmake.
if(libbacktrace_FOUND AND NOT TARGET libbacktrace::libbacktrace)
	add_library(libbacktrace::libbacktrace UNKNOWN IMPORTED)
	set_target_properties(libbacktrace::libbacktrace PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${libbacktrace_INCLUDE_DIR}"
		IMPORTED_LINK_INTERFACE_LANGUAGES "C" IMPORTED_LOCATION "${libbacktrace_LIBRARY}")
endif()

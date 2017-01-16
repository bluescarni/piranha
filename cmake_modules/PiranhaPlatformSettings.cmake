if(UNIX)
	include(CheckCXXSymbolExists)
	check_cxx_symbol_exists("posix_memalign" "cstdlib" _PIRANHA_POSIX_MEMALIGN_TEST)
	if(_PIRANHA_POSIX_MEMALIGN_TEST)
		message(STATUS "POSIX memalign detected.")
		set(PIRANHA_POSIX_MEMALIGN "#define PIRANHA_HAVE_POSIX_MEMALIGN")
	else()
		message(STATUS "POSIX memalign not detected.")
	endif()
endif()

# Setup for the machinery to detect cache line size in Windows. It's not supported everywhere, so we
# check for the existence of the SYSTEM_LOGICAL_PROCESSOR_INFORMATION type.
# http://msdn.microsoft.com/en-us/library/ms686694(v=vs.85).aspx
if(WIN32)
	include(CheckTypeSize)
	set(CMAKE_EXTRA_INCLUDE_FILES Windows.h)
	check_type_size("SYSTEM_LOGICAL_PROCESSOR_INFORMATION" _PIRANHA_SYSTEM_LOGICAL_PROCESSOR_INFORMATION)
	# Clear the variable.
	unset(CMAKE_EXTRA_INCLUDE_FILES)
	if(_PIRANHA_SYSTEM_LOGICAL_PROCESSOR_INFORMATION)
		message(STATUS "SYSTEM_LOGICAL_PROCESSOR_INFORMATION type found.")
		set(PIRANHA_SYSTEM_LOGICAL_PROCESSOR_INFORMATION "#define PIRANHA_HAVE_SYSTEM_LOGICAL_PROCESSOR_INFORMATION")
	else()
		MESSAGE(STATUS "SYSTEM_LOGICAL_PROCESSOR_INFORMATION type not found.")
	endif()
endif()

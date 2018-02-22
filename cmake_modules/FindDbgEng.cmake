include(FindPackageHandleStandardArgs)

if(DbgEng_LIBRARY)
	# Already in cache, be silent
	set(DbgEng_FIND_QUIETLY TRUE)
endif()

find_library(DbgEng_LIBRARY NAMES "dbgeng")

if(NOT DbgEng_LIBRARY)
    set(DbgEng_USE_DIRECTLY TRUE)
    set(DbgEng_LIBRARY "dbgeng.lib" CACHE FILEPATH "" FORCE)
endif()

if(DbgEng_USE_DIRECTLY)
    message(STATUS "dbgeng.lib will be linked directly.")
endif()

find_package_handle_standard_args(DbgEng DEFAULT_MSG DbgEng_LIBRARY)

mark_as_advanced(DbgEng_LIBRARY)

if(DbgEng_FOUND AND NOT TARGET DbgEng::DbgEng)
    message(STATUS "Creating the 'DbgEng::DbgEng' imported target.")
    if(DbgEng_USE_DIRECTLY)
        # If we are using it directly, we must define an interface library,
        # as we do not have the full path to the shared library.
        add_library(DbgEng::DbgEng INTERFACE IMPORTED)
        set_target_properties(DbgEng::DbgEng PROPERTIES INTERFACE_LINK_LIBRARIES "${DbgEng_LIBRARY}")
    else()
        # Otherwise, we proceed as usual.
        add_library(DbgEng::DbgEng UNKNOWN IMPORTED)
        set_target_properties(DbgEng::DbgEng PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${DbgEng_LIBRARY}")
    endif()
endif()

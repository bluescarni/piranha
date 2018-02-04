# We always link to the Boost::serialization library.
set(_PIRANHA_REQUIRED_BOOST_LIBS serialization)

# Boost::iostreams is needed if either:
# - any compression is enabled (in which case we need the iostreams filter), or
# - msgpack support is enabled (in which case we need the memory mapped file wrapper).
if(PIRANHA_WITH_ZLIB OR PIRANHA_WITH_BZIP2 OR PIRANHA_WITH_MSGPACK)
	list(APPEND _PIRANHA_REQUIRED_BOOST_LIBS iostreams)
endif()

# Boost::system and Boost::filesystem are needed only if building the tests or the benchmarks.
if(PIRANHA_BUILD_TESTS OR PIRANHA_BUILD_BENCHMARKS)
	list(APPEND _PIRANHA_REQUIRED_BOOST_LIBS filesystem system)
endif()

# Boost::python.
if(PIRANHA_BUILD_PYRANHA)
	if(${PYTHON_VERSION_MAJOR} EQUAL 2)
		list(APPEND _PIRANHA_REQUIRED_BOOST_LIBS python)
	else()
		list(APPEND _PIRANHA_REQUIRED_BOOST_LIBS python3)
	endif()
endif()

message(STATUS "Required Boost libraries: ${_PIRANHA_REQUIRED_BOOST_LIBS}")

find_package(Boost 1.58.0 REQUIRED COMPONENTS "${_PIRANHA_REQUIRED_BOOST_LIBS}")

if(NOT Boost_FOUND)
	message(FATAL_ERROR "Not all requested Boost components were found, exiting.")
endif()

message(STATUS "Detected Boost version: ${Boost_VERSION}")
message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")

# Might need to recreate these targets if they are missing (e.g., older CMake versions).
if(NOT TARGET Boost::boost)
    message(STATUS "The 'Boost::boost' target is missing, creating it.")
    add_library(Boost::boost INTERFACE IMPORTED)
    set_target_properties(Boost::boost PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${Boost_INCLUDE_DIRS}")
endif()

if(NOT TARGET Boost::disable_autolinking)
    message(STATUS "The 'Boost::disable_autolinking' target is missing, creating it.")
    add_library(Boost::disable_autolinking INTERFACE IMPORTED)
    if(WIN32)
        set_target_properties(Boost::disable_autolinking PROPERTIES INTERFACE_COMPILE_DEFINITIONS "BOOST_ALL_NO_LIB")
    endif()
endif()

foreach(_PIRANHA_BOOST_COMPONENT ${_PIRANHA_REQUIRED_BOOST_LIBS})
	if(NOT TARGET Boost::${_PIRANHA_BOOST_COMPONENT})
		# NOTE: CMake's Boost finding module will not provide imported targets for recent Boost versions, as it needs
		# an explicit mapping specifying the dependencies between the various Boost libs (and this is version-dependent).
		# If we are here, it means that Boost was correctly found with all the needed components, but the Boost version
		# found is too recent and imported targets are not available. We will reconstruct them here in order to be able
		# to link to targets rather than using the variables defined by the FindBoost.cmake module.
		# NOTE: in Piranha's case, we are lucky because all the Boost libs we need don't have any interdependency
		# with other boost libs.
		message(STATUS "The 'Boost::${_PIRANHA_BOOST_COMPONENT}' imported target is missing, creating it manually.")
		string(TOUPPER ${_PIRANHA_BOOST_COMPONENT} _PIRANHA_BOOST_UPPER_COMPONENT)
		if(Boost_USE_STATIC_LIBS)
			add_library(Boost::${_PIRANHA_BOOST_COMPONENT} STATIC IMPORTED)
		else()
			add_library(Boost::${_PIRANHA_BOOST_COMPONENT} UNKNOWN IMPORTED)
		endif()
		set_target_properties(Boost::${_PIRANHA_BOOST_COMPONENT} PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${Boost_INCLUDE_DIRS}")
		set_target_properties(Boost::${_PIRANHA_BOOST_COMPONENT} PROPERTIES
			IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
			IMPORTED_LOCATION "${Boost_${_PIRANHA_BOOST_UPPER_COMPONENT}_LIBRARY}")
		unset(_PIRANHA_BOOST_UPPER_COMPONENT)
	else()
		message(STATUS "The 'Boost::${_PIRANHA_BOOST_COMPONENT}' imported target is available.")
		if(_PIRANHA_BOOST_COMPONENT STREQUAL "iostreams")
			# FindBoost.cmake marks regexp as a dependency for iostreams, but it is true only if one uses the iostreams
			# regexp filter (we don't). Remove the dependency.
			message(STATUS "Removing the dependency of 'Boost::iostreams' on 'Boost::regex'.")
			get_target_property(_PIRANHA_BOOST_IOSTREAMS_LINK_LIBRARIES Boost::iostreams INTERFACE_LINK_LIBRARIES)
			list(REMOVE_ITEM _PIRANHA_BOOST_IOSTREAMS_LINK_LIBRARIES "Boost::regex")
			set_target_properties(Boost::iostreams PROPERTIES INTERFACE_LINK_LIBRARIES "${_PIRANHA_BOOST_IOSTREAMS_LINK_LIBRARIES}")
		endif()
	endif()
endforeach()

# Cleanup.
unset(_PIRANHA_REQUIRED_BOOST_LIBS)

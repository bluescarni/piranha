include(YACMACompilerLinkerSettings)

# Python version check.
if(${PYTHON_VERSION_MAJOR} LESS 2 OR (${PYTHON_VERSION_MAJOR} EQUAL 2 AND ${PYTHON_VERSION_MINOR} LESS 7))
	message(FATAL_ERROR "The minimum supported Python version is 2.7.")
endif()

# Helper file with version number.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/_version.py.in" "${CMAKE_CURRENT_BINARY_DIR}/_version.py" @ONLY)

YACMA_PYTHON_MODULE(_core
	# General.
	core.cpp
	type_system.cpp
	python_converters.hpp
	exceptions.hpp
	python_includes.hpp
	type_system.hpp
	expose_utils.hpp
	expose_utils.cpp
	# Polynomials.
	polynomial_descriptor.hpp
	expose_polynomials.hpp
	expose_polynomials_0.cpp
	expose_polynomials_1.cpp
	expose_polynomials_2.cpp
	expose_polynomials_3.cpp
	expose_polynomials_4.cpp
	expose_polynomials_5.cpp
	expose_polynomials_6.cpp
	expose_polynomials_7.cpp
	expose_polynomials_8.cpp
	expose_polynomials_9.cpp
	expose_polynomials_10.cpp
	# Poisson series.
	poisson_series_descriptor.hpp
	expose_poisson_series.hpp
	expose_poisson_series_0.cpp
	expose_poisson_series_1.cpp
	expose_poisson_series_2.cpp
	expose_poisson_series_3.cpp
	expose_poisson_series_4.cpp
	expose_poisson_series_5.cpp
	expose_poisson_series_6.cpp
	expose_poisson_series_7.cpp
	expose_poisson_series_8.cpp
	expose_poisson_series_9.cpp
	expose_poisson_series_10.cpp
	expose_poisson_series_11.cpp
	# Divisor series.
	divisor_series_descriptor.hpp
	expose_divisor_series.hpp
	expose_divisor_series_0.cpp
	expose_divisor_series_1.cpp
	expose_divisor_series_2.cpp
	expose_divisor_series_3.cpp
	expose_divisor_series_4.cpp
	expose_divisor_series_5.cpp
)

 # Setup the installation path.
set(PYRANHA_INSTALL_PATH "${YACMA_PYTHON_MODULES_INSTALL_PATH}/pyranha")

# Boost.Python version 1.64 (and possibly later?) has issues with default hidden visibility.
if(${Boost_MAJOR_VERSION} GREATER 1 OR (${Boost_MAJOR_VERSION} EQUAL 1 AND ${Boost_MINOR_VERSION} GREATER 63))
	list(FIND PIRANHA_CXX_FLAGS_DEBUG "-fvisibility=hidden" _PYGMO_HAS_VHIDDEN)
	if (NOT ${_PYGMO_HAS_VHIDDEN} EQUAL -1)
		message(STATUS "Removing the '-fvisibility=hidden' flag for Boost.Python > 1.63.")
		list(REMOVE_ITEM PIRANHA_CXX_FLAGS_DEBUG "-fvisibility=hidden")
		list(REMOVE_ITEM PIRANHA_CXX_FLAGS_RELEASE "-fvisibility=hidden")
	endif()
endif()

# The name of the BP target to link to depends on the Python version.
if(${PYTHON_VERSION_MAJOR} EQUAL 2)
	set(PYRANHA_BP_TARGET "Boost::python")
else()
	set(PYRANHA_BP_TARGET "Boost::python3")
endif()

target_compile_options(_core PRIVATE "$<$<CONFIG:DEBUG>:${PIRANHA_CXX_FLAGS_DEBUG}>" "$<$<CONFIG:RELEASE>:${PIRANHA_CXX_FLAGS_RELEASE}>")
# The module needs Boost.Python and the mandatory libraries. Python libs and includes are already linked in
# by YACMA_PYTHON_MODULE().
target_link_libraries(_core PRIVATE piranha ${PYRANHA_BP_TARGET})

# NOTE: for clang-cl, cmake tries to set -std=c++11 here, which makes
# it error out. Disable it as MSVC 2015 is implicitly C++14 anyway.
if(NOT (YACMA_COMPILER_IS_MSVC AND YACMA_COMPILER_IS_CLANGXX))
	# Let's setup the target C++ standard, but only if the user did not provide it manually.
	if(NOT CMAKE_CXX_STANDARD)
		set_property(TARGET _core PROPERTY CXX_STANDARD 11)
	endif()
	set_property(TARGET _core PROPERTY CXX_STANDARD_REQUIRED YES)
	set_property(TARGET _core PROPERTY CXX_EXTENSIONS NO)
endif()

install(TARGETS _core
	RUNTIME DESTINATION ${PYRANHA_INSTALL_PATH}
	LIBRARY DESTINATION ${PYRANHA_INSTALL_PATH}
)

# Add the global init file and the additional submodules.
install(FILES __init__.py _common.py celmec.py math.py test.py types.py "${CMAKE_CURRENT_BINARY_DIR}/_version.py" DESTINATION ${PYRANHA_INSTALL_PATH})

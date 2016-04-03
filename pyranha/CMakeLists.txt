INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_DIRS})

SET(PYRANHA_INSTALL_PATH "${PYTHON_MODULES_PATH}/pyranha")

# Tutorial files.
set(PYRANHA_TUTORIAL_FILES
	tutorial/__init__.py
	tutorial/hello_piranha.py
	tutorial/integer.py
	tutorial/rational.py
)

ADD_LIBRARY(_core SHARED
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
	expose_polynomials_11.cpp
	expose_polynomials_12.cpp
	expose_polynomials_13.cpp
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
	expose_poisson_series_12.cpp
	expose_poisson_series_13.cpp
	expose_poisson_series_14.cpp
	# Divisor series.
	divisor_series_descriptor.hpp
	expose_divisor_series.hpp
	expose_divisor_series_0.cpp
	expose_divisor_series_1.cpp
	expose_divisor_series_2.cpp
	expose_divisor_series_3.cpp
	expose_divisor_series_4.cpp
	expose_divisor_series_5.cpp
	expose_divisor_series_6.cpp
	expose_divisor_series_7.cpp
	expose_divisor_series_8.cpp
	# Rational functions.
	expose_rational_functions.hpp
	expose_rational_functions_0.cpp
	expose_rational_functions_1.cpp
	# Python files.
	__init__.py _common.py celmec.py math.py test.py types.py
	${PYRANHA_TUTORIAL_FILES}
)

# Initial definition of extra compile flags.
set(PYRANHA_EXTRA_COMPILE_FLAGS "")

# Prefix and suffix setup.
SET_TARGET_PROPERTIES(_core PROPERTIES PREFIX "")
IF(PYDEXTENSION)
	SET_TARGET_PROPERTIES(_core PROPERTIES SUFFIX ".pyd")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	# Apparently on OS X Python expects the .so extension for compiled modules.
	MESSAGE(STATUS "OS X detected, pyranha's module suffix will be '.so'.")
	SET_TARGET_PROPERTIES(_core PROPERTIES SUFFIX ".so")
ENDIF()

# Strict aliasing setup.
# NOTE: Python < 3 is not strict C, as per:
# http://www.python.org/dev/peps/pep-3123/
# When using GCC or Clang, we add -fno-strict-aliasing here and when compiling core.cpp to prevent possible problems.
# Other compilers should probably use similar flags.
if(${PYTHON_LIBRARY_VERSION} LESS 30 AND (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX))
	message(STATUS "Python < 3 version detected: Pyranha will be compiled with the '-fno-strict-aliasing' flag.")
	set(PYRANHA_EXTRA_COMPILE_FLAGS "${PYRANHA_EXTRA_COMPILE_FLAGS} -fno-strict-aliasing")
	set_target_properties(_core PROPERTIES LINK_FLAGS "-fno-strict-aliasing")
endif()

# Setup the extra compile flags.
message(STATUS "Extra compile flags for pyranha: ${PYRANHA_EXTRA_COMPILE_FLAGS}")
set_target_properties(_core PROPERTIES COMPILE_FLAGS "${PYRANHA_EXTRA_COMPILE_FLAGS}")

# The module needs Boost.Python and the Python libs, plus the mandatory libraries.
TARGET_LINK_LIBRARIES(_core ${MANDATORY_LIBRARIES} ${Boost_PYTHON_LIBRARY} ${PYTHON_LIBRARIES})
INSTALL(TARGETS _core
	RUNTIME DESTINATION ${PYRANHA_INSTALL_PATH}
	LIBRARY DESTINATION ${PYRANHA_INSTALL_PATH}
)

# Add the global init file and the additional submodules.
INSTALL(FILES __init__.py _common.py celmec.py math.py test.py types.py DESTINATION ${PYRANHA_INSTALL_PATH})

# Install the tutorial files.
install(FILES ${PYRANHA_TUTORIAL_FILES} DESTINATION "${PYRANHA_INSTALL_PATH}/_tutorial")

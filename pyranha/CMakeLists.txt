INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

SET(PYRANHA_INSTALL_PATH ${PYTHON_MODULES_PATH}/pyranha)

ADD_LIBRARY(_core SHARED core.cpp type_system.cpp python_converters.hpp exceptions.hpp type_system.hpp expose_utils.hpp expose_utils.cpp
	expose_polynomials.hpp expose_polynomials.cpp
	expose_poisson_series.hpp expose_poisson_series.cpp
)
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
IF(${PYTHON_LIBRARY_VERSION} LESS 30 AND (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX))
	MESSAGE(STATUS "Python < 3 version detected: Pyranha will be compiled with the '-fno-strict-aliasing' flag.")
	SET_TARGET_PROPERTIES(_core PROPERTIES COMPILE_FLAGS "-fno-strict-aliasing" LINK_FLAGS "-fno-strict-aliasing")
ENDIF()

# Module needs to Boost.Python and the Python libs, plus the mandatory libraries.
TARGET_LINK_LIBRARIES(_core ${MANDATORY_LIBRARIES} ${Boost_PYTHON_LIBRARY} ${PYTHON_LIBRARIES})
INSTALL(TARGETS _core
	RUNTIME DESTINATION ${PYRANHA_INSTALL_PATH}
	LIBRARY DESTINATION ${PYRANHA_INSTALL_PATH}
)

# Add the global init file and the additional submodules.
INSTALL(FILES __init__.py _common.py celmec.py math.py test.py types.py DESTINATION ${PYRANHA_INSTALL_PATH})

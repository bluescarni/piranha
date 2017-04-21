# Python version check.
if(${PYTHON_VERSION_MAJOR} LESS 2 OR (${PYTHON_VERSION_MAJOR} EQUAL 2 AND ${PYTHON_VERSION_MINOR} LESS 6))
	message(FATAL_ERROR "Minimum supported Python version is 2.6.")
endif()

# Helper file with version number.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/_version.py.in" "${CMAKE_CURRENT_BINARY_DIR}/_version.py" @ONLY)

# Tutorial files.
set(PYRANHA_TUTORIAL_FILES
	tutorial/__init__.py
	tutorial/hello_piranha.py
	tutorial/integer.py
	tutorial/rational.py
)

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

target_compile_options(_core PRIVATE "$<$<CONFIG:DEBUG>:${PIRANHA_CXX_FLAGS_DEBUG}>" "$<$<CONFIG:RELEASE>:${PIRANHA_CXX_FLAGS_RELEASE}>")
# The module needs Boost.Python and the mandatory libraries. Python libs and includes are already linked in
# by YACMA_PYTHON_MODULE().
target_link_libraries(_core PRIVATE piranha Boost::python ${PIRANHA_MANDATORY_LINK_TARGETS})
set_property(TARGET _core PROPERTY CXX_STANDARD 11)
set_property(TARGET _core PROPERTY CXX_STANDARD_REQUIRED YES)
set_property(TARGET _core PROPERTY CXX_EXTENSIONS NO)
install(TARGETS _core
	RUNTIME DESTINATION ${PYRANHA_INSTALL_PATH}
	LIBRARY DESTINATION ${PYRANHA_INSTALL_PATH}
)

# Add the global init file and the additional submodules.
install(FILES __init__.py _common.py celmec.py math.py test.py types.py "${CMAKE_CURRENT_BINARY_DIR}/_version.py" DESTINATION ${PYRANHA_INSTALL_PATH})

# Install the tutorial files.
install(FILES ${PYRANHA_TUTORIAL_FILES} DESTINATION "${PYRANHA_INSTALL_PATH}/_tutorial")

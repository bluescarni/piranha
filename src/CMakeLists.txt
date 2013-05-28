SET(SRC_LIST
	degree_truncator_settings.cpp
	environment.cpp
	exceptions.cpp
	real.cpp
	runtime_info.cpp
	settings.cpp
	symbol.cpp
	thread_barrier.cpp
	thread_management.cpp
	tracing.cpp
)

SET(HEADERS_LIST
	forwarding.hpp
	degree_truncator_settings.hpp
	series.hpp
	concepts/series.hpp
	concepts/power_series_term.hpp
	concepts/coefficient.hpp
	concepts/truncator.hpp
	concepts/multipliable_term.hpp
	concepts/container_element.hpp
	concepts/power_series.hpp
	concepts/term.hpp
	concepts/differentiable_term.hpp
	power_series_term.hpp
	poisson_series.hpp
	polynomial_term.hpp
	poisson_series_term.hpp
	settings.hpp
	task_group.hpp
	exceptions.hpp
	tracing.hpp
	base_term.hpp
	rational.hpp
	thread_barrier.hpp
	polynomial.hpp
	kronecker_monomial.hpp
	echelon_size.hpp
	hash_set.hpp
	truncator.hpp
	debug_access.hpp
	real_trigonometric_kronecker_monomial.hpp
	series_binary_operators.hpp
	thread_management.hpp
	malloc_allocator.hpp
	kronecker_array.hpp
	concepts.hpp
	static_vector.hpp
	real.hpp
	piranha.hpp
	symbol_set.hpp
	runtime_info.hpp
	power_series.hpp
	series_multiplier.hpp
	array_key.hpp
	symbol.hpp
	t_substitutable_series.hpp
	detail/series_multiplier_fwd.hpp
	detail/mpfr.hpp
	detail/config_clang.hpp
	detail/gcc_int128.hpp
	detail/math_tt_fwd.hpp
	detail/sfinae_types.hpp
	detail/real_fwd.hpp
	detail/truncator_fwd.hpp
	detail/degree_commons.hpp
	detail/base_term_fwd.hpp
	detail/km_commons.hpp
	detail/integer_fwd.hpp
	detail/poisson_series_fwd.hpp
	detail/series_fwd.hpp
	detail/rational_fwd.hpp
	detail/toolbox.hpp
	detail/polynomial_fwd.hpp
	detail/config_gcc.hpp
	detail/symbol_set_fwd.hpp
	print_coefficient.hpp
	type_traits.hpp
	univariate_monomial.hpp
	power_series_truncator.hpp
	timeit.hpp
	integer.hpp
	math.hpp
	environment.hpp
	print_tex_coefficient.hpp
	cache_aligning_allocator.hpp
	trigonometric_series.hpp
	monomial.hpp
	threading.hpp
)

IF(BUILD_MAIN OR BUILD_TESTS)
	ADD_LIBRARY(piranha STATIC ${SRC_LIST} ${HEADERS_LIST})
	TARGET_LINK_LIBRARIES(piranha ${MANDATORY_LIBRARIES})
ENDIF(BUILD_MAIN OR BUILD_TESTS)

IF(BUILD_PYRANHA)
	ADD_LIBRARY(pyranha SHARED ${SRC_LIST} ${HEADERS_LIST})
	# NOTE: Python < 3 is not strict C, as per:
	# http://www.python.org/dev/peps/pep-3123/
	# When using GCC, we add -fno-strict-aliasing here and when compiling core.cpp to prevent possible problems.
	# Other compilers should probably use similar flags.
	IF(${PYTHON_LIBRARY_VERSION} LESS 30 AND CMAKE_COMPILER_IS_GNUCXX)
		MESSAGE(STATUS "Python < 3 version detected: Pyranha will be compiled with the '-fno-strict-aliasing' flag.")
		SET_TARGET_PROPERTIES(pyranha PROPERTIES COMPILE_FLAGS "-fno-strict-aliasing" LINK_FLAGS "-fno-strict-aliasing"
			COMPILE_DEFINITIONS "PIRANHA_BUILDING_DLL")
	ENDIF(${PYTHON_LIBRARY_VERSION} LESS 30 AND CMAKE_COMPILER_IS_GNUCXX)
	TARGET_LINK_LIBRARIES(pyranha ${MANDATORY_LIBRARIES})
	# TODO this needs to be fixed in Windows.
	IF(NOT WIN32)
		INSTALL(TARGETS pyranha
			RUNTIME DESTINATION ${LIB_INSTALL_PATH}
			LIBRARY DESTINATION ${LIB_INSTALL_PATH}
		)
	ENDIF(NOT WIN32)
ENDIF(BUILD_PYRANHA)

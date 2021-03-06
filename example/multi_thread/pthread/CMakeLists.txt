# -----------------------------------------------------------------------------
# CppAD: C++ Algorithmic Differentiation: Copyright (C) 2003-18 Bradley M. Bell
#
# CppAD is distributed under multiple licenses. This distribution is under
# the terms of the
#                     Eclipse Public License Version 1.0.
#
# A copy of this license is included in the COPYING file of this distribution.
# Please visit http://www.coin-or.org/CppAD/ for information on other licenses.
# -----------------------------------------------------------------------------
# Build the example/multi_thread/pthread directory tests
# Inherit build type from ../CMakeList.txt

# Local include directories to search (not in package_prefix/include)
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/.. )

# add_executable(<name> [WIN32] [MACOSX_BUNDLE] [EXCLUDE_FROM_ALL]
#                 source1 source2 ... sourceN
# )
SET(source_list ../thread_test.cpp
	${CMAKE_SOURCE_DIR}/speed/src/microsoft_timer.cpp
	../team_example.cpp
	../harmonic.cpp
	../multi_atomic.cpp
	../checkpoint.cpp
	../multi_newton.cpp
	a11c_pthread.cpp
	simple_ad_pthread.cpp
	team_pthread.cpp
)
set_compile_flags( example_multi_thread_pthread "${cppad_debug_which}" "${source_list}" )
#
ADD_EXECUTABLE( example_multi_thread_pthread EXCLUDE_FROM_ALL ${source_list} )

# Paths in which the linker will search for libraries,
# only applies to targets created after it is called
LINK_DIRECTORIES( ${pthread_lib_path} )

# List of libraries to be linked into the specified target
TARGET_LINK_LIBRARIES(example_multi_thread_pthread ${pthread_lib})

# Add the check_example_multi_thread_pthread target
ADD_CUSTOM_TARGET(check_example_multi_thread_pthread
	example_multi_thread_pthread simple_ad
	DEPENDS example_multi_thread_pthread
)
MESSAGE(STATUS "make check_example_multi_thread_pthread: available")

# Change check depends in parent environment
add_to_list(check_example_multi_thread_depends check_example_multi_thread_pthread)
SET(check_example_multi_thread_depends "${check_example_multi_thread_depends}" PARENT_SCOPE)

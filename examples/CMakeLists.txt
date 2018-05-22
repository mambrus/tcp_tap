cmake_minimum_required (VERSION 2.6)
project (examples C)

include(CheckLibraryExists)
include(CheckTypeSize)
include(CheckCSourceCompiles)
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)

# System libraries deduction (begin)
CHECK_LIBRARY_EXISTS(rt clock_gettime "" HAVE_LIB_RT)
CHECK_LIBRARY_EXISTS(pthread pthread_create "" HAVE_LIB_PTHREAD)

if (HAVE_LIB_RT)
    set(EXTRA_LIBS ${EXTRA_LIBS} rt)
endif (HAVE_LIB_RT)

if (HAVE_LIB_PTHREAD)
    set(EXTRA_LIBS ${EXTRA_LIBS} pthread)
endif (HAVE_LIB_PTHREAD)

message (STATUS "Checking for (p)Threads")
find_package(Threads)

# Functions that might be missing and need worka-arounding
# Both of these are needed for either of the test to work (strange...)
set(CMAKE_REQUIRED_INCLUDES pthread.h)
set(CMAKE_REQUIRED_LIBRARIES pthread)

CHECK_FUNCTION_EXISTS("pthread_cancel"  HAVE_PTHREAD_CANCEL_F)
CHECK_SYMBOL_EXISTS("pthread_cancel" "pthread.h" HAVE_PTHREAD_CANCEL_S)

CHECK_FUNCTION_EXISTS("pthread_create"  HAVE_PTHREAD_CREATE_F)
CHECK_SYMBOL_EXISTS("pthread_create" "pthread.h" HAVE_PTHREAD_CREATE_S)

################################################################################
# Configuration options
################################################################################

#-------------------------------------------------------------------------------
# X buildtool options (gcc)
#-------------------------------------------------------------------------------
set(SYSROOT
	${CMAKE_SYSROOT}
	CACHE STRING
	"System path (--system=)")

#-------------------------------------------------------------------------------
# System (project supported) options
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Build-tool options
#-------------------------------------------------------------------------------

set(CMAKE_EXTRA_C_FLAGS
	"${CMAKE_EXTRA_C_FLAGS}"
	CACHE STRING
	"Compiler options appended to CMAKE_C_FLAGS")

#-------------------------------------------------------------------------------
# X build-tool options (this project's x-tool handling)
#-------------------------------------------------------------------------------
if (NOT SYSROOT STREQUAL "")
	message( "** INFO: SYSROOT was either set or defaulted from toolchain file" )
	set(CMAKE_SYSROOT "${SYSROOT}")
	set(CMAKE_EXTRA_C_FLAGS "${CMAKE_EXTRA_C_FLAGS} --sysroot=${CMAKE_SYSROOT}")
endif (NOT SYSROOT STREQUAL "")

set(CMAKE_C_FLAGS "${CMAKE_EXTRA_C_FLAGS} -no-integrated-cpp -Wno-unused-function -Wall")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_EXTRA_C_FLAGS} -g3 -ggdb3 -O0")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_EXTRA_C_FLAGS} -no-integrated-cpp -Wno-unused-function -O2 -Wall")

################################################################################
# Libraries (src binding)
################################################################################

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
	"${PROJECT_SOURCE_DIR}/config.h.in"
	"${PROJECT_BINARY_DIR}/config.h"
)
# add the binary tree to the search path for include files
# so that we will find Config.h
include_directories("${PROJECT_BINARY_DIR}")

# Example targets
add_executable(echo_multi echo_multi.c)
target_link_libraries (echo_multi atcptap ${EXTRA_LIBS})

add_executable(echo_simple echo_simple.c)
target_link_libraries (echo_simple atcptap ${EXTRA_LIBS})

add_executable(client client.c)
target_link_libraries (client atcptap ${EXTRA_LIBS})

add_executable(switchboard_simple switchboard_simple.c)
target_link_libraries (switchboard_simple atcptap ${EXTRA_LIBS})

add_executable(switchboard_threaded switchboard_threaded.c)
target_link_libraries (switchboard_threaded atcptap ${EXTRA_LIBS})

add_executable(myrsh myrsh.c)
target_link_libraries (myrsh atcptap ${EXTRA_LIBS})

# Targets installation (provided for reference):
install(TARGETS switchboard_simple echo_multi
        RUNTIME       DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        LIBRARY       DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        ARCHIVE       DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME})


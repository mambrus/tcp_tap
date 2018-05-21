cmake_minimum_required (VERSION 2.6)
project (tcp_tap C)

include(CheckLibraryExists)
include(CheckTypeSize)
include(CheckCSourceCompiles)
include(CheckIncludeFiles)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/include")

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
# Defaults to corresponding environment variables
#-------------------------------------------------------------------------------
set (TCP_TAP_EXEC
	${TCP_TAP_EXEC} "/bin/sh"
	CACHE STRING
	"The binary you want to run and extend with tap-ability")

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

##-------------------------------------------------------------------------------
## some-lib - target system dependent
##-------------------------------------------------------------------------------
#if (NOT SOMEVAR)
#	include_directories ("${PROJECT_SOURCE_DIR}/some-lib")
#	add_subdirectory (some-lib)
#	set (EXTRA_LIBS ${EXTRA_LIBS} some-lib)
#endif (NOT EHWE)

################################################################################
# Project
################################################################################
# The version number.
set (TCP-TAP_VERSION_MAJOR 0)
set (TCP-TAP_VERSION_MINOR 1)
set (TCP-TAP_VERSION_PATCH 0)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
	"${PROJECT_SOURCE_DIR}/tcp-tap_config.h.in"
	"${PROJECT_BINARY_DIR}/tcp-tap_config.h"
)

# add the binary tree to the search path for include files
# so that we will find Config.h
include_directories("${PROJECT_BINARY_DIR}")

# System libraries deduction (begin)
CHECK_LIBRARY_EXISTS(rt clock_gettime "" HAVE_LIB_RT)
CHECK_LIBRARY_EXISTS(pthread pthread_create "" HAVE_LIB_PTHREAD)

if (HAVE_LIB_RT)
    set(EXTRA_LIBS ${EXTRA_LIBS} rt)
endif (HAVE_LIB_RT)
if (HAVE_LIB_PTHREAD)
    set(EXTRA_LIBS ${EXTRA_LIBS} pthread)
endif (HAVE_LIB_PTHREAD)
# System libraries deduction (end)

# This level as Library
set(LIB_TCP_TAP_SOURCE
	server.c  switchboard.c
)
add_library(tcptap ${LIB_TCP_TAP_SOURCE})
target_link_libraries (tcptap ${EXTRA_LIBS})

# This level - program
set (EXTRA_LIBS ${EXTRA_LIBS} tcptap)

set(TCP-TAP_SOURCE
	main.c  sig_mngr.c
)
add_executable(tcp-tap ${TCP-TAP_SOURCE})
target_link_libraries (tcp-tap tcptap ${EXTRA_LIBS})

# Installation
install(PROGRAMS ${PROJECT_BINARY_DIR}/src DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

cmake_minimum_required (VERSION 3.0.2)
project (tcp-tap C)

include(CheckLibraryExists)
include(CheckTypeSize)
include(CheckCSourceCompiles)
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/include")

# System libraries deduction (begin)

CHECK_LIBRARY_EXISTS(rt clock_gettime "" HAVE_LIB_RT)
CHECK_LIBRARY_EXISTS(pthread pthread_create "" HAVE_LIB_PTHREAD)
CHECK_LIBRARY_EXISTS(log log_set_verbosity "/usr/local/lib/" HAVE_LIB_LIBLOG)

if (HAVE_LIB_RT)
    set(EXTRA_LIBS ${EXTRA_LIBS} rt)
endif (HAVE_LIB_RT)

if (HAVE_LIB_PTHREAD)
    set(EXTRA_LIBS ${EXTRA_LIBS} pthread)
endif (HAVE_LIB_PTHREAD)

if (HAVE_LIB_LIBLOG)
    set (EXTRA_LIBS ${EXTRA_LIBS} log)
else (HAVE_LIB_LIBLOG)
   message (SEND_ERROR
      "Dependent library liblog isn't installed"
      "Please install or build from: "
      "https://github.com/mambrus/liblog")
endif (HAVE_LIB_LIBLOG)

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
# Build-system options
#-------------------------------------------------------------------------------
option (BUILD_EXAMPLES "Include examples in this build" OFF)

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
if (CMAKE_COMPILER_IS_GNUCC)
	if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5.4)
		set(HAVE_SIGUNUSED)
	endif ()
endif (CMAKE_COMPILER_IS_GNUCC)


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
## Target system dependent
##-------------------------------------------------------------------------------

#if (NOT HAVE_LIB_MQUEUE)
#	include_directories ("${PROJECT_SOURCE_DIR}/lib/mqueue/include")
#	add_subdirectory (lib/mqueue)
#	set (EXTRA_LIBS ${EXTRA_LIBS} libmqueue)
#endif (NOT HAVE_LIB_MQUEUE)

################################################################################
# Examples (src binding)
################################################################################
if (NOT USE_LIB_MQUEUE)
	add_subdirectory (examples)
endif (NOT USE_LIB_MQUEUE)

################################################################################
# Project
################################################################################
# The version number.
set (TCP-TAP_VERSION_MAJOR 0)
set (TCP-TAP_VERSION_MINOR 6)
set (TCP-TAP_VERSION_PATCH 3)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
	"${PROJECT_SOURCE_DIR}/tcp-tap_config.h.in"
	"${PROJECT_BINARY_DIR}/tcp-tap_config.h"
)

# add the binary tree to the search path for include files
# so that we will find Config.h
include_directories("${PROJECT_BINARY_DIR}")

# This level as Library
set(LIB_TCP_TAP_SOURCE
	clientserver.c  switchboard.c
)
add_library(atcptap STATIC ${LIB_TCP_TAP_SOURCE})
target_link_libraries (atcptap ${EXTRA_LIBS})

add_library(sotcptap SHARED ${LIB_TCP_TAP_SOURCE})
target_link_libraries (sotcptap ${EXTRA_LIBS})
set_target_properties(
  sotcptap PROPERTIES PUBLIC_HEADER
  "include/tcp-tap/clientserver.h;include/tcp-tap/switchboard.h")

# This level - program
set (EXTRA_LIBS ${EXTRA_LIBS} atcptap)

set(TCP-TAP_SOURCE
	main.c  sig_mngr.c
)
add_executable(tcp-tap ${TCP-TAP_SOURCE})
target_link_libraries (tcp-tap atcptap ${EXTRA_LIBS})

# Installation
# Shells (ignore for now)
#install(PROGRAMS ${PROJECT_BINARY_DIR}/gdb.tap.sh DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

# Targets:
install(TARGETS tcp-tap atcptap sotcptap
        RUNTIME       DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        LIBRARY       DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        ARCHIVE       DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME})


# Install package generation
# --------------------------
SET(CPACK_PACKAGE_CONTACT "Michael Ambrus")
SET(CPACK_PACKAGE_VERSION_MAJOR ${TCP-TAP_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${TCP-TAP_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${TCP-TAP_VERSION_PATCH})
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY
	"TCP-TAP: A multitheaded/multi-sessioned\"tap\" to reside between two"
	"processes stdio adding tcp network connectivity"
	"LICENSE: GPL v2.0"
	"Copyright (c) 2011 Michael Ambrus")
SET(CPACK_RESOURCE_FILE_LICENSE
	${PROJECT_SOURCE_DIR}/COPYING)

set(SPECIFIED_CPACK
	${SPECIFIED_CPACK}
	CACHE STRING
	"Specify cpack generator: [NSIS|ZIP|DEB|TGZ|RPM]")

if (NOT SPECIFIED_CPACK STREQUAL "")
	SET(CPACK_GENERATOR ${SPECIFIED_CPACK})
endif ()

INCLUDE(CPack)


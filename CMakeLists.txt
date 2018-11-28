# -*- coding: utf-8 -*-

cmake_minimum_required(VERSION 2.8.0)
project(mujinplc)

# Define here the needed parameters
set (MUJINPLC_VERSION_MAJOR 0)
set (MUJINPLC_VERSION_MINOR 1)
set (MUJINPLC_VERSION_PATCH 0)
set (MUJINPLC_VERSION ${MUJINPLC_VERSION_MAJOR}.${MUJINPLC_VERSION_MINOR}.${MUJINPLC_VERSION_PATCH})
set (MUJINPLC_SOVERSION ${MUJINPLC_VERSION_MAJOR}.${MUJINPLC_VERSION_MINOR})
message(STATUS "Compiling MUJIN PLC Library Version ${MUJINPLC_VERSION}, soversion=${MUJINPLC_SOVERSION}")

set(MUJINPLC_CMAKE_INSTALL_DIR "mujinplc-${MUJINPLC_VERSION_MAJOR}.${MUJINPLC_VERSION_MINOR}" CACHE STRING "Directory to install the cmake config files.")
set(MUJINPLC_TARGET_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR} CACHE STRING "The target processor architecture to build for, this is combined with the generator toolchain")

message(STATUS "Using cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" )
# http://www.cmake.org/cmake/help/cmake-2.6.html#policy:CMP0002
cmake_policy(SET CMP0002 NEW)
# http://www.cmake.org/cmake/help/cmake-2.6.html#policy:CMP0003
cmake_policy(SET CMP0003 NEW)

# Use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH FALSE)

# When building, don't use the install RPATH already
# (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

set(LIB_SUFFIX CACHE STRING "suffix for the library directory need for x86-64 systems that use lib64")

# The RPATH to be used when installing
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")

# Add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fPIC -Werror=return-type -fvisibility=hidden")
set(CMAKE_CXX_FLAGS_OPTIMIZED "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g3 -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -D_DEBUG")

include(CheckIncludeFile)
include(CheckIncludeFileCXX)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_BINARY_DIR}/include)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/mujinplc/config.h IMMEDIATE @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/mujinplc/config.h DESTINATION include/mujinplc)

# dependencies
find_package(RapidJSON REQUIRED)

find_package(PkgConfig) # pkg_check_modules
pkg_check_modules(libzmq REQUIRED libzmq)

include_directories(${libzmq_INCLUDE_DIRS})
link_directories(${libzmq_LIBRARY_DIRS})

# source
add_subdirectory(src)

# install headers
install(DIRECTORY include/ DESTINATION include FILES_MATCHING PATTERN "*.h")

# install cmake config files
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/mujinplc-config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/mujinplc-config.cmake" IMMEDIATE @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/mujinplc-config-version.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/mujinplc-config-version.cmake" IMMEDIATE @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mujinplc-config.cmake" "${CMAKE_CURRENT_BINARY_DIR}/mujinplc-config-version.cmake" DESTINATION "lib${LIB_SUFFIX}/cmake/${MUJINPLC_CMAKE_INSTALL_DIR}")


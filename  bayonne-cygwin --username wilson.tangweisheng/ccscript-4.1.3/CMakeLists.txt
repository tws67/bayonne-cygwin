# Copyright (C) 2009 David Sugar, Tycho Softworks
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#

cmake_minimum_required(VERSION 2.6)
PROJECT(ccscript)
set (VERSION 4.1.2)
set (SOVERSION 4)

MESSAGE( STATUS "Configuring GNU ccscript ${VERSION}...")

# set to true for debug and trace during CMakeLists development
set(CMAKE_VERBOSE_MAKEFILE FALSE)
 
# add module path of project if it exists...
if (EXISTS "${CMAKE_SOURCE_DIR}/inc/")
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/inc/")
endif()

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# Set defaults and pass common options.  Common build options can be passed 
# to cmake using cmake -DWITH_CFLAGS="...", WITH_LIBS, and WITH_INCLUDES
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/inc ${WITH_INCLUDES})
add_definitions(${WITH_CFLAGS})
link_libraries(${WITH_LIBS})

if (USES_UCOMMON_INCLUDE_DIRS)
	message(STATUS "  Using local ucommon dependency")
else()
	find_package(PkgConfig)
	pkg_check_modules(USES_UCOMMON REQUIRED ucommon>=2.0.7)
endif()

include_directories(${USES_UCOMMON_INCLUDE_DIRS})
link_directories(${USES_UCOMMON_LIBRARY_DIRS})
add_definitions(${USES_UCOMMON_CFLAGS})

# by default we build static libs for windows, shared libs for unix.  
# we may also set this from a top level cmake or -DWITH_XX_LIBS
if(UNIX)
	set(BUILD_LIBRARY_TYPE SHARED)
else()
	set(BUILD_LIBRARY_TYPE STATIC)
endif()

if(WITH_SHARED_LIBS)
	set(BUILD_LIBRARY_TYPE SHARED)
endif()

if(WITH_STATIC_LIBS)
	set(BUILD_LIBRARY_TYPE STATIC)
endif()

file(GLOB ccscript_src src/*.cpp)
add_library(ccscript ${BUILD_LIBRARY_TYPE} ${ccscript_src})
set_target_properties(ccscript PROPERTIES VERSION ${VERSION} SOVERSION ${SOVERSION})
add_dependencies(ccscript ucommon)
target_link_libraries(ccscript ${USES_UCOMMON_LIBRARIES})
install(TARGETS ccscript DESTINATION lib)

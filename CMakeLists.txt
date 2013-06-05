#
# CMakeLists.txt -- CMake build system for irccd
#
# Copyright (c) 2011, 2012, 2013 David Demelier <markand@malikania.fr>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

# General
cmake_minimum_required(VERSION 2.8)

# Project
project(irccd-all)

set(MAJOR "0")
set(MINOR "1")

# General settings
if (UNIX)
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++11 -stdlib=libc++")	# c++11
endif ()

if (WIN32)
	add_definitions("-D_CRT_SECURE_NO_WARNINGS")
	set(CMAKE_CXX_FLAGS_DEBUG_INIT "/MDd /EHsc")
endif ()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")		# own modules

# Installation paths
set(PREFIX "${CMAKE_INSTALL_PREFIX}")
set(MODDIR "${PREFIX}/share/irccd/plugins"
    CACHE STRING "Module prefix where to install")
set(DOCDIR "${PREFIX}/share/doc/irccd"
    CACHE STRING "Documentation directory")
set(MANDIR "${PREFIX}/share/man"
    CACHE STRING "Man directory")
set(ETCDIR "${PREFIX}/etc"
    CACHE STRING "Configuration directory")

# Include path to common
include_directories(
	"${CMAKE_BINARY_DIR}"
	"${CMAKE_SOURCE_DIR}/common"
)

# Some non portable code
set(EXTERNDIR "${CMAKE_SOURCE_DIR}/extern")

include(CheckFunctionExists)
include(CheckIncludeFile)

# Check of getopt(3) function.
check_function_exists(getopt HAVE_GETOPT)
if (NOT HAVE_GETOPT)
	include_directories("${EXTERNDIR}/getopt")
	list(
		APPEND
		EXTSOURCES
		"${EXTERNDIR}/getopt/getopt.c"
		"${EXTERNDIR}/getopt/getopt.h"
	)
endif ()

# Check of setprogname(3) function.
check_function_exists(setprogname HAVE_SETPROGNAME)
if (NOT HAVE_SETPROGNAME)
	include_directories("${EXTERNDIR}/setprogname")
	list(
		APPEND
		EXTSOURCES
		"${EXTERNDIR}/setprogname/setprogname.c"
		"${EXTERNDIR}/setprogname/setprogname.h"
	)
endif ()

# unistd.h has some useful routines.
check_include_file(unistd.h HAVE_UNISTD_H)

# For documentation and man pages
add_subdirectory(doc)

# Common code (static library)
add_subdirectory(common)

# irccd main application
add_subdirectory(irccd)

# irccd controller
add_subdirectory(irccdctl)

# Unit tests
add_subdirectory(tests)

message("Installation directories:")
message("\tPREFIX\t\t${PREFIX}")
message("\tMANDIR\t\t${MANDIR}")
message("\tDOCDIR\t\t${DOCDIR}")
message("\tMODDIR\t\t${MODDIR}")
message("")

message("Compiling irccd with following options:")
message("\tLua support:\t${LUA_SUPPORT}")
message("\tUnit tests:\t${UNIT_TESTS}")
message("\tDefault config:\t${DEFAULT_IRCCD_CONFIG}")
message("")

# Configure the file after so every project can set more variables
# if needed
configure_file(
	"${CMAKE_SOURCE_DIR}/cmake/config.h.in"
	"${CMAKE_BINARY_DIR}/config.h"
)

# CPack
if (WIN32)
else ()
	set(CPACK_GENERATOR "TGZ")
endif ()

set(CPACK_PACKAGE_VENDOR "Malikania")
set(CPACK_PACKAGE_VERSION "${MAJOR}.${MINOR}")
set(CPACK_PACKAGE_VERSION_MAJOR ${MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${MINOR})
set(CPACK_PACKAGE_FILE_NAME "irccd-${MAJOR}.${MINOR}-${CMAKE_SYSTEM_NAME}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "IRC Client Daemon")

set(CPACK_SOURCE_PACKAGE_FILE_NAME "irccd-${MAJOR}.${MINOR}-source")
set(CPACK_SOURCE_GENERATOR "ZIP;TGZ")
set(CPACK_SOURCE_IGNORE_FILES ".hg")

include(CPack)

# vim: set syntax=cmake:

#
# CMakeLists.txt -- CMake build system for irccd
#
# Copyright (c) 2013, 2014 David Demelier <markand@malikania.fr>
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
project(irccd)

# ---------------------------------------------------------
# Irccd project version
# ---------------------------------------------------------

set(MAJOR "1")
set(MINOR "2")
set(PATCH "0")
set(VERSION "${MAJOR}.${MINOR}-dev")

# ---------------------------------------------------------
# Package name
# ---------------------------------------------------------

#
# The following settings are used for the doc/ directory and CPack.
#
if (WIN32)
	if (CMAKE_CL_64)
		set(IRCCD_PACKAGE_NAME "Irccd (x64)")
	else ()
		set(IRCCD_PACKAGE_NAME "Irccd")
	endif ()
endif ()

add_subdirectory(cmake)

# Compatibility options
option(COMPAT_1_1 "Be compatible with 1.1" On)

# Some non portable code
set(EXTERNDIR "${CMAKE_SOURCE_DIR}/extern")

# Common code (static library)
add_subdirectory(common)

# For documentation and man pages
add_subdirectory(doc)

# irccd main application
add_subdirectory(irccd)

# irccd controller
add_subdirectory(irccdctl)

# Contributions
add_subdirectory(contrib)

# Tests
include(CTest)
add_subdirectory(tests)

message("Installation directories:")
message("    MANDIR            ${CMAKE_INSTALL_PREFIX}/${MANDIR}")
message("    DOCDIR            ${CMAKE_INSTALL_PREFIX}/${DOCDIR}")
message("    MODDIR            ${CMAKE_INSTALL_PREFIX}/${MODDIR}")
message("")

message("Compiling with the following flags:")
message("    General flags:    ${CMAKE_CXX_FLAGS}")
message("    Debug flags:      ${CMAKE_CXX_FLAGS_DEBUG}")
message("    Release flags:    ${CMAKE_CXX_FLAGS_RELEASE}")
message("")

message("Compiling irccd with following options:")
message("    Build type:       ${CMAKE_BUILD_TYPE}")
message("    Lua support:      ${HAVE_LUA_MESSAGE}")
message("    Tests:            ${HAVE_TESTS_MESSAGE}")
message("    Iconv:            ${HAVE_LIBICONV_MESSAGE}")
message("    Doxygen:          ${HAVE_DOXYGEN_MESSAGE}")
message("    irccd config:     ${CMAKE_INSTALL_PREFIX}/${ETCDIR}/irccd.conf")
message("    irccdctl config:  ${CMAKE_INSTALL_PREFIX}/${ETCDIR}/irccdctl.conf")
message("")

# CPack
include(cmake/CPackConfig.cmake)
include(CPack)

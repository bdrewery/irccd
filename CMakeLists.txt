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
project(irccd)

# General settings
set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++11 -stdlib=libc++")	# c++11
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")		# own modules

# Installation paths
set(PREFIX "${CMAKE_INSTALL_PREFIX}")
set(MODDIR "${PREFIX}/share/irccd/modules"
    CACHE STRING "Module prefix where to install")
set(DOCDIR "${PREFIX}/share/doc/irccd"
    CACHE STRING "Documentation directory")
set(MANDIR "${PREFIX}/share/man"
    CACHE STRING "Man directory")

configure_file(
	"${CMAKE_SOURCE_DIR}/cmake/config.h.in"
	"${CMAKE_BINARY_DIR}/config.h"
)

# Include path to common
include_directories(
	"${CMAKE_BINARY_DIR}"
	"${CMAKE_SOURCE_DIR}/common"
)

message("Configuration:")
message("\tPREFIX\t${PREFIX}")
message("\tMANDIR\t${MANDIR}")
message("\tDOCDIR\t${DOCDIR}")
message("\tMODDIR\t${MODDIR}")

# For documentation and man pages
add_subdirectory(doc)

# Common code (static library)
add_subdirectory(common)

# irccd main application
add_subdirectory(irccd)

# irccd controller
add_subdirectory(irccdctl)

# vim: set syntax=cmake:

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

project(common)

# Use bundled xdg-basedir or not
option(WITH_BUNDLE_XDGBASEDIR "Build with bundled libxdg-basedir" On)

set(SOURCES
	Date.cpp
	Date.h
	Directory.cpp
	Directory.h
	Logger.cpp
	Logger.h
	Parser.cpp
	Parser.h
	Socket.cpp
	Socket.h
	SocketAddress.cpp
	SocketAddress.h
	SocketListener.cpp
	SocketListener.h
	System.cpp
	System.h
	Util.cpp
	Util.h
	${EXTSOURCES}
)

if (UNIX)
	if (WITH_BUNDLE_XDGBASEDIR)
		add_subdirectory(
			"${CMAKE_SOURCE_DIR}/extern/libxdg-basedir"
			"${CMAKE_BINARY_DIR}/libxdg-basedir"
		)

		list(
			APPEND
			INCLUDES
			"${CMAKE_SOURCE_DIR}/extern/libxdg-basedir/include"
		)

		list(
			APPEND
			LIBRARIES
			xdg-basedir
		)
	else ()
		find_package(XDG REQUIRED)

		list(APPEND INCLUDES ${XDG_INCLUDE_DIR})
		list(APPEND LIBRARIES ${XDG_LIBRARY})
	endif ()
endif ()

if (WIN32)
	list(APPEND LIBRARIES "ws2_32")
	list(APPEND LIBRARIES "gdi32")
endif ()

include_directories(
	${INCLUDES}
)

add_library(common STATIC ${SOURCES})

target_link_libraries(
	common
	${LIBRARIES}
)

# vim: set syntax=cmake:

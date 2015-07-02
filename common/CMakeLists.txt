#
# CMakeLists.txt -- CMake build system for irccd
#
# Copyright (c) 2013, 2014, 2015 David Demelier <markand@malikania.fr>
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

set(
	SOURCES
	Date.cpp
	Date.h
	Directory.cpp
	Directory.h
	Dynlib.cpp
	Dynlib.h
	ElapsedTimer.cpp
	ElapsedTimer.h
	Filesystem.cpp
	Filesystem.h
	Ini.cpp
	Ini.h
	Json.cpp
	Json.h
	Logger.cpp
	Logger.h
	OptionParser.cpp
	OptionParser.h
	Signals.h
	Socket.cpp
	Socket.h
	SocketAddress.cpp
	SocketAddress.h
	SocketListener.cpp
	SocketListener.h
	System.h
	System.cpp
	Util.cpp
	Util.h
)

if (UNIX)
	list(APPEND SOURCES Xdg.cpp Xdg.h)

	if (IRCCD_SYSTEM_LINUX)
		list(APPEND LIBRARIES dl)
	endif ()
elseif (WIN32)
	list(APPEND LIBRARIES ws2_32)
	list(APPEND LIBRARIES gdi32)
	list(APPEND LIBRARIES shlwapi)
endif ()

find_package(Jansson REQUIRED)

irccd_define_library(
	TARGET common
	SOURCES ${SOURCES}
	LIBRARIES
		${LIBRARIES}
		${Jansson_LIBRARIES}
	LOCAL_INCLUDES ${INCLUDES}
	PUBLIC_INCLUDES
		${common_SOURCE_DIR}
		${Jansson_INCLUDE_DIRS}
)

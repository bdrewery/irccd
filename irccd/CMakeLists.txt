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

# Project
project(irccd)

add_subdirectory(modules)

set(SOURCES
	Server.cpp
	Plugin.cpp
	LuaLogger.cpp
	LuaParser.cpp
	LuaPlugin.cpp
	LuaServer.cpp
	LuaUtil.cpp
	Irccd.cpp
	main.cpp
	Test.cpp
)

find_package(Libircclient REQUIRED)
find_package(XDG REQUIRED)
find_package(Lua52 REQUIRED)

include_directories(
	${LIBIRCCLIENT_INCLUDE_DIR}
	${XDG_INCLUDE_DIR}
	${LUA_INCLUDE_DIR}
)

add_executable(irccd ${SOURCES})

target_link_libraries(
	irccd
	common
	pthread
	${LIBIRCCLIENT_LIBRARIES}
	${XDG_LIBRARY}
	${LUA_LIBRARIES}
)

install(TARGETS irccd RUNTIME DESTINATION bin)

# vim: set syntax=cmake:

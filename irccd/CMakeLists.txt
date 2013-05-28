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

option(WITH_LUA "Build with Lua plugin support" On)

if (WITH_LUA)
	add_subdirectory(modules)
endif ()

set(SOURCES
	Server.cpp
	Plugin.cpp
	Irccd.cpp
	main.cpp
	Test.cpp
)

set(LUA_SUPPORT "disabled")

if (WITH_LUA)
	find_package(Lua52 QUIET)

	if (LUA52_FOUND)
		# Optional Lua sources
		list(
			APPEND
			SOURCES
			Lua/LuaLogger.cpp
			Lua/LuaParser.cpp
			Lua/LuaPlugin.cpp
			Lua/LuaServer.cpp
			Lua/LuaUtil.cpp
		)

		# Optional Lua includes
		list(
			APPEND
			INCLUDES
			${LUA_INCLUDE_DIR}
			${CMAKE_CURRENT_SOURCE_DIR}
		)

		# Optional Lua libraries
		list(
			APPEND
			LIBRARIES
			${LUA_LIBRARIES}
		)

		set(LUA_SUPPORT "enabled")
		set(USE_LUA 1 PARENT_SCOPE)		# for config.h
	endif ()

endif ()

find_package(Libircclient REQUIRED)
find_package(XDG REQUIRED)

include_directories(
	${LIBIRCCLIENT_INCLUDE_DIR}
	${XDG_INCLUDE_DIR}
	${INCLUDES}
)

add_executable(irccd ${SOURCES})

target_link_libraries(
	irccd
	common
	pthread
	${LIBIRCCLIENT_LIBRARIES}
	${XDG_LIBRARY}
	${LIBRARIES}
)

message("Compiling irccd with following options:")
message("\tLua support:\t${LUA_SUPPORT}")

install(TARGETS irccd RUNTIME DESTINATION bin)

# vim: set syntax=cmake:

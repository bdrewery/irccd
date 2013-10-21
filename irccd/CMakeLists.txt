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

# Optional Lua support
option(WITH_LUA "Build with Lua plugin support" On)

set(SOURCES
	Server.cpp
	Server.h
	Irccd.cpp
	Irccd.h
	main.cpp
)

set(LUA_SUPPORT "disabled" PARENT_SCOPE)

if (WITH_LUA)
	find_package(Lua52)

	if (LUA52_FOUND)
		add_subdirectory(plugins)

		# Optional Lua sources
		list(
			APPEND
			SOURCES
			Lua/LuaIrccd.cpp
			Lua/LuaIrccd.h
			Lua/LuaLogger.cpp
			Lua/LuaLogger.h
			Lua/LuaParser.cpp
			Lua/LuaParser.h
			Lua/LuaPlugin.cpp
			Lua/LuaPlugin.h
			Lua/LuaServer.cpp
			Lua/LuaServer.h
			Lua/LuaUtil.cpp
			Lua/LuaUtil.h
			DefCall.cpp
			DefCall.h
			Luae.cpp
			Luae.h
			Plugin.cpp
			Plugin.h
			Test.cpp
			Test.h
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

		set(LUA_SUPPORT "enabled" PARENT_SCOPE)
		set(USE_LUA 1 PARENT_SCOPE)		# for config.h
	endif ()

endif ()

find_package(Libircclient REQUIRED)

include_directories(
	${LIBIRCCLIENT_INCLUDE_DIR}
	${INCLUDES}
)

add_executable(irccd ${SOURCES})

if (UNIX)
	list(APPEND LIBRARIES "pthread")
endif ()

target_link_libraries(
	irccd
	common
	${LIBIRCCLIENT_LIBRARIES}
	${LIBRARIES}
)

install(TARGETS irccd RUNTIME DESTINATION bin)

install(
	FILES "${CMAKE_SOURCE_DIR}/doc/examples/irccd.conf"
	DESTINATION "${ETCDIR}"
	RENAME "irccd.conf.sample"
)

# vim: set syntax=cmake:

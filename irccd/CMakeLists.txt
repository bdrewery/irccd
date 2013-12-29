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
option(WITH_LUA52 "Build with Lua plugin support" On)
option(WITH_LUAJIT "Build with LuaJIT plugin support" Off)

if (WITH_LUA52 AND WITH_LUAJIT)
	message(FATAL_ERROR "Please select WITH_LUA52 or WITH_LUAJIT")
endif ()

# Use bundled libircclient or not
option(WITH_BUNDLE_IRCCLIENT "Build with bundled libircclient" On)

set(SOURCES
	server/ServerConnecting.cpp
	server/ServerConnecting.h
	server/ServerDead.cpp
	server/ServerDead.h
	server/ServerDisconnected.cpp
	server/ServerDisconnected.h
	server/ServerRunning.cpp
	server/ServerRunning.h
	server/ServerState.cpp
	server/ServerState.h
	server/ServerUninitialized.cpp
	server/ServerUninitialized.h
	Server.cpp
	Server.h
	Irccd.cpp
	Irccd.h
	Message.cpp
	Message.h
	Listener.cpp
	Listener.h
	main.cpp
)

set(LUA_SUPPORT "disabled" PARENT_SCOPE)

if (WITH_LUA52 OR WITH_LUAJIT)
	if (WITH_LUA52)
		find_package(Lua52)
	elseif (WITH_LUAJIT)
		find_package(LuaJIT)
	endif ()

	if (LUA52_FOUND OR LUAJIT_FOUND)
		add_subdirectory(plugins)

		# Optional Lua sources
		list(
			APPEND
			SOURCES
			Lua/LuaIrccd.cpp
			Lua/LuaIrccd.h
			Lua/LuaLogger.cpp
			Lua/LuaLogger.h
			Lua/LuaFS.cpp
			Lua/LuaFS.h
			Lua/LuaParser.cpp
			Lua/LuaParser.h
			Lua/LuaPipe.cpp
			Lua/LuaPipe.h
			Lua/LuaPlugin.cpp
			Lua/LuaPlugin.h
			Lua/LuaServer.cpp
			Lua/LuaServer.h
			Lua/LuaSocket.cpp
			Lua/LuaSocket.h
			Lua/LuaSystem.cpp
			Lua/LuaSystem.h
			Lua/LuaThread.cpp
			Lua/LuaThread.h
			Lua/LuaUtil.cpp
			Lua/LuaUtil.h
			Luae.cpp
			Luae.h
			Pipe.cpp
			Pipe.h
			Plugin.cpp
			Plugin.h
			Process.cpp
			Process.h
			Test.cpp
			Test.h
			Thread.cpp
			Thread.h
		)

		if (WITH_LUA52)
			list(APPEND INCLUDES ${LUA52_INCLUDE_DIR})
			list(APPEND LIBRARIES ${LUA52_LIBRARIES})
			set(LUA_SUPPORT "enabled (Lua 5.2)" PARENT_SCOPE)
		elseif (WITH_LUAJIT)
			list(APPEND INCLUDES ${LUAJIT_INCLUDE_DIR})
			list(APPEND LIBRARIES ${LUAJIT_LIBRARIES})
			set(LUA_SUPPORT "enabled (LuaJIT)" PARENT_SCOPE)
		endif ()
	endif ()
endif ()

if (WITH_BUNDLE_IRCCLIENT)
	#
	# Compile libircclient as internal
	#
	add_subdirectory(
		"${CMAKE_SOURCE_DIR}/extern/libircclient"
		"${CMAKE_BINARY_DIR}/libircclient"
	)

	list(
		APPEND
		INCLUDES
		"${CMAKE_SOURCE_DIR}/extern/libircclient/include"
	)

	list(
		APPEND
		LIBRARIES
		ircclient
	)
else ()
	find_package(Libircclient REQUIRED)

	list(APPEND INCLUDES ${LIBIRCCLIENT_INCLUDE_DIR})
	list(APPEND LIBRARIES ${LIBIRCCLIENT_LIBRARIES})
endif ()

include_directories(
	${INCLUDES}
	${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(irccd ${SOURCES})

if (UNIX)
	list(APPEND LIBRARIES "pthread")
endif ()

target_link_libraries(
	irccd
	common
	${LIBRARIES}
)

install(TARGETS irccd RUNTIME DESTINATION bin)

install(
	FILES "${CMAKE_SOURCE_DIR}/doc/examples/irccd.conf"
	DESTINATION "${ETCDIR}"
	RENAME "irccd.conf.sample"
)

# vim: set syntax=cmake:

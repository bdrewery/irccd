#
# CMakeLists.txt -- CMake build system for irccd
#
# Copyright (c) 2013 David Demelier <markand@malikania.fr>
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
	IrcSession.cpp
	IrcSession.h
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
			event/IrcEventChannelMode.cpp
			event/IrcEventChannelMode.h
			event/IrcEventChannelNotice.cpp
			event/IrcEventChannelNotice.h
			event/IrcEventConnect.cpp
			event/IrcEventConnect.h
			event/IrcEventInvite.cpp
			event/IrcEventInvite.h
			event/IrcEventJoin.cpp
			event/IrcEventJoin.h
			event/IrcEventKick.cpp
			event/IrcEventKick.h
			event/IrcEventLoad.cpp
			event/IrcEventLoad.h
			event/IrcEventMe.cpp
			event/IrcEventMe.h
			event/IrcEventMessage.cpp
			event/IrcEventMessage.h
			event/IrcEventMode.cpp
			event/IrcEventMode.h
			event/IrcEventNames.cpp
			event/IrcEventNames.h
			event/IrcEventNick.cpp
			event/IrcEventNick.h
			event/IrcEventNotice.cpp
			event/IrcEventNotice.h
			event/IrcEventPart.cpp
			event/IrcEventPart.h
			event/IrcEventQuery.cpp
			event/IrcEventQuery.h
			event/IrcEventReload.cpp
			event/IrcEventReload.h
			event/IrcEventTopic.cpp
			event/IrcEventTopic.h
			event/IrcEventUnload.cpp
			event/IrcEventUnload.h
			event/IrcEventWhois.cpp
			event/IrcEventWhois.h
			IrcEvent.cpp
			IrcEvent.h
			lua/LuaIrccd.cpp
			lua/LuaIrccd.h
			lua/LuaLogger.cpp
			lua/LuaLogger.h
			lua/LuaFS.cpp
			lua/LuaFS.h
			lua/LuaParser.cpp
			lua/LuaParser.h
			lua/LuaPipe.cpp
			lua/LuaPipe.h
			lua/LuaPlugin.cpp
			lua/LuaPlugin.h
			lua/LuaServer.cpp
			lua/LuaServer.h
			lua/LuaSocket.cpp
			lua/LuaSocket.h
			lua/LuaSystem.cpp
			lua/LuaSystem.h
			lua/LuaThread.cpp
			lua/LuaThread.h
			lua/LuaUtil.cpp
			lua/LuaUtil.h
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
	else ()
		set(WITH_LUA52 OFF CACHE INTERNAL "" FORCE)
		set(WITH_LUAJIT OFF CACHE INTERNAL "" FORCE)
	endif ()
endif ()

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

list(APPEND LIBRARIES ircclient)

include_directories(
	${INCLUDES}
	${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(irccd ${SOURCES})

#
# When Lua is disabled, a lot of variables are unused, disable warnings.
#
if (NOT WITH_LUA52 OR NOT WITH_LUAJIT)
	if (UNIX)
		set(FLAGS "-Wno-unused-parameter -Wno-unused-variable")
	endif ()
endif ()

if (UNIX)
	list(APPEND LIBRARIES "pthread")
else ()
	set(FLAGS "${FLAGS} /wd4996 /wd4244")
endif ()

if (APPLE)
	list(APPEND LIBRARIES resolv)
endif ()

set_target_properties(irccd PROPERTIES COMPILE_FLAGS "${FLAGS}")
target_link_libraries(irccd common ${LIBRARIES})

install(TARGETS irccd RUNTIME DESTINATION bin)

install(
	FILES "${CMAKE_SOURCE_DIR}/doc/examples/irccd.conf"
	DESTINATION "${ETCDIR}"
	RENAME "irccd.conf.sample"
)

if (WIN32)
    if (CMAKE_SIZEOF_VOID_P MATCHES "8")
        set(WINARCH x64)
    else ()
        set(WINARCH x32)
    endif ()

    install(
        FILES
            ${CMAKE_SOURCE_DIR}/win32/${WINARCH}/lua.dll
            ${CMAKE_SOURCE_DIR}/win32/${WINARCH}/libeay32.dll
            ${CMAKE_SOURCE_DIR}/win32/${WINARCH}/ssleay32.dll
        DESTINATION bin
    )
endif ()

# vim: set syntax=cmake:

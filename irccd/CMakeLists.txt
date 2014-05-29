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

# Project
project(irccd)

set(SOURCES
	CommandQueue.cpp
	CommandQueue.h
	IO.cpp
	IO.h
	event/Event.cpp
	event/Event.h
	event/EventConnect.cpp
	event/EventConnect.h
	event/EventMe.cpp
	event/EventMe.h
	event/EventMessage.cpp
	event/EventMessage.h
	event/EventChannelNotice.cpp
	event/EventChannelNotice.h
	event/EventInvite.cpp
	event/EventInvite.h
	event/EventJoin.cpp
	event/EventJoin.h
	event/EventKick.cpp
	event/EventKick.h
	event/EventMode.cpp
	event/EventMode.h
	event/EventNames.cpp
	event/EventNames.h
	event/EventNick.cpp
	event/EventNick.h
	event/EventNotice.cpp
	event/EventNotice.h
	event/EventPart.cpp
	event/EventPart.h
	event/EventQuery.cpp
	event/EventQuery.h
	event/EventTopic.cpp
	event/EventTopic.h
	event/EventUserMode.cpp
	event/EventUserMode.h
	event/EventWhois.cpp
	event/EventWhois.h
	command/Command.cpp
	command/Command.h
	command/CommandChannelNotice.cpp
	command/CommandChannelNotice.h
	command/CommandInvite.cpp
	command/CommandInvite.h
	command/CommandJoin.cpp
	command/CommandJoin.h
	command/CommandKick.cpp
	command/CommandKick.h
	command/CommandMe.cpp
	command/CommandMe.h
	command/CommandMessage.cpp
	command/CommandMessage.h
	command/CommandMode.cpp
	command/CommandMode.h
	command/CommandNames.cpp
	command/CommandNames.h
	command/CommandNick.cpp
	command/CommandNick.h
	command/CommandNotice.cpp
	command/CommandNotice.h
	command/CommandPart.cpp
	command/CommandPart.h
	command/CommandSend.cpp
	command/CommandSend.h
	command/CommandTopic.cpp
	command/CommandTopic.h
	command/CommandUserMode.cpp
	command/CommandUserMode.h
	command/CommandWhois.cpp
	command/CommandWhois.h
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
	System.cpp
	System.h
)

set(
	LIBRARIES
	ircclient
	common
)

set(
	INCLUDES
	${irccd_SOURCE_DIR}
)

set(LUA_SUPPORT "disabled" PARENT_SCOPE)

#
# Optional libiconv for rule reencoding
#
set(USE_ICONV "disabled" PARENT_SCOPE)

if(WITH_LIBICONV)
	find_package(Libiconv)

	if(Libiconv_FOUND)
		set(USE_ICONV "enabled" PARENT_SCOPE)

		list(
			APPEND
			SOURCES
			Converter.cpp
			Converter.h
		)
	else()
		set(WITH_LIBICONV Off)
	endif()
endif()

#
# Optional Lua
#
if(WITH_LUA52 OR WITH_LUAJIT)
	if(WITH_LUA52)
		find_package(Lua52)
	elseif(WITH_LUAJIT)
		find_package(LuaJIT)
	endif()

	if (LUA52_FOUND OR LUAJIT_FOUND)
		add_subdirectory(plugins)

		# Optional Lua sources
		list(
			APPEND
			SOURCES
			EventQueue.cpp
			EventQueue.h
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
			lua/LuaRule.cpp
			lua/LuaRule.h
			lua/LuaServer.cpp
			lua/LuaServer.h
			lua/LuaSocket.cpp
			lua/LuaSocket.h
			lua/LuaSystem.cpp
			lua/LuaSystem.h
			lua/LuaThread.cpp
			lua/LuaThread.h
			lua/LuaUtf8.cpp
			lua/LuaUtf8.h
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
			Rule.cpp
			Rule.h
			RuleManager.cpp
			RuleManager.h
			Test.cpp
			Test.h
			Thread.cpp
			Thread.h
			Utf8.cpp
			Utf8.h
		)

		list(
			APPEND
			SOURCES
			plugins/antiflood.lua
			plugins/ask.lua
			plugins/auth.lua
			plugins/badwords.lua
			plugins/date.lua
			plugins/history.lua
			plugins/logger.lua
			plugins/roulette.lua
		)

		if(WITH_LUA52)
			list(APPEND INCLUDES ${LUA52_INCLUDE_DIR})
			list(APPEND LIBRARIES ${LUA52_LIBRARIES})
			set(LUA_SUPPORT "enabled (Lua 5.2)" PARENT_SCOPE)
		elseif(WITH_LUAJIT)
			list(APPEND INCLUDES ${LUAJIT_INCLUDE_DIR})
			list(APPEND LIBRARIES ${LUAJIT_LIBRARIES})
			set(LUA_SUPPORT "enabled (LuaJIT)" PARENT_SCOPE)
		endif()
	endif()
endif()

if(UNIX)
	list(APPEND LIBRARIES "pthread")
endif()

define_executable(
	TARGET irccd
	SOURCES ${SOURCES}
	FLAGS ${FLAGS}
	INCLUDES ${INCLUDES}
	LIBRARIES ${LIBRARIES}
	INSTALL_RUNTIME bin/
)

install(
	FILES "${CMAKE_SOURCE_DIR}/doc/examples/irccd.conf"
	DESTINATION "${ETCDIR}"
	RENAME "irccd.conf.sample"
)

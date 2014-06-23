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

add_subdirectory(command)
add_subdirectory(event)
add_subdirectory(lua)
add_subdirectory(server)

source_group(command FILES ${COMMAND_SOURCES})
source_group(event FILES ${EVENT_SOURCES})
source_group(lua FILES ${LUA_SOURCES})
source_group(server FILES ${SERVER_SOURCES})

set(
	SOURCES
	${SERVER_SOURCES}
	${COMMAND_SOURCES}
	CommandQueue.cpp
	CommandQueue.h
	IO.cpp
	IO.h
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
			${LUA_SOURCES}
			${EVENT_SOURCES}
			EventQueue.cpp
			EventQueue.h
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

list(APPEND INCLUDES ${CMAKE_SOURCE_DIR})

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

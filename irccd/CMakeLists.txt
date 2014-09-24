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
	ServerManager.cpp
	ServerManager.h
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
	${CMAKE_SOURCE_DIR}
)

# ---------------------------------------------------------
# Optional libiconv
# ---------------------------------------------------------

if (WITH_LIBICONV)
	set(HAVE_LIBICONV_MESSAGE "disabled (iconv not found)" CACHE INTERNAL "")
	set(HAVE_LIBICONV FALSE CACHE INTERNAL "")

	find_package(Libiconv)

	if (Libiconv_FOUND)
		set(HAVE_LIBICONV_MESSAGE "enabled" CACHE INTERNAL "")
		set(HAVE_LIBICONV TRUE CACHE INTERNAL "")

		list(
			APPEND
			SOURCES
			Converter.cpp
			Converter.h
		)
	endif ()
else ()
	set(HAVE_LIBICONV_MESSAGE "disabled by user" CACHE INTERNAL "")
	set(HAVE_LIBICONV FALSE CACHE INTERNAL "")
endif ()

# ---------------------------------------------------------
# Optional Lua
# ---------------------------------------------------------

if (WITH_LUA)
	set(HAVE_LUA_MESSAGE "disabled (Lua not found)" CACHE INTERNAL "")
	set(HAVE_LUA FALSE CACHE INTERNAL "")

	if (WITH_LUAVER MATCHES "5.2")
		find_package(Lua52)
		list(APPEND INCLUDES ${LUA52_INCLUDE_DIR})
		list(APPEND LIBRARIES ${LUA52_LIBRARIES})

		if (LUA52_FOUND)
			set(HAVE_LUA_MESSAGE "enabled (Lua 5.2)" CACHE INTERNAL "")
			set(HAVE_LUA TRUE CACHE INTERNAL "")
		endif ()
	elseif (WITH_LUAVER MATCHES "5.1")
		find_package(Lua51)
		list(APPEND INCLUDES ${LUA_INCLUDE_DIR})
		list(APPEND LIBRARIES ${LUA_LIBRARIES})

		if (LUA51_FOUND)
			set(HAVE_LUA_MESSAGE "enabled (Lua 5.1)" CACHE INTERNAL "")
			set(HAVE_LUA TRUE CACHE INTERNAL "")
		endif ()
	elseif (WITH_LUAVER MATCHES "JIT")
		find_package(LuaJIT)
		list(APPEND INCLUDES ${LUAJIT_INCLUDE_DIR})
		list(APPEND LIBRARIES ${LUAJIT_LIBRARIES})

		if (LuaJIT_FOUND)
			set(HAVE_LUA_MESSAGE "enabled (LuaJIT)" CACHE INTERNAL "")
			set(HAVE_LUA TRUE CACHE INTERNAL "")
		endif ()
	else ()
		message(FATAL_ERROR "Unknown Lua version selected ${WITH_LUAVER}")
	endif ()

	if (HAVE_LUA)
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
			PipeManager.cpp
			PipeManager.h
			Plugin.cpp
			Plugin.h
			PluginManager.cpp
			PluginManager.h
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
	endif ()
else ()
	set(HAVE_LUA_MESSAGE "disabled by user" CACHE INTERNAL "")
	set(HAVE_LUA FALSE CACHE INTERNAL "")
endif ()

define_executable(
	TARGET irccd
	SOURCES ${SOURCES}
	FLAGS ${FLAGS}
	INCLUDES ${INCLUDES}
	LIBRARIES ${LIBRARIES}
	INSTALL_RUNTIME bin/
)

if (UNIX)
	set_target_properties(irccd PROPERTIES LINK_FLAGS "-pthread")
endif ()

install(
	FILES "${CMAKE_SOURCE_DIR}/doc/examples/irccd.conf"
	DESTINATION "${ETCDIR}"
	RENAME "irccd.conf.sample"
)

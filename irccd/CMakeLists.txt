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

project(irccd)

include(command/Command.cmake)
include(server/Server.cmake)

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
	include(event/Event.cmake)
	include(lua/Lua.cmake)

	add_subdirectory(plugins)

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

	list(APPEND LIBRARIES lua)

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
else ()
	set(HAVE_LUA_MESSAGE "disabled by user" CACHE INTERNAL "")
	set(HAVE_LUA FALSE CACHE INTERNAL "")
endif ()

define_executable(
	TARGET irccd
	SOURCES ${SOURCES}
	INCLUDES ${INCLUDES} ${CMAKE_SOURCE_DIR}
	LIBRARIES ${LIBRARIES} ircclient common
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

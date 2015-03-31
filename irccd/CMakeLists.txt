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

project(irccd)

#include(event/Event.cmake)
include(command/Command.cmake)
include(server/Server.cmake)
include(transportcommand/TransportCommand.cmake)
include(js/Js.cmake)

set(
	SOURCES
	Irccd.cpp
	Irccd.h
	main.cpp
	Plugin.cpp
	Plugin.h
	Server.cpp
	Server.h
	ServerCommand.h
	ServerManager.cpp
	ServerManager.h
	ServerState.h
	Transport.h
	TransportCommand.h
	TransportClient.cpp
	TransportClient.h
	TransportManager.cpp
	TransportManager.h
)

if (WITH_JS)
	list(
		APPEND
		SOURCES
		${JS_SOURCES}
		System.h
		System.cpp
	)

	list(APPEND LIBRARIES duktape)
endif ()

irccd_define_executable(
	TARGET irccd
	SOURCES
		${SOURCES}
		${COMMAND_SOURCES}
		${EVENT_SOURCES}
		${SERVER_SOURCES}
		${TRANSPORT_COMMAND_SOURCES}
	INCLUDES
		${irccd_SOURCE_DIR}
	LIBRARIES
		${LIBRARIES}
		ircclient
		common
	INSTALL_RUNTIME bin/
)

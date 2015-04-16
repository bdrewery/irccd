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

set(
	SOURCES
	Identity.h
	Irccd.cpp
	Irccd.h
	main.cpp
	Server.cpp
	Server.h
	ServerEvent.cpp
	ServerEvent.h
	ServerService.cpp
	ServerService.h
	ServerState.cpp
	ServerState.h
	Service.cpp
	Service.h
	Transport.h
	TransportCommand.cpp
	TransportCommand.h
	TransportClient.cpp
	TransportClient.h
	TransportService.cpp
	TransportService.h
)

if (WITH_JS)
	list(
		APPEND
		SOURCES
		Js.cpp
		Js.h
		JsFilesystem.cpp
		JsLogger.cpp
		JsPlugin.cpp
		JsServer.cpp
		JsSystem.cpp
		JsTimer.cpp
		JsUnicode.cpp
		JsUtil.cpp
		Plugin.cpp
		Plugin.h
		Timer.cpp
		Timer.h
		TimerEvent.cpp
		TimerEvent.h
		Unicode.cpp
		Unicode.h
	)

	list(APPEND LIBRARIES duktape)
endif ()

irccd_define_executable(
	TARGET irccd
	INSTALL
	SOURCES
		${SOURCES}
	INCLUDES
		${irccd_SOURCE_DIR}
	LIBRARIES
		${LIBRARIES}
		ircclient
		common
)

if (UNIX)
	set_target_properties(irccd PROPERTIES LINK_FLAGS "-pthread")
endif ()

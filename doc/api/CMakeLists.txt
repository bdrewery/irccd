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

project(api)

add_subdirectory(event)
add_subdirectory(module/irccd)
add_subdirectory(module/fs)
add_subdirectory(module/logger)
add_subdirectory(module/server)
add_subdirectory(module/system)
add_subdirectory(module/plugin)
add_subdirectory(module/parser)
add_subdirectory(module/rule)
add_subdirectory(module/socket)
add_subdirectory(module/socket-address)
add_subdirectory(module/socket-listener)
add_subdirectory(module/thread)
add_subdirectory(module/util)

set(
	SOURCES
	${api_SOURCE_DIR}/index.txt
	${IRCCD_SOURCES}
	${EVENT_SOURCES}
	${FS_SOURCES}
	${LOGGER_SOURCES}
	${SERVER_SOURCES}
	${SYSTEM_SOURCES}
	${PLUGIN_SOURCES}
	${PARSER_SOURCES}
	${RULE_SOURCES}
	${SOCKET_SOURCES}
	${SOCKET_ADDRESS_SOURCES}
	${SOCKET_LISTENER_SOURCES}
	${THREAD_SOURCES}
	${UTIL_SOURCES}
)

irccd_generate_html(
	MASTER api.html
	TARGET docs-api
	SOURCES ${SOURCES}
	PREFIX api
)

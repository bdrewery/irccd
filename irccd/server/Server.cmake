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

set(
	SERVER_SOURCES
	${CMAKE_CURRENT_LIST_DIR}/Connecting.cpp
	${CMAKE_CURRENT_LIST_DIR}/Connecting.h
	${CMAKE_CURRENT_LIST_DIR}/Disconnected.cpp
	${CMAKE_CURRENT_LIST_DIR}/Disconnected.h
	${CMAKE_CURRENT_LIST_DIR}/Running.cpp
	${CMAKE_CURRENT_LIST_DIR}/Running.h
	${CMAKE_CURRENT_LIST_DIR}/ServerState.h
	${CMAKE_CURRENT_LIST_DIR}/Uninitialized.cpp
	${CMAKE_CURRENT_LIST_DIR}/Uninitialized.h
	${CMAKE_CURRENT_LIST_DIR}/Waiting.cpp
	${CMAKE_CURRENT_LIST_DIR}/Waiting.h
)

source_group(server FILES ${SERVER_SOURCES})

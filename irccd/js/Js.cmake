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

set(
	JS_SOURCES
	${CMAKE_CURRENT_LIST_DIR}/JsFilesystem.cpp
	${CMAKE_CURRENT_LIST_DIR}/JsLogger.cpp
	${CMAKE_CURRENT_LIST_DIR}/JsServer.cpp
	${CMAKE_CURRENT_LIST_DIR}/JsSystem.cpp
	${CMAKE_CURRENT_LIST_DIR}/JsTimer.cpp
	${CMAKE_CURRENT_LIST_DIR}/JsUnicode.cpp
	${CMAKE_CURRENT_LIST_DIR}/JsUtil.cpp
	${CMAKE_CURRENT_LIST_DIR}/Js.cpp
	${CMAKE_CURRENT_LIST_DIR}/Js.h
	${CMAKE_CURRENT_LIST_DIR}/Plugin.cpp
	${CMAKE_CURRENT_LIST_DIR}/Plugin.h
	${CMAKE_CURRENT_LIST_DIR}/Timer.cpp
	${CMAKE_CURRENT_LIST_DIR}/Timer.h
	${CMAKE_CURRENT_LIST_DIR}/TimerEvent.cpp
	${CMAKE_CURRENT_LIST_DIR}/TimerEvent.h
	${CMAKE_CURRENT_LIST_DIR}/Unicode.cpp
	${CMAKE_CURRENT_LIST_DIR}/Unicode.h
)

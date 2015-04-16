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
	JS_UNICODE_SOURCES
	${irccd_SOURCE_DIR}/Js.cpp
	${irccd_SOURCE_DIR}/Js.h
	${irccd_SOURCE_DIR}/JsFilesystem.cpp
	${irccd_SOURCE_DIR}/JsLogger.cpp
	${irccd_SOURCE_DIR}/JsPlugin.cpp
	${irccd_SOURCE_DIR}/JsServer.cpp
	${irccd_SOURCE_DIR}/JsSystem.cpp
	${irccd_SOURCE_DIR}/JsTimer.cpp
	${irccd_SOURCE_DIR}/JsUnicode.cpp
	${irccd_SOURCE_DIR}/JsUtil.cpp
	${irccd_SOURCE_DIR}/Plugin.cpp
	${irccd_SOURCE_DIR}/Plugin.h
	${irccd_SOURCE_DIR}/Timer.cpp
	${irccd_SOURCE_DIR}/Timer.h
	${irccd_SOURCE_DIR}/Unicode.cpp
	${irccd_SOURCE_DIR}/Unicode.h
	TestJsUnicode.cpp
)

create_test(test-js-unicode "${JS_UNICODE_SOURCES}")

target_link_libraries(test-js-unicode duktape)
target_compile_definitions(
	test-js-unicode
	PRIVATE
		BINARY=\"${CMAKE_BINARY_DIR}\"
)

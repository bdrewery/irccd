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

include(${irccd_SOURCE_DIR}/servercommand/ServerCommand.cmake)
include(${irccd_SOURCE_DIR}/js/Js.cmake)

set(
	JS_FILESYSTEM_SOURCES
	${JS_SOURCES}
	${SERVER_COMMAND_SOURCES}
	TestJsFilesystem.cpp
)

create_test(test-js-filesystem "${JS_FILESYSTEM_SOURCES}")

target_link_libraries(test-js-filesystem duktape)
target_compile_definitions(
	test-js-filesystem
	PRIVATE
		BINARY=\"${CMAKE_BINARY_DIR}\"
)

#
# Create following tree:
#
# CMAKE_BINARY_DIR
#	| testassets
#	|	|-- file.txt
#	|	|-- lines.txt
#	|	|-- level-1
#	|	|	| -- file-1.txt
#	|	|	| -- level-2
#	|	|	|	| -- file-2.txt
#
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/testassets/level-1/level-2)
file(WRITE ${CMAKE_BINARY_DIR}/testassets/file.txt "file.txt")
file(WRITE ${CMAKE_BINARY_DIR}/testassets/lines.txt "a\nb\nc\n")
file(WRITE ${CMAKE_BINARY_DIR}/testassets/level-1/file-1.txt "file-1.txt")
file(WRITE ${CMAKE_BINARY_DIR}/testassets/level-1/level-2/file-2.txt "file-2.txt")

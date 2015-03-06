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
	CMAKE_SOURCES
	${CMAKE_CURRENT_LIST_DIR}/CMake.cmake
	${CMAKE_CURRENT_LIST_DIR}/IrccdCPackConfig.cmake
	${CMAKE_CURRENT_LIST_DIR}/IrccdFunctions.cmake
	${CMAKE_CURRENT_LIST_DIR}/IrccdOptions.cmake
	${CMAKE_CURRENT_LIST_DIR}/IrccdSystem.cmake
	${CMAKE_CURRENT_LIST_DIR}/IrccdVersion.cmake
)

set(
	CMAKE_INTERNAL_SOURCES
	${CMAKE_CURRENT_LIST_DIR}/internal/IrccdConfig.h.in
)

set(
	CMAKE_PACKAGES_SOURCES
	${CMAKE_CURRENT_LIST_DIR}/packages/FindLibiconv.cmake
	${CMAKE_CURRENT_LIST_DIR}/packages/FindPandoc.cmake
)

add_custom_target(
	cmake
	COMMENT "CMake sources files"
	SOURCES
		${CMAKE_SOURCES}
		${CMAKE_INTERNAL_SOURCES}
		${CMAKE_PACKAGES_SOURCES}
)

source_group(cmake FILES ${CMAKE_SOURCES})
source_group(cmake\\internal FILES ${CMAKE_INTERNAL_SOURCES})
source_group(cmake\\packages FILES ${CMAKE_PACKAGES_SOURCES})

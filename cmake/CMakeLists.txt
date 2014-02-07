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

project(cmake)

if (WIN32)
	#
	# First remove NSis extra commands
	#
	file(REMOVE ${CMAKE_BINARY_DIR}/nsis_extra_install.txt)
	file(REMOVE ${CMAKE_BINARY_DIR}/nsis_extra_uninstall.txt)


	file(
		APPEND
		${CMAKE_BINARY_DIR}/nsis_extra_install.txt 
		"
		  CreateDirectory \\\"$SMPROGRAMS\\\\${IRCCD_PACKAGE_NAME}\\\\Documentation\\\"
		  CreateDirectory \\\"$SMPROGRAMS\\\\${IRCCD_PACKAGE_NAME}\\\\Documentation\\\\Plugins\\\"
		"
	)
endif()

set(CMAKE_MODULE_PATH ${cmake_SOURCE_DIR} PARENT_SCOPE)

include(CMakeParseArguments)

include(Config.cmake)
include(Options.cmake)
include(Macros.cmake)
include(Extern.cmake)

configure_file(
	"${CMAKE_SOURCE_DIR}/cmake/config.h.in"
	"${CMAKE_BINARY_DIR}/config.h"
)

set(
	FILES
	FindLuaJIT.cmake
	Config.cmake
	config.h.in
	CPackConfig.cmake
	Extern.cmake
	FindCppunit.cmake
	FindLibircclient.cmake
	FindLua52.cmake
	FindXDG.cmake
	Macros.cmake
	NSIS.InstallOptions.ini.in
	NSIS.template.in
	Options.cmake
)

add_custom_target(
	cmake
	COMMENT "General CMake files for malikania"
	SOURCES ${FILES}
)

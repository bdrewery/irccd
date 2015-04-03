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

# Project
project(tests)

# Function to create a test
function(create_test name sources)
	include_directories(
		${CMAKE_BINARY_DIR}
		${CMAKE_SOURCE_DIR}
		${irccd_SOURCE_DIR}
		${extern_SOURCE_DIR}
	)

	add_executable(${name} ${sources})

	add_test(
		NAME ${name}
		COMMAND ${name}
	)

	target_link_libraries(
		${name}
		common
		ircclient
		gtest
	)

	if (UNIX)
		set_target_properties(${name} PROPERTIES LINK_FLAGS "-pthread")
	endif ()
endfunction()

# Create and copy a directory
function(create_dir target dirname)
	add_custom_command(
		TARGET ${target}
		POST_BUILD
		COMMAND
			${CMAKE_COMMAND} -E make_directory
			${CMAKE_CURRENT_BINARY_DIR}/${dirname}
		COMMAND
			${CMAKE_COMMAND} -E copy_directory
			${CMAKE_CURRENT_SOURCE_DIR}/${dirname}
			${CMAKE_CURRENT_BINARY_DIR}/${dirname}
	)
endfunction()

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/testassets)

if (WITH_TESTS)
	# JS API
	add_subdirectory(js-filesystem)
	add_subdirectory(js-system)
	add_subdirectory(js-timer)

	# Server stuff
	add_subdirectory(server)
	add_subdirectory(transport)
	add_subdirectory(rules)
	#add_subdirectory(converter)
	#add_subdirectory(lua-util)
	add_subdirectory(split)
	add_subdirectory(strip)
endif ()

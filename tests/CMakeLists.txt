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

# Project
project(tests)

# Option to enable / disable them
option(WITH_TESTS "Enable unit testing" Off)

# Set globally so message() can get it
set(UNIT_TESTS "disabled" PARENT_SCOPE)

# cppunit generate some warnings
if(UNIX)
	set(FLAGS "-Wno-unused-parameter")
endif()

# Function to create a test
function(create_test name sources)
	include_directories(
		${CPPUNIT_INCLUDE_DIR}
		${CMAKE_BINARY_DIR}
		${CMAKE_SOURCE_DIR}
	)

	add_executable(${name} ${sources})
	set_target_properties(${name} PROPERTIES COMPILE_FLAGS ${FLAGS})

	add_test(
		NAME ${name}
		COMMAND ${name}
	)

	target_link_libraries(
		${name}
		common
		${CPPUNIT_LIBRARY}
	)
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

if(WITH_TESTS)
	find_package(Cppunit QUIET)

	if(CPPUNIT_FOUND)
		set(UNIT_TESTS "enabled" PARENT_SCOPE)

		# Parser tests
		create_test(TestParser parser/TestParser.cpp)
		create_dir(TestParser parser/configs)

		# Sockets
		create_test(TestSocket sockets/TestSockets.cpp)

		# Conversion
		create_test(TestConverter converter/TestConverter.cpp)

		# Strip
		create_test(TestStrip strip/TestStrip.cpp)

		# Split
		create_test(TestSplit split/TestSplit.cpp)
	endif()
endif()

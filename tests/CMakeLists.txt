#
# CMakeLists.txt -- CMake build system for irccd
#
# Copyright (c) 2011, 2012, 2013 David Demelier <markand@malikania.fr>
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

function(CREATE_TEST name sources)
	include_directories(${CPPUNIT_INCLUDE_DIR})

	add_executable(${name} ${sources})

	target_link_libraries(${name} ${CPPUNIT_LIBRARY})
endfunction()

if (WITH_TESTS)
	find_package(Cppunit QUIET)

	if (CPPUNIT_FOUND)	
		set(UNIT_TESTS "enabled" PARENT_SCOPE)

		CREATE_TEST(TestParser TestParser.cpp)	
	endif ()
endif ()

# vim: set syntax=cmake:

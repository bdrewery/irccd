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

project(doc)

#
# Meta targets `docs`
#
add_custom_target(
	docs ALL
	COMMENT "Building irccd documentation"
)

# ---------------------------------------------------------
# Install manuals
# ---------------------------------------------------------

# Update man pages date
set(MONTH "January")
set(DAY 30)
set(YEAR 2014)

irccd_define_man(irccd.1 man1)
irccd_define_man(irccd.conf.5 man5)
irccd_define_man(irccdctl.1 man1)
irccd_define_man(irccdctl.conf.5 man5)

# ---------------------------------------------------------
# Create a target for doxygen
# ---------------------------------------------------------

if (WITH_DOCS_DOXYGEN)
	set(DOXYGEN_INPUT ${CMAKE_SOURCE_DIR})

	if (NOT DOXYGEN_DOT_FOUND)
		set(DOXYGEN_HAVE_DOT "NO")
		set(DOXYGEN_DOT_PATH "")
	else ()
		set(DOXYGEN_HAVE_DOT "YES")
		set(DOXYGEN_DOT_PATH "\"${DOXYGEN_DOT_EXECUTABLE}\"")
	endif ()

	configure_file(
		"${doc_SOURCE_DIR}/doxygen/Doxyfile.in"
		"${doc_BINARY_DIR}/doxygen/Doxyfile"
	)

	add_custom_target(
		docs-doxygen
		COMMENT "Generating doxygen documentation"
		COMMAND
			${CMAKE_COMMAND} -E make_directory ${WITH_DOCS_DIRECTORY}/doxygen
		COMMAND
			${DOXYGEN_EXECUTABLE} ${doc_BINARY_DIR}/doxygen/Doxyfile
		WORKING_DIRECTORY
			${CMAKE_SOURCE_DIR}
	)

	add_dependencies(docs docs-doxygen)
endif ()

# ---------------------------------------------------------
# User guides, Lua API
# ---------------------------------------------------------

if (WITH_DOCS_LUA OR WITH_DOCS_GUIDES_HTML OR WITH_DOCS_GUIDES_PDF)
	add_subdirectory(linkify)
	add_subdirectory(templates)
endif ()

if (WITH_DOCS_GUIDES_HTML OR WITH_DOCS_GUIDES_PDF)
	add_subdirectory(guides)
endif ()

if (WITH_DOCS_LUA)
	add_subdirectory(api)
endif ()

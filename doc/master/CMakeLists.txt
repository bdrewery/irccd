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

project(master)

# Other assets
set(
	CSS_SOURCES
	${doc_SOURCE_DIR}/css/style.css
)

# Arguments to pass to templator executable
set(
	TEMPLATOR_ARGS
	footer=${doc_SOURCE_DIR}/templates/footer.html
	header=${doc_SOURCE_DIR}/templates/header.html
	modules=${doc_SOURCE_DIR}/templates/modules.html
)

# Templates sources that can be edited from the IDE
set(
	TEMPLATES_SOURCES
	${doc_SOURCE_DIR}/templates/footer.html
	${doc_SOURCE_DIR}/templates/header.html
	${doc_SOURCE_DIR}/templates/modules.html
)

# Master files to be processed
set(
	MASTER_SOURCES
	${master_SOURCE_DIR}/api.html.in
)

# Build all master files
foreach (m ${MASTER_SOURCES})
	get_filename_component(input ${m} NAME)
	get_filename_component(inputfile ${m} ABSOLUTE)
	string(REGEX REPLACE "^(.*).html.in$" "\\1.html" outputname ${input})
	set(outputfile ${doc_BINARY_DIR}/master/${outputname})

	# Add the list of outputs files
	list(APPEND outputs ${outputfile})

	# Generate the file
	add_custom_command(
		OUTPUT ${outputfile}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${doc_BINARY_DIR}/master
		COMMAND templator ${inputfile} ${outputfile} ${TEMPLATOR_ARGS}
		DEPENDS ${inputfile}
	)
endforeach ()

# Copy all resources files
foreach (css ${CSS_SOURCES})
	get_filename_component(file ${css} NAME)

	add_custom_command(
		OUTPUT ${WITH_DOCS_DIRECTORY}/css/${file}
		COMMAND
			${CMAKE_COMMAND} -E make_directory ${WITH_DOCS_DIRECTORY}/css
		COMMAND
			${CMAKE_COMMAND} -E copy ${css} ${WITH_DOCS_DIRECTORY}/css/${file}
		DEPENDS ${css}
	)

	list(APPEND outputs ${WITH_DOCS_DIRECTORY}/css/${file})
endforeach ()

add_custom_target(
	docs-master
	DEPENDS ${outputs}
	SOURCES ${MASTER_SOURCES} ${TEMPLATES_SOURCES} ${CSS_SOURCES}
	COMMENT "Built master layouts"
)

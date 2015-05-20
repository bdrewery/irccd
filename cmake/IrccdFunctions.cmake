#
# Macros.cmake -- CMake build system for irccd
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

include(CMakeParseArguments)

# ---------------------------------------------------------
# Public functions
# ---------------------------------------------------------

# ---------------------------------------------------------
# irccd_define_library(
#	TARGET target name
#	SOURCES src1, src2, srcn
#	FLAGS (Optional) C/C++ flags (without -D)
#	LIBRARIES (Optional) libraries to link
#	LOCAL_INCLUDES (Optional) local includes for the target only
#	PUBLIC_INCLUDES (Optional) includes to share with target dependencies
# )
#
# Create a static library for internal use.
# ---------------------------------------------------------

function(irccd_define_library)
	set(oneValueArgs TARGET)
	set(multiValueArgs SOURCES FLAGS LIBRARIES LOCAL_INCLUDES PUBLIC_INCLUDES)
	set(mandatory TARGET SOURCES)

	cmake_parse_arguments(LIB "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	irccd_verify_args(LIB ${mandatory})
	add_library(${LIB_TARGET} STATIC ${LIB_SOURCES})
	apply_includes(${LIB_TARGET} LIB_LOCAL_INCLUDES LIB_PUBLIC_INCLUDES)
	apply_flags(${LIB_TARGET} LIB_FLAGS)
	apply_libraries(${LIB_TARGET} LIB_LIBRARIES)
endfunction()

# ---------------------------------------------------------
# irccd_define_executable(
#	TARGET target name
#	SOURCES src1, src2, srcn
#	FLAGS (Optional) C/C++ flags (without -D)
#	LIBRARIES (Optional) libraries to link
#	INCLUDES (Optional) includes for the target
#	INSTALL (Optional) install the executable or not (default: false)
# )
#
# Create an executable that can be installed or not.
# ---------------------------------------------------------

function(irccd_define_executable)
	set(options INSTALL)
	set(oneValueArgs TARGET INSTALL)
	set(multiValueArgs SOURCES FLAGS LIBRARIES INCLUDES)
	set(mandatory TARGET)

	cmake_parse_arguments(EXE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	irccd_verify_args(EXE ${mandatory})
	add_executable(${EXE_TARGET} ${EXE_SOURCES})
	apply_includes(${EXE_TARGET} EXE_INCLUDES DUMMY_VALUE_UNUSED)
	apply_flags(${EXE_TARGET} EXE_FLAGS)
	apply_libraries(${EXE_TARGET} EXE_LIBRARIES)

	# use fakeroot if relocatable
	if (IRCCD_RELOCATABLE)
		set_target_properties(
			${EXE_TARGET}
			PROPERTIES
				RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/fakeroot/${WITH_BINDIR}
				RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/fakeroot/${WITH_BINDIR}
				RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/fakeroot/${WITH_BINDIR}
		)
	endif ()

	# Install the target
	if (EXE_INSTALL)
		install(
			TARGETS ${EXE_TARGET}
			RUNTIME DESTINATION ${WITH_BINDIR}
		)
	endif ()
endfunction()

# ---------------------------------------------------------
# irccd_define_man(file man)
#
# Parameters:
#	file		- The file name to build
#	man		- The man section
#
# This function configure the manual and install it if WITH_MAN is set.
# ---------------------------------------------------------

function(irccd_define_man file man)
	if (WITH_MAN)
		set(path "${doc_SOURCE_DIR}/man/${file}.in")

		# install to fakeroot if applicable
		if (IRCCD_RELOCATABLE)
			set(output ${CMAKE_BINARY_DIR}/fakeroot/${MANDIR}/${man}/${file})
		else ()
			set(output ${CMAKE_BINARY_DIR}/docs/man/${man}/${file})
		endif ()

		configure_file(${path} ${output})

		install(
			FILES ${output}
			DESTINATION ${MANDIR}/${man}
		)
	endif ()
endfunction()

# ---------------------------------------------------------
# irccd_verify_args(prefix list)
#
# Parameters:
#	prefix		- The prefix of variables (e.g FOO)
#	list		- The list of variables that must be set(e.g. A B)
#
# Verify that some args are well defined when using CMakeParseArguments.
# ---------------------------------------------------------

function(irccd_verify_args prefix list)
	foreach (v ${list})
		if (NOT ${prefix}_${v})
			message(FATAL_ERROR "Please define ${v}")
		endif ()
	endforeach ()
endfunction()

function(irccd_define_test)
	set(oneValueArgs NAME)	
	set(multiValueArgs SOURCES LIBRARIES RESOURCES)
	set(mandatory NAME SOURCES)

	cmake_parse_arguments(TEST "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	irccd_verify_args(TEST ${mandatory})

	foreach (r ${TEST_RESOURCES})
		get_filename_component(output NAME ${r})
		add_custom_command(
			OUTPUT ${CMAKE_BINARY_DIR}/tests/${output}
			COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/tests/${output}
		)	
		list(APPEND RESOURCES ${CMAKE_BINARY_DIR}/tests/${output})
	endforeach ()

	# Always link to googletest
	list(APPEND TEST_LIBRARIES gtest)

	# Executable
	add_executable(test-${TEST_NAME} ${TEST_SOURCES} ${RESOURCES})
	apply_libraries(test-${TEST_NAME} TEST_LIBRARIES)
	source_group(Auto-generated FILES ${RESOURCES})
	target_include_directories(
		test-${TEST_NAME}
		PRIVATE
			${irccd_SOURCE_DIR}
			${tests_SOURCE_DIR}/libtest
	)

	# Tests are all in the same directory
	set_target_properties(
		test-${TEST_NAME}
		PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
			RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
			RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
			RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
			RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
	)

	if (UNIX)
		set_target_properties(test-${TEST_NAME} PROPERTIES LINK_FLAGS "-pthread")
	endif ()

	# And test
	add_test(
		NAME test-${TEST_NAME}
		COMMAND test-${TEST_NAME}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests
	)
endfunction()

# ---------------------------------------------------------
# irccd_generate_guide(target filename sources)
#
# Parameters:
#	target		- Generate docs-guide-${target}-html|latex
#	filename	- The filename generated without extension
#	sources		- The sources files
#
# Generate a guide in both PDF and HTML formats.
# ---------------------------------------------------------

function(irccd_generate_guide target filename sources)
	if (WITH_DOCS_GUIDES_HTML)
		if (IRCCD_RELOCATABLE)
			set(outputdir ${CMAKE_BINARY_DIR}/fakeroot/${WITH_DOCDIR})
		else ()
			set(outputdir ${CMAKE_BINARY_DIR}/docs)
		endif ()

		set(
			args
			-s -S --toc -fmarkdown -thtml5 -Vguide:yes
			--template ${templates_SOURCE_DIR}/template.html
		)

		#
		# Note: the linkify is needed because the template use relative URLs for CSS.
		#
		add_custom_command(
			OUTPUT ${outputdir}/${filename}.html
			DEPENDS ${sources} linkify
			COMMAND
				${CMAKE_COMMAND} -E make_directory ${outputdir}
			COMMAND
				${Pandoc_EXECUTABLE} ${args} -o ${outputdir}/${filename}.tmp ${sources}
			COMMAND
				$<TARGET_FILE:linkify> ${outputdir}/${filename}.tmp ${outputdir}/${filename}.html "empty" "empty"
			COMMAND
				${CMAKE_COMMAND} -E remove ${outputdir}/${filename}.tmp
		)

		add_custom_target(
			docs-guide-${target}-html
			DEPENDS
				docs-templates
				${outputdir}/${filename}.html
			SOURCES ${sources}
		)

		add_dependencies(docs docs-guide-${target}-html)
	endif ()

	if (WITH_DOCS_GUIDES_PDF)
		if (IRCCD_RELOCATABLE)
			set(output ${CMAKE_BINARY_DIR}/fakeroot/${WITH_DOCDIR}/${filename}.pdf)
		else ()
			set(output ${CMAKE_BINARY_DIR}/docs/${filename}.pdf)
		endif ()

		pandoc(
			OUTPUT ${output}
			SOURCES ${sources}
			FROM markdown
			TO latex
			TARGET docs-guide-${target}-latex
			TOC STANDALONE MAKE_DIRECTORY
			TEMPLATE ${templates_SOURCE_DIR}/template.tex
		)

		add_dependencies(docs docs-guide-${target}-latex)
	endif ()
endfunction()

# ---------------------------------------------------------
# Private functions
# ---------------------------------------------------------

function(apply_includes target local public)
	# We always add CMake's binary dir for config.h
	target_include_directories(${target} PRIVATE ${CMAKE_BINARY_DIR})

	if (${local})
		target_include_directories(${target} PRIVATE ${${local}})
	endif ()

	if (${public})
		target_include_directories(${target} PUBLIC ${${public}})
	endif ()
endfunction()

function(apply_flags target flags)
	if (${flags})
		target_compile_definitions(${target} PRIVATE ${${flags}})
	endif ()

	if (WIN32)
		target_compile_options(${target} PRIVATE "-Wno-cpp")
	endif ()
endfunction()

function(apply_libraries target libs)
	if (${libs})
		target_link_libraries(${target} ${${libs}})
	endif ()

	if (TARGET port)
		target_link_libraries(${target} port)
	endif ()
endfunction()

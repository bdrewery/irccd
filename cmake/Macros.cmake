#
# Macros.cmake -- CMake build system for irccd
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

function(check_mandatory prefix list)
	# Check mandatory arguments
	foreach(a ${list})
		if (NOT ${prefix}_${a})
			message(FATAL_ERROR "Please specify ${a} parameter")
		endif ()
	endforeach()
endfunction()

function(apply_includes target local public)
	# We always add CMake's binary dir for config.h
	target_include_directories(${target} PRIVATE ${CMAKE_BINARY_DIR})

	if(${local})
		target_include_directories(${target} PRIVATE ${${local}})
	endif()

	if(${public})
		target_include_directories(${target} PUBLIC ${${public}})
	endif()
endfunction()

function(apply_flags target flags)
	if(${flags})
		target_compile_definitions(${target} PRIVATE ${${flags}})
	endif()

	# Remove Windows useless warnings
	if (WIN32 AND MSVC)
		target_compile_definitions(
			${target}
			PRIVATE "_CRT_SECURE_NO_WARNINGS"
		)

		target_compile_options(
			${target}
			PRIVATE /wd4996 /wd4244 /wd4267 /wd4133
		)
	endif ()
endfunction()

function(apply_libraries target libs)
	if (${libs})
		target_link_libraries(${target} ${${libs}})
	endif ()
endfunction()

function(add_nsis_install command)
	file(
		APPEND
		${CMAKE_BINARY_DIR}/nsis_extra_install.txt
		${command}\n
	)
endfunction()

function(add_nsis_uninstall command)
	file(
		APPEND
		${CMAKE_BINARY_DIR}/nsis_extra_uninstall.txt
		${command}\n
	)
endfunction()

# ---------------------------------------------------------
# define_library(
#	TARGET target name
#	SOURCES src1, src2, srcn
#	FLAGS optional C/C++ flags (without -D)
#	LIBRARIES optional libraries to link
#	LOCAL_INCLUDES optional local includes for the target only
#	PUBLIC_INCLUDES optional includes to share with target dependencies
# )
#
# Create a static library.
#
function(define_library)
	set(oneValueArgs TARGET)
	set(multiValueArgs SOURCES FLAGS LIBRARIES LOCAL_INCLUDES PUBLIC_INCLUDES)
	set(mandatory TARGET SOURCES)

	cmake_parse_arguments(LIB "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	check_mandatory(LIB ${mandatory})
	add_library(${LIB_TARGET} STATIC ${LIB_SOURCES})
	apply_includes(${LIB_TARGET} LIB_LOCAL_INCLUDES LIB_PUBLIC_INCLUDES)
	apply_flags(${LIB_TARGET} LIB_FLAGS)
	apply_libraries(${LIB_TARGET} LIB_LIBRARIES)
endfunction()

# ---------------------------------------------------------
# define_executable(
#	TARGET target name
#	SOURCES src1, src2, srcn
#	FLAGS optional C/C++ flags (without -D)
#	LIBRARIES optional libraries to link
#	INCLUDES optional includes for the target
#	INSTALL_RUNTIME optional relative path where to install the executable
# )
#
# Create an executable.
#
function(define_executable)
	set(oneValueArgs TARGET INSTALL_RUNTIME)
	set(multiValueArgs SOURCES FLAGS LIBRARIES INCLUDES)
	set(mandatory TARGET)

	cmake_parse_arguments(EXE "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	check_mandatory(EXE ${mandatory})
	add_executable(${EXE_TARGET} ${EXE_SOURCES})
	apply_includes(${EXE_TARGET} EXE_INCLUDES DUMMY_VALUE_UNUSED)
	apply_flags(${EXE_TARGET} EXE_FLAGS)
	apply_libraries(${EXE_TARGET} EXE_LIBRARIES)

	if(WIN32 AND MSVC)
		set_target_properties(
			${EXE_TARGET}
			PROPERTIES
			LINK_FLAGS /SAFESEH:NO
		)
	endif()

	# Install the target
	if(EXE_INSTALL_RUNTIME)
		install(
			TARGETS ${EXE_TARGET}
			RUNTIME DESTINATION ${EXE_INSTALL_RUNTIME}
		)
	endif()
endfunction()

# ---------------------------------------------------------
# define_plugin(name file description default)
#
# Parameters:
#	name		- The plugin name, an automatic option
#			  WITH_PLUGIN_<name> will be created using the default
#			  as On or Off value.
#	file		- The file (excluding its extension)
#	description	- The option description
#	default		- Default option (on or off)
#
# This function create a new option to install or not the plugin and its
# documentation.
#
macro(define_plugin name file description default)
	option(WITH_PLUGIN_${name} ${description} ${default})

	if(WITH_PLUGIN_${name})
		install(FILES ${file}.lua DESTINATION ${MODDIR})
	endif()
endmacro()

# ---------------------------------------------------------
# irccd_define_man(file man)
#
# Parameters:
#	file		- The file name to build
#	man		- The man section
#
# This function configure the manual and install it if WITH_MAN is set.
#
function(irccd_define_man file man)
	if(WITH_MAN)
		set(path "${doc_SOURCE_DIR}/man/${file}.in")
		set(output "${doc_BINARY_DIR}/${file}")

		configure_file(${path} ${output})

		install(
			FILES ${output}
			DESTINATION ${MANDIR}/${man}
		)
	endif()
endfunction()

# ---------------------------------------------------------
# irccd_verify_args(prefix list)
#
# Parameters:
#	prefix		- The prefix of variables (e.g FOO)
#	list		- The list of variables that must be set(e.g. A B)
#
# Verify that some args are well defined.
#
function(irccd_verify_args prefix list)
	foreach (v ${list})
		if (NOT ${prefix}_${v})
			message(FATAL_ERROR "Please define ${v}")
		endif ()
	endforeach ()
endfunction()

function(irccd_generate_html)
	set(oneValueArgs MASTER TARGET PREFIX)
	set(multiValueArgs SOURCES)
	set(mandatory MASTER TARGET SOURCES PREFIX)

	cmake_parse_arguments(HTML "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	irccd_verify_args(HTML "${mandatory}")

	foreach (s ${HTML_SOURCES})
		#
		# inputpath: full path to the input file
		# inputbase: base name relative to the CMakeLists.txt invocation
		#
		get_filename_component(inputpath ${s} ABSOLUTE)
		file(RELATIVE_PATH inputbase ${CMAKE_CURRENT_SOURCE_DIR} ${inputpath})

		#
		# outputpath: full path to the generated file
		# outputdirectory: base directory for the output file
		#
		string(REGEX REPLACE "^(.*)\\.txt$" "\\1.html" outputname ${inputbase})
		set(outputpath ${WITH_DOCS_DIRECTORY}/${HTML_PREFIX}/${outputname})
		get_filename_component(outputdirectory ${outputpath} DIRECTORY)

		#
		# outputs: list of output files
		# sources: list of input files
		#
		list(APPEND outputs ${outputpath})
		list(APPEND sources ${inputpath})

		set(
			linkify_args
			${WITH_DOCS_DIRECTORY}
			${outputdirectory}
		)

		set(
			pandoc_args
			-s
			-S
			-f markdown
			-t html5
			--template ${doc_BINARY_DIR}/master/${HTML_MASTER}
			-o ${outputpath}.tmp
			${inputpath}
		)

		add_custom_command(
			OUTPUT ${outputpath}
			COMMAND
				${CMAKE_COMMAND} -E make_directory ${outputdirectory}
			COMMAND
				${Pandoc_EXECUTABLE} ${pandoc_args}
			COMMAND
				cat ${outputpath}.tmp | $<TARGET_FILE:linkify> ${linkify_args} > ${outputpath}
			DEPENDS
				${inputpath}
				docs-master
		)
	endforeach ()

	add_custom_target(
		${HTML_TARGET}
		DEPENDS ${outputs} docs-master
		SOURCES ${sources}
		COMMENT "Generating HTML documentation `${HTML_TARGET}`"
	)

	add_dependencies(docs ${HTML_TARGET})
endfunction()

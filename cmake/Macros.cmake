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
		get_filename_component(input ${s} ABSOLUTE)
		file(RELATIVE_PATH base ${CMAKE_CURRENT_SOURCE_DIR} ${input})
		string(REGEX REPLACE "^(.*)\\.txt$" "\\1.html" outputname ${base})

		# Append to outputs and sources
		list(APPEND outputs ${WITH_DOCS_DIRECTORY}/${HTML_PREFIX}/${outputname})
		list(APPEND sources ${input})

		# Update links
		get_filename_component(directory ${input} DIRECTORY)

		pandoc(
			SOURCES ${input}
			OUTPUT ${WITH_DOCS_DIRECTORY}/${HTML_PREFIX}/${outputname}
			MAKE_DIRECTORY
			FROM markdown TO html5
			TEMPLATE ${doc_BINARY_DIR}/master/${HTML_MASTER}
			DEPENDS docs-master
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

## ---------------------------------------------------------
## irccd_generate_api(
##	TARGET target
##	CATEGORY category
##	DIRECTORY directory
##	SOURCES file ...
##	TYPE function|enum|table
##	[INDEX file]
## )
##
## Generate Lua API documentation from markdown using pandoc. Be sure
## that a target `api` is created before invoking this function.
##
#function(irccd_generate_api)
#	set(oneValueArgs CATEGORY DIRECTORY TARGET INDEX)
#	set(multiValueArgs SOURCES TYPE)
#	set(mandatory TARGET DIRECTORY CATEGORY SOURCES)
#
#	cmake_parse_arguments(API "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
#	irccd_verify_args(API "${mandatory}")
#
#	if (API_TYPE MATCHES "function")
#		set(template function.html)
#	elseif (API_TYPE MATCHES "enum")
#		set(template enum.html)
#	elseif (API_TYPE MATCHES "event")
#		set(template event.html)
#	elseif (API_TYPE MATCHES "table")
#		set(template table.html)
#	elseif (API_TYPE MATCHES "method")
#		set(template method.html)
#	else ()
#		message(FATAL_ERROR "invalid TYPE given: ${API_TYPE}")
#	endif ()
#
#	#
#	# Variables set:
#	#   outputbase		- Base directory for the files
#	#
#	set(outputbase ${WITH_DOCS_DIRECTORY}/html/doc/api/${API_DIRECTORY})
#
#	#
#	# Iterate over all sources and build them one by one
#	#
#	foreach (f ${API_SOURCES})
#		get_filename_component(inputpath ${f} ABSOLUTE)
#		get_filename_component(inputname ${f} NAME)
#		string(REGEX REPLACE "^(.*)\\.txt$" "\\1.html" outputname ${inputname})
#		list(APPEND depends ${outputbase}/${outputname})
#		list(APPEND sources ${inputpath})
#
#		pandoc(
#			MAKE_DIRECTORY
#			FROM markdown
#			TO html5
#			OUTPUT ${outputbase}/${outputname}
#			SOURCES ${inputpath}
#			TEMPLATE ${WITH_DOCS_DIRECTORY}/layouts/${template}
#			DEPENDS html
#		)
#	endforeach ()
#
#	# Optional index
#	if (API_INDEX)
#		get_filename_component(indexpath ${API_INDEX} ABSOLUTE)
#
#		list(APPEND sources ${indexpath})
#		list(APPEND depends ${outputbase}/index.html)
#
#		pandoc(
#			MAKE_DIRECTORY
#			FROM markdown
#			TO html5
#			OUTPUT ${outputbase}/index.html
#			SOURCES ${indexpath}
#			TEMPLATE ${WITH_DOCS_DIRECTORY}/layouts/module.html
#			DEPENDS html
#		)
#	endif ()
#
#	add_custom_target(
#		api-${API_TARGET}
#		COMMENT "Built Lua API ${API_CATEGORY}"
#		DEPENDS ${depends}
#		SOURCES ${sources}
#	)
#
#	if (NOT TARGET api)
#		message(FATAL_ERROR "No target `api' defined")
#	endif ()
#
#	add_dependencies(api api-${API_TARGET})
#endfunction()

# FindPandoc
# ----------
#
# Find Pandoc executable, this modules defines:
#
# Pandoc_EXECUTABLE, where to find pandoc's executable
# Pandoc_FOUND, if it is found
# Pandoc_VERSION, the version
#
# This module also defines the following macros:
#
# pandoc(
#	SOURCES file1 [file2 ...]
#	OUTPUT output
#	[FROM format]
#	[TO format]
#	[TARGET target]
#	[DEPENDS dependency ...]
#	[ALL]
#	[TOC]
#	[STANDALONE]
#	[MAKE_DIRECTORY]
#	[TEMPLATE file]
#	[FILTER filter]
#	[HEADER header ...]
#	[FOOTER footer ...]
#	[BODY body ...]
#	[VARIABLE var ...]
#	[METADATA meta ...]
#	[ARGS argument ...]
#	[WORKING_DIRECTORY directory]
# )
#
# The sources files are listed in the parameter SOURCES, all files are passed
# in the same order they are passed to that variable.
#
# The OUTPUT file is set with OUTPUT. It is generated only if one of the file
# has changed.
#
# The FROM (-f) and TO (-t) arguments specify respectively the source and
# destinations formats.
#
# If the parameter TARGET is set, then a target named `target` will be added
# with the OUTPUT file as the dependency but not listed as sources files.
# But the SOURCES files will be added as the target sources in the IDE.
#
# Optional dependencies can be added to the output command (not the target) with
# the DEPENDS parameter.
#
# If ALL is set and TARGET is also set, the target will be added to the ALL_BUILD.
#
# If TOC (--toc) is specified, a table of content will be automatically created.
#
# If STANDALONE (-s) is set, the compilation will assume that it is standalone
# and adds the necessary of the output format.
#
# Optional MAKE_DIRECTORY can be set to create the output directory before
# pandoc processes the file (recommended).
#
# The TEMPLATE parameter can be used to specify the formate template file.
#
# You can set a filter with the parameter FILTER. The filter will be added to
# the output dependencies so you can safely use CMake's targets.
#
# The HEADER (-H), FOOTER (-A) and BODY (-B) are copied verbatim before, just
# after and after the body respectively. They can be set more than once.
#
# You can pass variables (-V) and metadata (-M) to the parameters VARIABLE
# and METADATA, be sure to pass the same syntax as pandoc.  (e.g VARIABLE foo=1)
#
# ARGS is an optional list of additional arguments to pass to pandoc.
#
# The parameter WORKING_DIRECTORY can be set to change the directory when pandoc
# is invoked.
#

find_program(
	Pandoc_EXECUTABLE
	NAMES pandoc
	DOC "Pandoc executable"
)

include(FindPackageHandleStandardArgs)
include(CMakeParseArguments)

# Extract the version
if (Pandoc_EXECUTABLE)
	execute_process(
		COMMAND pandoc --version
		OUTPUT_VARIABLE _pandoc_version_tmp
	)

	string(REGEX REPLACE "^pandoc ([0-9]*\\.[0-9]*\\.[0-9]*).*$" "\\1" Pandoc_VERSION ${_pandoc_version_tmp})
endif ()

find_package_handle_standard_args(
	Pandoc
	FOUND_VAR Pandoc_FOUND
	VERSION_VAR Pandoc_VERSION
	REQUIRED_VARS Pandoc_EXECUTABLE
)

if (Pandoc_FOUND)
	function(pandoc)
		set(options MAKE_DIRECTORY STANDALONE TOC)
		set(oneValueArgs FILTER FROM TARGET TEMPLATE TO OUTPUT WORKING_DIRECTORY)
		set(multiValueArgs ARGS FOOTER HEADER METADATA SOURCES VARIABLE DEPENDS)

		#
		# The following variables will be set in that scope:
		#   _pandoc_arguments		- List of all arguments that will passed to pandoc invocation.
		#   _pandoc_depends		- List of all dependencies attached to the add_custom_command.
		#   _pandoc_mkdir		- The mkdir command if MAKE_DIRECTORY is set
		#   _pandoc_output_base		- The base output directory
		#
		cmake_parse_arguments(PANDOC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

		#
		# Output and sources are mandatory
		#
		if (NOT PANDOC_OUTPUT)
			message(FATAL_ERROR "Please define OUTPUT")
		elseif (NOT PANDOC_SOURCES)
			message(FATAL_ERROR "Please defines SOURCES")
		endif ()

		#
		# Handle the filter with care.
		#
		# 1. If it is a target, depend on it and use a generator
		#    expression to get its full path on the disk.
		# 2. If it is not a target, just use the user provided path.
		#
		if (PANDOC_FILTER)
			# If it is a target, add a dependency so that it is built
			if (TARGET ${PANDOC_FILTER})
				list(APPEND _pandoc_arguments --filter "$<TARGET_FILE:${PANDOC_FILTER}>")
				list(APPEND _pandoc_depends ${PANDOC_FILTER})
			else ()
				list(APPEND _pandoc_arguments --filter ${PANDOC_FILTER})
			endif ()
		endif ()

		if (PANDOC_TOC)
			list(APPEND _pandoc_arguments --toc)
		endif ()
		if (PANDOC_STANDALONE)
			list(APPEND _pandoc_arguments -s)
		endif ()
		if (PANDOC_FROM)
			list(APPEND _pandoc_arguments -f ${PANDOC_FROM})
		endif ()
		if (PANDOC_TO)
			list(APPEND _pandoc_arguments -t ${PANDOC_TO})
		endif ()
		if (PANDOC_TEMPLATE)
			list(APPEND _pandoc_arguments --template ${PANDOC_TEMPLATE})
			list(APPEND _pandoc_depends ${PANDOC_TEMPLATE})
		endif ()

		# Header, footers and body
		foreach (h ${PANDOC_HEADER})
			list(APPEND _pandoc_arguments -H ${h})
			list(APPEND _pandoc_depends ${h})
		endforeach ()
		foreach (b ${PANDOC_BODY})
			list(APPEND _pandoc_arguments -B ${b})
			list(APPEND _pandoc_depends ${b})
		endforeach ()
		foreach (f ${PANDOC_FOOTER})
			list(APPEND _pandoc_arguments -A ${f})
			list(APPEND _pandoc_depends ${f})
		endforeach ()

		# Variables and metadata
		foreach (var ${PANDOC_VARIABLE})
			list(APPEND _pandoc_arguments -V ${var})
		endforeach ()
		foreach (meta ${PANDOC_METADATA})
			list(APPEND _pandoc_arguments -M ${meta})
		endforeach ()

		# Optional list of arguments
		foreach (arg ${PANDOC_ARGS})
			list(APPEND _pandoc_arguments ${arg})
		endforeach ()

		# Output and sources
		list(APPEND _pandoc_arguments -o ${PANDOC_OUTPUT})

		#
		# The following variables are set within the loop:
		#
		#   _pandoc_input		- The absolute path to the input file.
		#   _pandoc_output_base		- The base output directory.
		#
		foreach (s ${PANDOC_SOURCES})
			get_filename_component(_pandoc_input ${s} ABSOLUTE)
			get_filename_component(_pandoc_output_base ${PANDOC_OUTPUT} DIRECTORY)
			list(APPEND _pandoc_depends ${_pandoc_input})
			list(APPEND _pandoc_arguments ${_pandoc_input})
		endforeach ()

		# Create the output directory if requested
		if (PANDOC_MAKE_DIRECTORY)
			set(_pandoc_mkdir ${CMAKE_COMMAND} -E make_directory ${_pandoc_output_base})
		endif ()

		add_custom_command(
			OUTPUT ${PANDOC_OUTPUT}
			COMMAND	${_pandoc_mkdir}
			COMMAND	${Pandoc_EXECUTABLE} ${_pandoc_arguments}
			DEPENDS ${_pandoc_depends} ${PANDOC_DEPENDS}
			WORKING_DIRECTORY ${PANDOC_WORKING_DIRECTORY}
			VERBATIM
		)

		if (PANDOC_TARGET)
			add_custom_target(
				${PANDOC_TARGET} ${PANDOC_ALL}
				SOURCES ${_pandoc_depends}
				DEPENDS ${PANDOC_OUTPUT}
				WORKING_DIRECTORY ${PANDOC_WORKING_DIRECTORY}
			)
		endif ()
	endfunction()
endif ()

mark_as_advanced(Pandoc_EXECUTABLE)

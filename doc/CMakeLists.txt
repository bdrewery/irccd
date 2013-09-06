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

option(WITH_LDOC "Install Lua API documentation" On)
option(WITH_DOC "Install user guides" On)

# Manual pages on Windows are pretty useless
if (WIN32)
	set(USE_MAN "No")
else ()
	set(USE_MAN "Yes")
endif ()

option(WITH_MAN "Install man pages" ${USE_MAN})

# Update man pages date
set(MONTH "September")
set(DAY 5)
set(YEAR 2013)

function(DEFINE_MAN file man)
	if (WITH_MAN)
		# Remove .in end
		string(REGEX REPLACE "(.*)\\.in$" "\\1" output ${file})
		configure_file("man/${file}" "${CMAKE_BINARY_DIR}/${output}")

		install(
			FILES "${CMAKE_BINARY_DIR}/${output}"
			DESTINATION "${MANDIR}/${man}"
		)
	endif()
endfunction()

DEFINE_MAN(irccd.1.in man1)
DEFINE_MAN(irccd.conf.5.in man5)
DEFINE_MAN(irccdctl.1.in man1)
DEFINE_MAN(irccdctl.conf.5.in man5)

find_file(
	LDOC ldoc
	NAMES ldoc ldoc.lua
)

find_file(
	ASCIIDOC asciidoc
	NAMES asciidoc asciidoc.py
)

#
# Install Lua API
#
if (WITH_LDOC)
	if (LDOC)
		# Add a target make generate-ldoc for
		# developers.
		add_custom_target(
			generate-ldoc
			COMMAND ${LDOC} -q .
			    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/doc/ldoc"
			COMMAND cmake -E copy
			    "${CMAKE_CURRENT_SOURCE_DIR}/ldoc/style/logo.png"
			    "${CMAKE_CURRENT_SOURCE_DIR}/generated/lua"
			COMMAND cmake -E copy
			    "${CMAKE_CURRENT_SOURCE_DIR}/ldoc/style/robot.png"
			    "${CMAKE_CURRENT_SOURCE_DIR}/generated/lua/"
		)

	endif ()

	install(
		DIRECTORY "generated/lua"
		DESTINATION "${DOCDIR}"
	)
endif ()

#
# User and developer guides.
#
if (WITH_DOC)
	if (ASCIIDOC)
		# Macro to define new guides to be processed.
		macro(DEFINE_GUIDE file)
			string(REGEX REPLACE "(.*)\\.txt$" "\\1" output ${file})

			list(
				APPEND
				COMMANDS
				    COMMAND ${ASCIIDOC}
				    -b html5
				    -a themedir="${CMAKE_CURRENT_SOURCE_DIR}/guides/themes/irccd"
				    -a theme=irccd
				    -o "${CMAKE_CURRENT_SOURCE_DIR}/generated/guides/${output}.html"
				       "${CMAKE_CURRENT_SOURCE_DIR}/guides/${file}"
				)
		endmacro ()

		DEFINE_GUIDE(user-guide.txt)
		DEFINE_GUIDE(plugin-antiflood.txt)	# antiflood.lua
		DEFINE_GUIDE(plugin-ask.txt)		# ask.lua
		DEFINE_GUIDE(plugin-badwords.txt)	# badwords.lua
		DEFINE_GUIDE(plugin-history.txt)	# history.lua
		DEFINE_GUIDE(plugin-logger.txt)		# logger.lua
		DEFINE_GUIDE(plugin-roulette.txt)	# roulette.lua

		add_custom_target(
			generate-doc
			COMMAND cmake -E make_directory
			    "${CMAKE_CURRENT_SOURCE_DIR}/generated/guides"
			COMMAND cmake -E make_directory
			    "${CMAKE_CURRENT_SOURCE_DIR}/generated/guides/icons"
			COMMAND cmake -E copy_directory
			    "${CMAKE_CURRENT_SOURCE_DIR}/guides/icons/"
			    "${CMAKE_CURRENT_SOURCE_DIR}/generated/guides/icons/"
			${COMMANDS}
		)
	endif ()

	install(
		DIRECTORY "generated/guides"
		DESTINATION "${DOCDIR}"
	)
endif ()

# vim: set syntax=cmake:

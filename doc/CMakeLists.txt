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
set(DAY 10)
set(YEAR 2013)

function(define_man file man)
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

define_man(irccd.1.in man1)
define_man(irccd.conf.5.in man5)
define_man(irccdctl.1.in man1)
define_man(irccdctl.conf.5.in man5)

find_file(
	LDOC ldoc
	NAMES ldoc ldoc.lua
)

find_file(
	ASCIIDOC asciidoc
	NAMES asciidoc asciidoc.py
)


#
# Create the following installation hierarchy
#
# StartupMenu\Irccd\Documentation
# StartupMenu\Irccd\Documentation\Plugins
#
list(
	APPEND
	EXTRA_INSTALL_COMMANDS
	"
	  CreateDirectory \\\"$SMPROGRAMS\\\\${CPACK_NSIS_PACKAGE_NAME}\\\\Documentation\\\"
	  CreateDirectory \\\"$SMPROGRAMS\\\\${CPACK_NSIS_PACKAGE_NAME}\\\\Documentation\\\\Plugins\\\"
	"
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

	#
	# Add documentation to installation
	#
	if (WIN32)
		list(
			APPEND
			EXTRA_INSTALL_COMMANDS
			"CreateShortCut \\\"$SMPROGRAMS\\\\${CPACK_NSIS_PACKAGE_NAME}\\\\Documentation\\\\Lua API.lnk\\\" \\\"$INSTDIR\\\\doc\\\\lua\\\\index.html\\\""
		)

		list(
			APPEND
			EXTRA_UNINSTALL_COMMANDS
			"Delete \\\"$SMPROGRAMS\\\\${CPACK_NSIS_PACKAGE_NAME}\\\\Documentation\\\\Lua API.lnk\\\""
		)
	endif ()
endif ()

#
# User and developer guides.
#
if (WITH_DOC)
	# Macro to define new guides to be processed.
	macro(define_guide file name isplugin)
		string(REGEX REPLACE "(.*)\\.txt$" "\\1" output ${file})

		if (ASCIIDOC)
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
		endif ()

		# Install the documentations
		if (${isplugin})
			set(smprefix "Plugins\\\\")
		endif ()

		list(
			APPEND
			EXTRA_INSTALL_COMMANDS
			"CreateShortCut \\\"$SMPROGRAMS\\\\${CPACK_NSIS_PACKAGE_NAME}\\\\Documentation\\\\${smprefix}${name}.lnk\\\" \\\"$INSTDIR\\\\doc\\\\guides\\\\${output}.html\\\""
		)

		list(
			APPEND
			EXTRA_UNINSTALL_COMMANDS
			"Delete \\\"$SMPROGRAMS\\\\${CPACK_NSIS_PACKAGE_NAME}\\\\Documentation\\\\${smprefix}${name}.lnk\\\""
		)

		unset(output)
		unset(smprefix)
	endmacro ()

	define_guide(user-guide.txt "User guide" false)
	define_guide(socket-guide.txt "Socket guide" false)
	define_guide(plugin-antiflood.txt "Antiflood" true)		# antiflood.lua
	define_guide(plugin-ask.txt "Ask" true)				# ask.lua
	define_guide(plugin-auth.txt "Auth" true)			# auth.lua
	define_guide(plugin-badwords.txt "Badwords" true)		# badwords.lua
	define_guide(plugin-history.txt "History" true)			# history.lua
	define_guide(plugin-logger.txt "Logger" true)			# logger.lua
	define_guide(plugin-roulette.txt "Roulette" true)		# roulette.lua

	#
	# Regenerate the documentation inside using make generate-doc
	#
	if (ASCIIDOC)
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

#
# Delete the documentation from startup menu after the objects
#
list(
	APPEND
	EXTRA_UNINSTALL_COMMANDS
	"
	  RMDir \\\"$SMPROGRAMS\\\\${CPACK_NSIS_PACKAGE_NAME}\\\\Documentation\\\\Plugins\\\"
	  RMDir \\\"$SMPROGRAMS\\\\${CPACK_NSIS_PACKAGE_NAME}\\\\Documentation\\\"
	"
)

# Set them to parent scope so NSIS can generate well
string(REPLACE ";" "\n" EXTRA_INSTALL_COMMANDS "${EXTRA_INSTALL_COMMANDS}")
string(REPLACE ";" "\n" EXTRA_UNINSTALL_COMMANDS "${EXTRA_UNINSTALL_COMMANDS}")

set(EXTRA_INSTALL_COMMANDS ${EXTRA_INSTALL_COMMANDS} PARENT_SCOPE)
set(EXTRA_UNINSTALL_COMMANDS ${EXTRA_UNINSTALL_COMMANDS} PARENT_SCOPE)

# vim: set syntax=cmake:

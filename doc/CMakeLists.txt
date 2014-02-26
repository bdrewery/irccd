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

project(doc)

# ---------------------------------------------------------
# Install manuals
# ---------------------------------------------------------

# Update man pages date
set(MONTH "January")
set(DAY 30)
set(YEAR 2014)

define_man(man/irccd.1.in man1)
define_man(man/irccd.conf.5.in man5)
define_man(man/irccdctl.1.in man1)
define_man(man/irccdctl.conf.5.in man5)

# ---------------------------------------------------------
# Create a target for building Lua API documentation
# ---------------------------------------------------------

if(LDOC)
	add_custom_target(
		generate-ldoc
		COMMENT "Generating Lua API documentation"
		COMMAND ${LDOC} -q .
			WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/doc/ldoc"
		COMMAND cmake -E copy
			"${CMAKE_CURRENT_SOURCE_DIR}/ldoc/style/logo.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/generated/lua"
		COMMAND cmake -E copy
			"${CMAKE_CURRENT_SOURCE_DIR}/ldoc/style/robot.png"
			"${CMAKE_CURRENT_SOURCE_DIR}/generated/lua/"
	)
endif()

# ---------------------------------------------------------
# Create asciidoc files and guides
# ---------------------------------------------------------

#
# Create an empty target that we will fill with more dependencies with
# the define_guide function
#
if(ASCIIDOC)
	add_custom_target(
		generate-doc
		COMMENT "Generated .html files from .txt"
		COMMAND cmake -E make_directory
			"${CMAKE_CURRENT_SOURCE_DIR}/generated/guides"
		COMMAND cmake -E make_directory
			"${CMAKE_CURRENT_SOURCE_DIR}/generated/guides/icons"
		COMMAND cmake -E copy_directory
			"${CMAKE_CURRENT_SOURCE_DIR}/guides/icons/"
			"${CMAKE_CURRENT_SOURCE_DIR}/generated/guides/icons/"
	)
endif()

define_guide(user-guide "User guide")
define_guide(socket-guide "Socket guide")

# ---------------------------------------------------------
# Install some directories
# ---------------------------------------------------------

if(WITH_DOC)
	install(
		DIRECTORY "generated/guides/icons"
		DESTINATION "${DOCDIR}/guides"
	)

	#
	# Create the following installation hierarchy
	#
	# StartupMenu\Irccd\Documentation
	# StartupMenu\Irccd\Documentation\Plugins
	#
	if(WIN32)
		list(
			APPEND
			EXTRA_INSTALL_COMMANDS
			"
			  CreateDirectory \\\"$SMPROGRAMS\\\\${IRCCD_PACKAGE_NAME}\\\\Documentation\\\"
			  CreateDirectory \\\"$SMPROGRAMS\\\\${IRCCD_PACKAGE_NAME}\\\\Documentation\\\\Plugins\\\"
			"
		)
	endif()
endif()

if(WITH_LDOC)
	install(
		DIRECTORY "generated/lua"
		DESTINATION "${DOCDIR}"
	)

	#
	# Add documentation to installation
	#
	if(WIN32)
		add_nsis_install(
			"CreateShortCut \\\"$SMPROGRAMS\\\\${IRCCD_PACKAGE_NAME}\\\\Documentation\\\\Lua API.lnk\\\" \\\"$INSTDIR\\\\doc\\\\lua\\\\index.html\\\""
		)

		add_nsis_uninstall(
			"Delete \\\"$SMPROGRAMS\\\\${IRCCD_PACKAGE_NAME}\\\\Documentation\\\\Lua API.lnk\\\""
		)
	endif()
endif()

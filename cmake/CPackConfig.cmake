#
# CPackConfing.cmake -- CMake build system for irccd
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

#
# Delete the documentation from startup menu after the objects
#
if(WIN32)
	add_nsis_uninstall(
		"
		  RMDir \\\"$SMPROGRAMS\\\\${IRCCD_PACKAGE_NAME}\\\\Documentation\\\\Plugins\\\"
		  RMDir \\\"$SMPROGRAMS\\\\${IRCCD_PACKAGE_NAME}\\\\Documentation\\\"
		"
	)
endif()

if (WIN32)
	message("${IRCCD_PACKAGE_NAME}")
	set(CPACK_NSIS_PACKAGE_NAME ${IRCCD_PACKAGE_NAME})

	if(CMAKE_CL_64)
		set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
		set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION} (x64)")
	else()
		set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
		set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION}")
	endif()

	# This determine the *target* architecture
	if (CMAKE_SIZEOF_VOID_P MATCHES "8")
		set(WINARCH "amd64")
	else ()
		set(WINARCH "x86")
	endif ()

	# Embed Visual C++ 2012 redistributable
	if (${WINARCH} MATCHES "amd64")
		set(REDIST_FILE "vcredist_x64.exe")
	else ()
		set(REDIST_FILE "vcredist_x86.exe")
	endif ()

	install(
		PROGRAMS
		"${CMAKE_SOURCE_DIR}/win32/${REDIST_FILE}"
		DESTINATION tmp
	)

	list(
		APPEND
		CPACK_NSIS_EXTRA_INSTALL_COMMANDS
		"ExecWait '$INSTDIR\\\\tmp\\\\${REDIST_FILE} /passive'"
	)

	set(CPACK_GENERATOR "NSIS")
	set(CPACK_MONOLITHIC_INSTALL FALSE)

	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
	set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README")

	set(CPACK_NSIS_DISPLAY_NAME "Irccd")
	set(CPACK_NSIS_EXECUTABLES_DIRECTORY "bin")
	set(CPACK_NSIS_CONTACT "demelier.david@gmail.com")

	set(CPACK_PACKAGE_VENDOR "Malikania")
	set(CPACK_PACKAGE_VERSION "${VERSION}")
	set(CPACK_PACKAGE_VERSION_MAJOR ${MAJOR})
	set(CPACK_PACKAGE_VERSION_MINOR ${MINOR})
	set(CPACK_PACKAGE_FILE_NAME "irccd-${VERSION}-${CMAKE_SYSTEM_NAME}-${WINARCH}")
	set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Irccd")

	# Read from file the extra commands
	file(READ ${CMAKE_BINARY_DIR}/nsis_extra_install.txt INSTALL_COMMANDS)
	file(READ ${CMAKE_BINARY_DIR}/nsis_extra_uninstall.txt UNINSTALL_COMMANDS)

	# Startup menu for documentation
	set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS ${INSTALL_COMMANDS})
	set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS ${UNINSTALL_COMMANDS})

	# Wizards images (currently not working)
	set(CPACK_PACKAGE_NAME "Irccd")
	set(CPACK_NSIS_WELCOME_BITMAP "${CMAKE_SOURCE_DIR}/win32/left.bmp")
	set(CPACK_NSIS_HEADER_BITMAP "${CMAKE_SOURCE_DIR}/win32/top.bmp")
else ()
	set(CPACK_GENERATOR "TGZ")
endif ()

set(CPACK_SOURCE_PACKAGE_FILE_NAME "irccd-${VERSION}-source")
set(CPACK_SOURCE_GENERATOR "ZIP;TGZ")
set(CPACK_SOURCE_IGNORE_FILES ".hg;_build_")

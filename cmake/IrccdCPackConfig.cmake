#
# CPackConfing.cmake -- CMake build system for irccd
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

#
# Irccd uses InnoSetup instead of NSIS so we are using CPack only for
# source package, we do provide package_inno target though.
#

set(
	plugins
	antiflood
	ask
	auth
	badwords
	date
	history
	logger
	plugin
	reminder
	roulette
)

if (WIN32)
	set(IRCCD_PROVIDE_INNO TRUE)

	if (NOT InnoSetup_FOUND)
		set(IRCCD_PROVIDE_INNO FALSE)
		message("Note: InnoSetup not found, no package_inno target provided")
	elseif (NOT IRCCD_RELOCATABLE)
		set(IRCCD_PROVIDE_INNO FALSE)
		# No need to write a warning, the main CMakeLists already did.
	elseif (NOT WITH_DOCS_JS)
		set(IRCCD_PROVIDE_INNO FALSE)
		message("Note: JavaScript documentation disabled, no package_inno target provided")
	elseif (NOT WITH_DOCS_DOXYGEN)
		set(IRCCD_PROVIDE_INNO FALSE)
		message("Note: Doxygen documentation disabled, no package_inno target provided")
	elseif (NOT WITH_DOCS_GUIDES_PDF)
		set(IRCCD_PROVIDE_INNO FALSE)
		message("Note: PDF guides documentation disabled, no package_inno target provided")
	elseif (NOT WITH_DOCS_GUIDES_HTML)
		set(IRCCD_PROVIDE_INNO FALSE)
		message("Note: HTML guides documentation disabled, no package_inno target provided")
	endif ()
else ()
	set(IRCCD_PROVIDE_INNO FALSE)
endif ()

if (IRCCD_PROVIDE_INNO)
	macro(irccd_list_to_inno list var)
		#
		# We can't replace ';' directly because it is a reserved
		# character in InnoSetup syntax.
		#
		foreach (l ${list})
			set(${var} "${${var}}${l}\n")
		endforeach()
	endmacro ()

	if (IRCCD_64BIT)
		set(IRCCD_PACKAGE_NAME "Irccd (x64)")
		set(IRCCD_PACKAGE_FILENAME "irccd-Windows-amd64-${IRCCD_VERSION}")
		set(IRCCD_PACKAGE_SETUP_EXTRA "ArchitecturesAllowed=x64\nArchitecturesInstallIn64BitMode=x64")

		set(
			DLL_FILES
			"Source: \"${CMAKE_SOURCE_DIR}/win32/amd64/libeay32.dll\"\; DestDir: \"{app}\\\\bin\""
			"Source: \"${CMAKE_SOURCE_DIR}/win32/amd64/libgcc_s_seh-1.dll\"\; DestDir: \"{app}\\\\bin\""
			"Source: \"${CMAKE_SOURCE_DIR}/win32/amd64/libstdc++-6.dll\"\; DestDir: \"{app}\\\\bin\""
			"Source: \"${CMAKE_SOURCE_DIR}/win32/amd64/libwinpthread-1.dll\"\; DestDir: \"{app}\\\\bin\""
			"Source: \"${CMAKE_SOURCE_DIR}/win32/amd64/ssleay32.dll\"\; DestDir: \"{app}\\\\bin\""
		)
	else ()
		set(IRCCD_PACKAGE_NAME "Irccd")
		set(IRCCD_PACKAGE_FILENAME "irccd-Windows-x86-${IRCCD_VERSION}")

		set(
			DLL_FILES
			"Source: \"${CMAKE_SOURCE_DIR}/win32/x86/libeay32.dll\"\; DestDir: \"{app}\\\\bin\""
			"Source: \"${CMAKE_SOURCE_DIR}/win32/x86/libgcc_s_dw2-1.dll\"\; DestDir: \"{app}\\\\bin\""
			"Source: \"${CMAKE_SOURCE_DIR}/win32/x86/libstdc++-6.dll\"\; DestDir: \"{app}\\\\bin\""
			"Source: \"${CMAKE_SOURCE_DIR}/win32/x86/libwinpthread-1.dll\"\; DestDir: \"{app}\\\\bin\""
			"Source: \"${CMAKE_SOURCE_DIR}/win32/x86/ssleay32.dll\"\; DestDir: \"{app}\\\\bin\""
		)
	endif ()

	foreach (p ${plugins})
		list(
			APPEND
			PLUGIN_COMPONENTS
			"Name: \"plugins\\\\${p}\"\; Description: \"Irccd ${p} plugin\"\; Types: full"
		)

		list(
			APPEND
			PLUGIN_FILES
			"Source: \"${CMAKE_SOURCE_DIR}/plugins/${p}.lua\"\; DestDir: \"{app}\\\\${MODDIR}\"\; Components: plugins\\\\${p}"
		)
	endforeach ()

	irccd_list_to_inno("${PLUGIN_COMPONENTS}" IRCCD_PACKAGE_COMPONENTS_EXTRA)
	irccd_list_to_inno("${PLUGIN_FILES}" IRCCD_PACKAGE_FILES_EXTRA)
	irccd_list_to_inno("${DLL_FILES}" IRCCD_PACKAGE_DLL_EXTRA)

	configure_file(
		${CMAKE_SOURCE_DIR}/cmake/internal/setup.iss.in
		${CMAKE_BINARY_DIR}/setup.iss
	)

	add_custom_target(
		package_inno
		DEPENDS
			irccd
			irccdctl
			docs
			${CMAKE_BINARY_DIR}/setup.iss
		COMMAND	${InnoSetup_COMPILER} ${CMAKE_BINARY_DIR}/setup.iss
	)
endif ()

set(CPACK_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "irccd-${IRCCD_VERSION}")
set(CPACK_SOURCE_GENERATOR "ZIP;TGZ")
set(CPACK_SOURCE_IGNORE_FILES ".hg;_build_")

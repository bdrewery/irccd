#
# CMakeLists.txt -- CMake build system for irccd
#
# Copyright (c) 2013 David Demelier <markand@malikania.fr>
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

# General
cmake_minimum_required(VERSION 2.8)

# Project
project(irccd)

set(MAJOR "1")
set(MINOR "1")
set(PATCH "3")
set(VERSION "${MAJOR}.${MINOR}.${PATCH}")

# Compatibility options
option(COMPAT_1_0 "Be compatible with 1.0" On)

# Default to Release
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif ()

# General settings
if (UNIX)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++11")

	# FreeBSD use clang
	if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
	endif ()
elseif (WIN32)
	add_definitions("-D_CRT_SECURE_NO_WARNINGS")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
endif ()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")			# own modules

#
# Installation paths. On Windows, we just use the suffix relative
# to the installation path.
#
if (WIN32)
	set(MODDIR "plugins"
	    CACHE STRING "Module prefix where to install")
	set(DOCDIR "doc"
	    CACHE STRING "Documentation directory")
	set(MANDIR "man"
	    CACHE STRING "Man directory")
	set(ETCDIR "etc"
	    CACHE STRING "Configuration directory")
else ()
	set(MODDIR "share/irccd/plugins"
	    CACHE STRING "Module prefix where to install")
	set(DOCDIR "share/doc/irccd"
	    CACHE STRING "Documentation directory")
	set(MANDIR "share/man"
	    CACHE STRING "Man directory")
	set(ETCDIR "etc"
	    CACHE STRING "Configuration directory")
endif ()

# Include path to common
include_directories(
	"${CMAKE_BINARY_DIR}"
	"${CMAKE_SOURCE_DIR}/common"
)

# Some non portable code
set(EXTERNDIR "${CMAKE_SOURCE_DIR}/extern")

include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckStructHasMember)
include(CheckSymbolExists)

# Check of getopt(3) function.
check_function_exists(getopt HAVE_GETOPT)
if (NOT HAVE_GETOPT)
	include_directories("${EXTERNDIR}/getopt")
	list(
		APPEND
		EXTSOURCES
		"${EXTERNDIR}/getopt/getopt.c"
		"${EXTERNDIR}/getopt/getopt.h"
	)
endif ()

# Check of setprogname(3) function.
check_function_exists(setprogname HAVE_SETPROGNAME)
if (NOT HAVE_SETPROGNAME)
	include_directories("${EXTERNDIR}/setprogname")
	list(
		APPEND
		EXTSOURCES
		"${EXTERNDIR}/setprogname/setprogname.c"
		"${EXTERNDIR}/setprogname/setprogname.h"
	)
endif ()

# unistd.h has some useful routines.
check_include_file(unistd.h HAVE_UNISTD_H)

# Check for struct stat fields.
check_struct_has_member("struct stat" st_dev sys/stat.h HAVE_STAT_ST_DEV)
check_struct_has_member("struct stat" st_ino sys/stat.h HAVE_STAT_ST_INO)
check_struct_has_member("struct stat" st_nlink sys/stat.h HAVE_STAT_ST_NLINK)
check_struct_has_member("struct stat" st_uid sys/stat.h HAVE_STAT_ST_UID)
check_struct_has_member("struct stat" st_gid sys/stat.h HAVE_STAT_ST_GID)
check_struct_has_member("struct stat" st_atime sys/stat.h HAVE_STAT_ST_ATIME)
check_struct_has_member("struct stat" st_mtime sys/stat.h HAVE_STAT_ST_MTIME)
check_struct_has_member("struct stat" st_ctime sys/stat.h HAVE_STAT_ST_CTIME)
check_struct_has_member("struct stat" st_size sys/stat.h HAVE_STAT_ST_SIZE)
check_struct_has_member("struct stat" st_blksize sys/stat.h HAVE_STAT_ST_BLKSIZE)
check_struct_has_member("struct stat" st_blocks sys/stat.h HAVE_STAT_ST_BLOCKS)

# These settings are needed through doc/ and CPack.
if (WIN32)
	if (CMAKE_CL_64)
		SET(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
		SET(CPACK_NSIS_PACKAGE_NAME "Irccd (x64)")
		SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION} (x64)")
	else ()
		SET(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
		SET(CPACK_NSIS_PACKAGE_NAME "Irccd")
		SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION}")
	endif ()
endif ()

# For documentation and man pages
add_subdirectory(doc)

# Common code (static library)
add_subdirectory(common)

# irccd main application
add_subdirectory(irccd)

# irccd controller
add_subdirectory(irccdctl)

# Contributions
add_subdirectory(contrib)

# Tests
include(CTest)
add_subdirectory(tests)

message("Installation directories:")
message("\tMANDIR\t\t\t${CMAKE_INSTALL_PREFIX}/${MANDIR}")
message("\tDOCDIR\t\t\t${CMAKE_INSTALL_PREFIX}/${DOCDIR}")
message("\tMODDIR\t\t\t${CMAKE_INSTALL_PREFIX}/${MODDIR}")
message("")

message("Compiling irccd with following options:")
message("\tBuild Type:\t\t${CMAKE_BUILD_TYPE}")
message("\tLua support:\t\t${LUA_SUPPORT}")
message("\tTests:\t\t\t${UNIT_TESTS}")
message("\tirccd config:\t\t${CMAKE_INSTALL_PREFIX}/${ETCDIR}/irccd.conf")
message("\tirccdctl config:\t${CMAKE_INSTALL_PREFIX}/${ETCDIR}/irccdctl.conf")
message("")

# Configure the file after so every project can set more variables
# if needed
configure_file(
	"${CMAKE_SOURCE_DIR}/cmake/config.h.in"
	"${CMAKE_BINARY_DIR}/config.h"
)

# CPack
if (WIN32)
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
		"${CMAKE_CURRENT_SOURCE_DIR}/win32/${REDIST_FILE}"
	        DESTINATION tmp
	)

	list(
		APPEND CPACK_NSIS_EXTRA_INSTALL_COMMANDS
		"ExecWait '$INSTDIR\\\\tmp\\\\${REDIST_FILE} /passive'"
	)

	set(CPACK_GENERATOR "NSIS")
	set(CPACK_MONOLITHIC_INSTALL FALSE)

	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
	set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README")

	set(CPACK_NSIS_DISPLAY_NAME "Irccd")
	set(CPACK_NSIS_EXECUTABLES_DIRECTORY "bin")
	set(CPACK_NSIS_CONTACT "demelier.david@gmail.com")

	set(CPACK_PACKAGE_VENDOR "Malikania")
	set(CPACK_PACKAGE_VERSION "${VERSION}")
	set(CPACK_PACKAGE_VERSION_MAJOR ${MAJOR})
	set(CPACK_PACKAGE_VERSION_MINOR ${MINOR})
	set(CPACK_PACKAGE_FILE_NAME "irccd-${VERSION}-${CMAKE_SYSTEM_NAME}-${WINARCH}")
	set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Irccd")

	# Startup menu for documentation
	set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS ${EXTRA_INSTALL_COMMANDS})
	set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS ${EXTRA_UNINSTALL_COMMANDS})

	# Wizards images (currently not working)
	set(CPACK_PACKAGE_NAME "Irccd")
	set(CPACK_NSIS_WELCOME_BITMAP "${CMAKE_CURRENT_SOURCE_DIR}/win32/left.bmp")
	set(CPACK_NSIS_HEADER_BITMAP "${CMAKE_CURRENT_SOURCE_DIR}/win32/top.bmp")
else ()
	set(CPACK_GENERATOR "TGZ")
endif ()

set(CPACK_SOURCE_PACKAGE_FILE_NAME "irccd-${VERSION}-source")
set(CPACK_SOURCE_GENERATOR "ZIP;TGZ")
set(CPACK_SOURCE_IGNORE_FILES ".hg;_build_")

include(CPack)

# vim: set syntax=cmake:

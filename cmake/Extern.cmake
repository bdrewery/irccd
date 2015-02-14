#
# Extern.cmake -- CMake build system for irccd
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

set(EXTERNDIR ${CMAKE_SOURCE_DIR}/extern)

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
		${EXTERNDIR}/getopt/getopt.c
		${EXTERNDIR}/getopt/getopt.h
	)

	list(
		APPEND
		EXTINCLUDES
		${EXTERNDIR}/getopt
	)
endif ()

# Check of setprogname(3) function.
check_function_exists(setprogname HAVE_SETPROGNAME)
if (NOT HAVE_SETPROGNAME)
	include_directories("${EXTERNDIR}/setprogname")
	list(
		APPEND
		EXTSOURCES
		${EXTERNDIR}/setprogname/setprogname.c
		${EXTERNDIR}/setprogname/setprogname.h
	)

	list(
		APPEND
		EXTINCLUDES
		${EXTERNDIR}/setprogname
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

set(EXTSOURCES ${EXTSOURCES} PARENT_SCOPE)
set(EXTINCLUDES ${EXTINCLUDES} PARENT_SCOPE)

add_subdirectory(
	"${CMAKE_SOURCE_DIR}/extern/libircclient"
	"${CMAKE_BINARY_DIR}/libircclient"
)

if (UNIX)
	add_subdirectory(
		"${CMAKE_SOURCE_DIR}/extern/libxdg-basedir"
		"${CMAKE_BINARY_DIR}/libxdg-basedir"
	)
endif ()

add_subdirectory(
	"${CMAKE_SOURCE_DIR}/extern/gtest"
	"${CMAKE_BINARY_DIR}/gtest"
)

#
# Config.cmake -- CMake build system for irccd
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

include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckStructHasMember)
include(CheckSymbolExists)
include(CheckTypeSize)

if (WIN32)
	find_package(InnoSetup)
endif ()

# ---------------------------------------------------------
# Global compile flags
# ---------------------------------------------------------

if (UNIX)
	set(CMAKE_C_FLAGS "-Wall -Wextra ${CMAKE_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14 ${CMAKE_CXX_FLAGS}")
elseif (WIN32)
	if (MINGW)
		set(CMAKE_C_FLAGS "-Wall -Wextra -D_WIN32_WINNT=0x0600 ${CMAKE_C_FLAGS}")
		set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14 -D_WIN32_WINNT=0x0600 ${CMAKE_CXX_FLAGS}")
	else ()
		message(FATAL_ERROR "Irccd does not support Visual Studio yet")
	endif ()
endif ()

if (CMAKE_SIZEOF_VOID_P MATCHES "8")
	set(IRCCD_64BIT TRUE)
else ()
	set(IRCCD_64BIT FALSE)
endif ()

# ---------------------------------------------------------
# Portability requirements
# ---------------------------------------------------------

check_type_size(int8_t HAVE_INT8)
check_type_size(uint8_t HAVE_UINT8)
check_type_size(int16_t HAVE_INT16)
check_type_size(uint16_t HAVE_UINT16)
check_type_size(int32_t HAVE_INT32)
check_type_size(uint32_t HAVE_UINT32)
check_type_size(int64_t HAVE_INT64)
check_type_size(uint64_t HAVE_UINT64)

if (NOT HAVE_STDINT_H)
	message("irccd requires stdint.h or cstdint header")
endif ()

# ---------------------------------------------------------
# Port library
# ---------------------------------------------------------

#
# The CMake build system adds a static port library which add symbols and
# path to includes directories to wrap missing features.
#
# Where any of these function / feature is required, include the
# <IrccdConfig.h> file.
#
# The following variables are defined in IrccdConfig.h
#
# HAVE_GETOPT		- True if getopt(3) is available from C library,
# HAVE_SETPROGNAME	- True if setprogname(3) is available from C library,
# HAVE_SYS_STAT_H	- True if sys/stat.h is available
# HAVE_STAT		- True if stat() is also available
# HAVE_STAT_ST_DEV	- The struct stat has st_dev field,
# HAVE_STAT_ST_INO	- The struct stat has st_ino field,
# HAVE_STAT_ST_NLINK	- The struct stat has st_nlink field,
# HAVE_STAT_ST_UID	- The struct stat has st_uid field,
# HAVE_STAT_ST_GID	- The struct stat has st_gid field,
# HAVE_STAT_ST_ATIME	- The struct stat has st_atime field,
# HAVE_STAT_ST_MTIME	- The struct stat has st_mtime field,
# HAVE_STAT_ST_CTIME	- The struct stat has st_ctime field,
# HAVE_STAT_ST_SIZE	- The struct stat has st_size field,
# HAVE_STAT_ST_BLKSIZE	- The struct stat has st_blksize field,
# HAVE_STAT_ST_BLOCKS	- The struct stat has st_blocks field.
#

# Check of getopt(3) function.
check_function_exists(getopt HAVE_GETOPT)

if (NOT HAVE_GETOPT)
	list(
		APPEND
		PORT_SOURCES
		${CMAKE_SOURCE_DIR}/extern/getopt/getopt.c
		${CMAKE_SOURCE_DIR}/extern/getopt/getopt.h
	)

	list(
		APPEND
		PORT_INCLUDES
		${CMAKE_SOURCE_DIR}/extern/getopt
	)
endif ()

# Check of setprogname(3) function.
check_function_exists(setprogname HAVE_SETPROGNAME)

if (NOT HAVE_SETPROGNAME)
	list(
		APPEND
		PORT_SOURCES
		${CMAKE_SOURCE_DIR}/extern/setprogname/setprogname.c
		${CMAKE_SOURCE_DIR}/extern/setprogname/setprogname.h
	)

	list(
		APPEND
		PORT_INCLUDES
		${CMAKE_SOURCE_DIR}/extern/setprogname
	)
endif ()

# access() POSIX function
check_function_exists(access HAVE_ACCESS)

# stat(2) function
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_function_exists(stat HAVE_STAT)

# Check for struct stat fields.
check_struct_has_member("struct stat" st_atime sys/stat.h HAVE_STAT_ST_ATIME)
check_struct_has_member("struct stat" st_blksize sys/stat.h HAVE_STAT_ST_BLKSIZE)
check_struct_has_member("struct stat" st_blocks sys/stat.h HAVE_STAT_ST_BLOCKS)
check_struct_has_member("struct stat" st_ctime sys/stat.h HAVE_STAT_ST_CTIME)
check_struct_has_member("struct stat" st_dev sys/stat.h HAVE_STAT_ST_DEV)
check_struct_has_member("struct stat" st_gid sys/stat.h HAVE_STAT_ST_GID)
check_struct_has_member("struct stat" st_ino sys/stat.h HAVE_STAT_ST_INO)
check_struct_has_member("struct stat" st_mode sys/stat.h HAVE_STAT_ST_MODE)
check_struct_has_member("struct stat" st_mtime sys/stat.h HAVE_STAT_ST_MTIME)
check_struct_has_member("struct stat" st_nlink sys/stat.h HAVE_STAT_ST_NLINK)
check_struct_has_member("struct stat" st_rdev sys/stat.h HAVE_STAT_ST_RDEV)
check_struct_has_member("struct stat" st_size sys/stat.h HAVE_STAT_ST_SIZE)
check_struct_has_member("struct stat" st_uid sys/stat.h HAVE_STAT_ST_UID)

# Configuration file
configure_file(
	${CMAKE_CURRENT_LIST_DIR}/internal/IrccdConfig.h.in
	${CMAKE_BINARY_DIR}/IrccdConfig.h
)

# Port library that every target links to
if (PORT_SOURCES)
	add_library(port STATIC ${PORT_SOURCES})

	target_include_directories(
		port
		PUBLIC
			${PORT_INCLUDES}
			${CMAKE_BINARY_DIR}
	)
else ()
	# dummy target
	set(port)
endif ()



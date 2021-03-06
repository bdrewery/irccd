#
# CMakeLists.txt -- CMake build system for irccd
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

# Libircclient bundled
project(extern-libircclient)

set(FLAGS "ENABLE_THREADS")

# Portability checks
include(CheckFunctionExists)

check_function_exists(gethostbyname_r HAVE_GETHOSTBYNAME_R)
check_function_exists(localtime_r HAVE_LOCALTIME_R)

if (HAVE_GETHOSTBYNAME_R)
	list(APPEND FLAGS "HAVE_GETHOSTBYNAME_R")
endif ()
if (HAVE_LOCALTIME_R)
	list(APPEND FLAGS "HAVE_LOCALTIME_R")
endif ()

# SSL is optional
if (WITH_SSL)
	list(APPEND INCLUDES ${OPENSSL_INCLUDE_DIR})
	list(APPEND LIBRARIES ${OPENSSL_LIBRARIES})
	list(APPEND FLAGS "ENABLE_SSL")
endif ()

# Enable or disable IPv6
if (WITH_IPV6)
	list(APPEND FLAGS "ENABLE_IPV6")
endif ()

irccd_define_library(
	TARGET ircclient
	SOURCES src/libircclient.c
	FLAGS ${FLAGS}
	LIBRARIES ${LIBRARIES}
	PUBLIC_INCLUDES "${extern-libircclient_SOURCE_DIR}/include"
	LOCAL_INCLUDES ${INCLUDES}
)

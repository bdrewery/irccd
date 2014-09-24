#
# Config.cmake -- CMake build system for irccd
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

# ---------------------------------------------------------
# Global compile flags
# ---------------------------------------------------------

if (UNIX)
	set(C_FLAGS "-Wall -Wextra ${C_FLAGS}")
	set(CXX_FLAGS "-Wall -Wextra -std=c++14 ${CXX_FLAGS}")
elseif (WIN32)
	# MSVC not supported until Visual Studio 2014 (I hope)
	if (MINGW)
		set(C_FLAGS "-Wall -Wextra ${C_FLAGS}")
		set(CXX_FLAGS "-Wall -Wextra -std=c++14 ${CXX_FLAGS}")
	else ()
		message(FATAL_ERROR "Irccd does not support Visual Studio yet")

		set(C_FLAGS "/D _CRT_SECURE_NO_WARNINGS ${C_FLAGS}")
		set(CXX_FLAGS "/D _CRT_SECURE_NO_WARNINGS ${CXX_FLAGS}")
	endif ()
endif ()

# Add global flags
set(CMAKE_C_FLAGS "${C_FLAGS} ${CMAKE_C_FLAGS}" PARENT_SCOPE)
set(CMAKE_CXX_FLAGS "${CXX_FLAGS} ${CMAKE_CXX_FLAGS}" PARENT_SCOPE)

# ---------------------------------------------------------
# Installation paths
# ---------------------------------------------------------

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

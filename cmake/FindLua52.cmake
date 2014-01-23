# Locate Lua library
# This module defines
#  LUA52_FOUND, if false, do not try to link to Lua 
#  LUA52_LIBRARIES
#  LUA52_INCLUDE_DIR, where to find lua.h
#  LUA52_VERSION_STRING, the version of Lua found (since CMake 2.8.8)
#
# Note that the expected include convention is
#  #include "lua.h"
# and not
#  #include <lua/lua.h>
# This is because, the lua location is not standardized and may exist
# in locations other than lua/

#=============================================================================
# Copyright 2007-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

find_path(LUA52_INCLUDE_DIR lua.h
	HINTS
	$ENV{LUA_DIR}
	PATH_SUFFIXES include/lua52 include/lua5.2 include/lua include
	PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
)

find_library(LUA52_LIBRARY
	NAMES lua52 lua5.2 lua-5.2 lua
	HINTS
	$ENV{LUA_DIR}
	PATH_SUFFIXES lib64 lib
	PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/sw
	/opt/local
	/opt/csw
	/opt
)

if (LUA52_LIBRARY)
	# include the math library for Unix
	if (UNIX AND NOT APPLE)
		FIND_LIBRARY(LUA52_MATH_LIBRARY m)
		SET(LUA52_LIBRARIES "${LUA52_LIBRARY};${LUA52_MATH_LIBRARY}" CACHE STRING "Lua Libraries")
	# For Windows and Mac, don't need to explicitly include the math library
	else (UNIX AND NOT APPLE)
		SET(LUA52_LIBRARIES "${LUA52_LIBRARY}" CACHE STRING "Lua Libraries")
	endif (UNIX AND NOT APPLE)
endif ()

if (LUA52_INCLUDE_DIR AND EXISTS "${LUA52_INCLUDE_DIR}/lua.h")
	file(STRINGS "${LUA52_INCLUDE_DIR}/lua.h" lua52_version_str REGEX "^#define[ \t]+LUA_RELEASE[ \t]+\"Lua .+\"")

	string(REGEX REPLACE "^#define[ \t]+LUA_RELEASE[ \t]+\"Lua ([^\"]+)\".*" "\\1" LUA52_VERSION_STRING "${lua52_version_str}")
	unset(lua52_version_str)
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LUA52_FOUND to TRUE if 
# all listed variables are TRUE
find_package_handle_standard_args(Lua52
                                  REQUIRED_VARS LUA52_LIBRARIES LUA52_INCLUDE_DIR
                                  VERSION_VAR LUA52_VERSION_STRING)

mark_as_advanced(LUA52_INCLUDE_DIR LUA52_LIBRARIES LUA52_LIBRARY LUA52_MATH_LIBRARY)

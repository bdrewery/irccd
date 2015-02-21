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

project(lua)

set(
	SOURCES
	src/lapi.cpp
	src/lapi.h
	src/lauxlib.cpp
	src/lauxlib.h
	src/lbaselib.cpp
	src/lbitlib.cpp
	src/lcode.cpp
	src/lcode.h
	src/lcorolib.cpp
	src/lctype.cpp
	src/lctype.h
	src/ldblib.cpp
	src/ldebug.cpp
	src/ldebug.h
	src/ldo.cpp
	src/ldo.h
	src/ldump.cpp
	src/lfunc.cpp
	src/lfunc.h
	src/lgc.cpp
	src/lgc.h
	src/linit.cpp
	src/liolib.cpp
	src/llex.cpp
	src/llex.h
	src/llimits.h
	src/lmathlib.cpp
	src/lmem.cpp
	src/lmem.h
	src/loadlib.cpp
	src/lobject.cpp
	src/lobject.h
	src/lopcodes.cpp
	src/lopcodes.h
	src/loslib.cpp
	src/lparser.cpp
	src/lparser.h
	src/lprefix.h
	src/lstate.cpp
	src/lstate.h
	src/lstring.cpp
	src/lstring.h
	src/lstrlib.cpp
	src/ltable.cpp
	src/ltable.h
	src/ltablib.cpp
	src/ltm.cpp
	src/ltm.h
	src/luaconf.h
	src/lua.h
	src/lua.hpp
	src/lualib.h
	src/lundump.cpp
	src/lundump.h
	src/lutf8lib.cpp
	src/lvm.cpp
	src/lvm.h
	src/lzio.cpp
	src/lzio.h
)

add_library(lua STATIC ${SOURCES})
target_include_directories(lua PUBLIC ${lua_SOURCE_DIR}/src)

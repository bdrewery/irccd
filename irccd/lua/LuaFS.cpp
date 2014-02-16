/*
 * LuaFS.cpp -- Lua bindings for file dependent operations
 *
 * Copyright (c) 2013, 2014 David Demelier <markand@malikania.fr>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <cerrno>
#include <cstring>
#include <unordered_map>

#include <sys/stat.h>

#include <Date.h>
#include <Directory.h>
#include <Util.h>

#include "Luae.h"
#include "LuaFS.h"
#include "LuaUtil.h"

#define DIR_TYPE "Directory"

namespace irccd {

namespace {

int l_mkdir(lua_State *L)
{
	auto path = luaL_checkstring(L, 1);
	auto mode = 0700;

	if (lua_gettop(L) >= 2)
		mode = luaL_checkinteger(L, 2);

	try {
		Util::mkdir(path, mode);
	} catch (Util::ErrorException ex) {
		lua_pushboolean(L, false);
		lua_pushstring(L, ex.what());

		return 2;
	}

	lua_pushboolean(L, true);

	return 1;
}

int l_opendir(lua_State *L)
{
	auto path = luaL_checkstring(L, 1);
	auto skipParents = false;

	// Optional boolean
	if (lua_gettop(L) >= 2) {
		luaL_checktype(L, 2, LUA_TBOOLEAN);
		skipParents = (lua_toboolean(L, 2) != 0);
	}

	Directory d(path);
	if (!d.open(skipParents)) {
		lua_pushnil(L);
		lua_pushstring(L, d.getError().c_str());

		return 2;
	}

	new (L, DIR_TYPE) Directory(d);

	return 1;
}

int l_exists(lua_State *L)
{
	auto path = luaL_checkstring(L, 1);

	lua_pushboolean(L, Util::exist(path));

	return 1;
}

int l_basename(lua_State *L)
{
	auto path = luaL_checkstring(L, 1);
	auto ret = Util::baseName(path);

	lua_pushlstring(L, ret.c_str(), ret.length());

	return 1;
}

int l_dirname(lua_State *L)
{
	auto path = luaL_checkstring(L, 1);
	auto ret = Util::dirName(path);

	lua_pushlstring(L, ret.c_str(), ret.length());

	return 1;
}

int l_stat(lua_State *L)
{
	auto path = luaL_checkstring(L, 1);
	struct stat st;

	if (stat(path, &st) < 0) {
		lua_pushnil(L);
		lua_pushstring(L, std::strerror(errno));
	}

	lua_createtable(L, 0, 0);

#if defined(HAVE_STAT_ST_DEV)
	lua_pushinteger(L, st.st_dev);
	lua_setfield(L, -2, "device");
#endif
#if defined(HAVE_STAT_ST_INO)
	lua_pushinteger(L, st.st_ino);
	lua_setfield(L, -2, "inode");
#endif
#if defined(HAVE_STAT_ST_NLINK)
	lua_pushinteger(L, st.st_nlink);
	lua_setfield(L, -2, "nlink");
#endif
#if defined(HAVE_STAT_ST_ATIME)
	new (L, DATE_TYPE) Date(st.st_atime);
	lua_setfield(L, -2, "atime");
#endif
#if defined(HAVE_STAT_ST_MTIME)
	new (L, DATE_TYPE) Date(st.st_mtime);
	lua_setfield(L, -2, "mtime");
#endif
#if defined(HAVE_STAT_ST_CTIME)
	new (L, DATE_TYPE) Date(st.st_ctime);
	lua_setfield(L, -2, "ctime");
#endif
#if defined(HAVE_STAT_ST_SIZE)
	lua_pushinteger(L, st.st_size);
	lua_setfield(L, -2, "size");
#endif
#if defined(HAVE_STAT_ST_BLKSIZE)
	lua_pushinteger(L, st.st_blksize);
	lua_setfield(L, -2, "blocksize");
#endif
#if defined(HAVE_STAT_ST_BLOCKS)
	lua_pushinteger(L, st.st_blocks);
	lua_setfield(L, -2, "blocks");
#endif

	return 1;
}

const luaL_Reg functions[] = {
	{ "mkdir",		l_mkdir		},
	{ "opendir",		l_opendir	},
	{ "exists",		l_exists	},
	{ "basename",		l_basename	},
	{ "dirname",		l_dirname	},
	{ "stat",		l_stat		},
	{ nullptr,		nullptr		}
};

/* --------------------------------------------------------
 * Directory methods
 * -------------------------------------------------------- */

namespace dir {

int l_count(lua_State *L)
{
	Directory *d;

	d = Luae::toType<Directory *>(L, 1, DIR_TYPE);
	lua_pushinteger(L, d->getEntries().size());

	return 1;
}

int l_read(lua_State *L)
{
	auto d = Luae::toType<Directory *>(L, 1, DIR_TYPE);
	auto iterator = [] (lua_State *L) -> int {
		auto d = reinterpret_cast<const Directory *>(lua_topointer(L, lua_upvalueindex(1)));
		auto idx = lua_tointeger(L, lua_upvalueindex(2));

		// End
		if (static_cast<size_t>(idx) >= d->getEntries().size())
			return 0;

		// Push name + isDirectory
		lua_pushstring(L, d->getEntries()[idx].m_name.c_str());
		lua_pushboolean(L, d->getEntries()[idx].m_isDirectory);

		lua_pushinteger(L, ++idx);
		lua_replace(L, lua_upvalueindex(2));

		return 2;
	};

	lua_pushlightuserdata(L, d);
	lua_pushinteger(L, 0);
	lua_pushcclosure(L, iterator, 2);

	return 1;
}

} // !dir

/* --------------------------------------------------------
 * Directory metamethods
 * -------------------------------------------------------- */

namespace dirMt {

int l_eq(lua_State *L)
{
	Directory *d1, *d2;

	d1 = Luae::toType<Directory *>(L, 1, DIR_TYPE);
	d2 = Luae::toType<Directory *>(L, 2, DIR_TYPE);

	lua_pushboolean(L, *d1 == *d2);

	return 1;
}

int l_gc(lua_State *L)
{
	Luae::toType<Directory *>(L, 1, DIR_TYPE)->~Directory();

	return 0;
}

int l_tostring(lua_State *L)
{
	auto d = Luae::toType<Directory *>(L, 1, DIR_TYPE);
	lua_pushfstring(L, "Directory %s has %d entries", d->getPath().c_str(),
	    d->getEntries().size());

	return 1;
}

} // !dirMt

const luaL_Reg dirMethodsList[] = {
	{ "count",		dir::l_count		},
	{ "read",		dir::l_read		},
	{ nullptr,		nullptr			}
};

const luaL_Reg dirMtList[] = {
	{ "__eq",		dirMt::l_eq		},
	{ "__gc",		dirMt::l_gc		},
	{ "__tostring",		dirMt::l_tostring	},
	{ nullptr,		nullptr			}
};

}

int luaopen_fs(lua_State *L)
{
	luaL_newlib(L, functions);

	// Directory type
	luaL_newmetatable(L, DIR_TYPE);
	luaL_setfuncs(L, dirMtList, 0);
	luaL_newlib(L, dirMethodsList);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	return 1;
}

} // !irccd

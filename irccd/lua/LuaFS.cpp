/*
 * LuaFS.cpp -- Lua bindings for file dependent operations
 *
 * Copyright (c) 2013, 2014, 2015 David Demelier <markand@malikania.fr>
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

#include <IrccdConfig.h>

#include <common/Date.h>
#include <common/Directory.h>
#include <common/Util.h>

#include <irccd/Luae.h>

#include "LuaFS.h"
#include "LuaUtil.h"

namespace irccd {

/**
 * The directory type.
 */
const char *DirType = "Directory";

/**
 * Overload for Directory
 */
template <>
struct Luae::IsUserdata<Directory> : std::true_type {
	/**
	 * The metatable name.
	 */
	static const char *MetatableName;
};

const char *Luae::IsUserdata<Directory>::MetatableName = DirType;

namespace {

int l_mkdir(lua_State *L)
{
	auto path = Luae::check<std::string>(L, 1);
	auto mode = 0700;

	if (Luae::gettop(L) >= 2)
		mode = Luae::check<int>(L, 2);

	try {
		Util::mkdir(path, mode);
	} catch (std::runtime_error ex) {
		Luae::push(L, false);
		Luae::push(L, ex.what());

		return 2;
	}

	Luae::push(L, true);

	return 1;
}

int l_opendir(lua_State *L)
{
	auto path = Luae::check<std::string>(L, 1);
	auto flags = 0;

	// Optional boolean
	if (Luae::gettop(L) >= 2 && Luae::get<bool>(L, 2))
		flags |= Directory::NotDot | Directory::NotDotDot;

	try {
		Luae::push(L, Directory(path, flags));
	} catch (std::runtime_error error) {
		Luae::push(L, nullptr);
		Luae::push(L, error.what());

		return 2;
	}

	return 1;
}

int l_exists(lua_State *L)
{
	Luae::push(L, Util::exist(Luae::check<std::string>(L, 1)));

	return 1;
}

int l_basename(lua_State *L)
{
	Luae::push(L, Util::baseName(Luae::check<std::string>(L, 1)));

	return 1;
}

int l_dirname(lua_State *L)
{
	Luae::push(L, Util::dirName(Luae::check<std::string>(L, 1)));

	return 1;
}

int l_separator(lua_State *L)
{
#if defined(_WIN32)
	Luae::push(L, "\\");
#else
	Luae::push(L, "/");
#endif

	return 1;
}

int l_stat(lua_State *L)
{
	auto path = Luae::check<std::string>(L, 1);
	struct stat st;

	if (stat(path.c_str(), &st) < 0) {
		Luae::push(L, nullptr);
		Luae::push(L, static_cast<const char *>(std::strerror(errno)));
	}

	LuaeTable::create(L, 0, 0);

#if defined(HAVE_STAT_ST_DEV)
	LuaeTable::set(L, -1, "device", static_cast<int>(st.st_dev));
#endif
#if defined(HAVE_STAT_ST_INO)
	LuaeTable::set(L, -1, "inode", static_cast<int>(st.st_ino));
#endif
#if defined(HAVE_STAT_ST_NLINK)
	LuaeTable::set(L, -1, "nlink", static_cast<int>(st.st_nlink));
#endif
#if defined(HAVE_STAT_ST_ATIME)
	LuaeTable::set(L, -1, "atime", Date(st.st_atime));
#endif
#if defined(HAVE_STAT_ST_MTIME)
	LuaeTable::set(L, -1, "mtime", Date(st.st_mtime));
#endif
#if defined(HAVE_STAT_ST_CTIME)
	LuaeTable::set(L, -1, "ctime", Date(st.st_ctime));
#endif
#if defined(HAVE_STAT_ST_SIZE)
	LuaeTable::set(L, -1, "size", static_cast<int>(st.st_size));
#endif
#if defined(HAVE_STAT_ST_BLKSIZE)
	LuaeTable::set(L, -1, "blocksize", static_cast<int>(st.st_blksize));
#endif
#if defined(HAVE_STAT_ST_BLOCKS)
	LuaeTable::set(L, -1, "blocks", static_cast<int>(st.st_blocks));
#endif

	return 1;
}

const Luae::Reg functions {
	{ "mkdir",		l_mkdir		},
	{ "opendir",		l_opendir	},
	{ "exists",		l_exists	},
	{ "basename",		l_basename	},
	{ "dirname",		l_dirname	},
	{ "separator",		l_separator	},
	{ "stat",		l_stat		},
};

/* --------------------------------------------------------
 * Directory methods
 * -------------------------------------------------------- */

#if defined(COMPAT_1_1)

int l_read(lua_State *L)
{
	using DirectoryIterator = Luae::Iterator<Directory::const_iterator>;

	Luae::deprecate(L, "read", "pairs");

	auto d = Luae::check<Directory>(L, 1);

	new (L) DirectoryIterator(d->cbegin(), d->cend());
	LuaeTable::create(L);
	Luae::pushfunction(L, [] (lua_State *L) -> int {
		Luae::toType<DirectoryIterator *>(L, Luae::upvalueindex(1))->~DirectoryIterator();

		return 0;
	});;
	Luae::setfield(L, -2, "__gc");
	Luae::setmetatable(L, -2);

	Luae::pushfunction(L, [] (lua_State *L) -> int {
		auto it = Luae::toType<DirectoryIterator *>(L, Luae::upvalueindex(1));

		if (it->current == it->end)
			return 0;

		auto value = it->current++;

		// Push name + isDirectory
		Luae::push(L, value->name);
		Luae::push(L, value->type == Directory::Dir);

		return 2;
	}, 1);

	return 1;
}

#endif

int l_count(lua_State *L)
{
	auto d = Luae::check<Directory>(L, 1);

	Luae::push(L, d->count());

	return 1;
}

/* --------------------------------------------------------
 * Directory metamethods
 * -------------------------------------------------------- */

int l_eq(lua_State *L)
{
	auto d1 = Luae::check<Directory>(L, 1);
	auto d2 = Luae::check<Directory>(L, 2);

	Luae::push(L, *d1 == *d2);

	return 1;
}

int l_gc(lua_State *L)
{
	Luae::check<Directory>(L, 1)->~Directory();

	return 0;
}

int l_tostring(lua_State *L)
{
	auto d = Luae::check<Directory>(L, 1);

	Luae::pushfstring(L, "Directory with %d entries", d->count());

	return 1;
}

int l_pairs(lua_State *L)
{
	using DirectoryIterator = Luae::Iterator<Directory::const_iterator>;

	auto d = Luae::check<Directory>(L, 1);

	new (L) DirectoryIterator(d->cbegin(), d->cend());
	LuaeTable::create(L);
	Luae::pushfunction(L, [] (lua_State *L) -> int {
		Luae::toType<DirectoryIterator *>(L, Luae::upvalueindex(1))->~DirectoryIterator();

		return 0;
	});;
	Luae::setfield(L, -2, "__gc");
	Luae::setmetatable(L, -2);

	Luae::pushfunction(L, [] (lua_State *L) -> int {
		auto it = Luae::toType<DirectoryIterator *>(L, Luae::upvalueindex(1));

		if (it->current == it->end)
			return 0;

		auto value = it->current++;

		// Push name + isDirectory
		Luae::push(L, value->name);
		Luae::push(L, value->type == Directory::Dir);

		return 2;
	}, 1);

	return 1;
}

const Luae::Reg dirMethodsList {
	{ "count",		l_count		},
/*
 * DEPRECATION:	1.2-002
 *
 * All the following functions have been moved to the irccd.system.
 */
#if defined(COMPAT_1_1)
	{ "read",		l_read		},
#endif
};

const Luae::Reg dirMtList {
	{ "__eq",		l_eq		},
	{ "__gc",		l_gc		},
	{ "__tostring",		l_tostring	},
	{ "__pairs",		l_pairs		},
};

}

int luaopen_fs(lua_State *L)
{
	Luae::newlib(L, functions);

	// Directory type
	Luae::newmetatable(L, DirType);
	Luae::setfuncs(L, dirMtList);
	Luae::newlib(L, dirMethodsList);
	Luae::setfield(L, -2, "__index");
	Luae::pop(L, 1);

	return 1;
}

} // !irccd

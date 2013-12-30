/*
 * Process.h -- Lua thread or plugin process
 *
 * Copyright (c) 2013 David Demelier <markand@malikania.fr>
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

#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <unordered_map>

#include "config.h"

#include "Luae.h"

namespace irccd {

class Plugin;

/**
 * @class Process
 * @brief A thread or / process that owns a Lua state
 */
class Process {
public:
	using Ptr	= std::shared_ptr<Process>;
	using Libraries	= std::unordered_map<const char *, lua_CFunction>;

	/**
	 * @struct Info
	 * @brief Process information
	 *
	 * This structure is store within the Lua state registry instead of the
	 * process, thus it's possible to get it from any Lua C function.
	 */
	struct Info {
		std::string	name;		//! name like "foo"
		std::string	path;		//! the full path
		std::string	home;		//! plugin's home
		std::string	author;		//! the author (optional)
		std::string	comment;	//! a summary (optional)
		std::string	version;	//! a version (optional)
		std::string	license;	//! a license (optional)
	};

private:
	LuaState	m_state;

	Process() = default;

public:
	/*
	 * The following fields are store in the lua_State * registry
	 * and may be retrieved at any time from any Lua API.
	 */
	static const char *	FieldInfo;

	/*
	 * The following tables are libraries to load, luaLibs are
	 * required and irccdLibs are preloaded.
	 */
	static const Libraries luaLibs;
	static const Libraries irccdLibs;

	/**
	 * Create a process with a new Lua state.
	 *
	 * @return the process
	 */
	static Ptr create();

	/**
	 * Initialize the process, adds the name and home
	 *
	 * @param process the process
	 * @param info the info
	 */
	static void initialize(Ptr process, const Info &info);

	/**
	 * Get the info from the registry. Calls luaL_error if any functions
	 * are called directly in the plugin.
	 *
	 * @param L the state to get info from
	 * @return the info
	 */
	static Info info(lua_State *L);

	/**
	 * Convert the process to a C lua_State.
	 */
	operator lua_State *();
};

} // !irccd

#endif // !_PROCESS_H_

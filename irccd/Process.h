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

private:
	LuaState	m_state;

	Process() = default;

public:
	/*
	 * The following fields are store in the lua_State * registry
	 * and may be retrieved at any time from any Lua API.
	 */
	static const char *	FieldName;
	static const char *	FieldHome;

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
	static Ptr create() noexcept;

	/*
	 * Initialize the process, adds the name and home
	 *
	 * @param process the process
	 * @param name the name
	 * @param home the home
	 */
	static void initialize(Process::Ptr process,
			       const std::string &name,
			       const std::string &home) noexcept;

	/*
	 * Get the name from the registry.
	 *
	 * @param L the Lua state
	 * @return the name
	 */
	static std::string getName(lua_State *L) noexcept;

	/*
	 * Get the home from the registry.
	 *
	 * @param L the Lua state
	 * @return the name
	 */
	static std::string getHome(lua_State *L) noexcept;

	/**
	 * Convert the process to a C lua_State.
	 */
	operator lua_State *() noexcept;
};

} // !irccd

#endif // !_PROCESS_H_

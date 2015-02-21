/*
 * Process.h -- Lua thread or plugin process
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

#ifndef _IRCCD_PROCESS_H_
#define _IRCCD_PROCESS_H_

/**
 * @file Process.h
 * @brief Lua process (thread or plugin)
 * @warning Do not rename the include guard, it conflicts with a Windows header
 */

#include <IrccdConfig.h>

#if defined(WITH_LUA)

#include <mutex>
#include <memory>
#include <unordered_map>

#include "Luae.h"
#include "Timer.h"

namespace irccd {

class Plugin;

/**
 * @class Process
 * @brief A thread or / process that owns a Lua state
 */
class Process final {
public:
	/**
	 * The libraries
	 */
	using Libraries	= std::unordered_map<const char *, lua_CFunction>;
	using Timers = std::vector<std::unique_ptr<Timer>>;
	using Mutex = std::recursive_mutex;
	using Lock = std::unique_lock<Mutex>;

	/**
	 * @struct Info
	 * @brief Process information
	 *
	 * This structure is store within the Lua state registry instead of the
	 * process, thus it's possible to get it from any Lua C function.
	 */
	struct Info {
		std::string	name;		//!< name like "foo"
		std::string	path;		//!< the full path
		std::string	home;		//!< plugin's home
		std::string	author;		//!< the author (optional)
		std::string	comment;	//!< a summary (optional)
		std::string	version;	//!< a version (optional)
		std::string	license;	//!< a license (optional)
	};

private:
	LuaeState m_state;
	Timers m_timers;
	Mutex m_mutex;

	void timerCall(Timer &);
	bool timerIsDead(const std::unique_ptr<Timer> &timer) const noexcept;

public:
	/**
	 * The following fields are store in the lua_State * registry
	 * and may be retrieved at any time from any Lua API.
	 */
	static const char *FieldInfo;

	/**
	 * The standard Lua libraries.
	 */
	static const Libraries luaLibs;

	/**
	 * The irccd libraries.
	 */
	static const Libraries irccdLibs;

	/**
	 * Initialize the process, adds the name and home
	 *
	 * @param process the process
	 * @param info the info
	 */
	static void initialize(std::shared_ptr<Process> &process, const Info &info);

	/**
	 * Get the info from the registry. Calls luaL_error if any functions
	 * are called directly in the plugin.
	 *
	 * @param L the state to get info from
	 * @return the info
	 */
	static Info info(lua_State *L);

	/**
	 * Destroy all timers.
	 */
	~Process();

	/**
	 * Convert the process to a C lua_State.
	 */
	operator lua_State *();

	/**
	 * Add a new timer in that process.
	 *
	 * @param type the type
	 * @param delay the delay in milliseconds
	 * @param reference the Lua function
	 */
	void addTimer(TimerType type, int delay, int reference);

	/**
	 * Stop all timers.
	 */
	void clearTimers();

	/**
	 * Lock the process until the returned object is destroyed.
	 *
	 * @return the lock object
	 */
	inline Lock lock()
	{
		return Lock(m_mutex);
	}
};

} // !irccd

#endif // !_WITH_LUA

#endif // !_PROCESS_H_

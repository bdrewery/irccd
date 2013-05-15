/*
 * Plugin.h -- irccd Lua plugin interface
 *
 * Copyright (c) 2011, 2012, 2013 David Demelier <markand@malikania.fr>
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

#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#include <map>
#include <string>

#include <lua.hpp>

namespace irccd {

class Server;

class Plugin {
private:
	/*
	 * Store plugins mapped by lua_State so C function can retrieve their
	 * respective Plugin from anywhere.
	 */
	static std::map<lua_State *, Plugin *> m_mapped;

	// Plugin identity
	std::string m_name;		//! name like "foo"
	std::string m_home;		//! home, usuall ~/.config/<name>/
	std::string m_error;		//! error message if needed

	lua_State *m_state;		//! Lua state

	bool loadLua(const std::string &path);
public:
	Plugin(void);
	~Plugin(void);

	const std::string & getName(void) const;
	const std::string & getHome(void) const;

	/**
	 * Get the plugin Lua state.
	 *
	 * @return the Lua state
	 */
	lua_State * getState(void) const;

	/**
	 * Get the error message if something failed.
	 *
	 * @return the message
	 */
	const std::string & getError(void) const;

	/**
	 * Open the plugin specified by path.
	 *
	 * @param path the plugin path
	 * @return true on success
	 */
	bool open(const std::string &path);

	/* ------------------------------------------------
	 * IRC commands
	 * ------------------------------------------------ */

	/**
	 * A Lua function "onConnect" will be called
	 * upon a successfull connect.
	 *
	 * @param server the current server
	 */
	void onConnect(Server *server);

	/**
	 * a Lua function triggered on a message event.
	 *
	 * The lua function should have this signature:
	 *	function onMessage(server, channel, nick, message)
	 *
	 * @param server the server
	 * @param channel on which channel
	 * @param who who spoke
	 * @param message the message sent
	 */
	void onMessage(Server *server, const std::string &channel, const std::string &who,
		       const std::string &message);

	/**
	 * A lua function triggered on a special command, this is a channel
	 * message but that starts with the command-char defined in the config.
	 * Some modules needs that, example:
	 *
	 * !google something
	 * !help
	 *
	 * @param server the server
	 * @param channel on which channel
	 * @param who who spoke
	 * @param message the message sent without the command and plugin
	 */
	void onCommand(Server *server, const std::string &channel, const std::string &who,
		       const std::string &message);
};

} // !irccd

#endif // !_PLUGIN_H_

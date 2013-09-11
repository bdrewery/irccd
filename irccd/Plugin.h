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
#include <memory>
#include <string>

#include "Luae.h"

namespace irccd {

class Server;

class Plugin {
public:
	class ErrorException : public std::exception {
	private:
		std::string m_error;
		std::string m_which;

	public:
		ErrorException();

		ErrorException(const std::string &which, const std::string &error);

		~ErrorException();

		/**
		 * Tells which plugin name has failed.
		 *
		 * @return the plugin name
		 */
		std::string which() const;

		virtual const char * what() const throw();
	};

private:
	// Plugin identity
	std::string m_name;		//! name like "foo"
	std::string m_home;		//! home, usually ~/.config/<name>/
	std::string m_error;		//! error message if needed

	LuaState m_state;

	/**
	 * Call the function plugin with optional parameters.
	 * 
	 * @throw ErrorException on failure
	 */
	void call(const std::string &func,
		  std::shared_ptr<Server> server = std::shared_ptr<Server>(),
		  std::vector<std::string> params = std::vector<std::string>());

public:
	Plugin();

	Plugin(const std::string &name);

	Plugin(Plugin &&src);

	Plugin & operator=(Plugin &&src);

	~Plugin();

	/**
	 * Get the plugin name.
	 *
	 * @return the name
	 */
	const std::string & getName() const;

	/**
	 * Get the plugin home directory.
	 *
	 * @return the directory
	 */
	const std::string & getHome() const;

	/**
	 * Get the plugin Lua state.
	 *
	 * @return the Lua state
	 */
	LuaState & getState();

	/**
	 * Get the error message if something failed.
	 *
	 * @return the message
	 */
	const std::string & getError() const;

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
	void onCommand(std::shared_ptr<Server> server,
		       const std::string &channel,
		       const std::string &who,
		       const std::string &message);

	/**
	 * A Lua function "onConnect" will be called
	 * upon a successfull connect.
	 *
	 * @param server the current server
	 */
	void onConnect(std::shared_ptr<Server> server);

	/**
	 * A Lua function triggered on a channel notice
	 *
	 * @param server the server
	 * @param nick who generated the message
	 * @param target the target channel
	 * @param notice the optional notice
	 */
	void onChannelNotice(std::shared_ptr<Server> server,
			     const std::string &nick,
			     const std::string &target,
			     const std::string &notice);

	/**
	 * A Lua function triggered on an invite event.
	 *
	 * @param server the server
	 * @param channel on which channel
	 * @param who who invited you
	 */
	void onInvite(std::shared_ptr<Server> server,
		      const std::string &channel,
		      const std::string &who);

	/**
	 * A Lua function triggered on a join event.
	 *
	 * @param server the server
	 * @param channel on which channel
	 * @param nickname the nickname
	 */
	void onJoin(std::shared_ptr<Server> server,
		    const std::string &channel,
		    const std::string &nickname);

	/**
	 * A Lua function triggered on a kick event.
	 *
	 * @param server the server
	 * @param channel from which channel
	 * @param who who kicked
	 * @param kicked the person kicked
	 * @param reason an optional reason
	 */
	void onKick(std::shared_ptr<Server> server,
		    const std::string &channel,
		    const std::string &who,
		    const std::string &kicked,
		    const std::string &reason);

	/**
	 * A Lua function triggered on a message event.
	 *
	 * @param server the server
	 * @param channel on which channel
	 * @param who who spoke
	 * @param message the message sent
	 */
	void onMessage(std::shared_ptr<Server> server,
		       const std::string &channel,
		       const std::string &who,
		       const std::string &message);

	/**
	 * A Lua function triggered on a channel mode event.
	 *
	 * @param server the server
	 * @param channel on which channel
	 * @param who who changed the mode
	 * @param mode the mode
	 * @param modeArg an optional mode argument
	 */
	void onMode(std::shared_ptr<Server> server,
		    const std::string &channel,
		    const std::string &who,
		    const std::string &mode,
		    const std::string &modeArg);

	/**
	 * A Lua function triggered on a nick event.
	 *
	 * @param server the server
	 * @param oldnick the old nickname
	 * @param newnick the new nickname
	 */
	void onNick(std::shared_ptr<Server> server,
		    const std::string &oldnick,
		    const std::string &newnick);

	/**
	 * A Lua function triggered on a private notice.
	 *
	 * @param server the server
	 * @param nick who generated the message
	 * @param target the target nickname
	 * @param notice the optional notice
	 */
	void onNotice(std::shared_ptr<Server> server,
		      const std::string &nick,
		      const std::string &target,
		      const std::string &notice);

	/**
	 * A Lua function triggered when someone leave a channel.
	 *
	 * @param server the server
	 * @param channel from which channel
	 * @param who who left
	 * @param reason an optional reason
	 */
	void onPart(std::shared_ptr<Server> server,
		    const std::string &channel,
		    const std::string &who,
		    const std::string &reason);

	/**
	 * A Lua function triggered on a private message.
	 *
	 * @param server the server
	 * @param who who sent
	 * @param message the message
	 */
	void onQuery(std::shared_ptr<Server> server,
		     const std::string &who,
		     const std::string &message);

	/**
	 * A Lua function triggered when user want's to reload the plugin.
	 */
	void onReload();

	/**
	 * A Lua function triggered when someone change the channel topic.
	 *
	 * @param server the server
	 * @param channel on which channel
	 * @param who who changed the topic
	 * @param topic the new topic
	 */
	void onTopic(std::shared_ptr<Server> server,
		     const std::string &channel,
		     const std::string &who,
		     const std::string &topic);

	/**
	 * A Lua function triggered on a user mode change event.
	 *
	 * @param server the server
	 * @param who who changed *your* mode
	 * @param mode the mode
	 */
	void onUserMode(std::shared_ptr<Server> server,
			const std::string &who,
			const std::string &mode);
};

} // !irccd

#endif // !_PLUGIN_H_

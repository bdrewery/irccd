/*
 * Plugin.h -- irccd Lua plugin interface
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

#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#include <unordered_map>
#include <memory>
#include <string>
#include <mutex>

#include "Luae.h"
#include "Process.h"
#include "Server.h"

namespace irccd {

class IrcEvent;

/**
 * @class Plugin
 * @brief Lua plugin
 *
 * A plugin is identified by name and can be loaded and unloaded
 * at runtime.
 */
class Plugin {
public:
	class ErrorException : public std::exception {
	private:
		std::string m_error;
		std::string m_which;

	public:
		ErrorException() = default;

		ErrorException(const std::string &which, const std::string &error);

		/**
		 * Tells which plugin name has failed.
		 *
		 * @return the plugin name
		 */
		std::string which() const;

		virtual const char * what() const throw();
	};

	using Ptr		= std::shared_ptr<Plugin>;
	using Dirs		= std::vector<std::string>;
	using List		= std::vector<Plugin::Ptr>;
	using Mutex		= std::recursive_mutex;
	using Lock		= std::lock_guard<Mutex>;

private:
	static Mutex		pluginLock;	//! lock for managing plugins
	static Dirs		pluginDirs;	//! list of plugin directories
	static List		plugins;	//! map of plugins loaded

	// Process for that plugin
	Process::Ptr		m_process;

	// Plugin identity
	std::string		m_name;		//! name like "foo"
	std::string		m_home;		//! home, usually ~/.config/<name>/
	std::string		m_path;		//! path used like "/opt/foo.lua"
	std::string		m_error;	//! error message if needed

	static void callPlugin(Plugin::Ptr p, const IrcEvent &ev);

	void callFunction(const std::string &func,
			  Server::Ptr server = Server::Ptr(),
			  std::vector<std::string> params = std::vector<std::string>());

	void callFunctionNum(const std::string &func,
			  Server::Ptr server,
			  int np = 0);

public:
	/**
	 * Check whether a plugin is loaded.
	 *
	 * @param name the name
	 * @return true if loaded
	 */
	static bool isLoaded(const std::string &name);

	/**
	 * Get a list of loaded plugins.
	 *
	 * @return the list of loaded plugins
	 */
	static std::vector<std::string> loaded();

	/**
	 * Add path for finding plugins.
	 *
	 * @param path the path
	 */
	static void addPath(const std::string &path);

	/**
	 * Try to load a plugin.
	 *
	 * @param path the full path
	 * @param relative tell if the path is relative
	 * @throw std::runtime_error on failure
	 */
	static void load(const std::string &path, bool relative = false);

	/**
	 * Unload a plugin.
	 *
	 * @param name the plugin name
	 */
	static void unload(const std::string &name);

	/**
	 * Reload a plugin.
	 *
	 * @param name the plugin to reload
	 */
	static void reload(const std::string &name);

	/**
	 * Find a plugin by its name.
	 *
	 * @param name the plugin name
	 * @return the plugin
	 * @throw std::out_of_range if not found
	 */
	static Ptr find(const std::string &name);

	/**
	 * Handle a IRC event.
	 *
	 * @param ev the event
	 */
#if defined(WITH_LUA)
	static void handleIrcEvent(const IrcEvent &ev);
#else
	static void handleIrcEvent(const IrcEvent &ev) { }
#endif

	/**
	 * Default constructor. (Forbidden)
	 */
	Plugin() = delete;

	/**
	 * Correct constructor.
	 *
	 * @param name the plugin name
	 * @param path the path to the plugin
	 */
	Plugin(const std::string &name,
	       const std::string &path);

	/**
	 * Get the plugin name.
	 *
	 * @return the name
	 */
	const std::string &getName() const;

	/**
	 * Get the plugin home directory.
	 *
	 * @return the directory
	 */
	const std::string &getHome() const;

	/**
	 * Find the plugin's home. It first tries to load user's one
	 * and system after.
	 */
	void setHome();

	/**
	 * Get the plugin Lua state.
	 *
	 * @return the Lua state
	 */
	lua_State *getState();

	/**
	 * Get the error message if something failed.
	 *
	 * @return the message
	 */
	const std::string &getError() const;

	/**
	 * Open the plugin.
	 *
	 * @return true on success
	 */
	bool open();

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
	void onCommand(Server::Ptr server,
		       const std::string &channel,
		       const std::string &who,
		       const std::string &message);

	/**
	 * A Lua function "onConnect" will be called
	 * upon a successfull connect.
	 *
	 * @param server the current server
	 */
	void onConnect(Server::Ptr server);

	/**
	 * A Lua function triggered on a channel notice
	 *
	 * @param server the server
	 * @param nick who generated the message
	 * @param target the target channel
	 * @param notice the optional notice
	 */
	void onChannelNotice(Server::Ptr server,
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
	void onInvite(Server::Ptr server,
		      const std::string &channel,
		      const std::string &who);

	/**
	 * A Lua function triggered on a join event.
	 *
	 * @param server the server
	 * @param channel on which channel
	 * @param nickname the nickname
	 */
	void onJoin(Server::Ptr server,
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
	void onKick(Server::Ptr server,
		    const std::string &channel,
		    const std::string &who,
		    const std::string &kicked,
		    const std::string &reason);

	/**
	 * Load the plugin.
	 */
	void onLoad();

	/**
	 * A Lua function triggered on a message event.
	 *
	 * @param server the server
	 * @param channel on which channel
	 * @param who who spoke
	 * @param message the message sent
	 */
	void onMessage(Server::Ptr server,
		       const std::string &channel,
		       const std::string &who,
		       const std::string &message);

	/**
	 * A Lua function called on CTCP Action.
	 *
	 * @param server the server
	 * @param channel on which channel
	 * @param who who spoke
	 * @param message the message sent
	 */
	void onMe(Server::Ptr server,
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
	void onMode(Server::Ptr server,
		    const std::string &channel,
		    const std::string &who,
		    const std::string &mode,
		    const std::string &modeArg);

	/**
	 * A Lua function triggered on names list of channel
	 *
	 * @param server the server
	 * @param list the names
	 */
	void onNames(Server::Ptr server,
		     const std::vector<std::string> &list);

	/**
	 * A Lua function triggered on a nick event.
	 *
	 * @param server the server
	 * @param oldnick the old nickname
	 * @param newnick the new nickname
	 */
	void onNick(Server::Ptr server,
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
	void onNotice(Server::Ptr server,
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
	void onPart(Server::Ptr server,
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
	void onQuery(Server::Ptr server,
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
	void onTopic(Server::Ptr server,
		     const std::string &channel,
		     const std::string &who,
		     const std::string &topic);

	/**
	 * Unload the plugin.
	 */
	void onUnload();

	/**
	 * A Lua function triggered on a user mode change event.
	 *
	 * @param server the server
	 * @param who who changed *your* mode
	 * @param mode the mode
	 */
	void onUserMode(Server::Ptr server,
			const std::string &who,
			const std::string &mode);

	/**
	 * A Lua function triggered on a whois information.
	 *
	 * @param server the server
	 * @param info the whois information
	 */
	void onWhois(Server::Ptr,
		     const Server::WhoisInfo &info);
};

} // !irccd

#endif // !_PLUGIN_H_

/*
 * Plugin.h -- irccd Lua plugin interface
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

#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#include <unordered_map>
#include <memory>
#include <string>
#include <mutex>

#include "Luae.h"
#include "Process.h"
#include "Server.h"

#include "lua/LuaServer.h"

namespace irccd {

class IrcEvent;

template <>
struct Luae::Convert<std::vector<std::string>> {
	static const bool supported = true;

	static void push(lua_State *L, const std::vector<std::string> &value)
	{
		int i = 0;

		lua_createtable(L, value.size(), 0);
		for (const auto &s : value) {
			lua_pushlstring(L, s.c_str(), s.length());
			lua_rawseti(L, -2, ++i);
		}
	}
};

template <>
struct Luae::Convert<Server::Ptr> {
	static const bool supported = true;

	static void push(lua_State *L, const Server::Ptr &server)
	{
		LuaeClass::pushShared<Server>(L, server, ServerType);
	}
};

template <>
struct Luae::Convert<IrcWhois> {
	static const bool supported = true;

	static void push(lua_State *L, const IrcWhois &whois)
	{	
		lua_createtable(L, 0, 0);
		lua_pushlstring(L, whois.nick.c_str(), whois.nick.length());
		lua_setfield(L, -2, "nickname");
		lua_pushlstring(L, whois.user.c_str(), whois.user.length());
		lua_setfield(L, -2, "user");
		lua_pushlstring(L, whois.host.c_str(), whois.host.length());
		lua_setfield(L, -2, "host");
		lua_pushlstring(L, whois.realname.c_str(), whois.realname.length());
		lua_setfield(L, -2, "realname");

		// Store optionnal channels
		lua_createtable(L, 0, 0);

		for (size_t i = 4; i < whois.channels.size(); ++i) {
			lua_pushstring(L, whois.channels[i].c_str());
			lua_rawseti(L, -2, i - 3);
		}

		lua_setfield(L, -2, "channels");
	}
};

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

		/**
		 * Get the error.
		 *
		 * @return the error
		 */
		std::string error() const;

		/**
		 * @copydoc std::exception::what
		 */
		virtual const char * what() const throw();
	};

	using Ptr		= std::shared_ptr<Plugin>;
	using Dirs		= std::vector<std::string>;
	using List		= std::vector<Plugin::Ptr>;
	using Mutex		= std::recursive_mutex;
	using Lock		= std::lock_guard<Mutex>;
	using MapFunc		= std::function<void (Ptr ptr)>;

private:
	static Mutex		pluginLock;	//! lock for managing plugins
	static Dirs		pluginDirs;	//! list of plugin directories
	static List		plugins;	//! map of plugins loaded

	Process::Ptr		m_process;	//! lua_State
	Process::Info		m_info;		//! plugin information

	std::string getGlobal(const std::string &name);

	void pushObjects(lua_State *) const
	{
		// Dummy, stop recursion
	}

	template <typename T, typename... Args>
	void pushObjects(lua_State *L, T &value, Args&&... args) const
	{
		Luae::push(L, value);
		pushObjects(L, args...);
	}

	/**
	 * Call a function and push arguments before.
	 *
	 * @param L the Lua state
	 * @param func the function to call
	 * @param args the arguments
	 * @throw Plugin::ErrorException on error
	 */
	template <typename... Args>
	void call(const std::string &func, Args&&... args) const
	{
		Luae::getglobal(*m_process, func);

		if (Luae::type(*m_process, -1) != LUA_TFUNCTION) {
			lua_pop(*m_process, 1);
		} else {
			auto before = lua_gettop(*m_process);
			auto after = 0;

			pushObjects(*m_process, std::forward<Args>(args)...);
			after = lua_gettop(*m_process);

			try {
				Luae::pcall(*m_process, after - before, 0);
			} catch (std::runtime_error error) {
				throw Plugin::ErrorException(m_info.name, error.what());
			}
		}
	}


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
	static std::vector<std::string> list();

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
	 * Iterate over all plugins.
	 *
	 * @param func the function to call
	 */
	static void forAll(MapFunc func);

	/**
	 * Collect garbage for all plugins.
	 */
	static void collectGarbage();

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
	 * Open the plugin.
	 *
	 * @throw ErrorException on error
	 */
	void open();

	/* ------------------------------------------------
	 * Plugin callbacks
	 * ------------------------------------------------ */

	/**
	 * On successful connection.
	 *
	 * @param server the server
	 */
	void onConnect(const Server::Ptr &server);

	/**
	 * On a channel notice.
	 *
	 * @param server the server
	 * @param who the user who sent the notice
	 * @param channel on which channel
	 * @param notice the message
	 */
	void onChannelNotice(const Server::Ptr &server,
			     const std::string &who,
			     const std::string &channel,
			     const std::string &notice);

	/**
	 * On invitation.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param who the user who invited you
	 */
	void onInvite(const Server::Ptr &server,
		      const std::string &channel,
		      const std::string &who);

	/**
	 * On join.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param nickname the user who joined
	 */
	void onJoin(const Server::Ptr &server,
		    const std::string &channel,
		    const std::string &nickname);

	/**
	 * On kick.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param who the user who kicked
	 * @param kicked the kicked target
	 * @param reason the optional reason
	 */
	void onKick(const Server::Ptr &server,
		    const std::string &channel,
		    const std::string &who,
		    const std::string &kicked,
		    const std::string &reason);

	/**
	 * On unload.
	 */
	void onLoad();

	/**
	 * On channel message. This event will call onMessage or
	 * onCommand if the messages starts with the command character
	 * plus the plugin name.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param nick the user who sent the message
	 * @param message the message or command
	 */
	void onMessage(const Server::Ptr &server,
		       const std::string &channel,
		       const std::string &nick,
		       const std::string &message);

	/**
	 * On CTCP Action.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param nick the user who sent the message
	 * @param message the message
	 */
	void onMe(const Server::Ptr &server,
		  const std::string &channel,
		  const std::string &nick,
		  const std::string &message);

	/**
	 * On channel mode.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param nickname the nickname
	 * @param mode the mode
	 * @param arg the optional mode argument
	 */
	void onMode(const Server::Ptr &server,
		    const std::string &channel,
		    const std::string &nickname,
		    const std::string &mode,
		    const std::string &arg);

	/**
	 * On names listing.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param list the list of nicknames
	 */
	void onNames(const Server::Ptr &server,
		     const std::string &channel,
		     const std::vector<std::string> &list);

	/**
	 * On nick change.
	 *
	 * @param server the server
	 * @param oldnick the old nickname
	 * @param newnick the new nickname
	 */
	void onNick(const Server::Ptr &server,
		    const std::string &oldnick,
		    const std::string &newnick);

	/**
	 * On user notice.
	 *
	 * @param server the server
	 * @param who the user who sent the notice
	 * @param target the target, usually your nickname
	 * @param notice the notice
	 */
	void onNotice(const Server::Ptr &server,
		      const std::string &who,
		      const std::string &target,
		      const std::string &notice);

	/**
	 * On part.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param nickname the user who left
	 * @param reason the optional reason
	 */
	void onPart(const Server::Ptr &server,
		    const std::string &channel,
		    const std::string &nickname,
		    const std::string &reason);

	/**
	 * On user query.
	 *
	 * @param server the server
	 * @param who the user who sent the notice
	 * @param message the message
	 */
	void onQuery(const Server::Ptr &server,
		     const std::string &who,
		     const std::string &message);

	/**
	 * On reload.
	 */
	void onReload();

	/**
	 * On topic change.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param who the user who sent the topic
	 * @param topic the new topic
	 */
	void onTopic(const Server::Ptr &server,
		     const std::string &channel,
		     const std::string &who,
		     const std::string &topic);

	/**
	 * On unload.
	 */
	void onUnload();

	/**
	 * On user mode change.
	 *
	 * @param server the server
	 * @param who the person who changed the mode
	 * @param mode the new mode
	 */
	void onUserMode(const Server::Ptr &server,
			const std::string &who,
			const std::string &mode);

	/**
	 * On whois information.
	 *
	 * @param server the server
	 * @param info the info
	 */
	void onWhois(const Server::Ptr &server,
		     const IrcWhois &info);
};

} // !irccd

#endif // !_PLUGIN_H_

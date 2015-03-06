/*
 * Plugin.h -- irccd Lua plugin interface
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

#ifndef _IRCCD_PLUGIN_H_
#define _IRCCD_PLUGIN_H_

/**
 * @file Plugin.h
 * @brief Irccd plugins
 */

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

/**
 * @brief Overload for IrcWhois
 */
template <>
struct Luae::Convert<IrcWhois> {
	static const bool hasPush = true;	//!< is supported

	/**
	 * Push the whois information.
	 *
	 * @param L the Lua state
	 * @param whois the whois information
	 */
	static void push(lua_State *L, const IrcWhois &whois)
	{
		LuaeTable::create(L);
		LuaeTable::set(L, -1, "nickname", whois.nick);
		LuaeTable::set(L, -1, "user", whois.user);
		LuaeTable::set(L, -1, "host", whois.host);
		LuaeTable::set(L, -1, "realname", whois.realname);
		LuaeTable::set(L, -1, "channels", whois.channels);
	}
};

/**
 * @class Plugin
 * @brief Lua plugin
 *
 * A plugin is identified by name and can be loaded and unloaded
 * at runtime.
 */
class Plugin final {
public:
	/**
	 * @class ErrorException
	 * @brief Error in plugins
	 */
	class ErrorException : public std::exception {
	private:
		std::string m_error;
		std::string m_which;

	public:
		/**
		 * Default constructor.
		 */
		ErrorException() = default;

		/**
		 * Construct an error.
		 *
		 * @param which the plugin name
		 * @param error the error
		 */
		ErrorException(std::string which, std::string error);

		/**
		 * Tells which plugin name has failed.
		 *
		 * @return the plugin name
		 */
		const std::string &which() const;

		/**
		 * Get the error.
		 *
		 * @return the error
		 */
		const std::string &error() const;

		/**
		 * Get the error.
		 *
		 * @return the error
		 */
		virtual const char *what() const noexcept;
	};

private:
	std::shared_ptr<Process> m_process;
	Process::Info m_info;

	std::string getGlobal(const std::string &name);

	void pushObjects(lua_State *) const
	{
		// Dummy, stop recursion
	}

	template <typename T, typename... Args>
	void pushObjects(lua_State *L, const T &value, Args&&... args) const
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
		Process::Lock lock = m_process->lock();

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
			} catch (const std::runtime_error &error) {
				throw Plugin::ErrorException(m_info.name, error.what());
			}
		}
	}

public:
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
	Plugin(std::string name, std::string path);

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

	/**
	 * Get the process for that plugin.
	 *
	 * @return the plugin
	 */
	inline const std::shared_ptr<Process> &process() const noexcept
	{
		return m_process;
	}

	/* ------------------------------------------------
	 * Plugin callbacks
	 * ------------------------------------------------ */

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
	void onCommand(std::shared_ptr<Server> server, std::string channel, std::string nick, std::string message);

	/**
	 * On successful connection.
	 *
	 * @param server the server
	 */
	void onConnect(std::shared_ptr<Server> server);

	/**
	 * On a channel notice.
	 *
	 * @param server the server
	 * @param who the user who sent the notice
	 * @param channel on which channel
	 * @param notice the message
	 */
	void onChannelNotice(std::shared_ptr<Server> server, std::string who, std::string channel, std::string notice);

	/**
	 * On invitation.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param who the user who invited you
	 */
	void onInvite(std::shared_ptr<Server> server, std::string channel, std::string who);

	/**
	 * On join.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param nickname the user who joined
	 */
	void onJoin(std::shared_ptr<Server> server, std::string channel, std::string nickname);

	/**
	 * On kick.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param who the user who kicked
	 * @param kicked the kicked target
	 * @param reason the optional reason
	 */
	void onKick(std::shared_ptr<Server> server, std::string channel, std::string who, std::string kicked, std::string reason);

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
	void onMessage(std::shared_ptr<Server> server, std::string channel, std::string nick, std::string message);

	/**
	 * On CTCP Action.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param nick the user who sent the message
	 * @param message the message
	 */
	void onMe(std::shared_ptr<Server> server, std::string channel, std::string nick, std::string message);

	/**
	 * On channel mode.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param nickname the nickname
	 * @param mode the mode
	 * @param arg the optional mode argument
	 */
	void onMode(std::shared_ptr<Server> server, std::string channel, std::string nickname, std::string mode, std::string arg);

	/**
	 * On names listing.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param list the list of nicknames
	 */
	void onNames(std::shared_ptr<Server> server, std::string channel, std::vector<std::string> list);

	/**
	 * On nick change.
	 *
	 * @param server the server
	 * @param oldnick the old nickname
	 * @param newnick the new nickname
	 */
	void onNick(std::shared_ptr<Server> server, std::string oldnick, std::string newnick);

	/**
	 * On user notice.
	 *
	 * @param server the server
	 * @param who the user who sent the notice
	 * @param target the target, usually your nickname
	 * @param notice the notice
	 */
	void onNotice(std::shared_ptr<Server> server, std::string who, std::string target, std::string notice);

	/**
	 * On part.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param nickname the user who left
	 * @param reason the optional reason
	 */
	void onPart(std::shared_ptr<Server> server, std::string channel, std::string nickname, std::string reason);

	/**
	 * On user query.
	 *
	 * @param server the server
	 * @param who the user who sent the query
	 * @param message the message
	 */
	void onQuery(std::shared_ptr<Server> server, std::string who, std::string message);

	/**
	 * On user query command.
	 *
	 * @param server the server
	 * @param who the user who sent the query
	 * @param message the message
	 */
	void onQueryCommand(std::shared_ptr<Server> server, std::string who, std::string message);

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
	void onTopic(std::shared_ptr<Server> server, std::string channel, std::string who, std::string topic);

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
	void onUserMode(std::shared_ptr<Server> server, std::string who, std::string mode);

	/**
	 * On whois information.
	 *
	 * @param server the server
	 * @param info the info
	 */
	void onWhois(std::shared_ptr<Server> server, IrcWhois info);
};

} // !irccd

#endif // !_IRCCD_PLUGIN_H_

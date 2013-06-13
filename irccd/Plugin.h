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

#include "LuaState.h"

namespace irccd {

class Server;
class Plugin;

/**
 * @enum DeferredType
 * @brief Type for deferred calls.
 *
 * This type is used to do specific action on a deferred call.
 */
enum class DeferredType {
	Names,
};

typedef std::vector<std::vector<std::string>> Params;

/**
 * @class DeferredCall
 * @brief Deferred call class.
 *
 * This class is used to call deferred functions.
 */
class DeferredCall {
private:
	DeferredType m_type;			//! type of call
	std::shared_ptr<Server> m_server;		//! for which server
	Params m_params;			//! list of list of string
	int m_ref;				//! function reference

public:
	/**
	 * Default constructor.
	 */
	DeferredCall();

	/**
	 * Constructor with specific parameters.
	 *
	 * @param type the type of deferred call
	 * @param server  for which server
	 * @param ref the function reference
	 */
	DeferredCall(DeferredType type, std::shared_ptr<Server> server, int ref);

	/**
	 * Get the type.
	 *
	 * @return the deferred call type
	 */
	DeferredType type() const;

	/**
	 * Get which server we are working on.
	 *
	 * @return the server
	 */
	std::shared_ptr<Server> server() const;

	/**
	 * Add a list of parameters.
	 *
	 * @param list the list of parameters
	 */
	void addParam(const std::vector<std::string> & list);
	
	/**
	 * Execute the function call and dereference the function
	 * reference.
	 *
	 * @param plugin which plugin
	 */
	void execute(Plugin &plugin);

	/**
	 * Test the DeferredCall equality.
	 *
	 * @param c1 the object to test
	 * @return true on equality
	 */
	bool operator==(const DeferredCall &c1);
};

class Plugin {
public:
	class ErrorException : public std::exception {
	private:
		std::string m_error;

	public:
		ErrorException();

		ErrorException(const std::string &error);

		~ErrorException();

		virtual const char * what() const throw();
	};

	friend class DeferredCall;
private:
	// Plugin identity
	std::string m_name;		//! name like "foo"
	std::string m_home;		//! home, usuall ~/.config/<name>/
	std::string m_error;		//! error message if needed

	LuaState m_state;

	// Deferred calls
	std::vector<DeferredCall> m_defcalls;	//! list of deferred call

	/**
	 * Load the Lua script file.
	 *
	 * @param path the path file
	 * @return true on success
	 */
	bool loadLua(const std::string &path);
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
	 * Deffered calls commands
	 * ------------------------------------------------ */

	/**
	 * Add a new deferred call to be ran by the server
	 * when the operation has complete.
	 *
	 * @param call the deferred call
	 */
	void addDeferred(DeferredCall call);

	/**
	 * Tell if the server has a deferred call to execute.
	 *
	 * @param type the type
	 * @param sv for which server
	 * @return true if has
	 */
	bool hasDeferred(DeferredType type, std::shared_ptr<Server> sv);

	/**
	 * Get a specified deferred call.
	 *
	 * @param type the type
	 * @param sv for which server
	 * @return the deferred call
	 */
	DeferredCall & getDeferred(DeferredType type, std::shared_ptr<Server> sv);

	/**
	 * Remove a deferred call.
	 *
	 * @param dc the deferred call
	 */
	void removeDeferred(DeferredCall &dc);

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

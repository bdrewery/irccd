/*
 * Server.h -- a IRC server to connect to
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

#ifndef _SERVER_H_
#define _SERVER_H_

#include <map>
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <libircclient.h>

#include <config.h>

#include "server/ServerState.h"

namespace irccd {

class Server;

/* --------------------------------------------------------
 * IRC Events, used by Server
 * -------------------------------------------------------- */

/**
 * @enum IrcEventType
 * @brief Type of IRC event
 */
enum class IrcEventType {
	Connection,					//! when connection
	ChannelNotice,					//! channel notices
	Invite,						//! invitation
	Join,						//! on joins
	Kick,						//! on kick
	Message,					//! on channel messages
	Me,						//! CTCP Action
	Mode,						//! channel mode
	Nick,						//! nick change
	Names,						//! (def) names listing
	Notice,						//! channel notice
	Part,						//! channel parts
	Query,						//! private query
	Topic,						//! topic change
	UserMode,					//! user mode change
	Whois						//! (def) whois response
};

/**
 * @enum IrcChanNickMode
 * @brief Prefixes for channels
 */
enum class IrcChanNickMode {
	Creator		= 'O',				//! channel creator
	HalfOperator	= 'h',				//! half operator
	Operator	= 'o',				//! channel operator
	Protection	= 'a',				//! unkillable
	Voiced		= 'v'				//! voice power
};

/**
 * @struct IrcWhois
 * @brief Describe a whois information
 */
struct IrcWhois {
	bool found = false;				//! if no such nick
	std::string nick;				//! user's nickname
	std::string user;				//! user's user
	std::string host;				//! hostname
	std::string realname;				//! realname
	std::vector<std::string> channels;		//! channels
};

using IrcEventParams	= std::vector<std::string>;
using IrcPrefixes	= std::map<IrcChanNickMode, char>;

/**
 * @class IrcEvent
 * @brief An IRC event
 */
class IrcEvent {
public:
	IrcEventType m_type;				//! event type
	IrcEventParams m_params;			//! parameters
	std::shared_ptr<Server> m_server;		//! on which server

	/**
	 * Better constructor.
	 *
	 * @param type the event type
	 * @param params eventual parameters
	 * @param server which server
	 */
	IrcEvent(IrcEventType type,
		 IrcEventParams params,
		 std::shared_ptr<Server> server);
};

/**
 * @class IrcDeleter
 * @brief Delete the irc_session_t
 */
class IrcDeleter {
public:
	void operator()(irc_session_t *s);
};

/**
 * @class IrcSession
 * @brief Wrapper for irc_session_t
 */
class IrcSession {
private:
	using Ptr	= std::unique_ptr<irc_session_t, IrcDeleter>;

	Ptr m_handle;

public:
	/**
	 * Default constructor.
	 */
	IrcSession();

	/**
	 * Move constructor.
	 *
	 * @param other the other IrcSession
	 */
	IrcSession(IrcSession &&other);

	/**
	 * Move assignment operator.
	 *
	 * @param other the other IrcSession
	 * @return self
	 */
	IrcSession &operator=(IrcSession &&other);

	/**
	 * Cast to irc_session_t for raw commands.
	 *
	 * @return the irc_session_t
	 */
	operator irc_session_t *();
};

/**
 * @class Server
 * @brief Connect to an IRC server
 *
 * Server class, each class define a server that irccd
 * can connect to
 */
class Server : public std::enable_shared_from_this<Server> {
public:
	/**
	 * @struct Channel
	 * @brief A channel joined or to join
	 */
	struct Channel {
		std::string	name;			//! channel name
		std::string	password;		//! channel optional password
	};

	using ChanList	= std::vector<Channel>;

	/**
	 * @struct RetryInfo
	 * @brief The reconnection mechanism
	 */
	struct RetryInfo {
		bool		enabled = true;		//! enable the reconnection mechanism
		int		maxretries = 0;		//! max number of test (0 = forever)
		int		noretried = 0;		//! current number of test
		int		timeout = 30;		//! seconds to wait before testing
	};

	/**
	 * @struct Options
	 * @brief Options for the server
	 */
	struct Options {
		std::string	commandChar = "!";	//! command token
		bool		joinInvite = true;	//! auto join on invites
	};

	/**
	 * @struct Info
	 * @brief Server information
	 */
	struct Info {
		std::string	name;			//! server's name
		std::string	host;			//! hostname
		std::string	password;		//! optional server password
		ChanList	channels;		//! list of channels
		IrcPrefixes	prefixes;		//! comes with event 5
		unsigned	port = 0;		//! server's port
		bool		ssl = false;		//! SSL usage
		bool		sslVerify = true;	//! SSL verification
	};

	/**
	 * @struct Identity
	 * @brief An identity used for the server
	 */
	struct Identity {
		std::string	name = "__irccd__";	//! identity name
		std::string	nickname = "irccd";	//! user nickname
		std::string	username = "irccd";	//! IRC client user

		//! The realname
		std::string	realname = "IRC Client Daemon";

		//! The CTCP version response
		std::string	ctcpVersion = "Irccd " VERSION;
	};

	using NameList	= std::unordered_map<
				std::string,
				std::vector<std::string>
			  >;

	using WhoisList	= std::unordered_map<
				std::string,
				IrcWhois
			  >;

	using Ptr	= std::shared_ptr<Server>;
	using List	= std::unordered_map<
				std::string,
				Server::Ptr
			  >;


	using Mutex	= std::recursive_mutex;
	using Lock	= std::lock_guard<Mutex>;
	using MapFunc	= std::function<void (Server::Ptr)>;

private:
	static List	servers;		//! all servers
	static Mutex	serverLock;		//! lock for server management

	// For deferred events
	NameList	m_nameLists;		//! channels names to receive
	WhoisList	m_whoisLists;		//! list of whois

	Mutex		m_lock;			//! lock for client operation
	IrcSession	m_session;		//! the current session

	// State
	ServerState::Ptr m_state;		//! the current state
	std::thread	m_thread;		//! the thread

protected:
	Info		m_info;			//! server info
	Identity	m_identity;		//! identity to use
	Options		m_options;		//! some options
	RetryInfo	m_reco;			//! reconnection settings

public:
	/**
	 * Add a new server to the registry. It also start the server
	 * immediately.
	 *
	 * @param server the server to add
	 */
	static void add(Ptr server);

	/**
	 * Remove the server from the registry.
	 *
	 * @param server
	 */
	static void remove(Ptr server);

	/**
	 * Get an existing server.
	 *
	 * @param name the server name
	 * @return the server
	 * @throw std::out_of_range if not found
	 */
	static Server::Ptr get(const std::string &name);

	/**
	 * Check if a server exists
	 *
	 * @param name the server name
	 * @return true if the server by this name is loaded
	 */
	static bool has(const std::string &name);

	/**
	 * Call a function for all servers.
	 *
	 * @param func the function
	 */
	static void forAll(MapFunc func);

	/**
	 * Remove dead servers.
	 */
	static void flush();

	/**
	 * Convert the s context to a shared_ptr<Server>.
	 *
	 * @param s the session
	 * @return the shared_ptr.
	 */
	static Ptr toServer(irc_session_t *s);

	/**
	 * Convert a channel line to Channel. The line must be in the
	 * following form:
	 *	#channel:password
	 *
	 * Password is optional and : must be removed.
	 *
	 * @param line the line to convert
	 * @return a channel
	 */
	static Channel toChannel(const std::string &line);

	/**
	 * Better constructor with information and options.
	 *
	 * @param info the host info
	 * @param identity the identity
	 * @param options some options
	 * @param reco the reconnection options
	 */
	Server(const Info &info,
	       const Identity &identity,
	       const Options &options,
	       const RetryInfo &reco);

	/**
	 * Default destructor.
	 */
	virtual ~Server();

	/**
	 * Extract the prefixes from the server.
	 *
	 * @param line the line like PREFIX=(ovh)@+&
	 */
	void extractPrefixes(const std::string &line);

	/**
	 * Get the name list being build for /names
	 * reporting.
	 *
	 * @return the current list
	 */
	NameList &getNameLists();

	/**
	 * Get the whois lists to build /whois reporting.
	 *
	 * @return the current list
	 */
	WhoisList &getWhoisLists();

	/**
	 * Get the server information.
	 *
	 * @return the server information
	 */
	Info &getInfo();

	/**
	 * Get the identity used for that server
	 *
	 * @return the identity
	 */
	Identity &getIdentity();

	/**
	 * Get the server options.
	 *
	 * @return the options
	 */
	Options &getOptions();

	/**
	 * Get the reconnection settings.
	 *
	 * @return the reconnection settings
	 */
	RetryInfo &getRecoInfo();

	/**
	 * Get the current IrcSession.
	 *
	 * @return the session
	 */
	IrcSession &getSession();

	/**
	 * Get all channels that will be auto joined
	 *
	 * @return the list of channels
	 */
	const ChanList &getChannels() const;

	/**
	 * Add a channel that the server will connect to when the
	 * connection is complete.
	 *
	 * @param channel the channel to add
	 */
	void addChannel(const Channel &channel);

	/**
	 * Tells if we already have a channel in the list.
	 *
	 * @param name the channel to test
	 * @return true if has
	 */
	bool hasChannel(const std::string &name);

	/**
	 * Check if the nick has a prefix or not according to this server.
	 *
	 * @param nick the nickname
	 * @return true if has
	 */
	bool hasPrefix(const std::string &nick) const;

	/**
	 * Remove a channel from the server list.
	 *
	 * @param name the channel name
	 */
	void removeChannel(const std::string &name);

	/**
	 * Start the server thread and state machine.
	 */
	void start();

	void stop();

	/* ------------------------------------------------
	 * IRC commands
	 * ------------------------------------------------ */

	/**
	 * Send a notice to a public channel.
	 *
	 * @param channel the target channel
	 * @param message the message to send
	 */
	virtual void cnotice(const std::string &channel,
			     const std::string &message);

	/**
	 * Invite someone to a channel.
	 *
	 * @param target the target nickname
	 * @param channel the channel
	 */
	virtual void invite(const std::string &target,
			    const std::string &channel);

	/**
	 * Join a channel.
	 *
	 * @param channel the channel name
	 * @param password an optional password
	 */
	virtual void join(const std::string &name,
			  const std::string &password = "");

	/**
	 * Kick someone from a channel.
	 *
	 * @param name the nick name
	 * @param channel the channel from
	 * @param reason an optional reason
	 */
	virtual void kick(const std::string &name,
			  const std::string &channel,
			  const std::string &reason = "");

	/**
	 * Send a CTCP ACTION known as /me.
	 *
	 * @param target the nickname or channel
	 * @param message the message to send
	 */
	virtual void me(const std::string &target,
			const std::string &message);

	/**
	 * Change the channel mode.
	 *
	 * @param channel the target channel
	 * @param mode the mode
	 */
	virtual void mode(const std::string &channel,
			  const std::string &mode);

	/**
	 * Get the list of names as a deferred call.
	 *
	 * @param channel which channel
	 * @param plugin the plugin to call on end of list
	 * @param ref the function reference
	 */
	virtual void names(const std::string &channel);

	/**
	 * Change your nickname.
	 *
	 * @param nick the new nickname
	 */
	virtual void nick(const std::string &nick);

	/**
	 * Send a notice to someone.
	 *
	 * @param nickname the target nickname
	 * @param message the message
	 */
	virtual void notice(const std::string &nickname,
			    const std::string &message);

	/**
	 * Leave a channel.
	 *
	 * @param channel the channel to leave
	 */
	virtual void part(const std::string &channel);

	/**
	 * Send a query message.
	 *
	 * @param who the target nickname
	 * @param message the message
	 */
	virtual void query(const std::string &who,
			   const std::string &message);

	/**
	 * Say something to a channel or to a nickname.
	 *
	 * @param target the nickname or channel
	 * @param message the message to send
	 */
	virtual void say(const std::string &target,
			 const std::string &message);

	/**
	 * Send a raw message to the server.
	 *
	 * @param message the message
	 */
	virtual void sendRaw(const std::string &msg);

	/**
	 * Change a channel topic.
	 *
	 * @param channel the channel target
	 * @param topic the new topic
	 */
	virtual void topic(const std::string &channel,
			   const std::string &topic);

	/**
	 * Change your own user mode.
	 *
	 * @param mode the mode
	 */
	virtual void umode(const std::string &mode);

	/**
	 * Get the whois information from a user.
	 *
	 * @param target the nickname target
	 */
	virtual void whois(const std::string &target);
};

} // !irccd

#endif // !_SERVER_H_

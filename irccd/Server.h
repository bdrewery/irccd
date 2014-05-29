/*
 * Server.h -- a IRC server to connect to
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

#ifndef _SERVER_H_
#define _SERVER_H_

/**
 * @file Server.h
 * @brief Irccd servers
 */

#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <vector>

#include <config.h>

#include "CommandQueue.h"
#include "IrcSession.h"

#include "server/ServerState.h"

namespace irccd {

/**
 * @enum IrcChanNickMode
 * @brief Prefixes for channels
 */
enum class IrcChanNickMode {
	Creator		= 'O',				//!< channel creator
	HalfOperator	= 'h',				//!< half operator
	Operator	= 'o',				//!< channel operator
	Protection	= 'a',				//!< unkillable
	Voiced		= 'v'				//!< voice power
};

/**
 * @struct IrcWhois
 * @brief Describe a whois information
 */
struct IrcWhois {
	bool found = false;				//!< if no such nick
	std::string nick;				//!< user's nickname
	std::string user;				//!< user's user
	std::string host;				//!< hostname
	std::string realname;				//!< realname
	std::vector<std::string> channels;		//!< channels
};

/**
 * Map for IRC prefixes to the character.
 */
using IrcPrefixes	= std::map<IrcChanNickMode, char>;

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
		std::string	name;			//!< channel name
		std::string	password;		//!< channel optional password
	};

	/**
	 * List of channels.
	 */
	using ChannelList	= std::vector<Channel>;

	/**
	 * @struct RetryInfo
	 * @brief The reconnection mechanism
	 */
	struct RetryInfo {
		bool		enabled = true;		//!< enable the reconnection mechanism
		int		maxretries = 0;		//!< max number of test (0 = forever)
		int		noretried = 0;		//!< current number of test
		int		timeout = 30;		//!< seconds to wait before testing

		/*
		 * The following variables are only used in the
		 * ServerDisconnected state. Because it waits for the timeout
		 * we check if the irccdctl wanted to stop / restart
		 * the server.
		 */
		bool		restarting = false;	//!< tells if we are restarting
		bool		stopping = false;	//!< tells that we want to stop
	};

	/**
	 * @struct Info
	 * @brief Server information
	 */
	struct Info {
		std::string	name;			//!< server's name
		std::string	host;			//!< hostname
		std::string	password;		//!< optional server password
		std::string	command = "!";		//!< the command character
		ChannelList	channels;		//!< list of channels
		IrcPrefixes	prefixes;		//!< comes with event 5
		unsigned	port = 0;		//!< server's port
	};

	/**
	 * @struct Identity
	 * @brief An identity used for the server
	 */
	struct Identity {
		std::string	name = "__irccd__";	//!< identity name
		std::string	nickname = "irccd";	//!< user nickname
		std::string	username = "irccd";	//!< IRC client user

		//! The realname
		std::string	realname = "IRC Client Daemon";

		//! The CTCP version response
		std::string	ctcpVersion = "Irccd " VERSION;
	};

	/**
	 * @enum Options
	 * @brief Some options for the server
	 */
	enum Options {
		OptionJoinInvite	= (1 << 0),	//! join on invite
		OptionAutoRejoin	= (1 << 1),	//! auto rejoin after kick
		OptionSsl		= (1 << 2),	//! use SSL
		OptionSslNoVerify	= (1 << 3)	//! do not verify SSL
	};

	/**
	 * List of NAMES being built.
	 */
	using NameList	= std::unordered_map<
				std::string,
				std::vector<std::string>
			  >;

	/**
	 * List of WHOIS being built.
	 */
	using WhoisList	= std::unordered_map<
				std::string,
				IrcWhois
			  >;

	/**
	 * Smart pointer for Server.
	 */
	using Ptr	= std::shared_ptr<Server>;

	/**
	 * The map function for function @ref forAll.
	 */
	using MapFunc	= std::function<void (Server::Ptr)>;

private:
	using Map	= std::unordered_map<std::string, Ptr>;
	using Threads	= std::unordered_map<std::string, std::thread>;
	using Mutex	= std::recursive_mutex;
	using Lock	= std::lock_guard<Mutex>;

private:
	static Map	servers;		//!< all servers
	static Threads	threads;		//!< all threads
	static Mutex	serverLock;		//!< lock for server management

	NameList	m_nameLists;		//!< channels names to receive
	WhoisList	m_whoisLists;		//!< list of whois
	Mutex		m_lock;			//!< lock for client operation
	IrcSession	m_session;		//!< the current session
	CommandQueue	m_queue;		//!< command queue
	ServerState::Ptr m_state;		//!< the current state

protected:
	Info		m_info;			//!< server info
	Identity	m_identity;		//!< identity to use
	RetryInfo	m_reco;			//!< reconnection settings
	unsigned	m_options;		//!< the options

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
	 * Clear all threads.
	 */
	static void clearThreads();

	/**
	 * Convert a channel line to Channel. The line must be in the
	 * following form:
	 *	\#channel:password
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
	 * @param reco the reconnection options
	 * @param options some options
	 */
	Server(const Info &info,
	       const Identity &identity,
	       const RetryInfo &reco,
	       unsigned options = 0);

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
	NameList &nameLists();

	/**
	 * Get the whois lists to build /whois reporting.
	 *
	 * @return the current list
	 */
	WhoisList &whoisLists();

	/**
	 * Get the server information.
	 *
	 * @return the server information
	 */
	Info &info();

	/**
	 * Get the identity used for that server
	 *
	 * @return the identity
	 */
	Identity &identity();

	/**
	 * Get the reconnection settings.
	 *
	 * @return the reconnection settings
	 */
	RetryInfo &reco();

	/**
	 * Get the current IrcSession.
	 *
	 * @return the session
	 */
	IrcSession &session();

	/**
	 * Get options.
	 *
	 * @return the options
	 */
	unsigned options() const;

	/**
	 * Get all channels that will be auto joined
	 *
	 * @return the list of channels
	 */
	const ChannelList &channels() const;

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
	 * Kill the server.
	 */
	void kill();

	/**
	 * Force a reconnection.
	 */
	void reconnect();

	/**
	 * Clear the command queue. Only used in ServerDead state.
	 */
	void clearCommands();

	/* ------------------------------------------------
	 * IRC commands
	 * ------------------------------------------------ */

	/*
	 * The following functions are just wrappers around the IrcSession
	 * so it becomes thread safe and any change in IrcSession or
	 * libircclient library does not impact this class.
	 */

	/**
	 * @copydoc IrcSession::cnotice
	 */
	virtual void cnotice(const std::string &channel,
			     const std::string &message);

	/**
	 * @copydoc IrcSession::invite
	 */
	virtual void invite(const std::string &target,
			    const std::string &channel);

	/**
	 * @copydoc IrcSession::join
	 */
	virtual void join(const std::string &channel,
			  const std::string &password = "");

	/**
	 * @copydoc IrcSession::kick
	 */
	virtual void kick(const std::string &name,
			  const std::string &channel,
			  const std::string &reason = "");

	/**
	 * @copydoc IrcSession::me
	 */
	virtual void me(const std::string &target,
			const std::string &message);

	/**
	 * @copydoc IrcSession::mode
	 */
	virtual void mode(const std::string &channel,
			  const std::string &mode);

	/**
	 * @copydoc IrcSession::names
	 */
	virtual void names(const std::string &channel);

	/**
	 * @copydoc IrcSession::nick
	 */
	virtual void nick(const std::string &newnick);

	/**
	 * @copydoc IrcSession::notice
	 */
	virtual void notice(const std::string &target,
			    const std::string &message);

	/**
	 * @copydoc IrcSession::part
	 */
	virtual void part(const std::string &channel,
			  const std::string &reason = "");

	/**
	 * @copydoc IrcSession::say
	 */
	virtual void query(const std::string &target,
			   const std::string &message);

	/**
	 * @copydoc IrcSession::say
	 */
	virtual void say(const std::string &target,
			 const std::string &message);

	/**
	 * @copydoc IrcSession::send
	 */
	virtual void send(const std::string &raw);

	/**
	 * @copydoc IrcSession::topic
	 */
	virtual void topic(const std::string &channel,
			   const std::string &topic);

	/**
	 * @copydoc IrcSession::umode
	 */
	virtual void umode(const std::string &mode);

	/**
	 * @copydoc IrcSession::whois
	 */
	virtual void whois(const std::string &target);
};

} // !irccd

#endif // !_SERVER_H_

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

		/*
		 * The following variables are only used in the
		 * ServerDisconnected state. Because it waits for the timeout
		 * we check if the irccdctl wanted to stop / restart
		 * the server.
		 */
		bool		restarting = false;	//! tells if we are restarting
		bool		stopping = false;	//! tells that we want to stop
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
	CommandQueue	m_queue;		//! command queue

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

	/**
	 * Restart a connection if it is running.
	 */
	void restart();

	/**
	 * Request for stopping the server.
	 */
	void stop();

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
	virtual void join(const std::string &name,
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
	virtual void nick(const std::string &nick);

	/**
	 * @copydoc IrcSession::notice
	 */
	virtual void notice(const std::string &nickname,
			    const std::string &message);

	/**
	 * @copydoc IrcSession::part
	 */
	virtual void part(const std::string &channel,
			  const std::string &reason = "");

	/**
	 * @copydoc IrcSession::query
	 */
	virtual void query(const std::string &who,
			   const std::string &message);

	/**
	 * @copydoc IrcSession::say
	 */
	virtual void say(const std::string &target,
			 const std::string &message);

	/**
	 * @copydoc IrcSession::send
	 */
	virtual void send(const std::string &msg);

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

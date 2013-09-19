/*
 * Server.h -- a IRC server to connect to
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

#ifndef _SERVER_H_
#define _SERVER_H_

#include <map>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <libircclient.h>

namespace irccd
{

class Server;

/* --------------------------------------------------------
 * IRC Events, used by Server
 * -------------------------------------------------------- */

enum class IrcEventType
{
	Connection,					//! when connection
	ChannelNotice,					//! channel notices
	Invite,						//! invitation
	Join,						//! on joins
	Kick,						//! on kick
	Message,					//! on channel messages
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

enum class IrcChanNickMode
{
	Creator		= 'O',				//! channel creator
	HalfOperator	= 'h',				//! half operator
	Operator	= 'o',				//! channel operator
	Protection	= 'a',				//! unkillable
	Voiced		= 'v'				//! voice power
};

typedef std::vector<std::string> IrcEventParams;
typedef std::map<IrcChanNickMode, char> IrcPrefixes;

struct IrcEvent
{
	IrcEventType m_type;				//! event type
	IrcEventParams m_params;			//! parameters

	std::shared_ptr<Server> m_server;		//! on which server

	/**
	 * Default constructor.
	 */
	IrcEvent();

	/**
	 * Better constructor.
	 *
	 * @param type the event type
	 * @param params eventual parameters
	 * @param server which server
	 */
	IrcEvent(IrcEventType type, IrcEventParams params,
		 std::shared_ptr<Server> server);

	/**
	 * Default destructor.
	 */
	~IrcEvent();
};

class IrcDeleter
{
public:
	void operator()(irc_session_t *s)
	{
		irc_destroy_session(s);

		delete reinterpret_cast<std::shared_ptr<Server> *>(irc_get_ctx(s));
	}
};

typedef std::unique_ptr<irc_session_t, IrcDeleter> IrcSession;

/**
 * Server class, each class define a server that irccd
 * can connect to
 */
class Server : public std::enable_shared_from_this<Server>
{
public:
	struct Channel
	{
		std::string m_name;			//! channel name
		std::string m_password;			//! channel optional password
	};

	struct Options
	{
		std::string m_commandChar;		//! command token
		bool m_joinInvite;			//! auto join on invites
		bool m_ctcpReply;			//! auto reply CTCP

		Options()
			: m_commandChar("!")
			, m_joinInvite(false)
			, m_ctcpReply(true)
		{
		}
	};

	struct Info
	{
		std::string m_name;			//! server's name
		std::string m_host;			//! hostname
		unsigned m_port;			//! server's port
		std::string m_password;			//! optional server password
		bool m_ssl;				//! SSL usage
		bool m_sslVerify;			//! SSL verification
		std::vector<Channel> m_channels;	//! list of channels
		IrcPrefixes m_prefixes;			//! comes with event 5

		Info()
			: m_ssl(false)
			, m_sslVerify(true)
		{
		}
	};

	struct WhoisInfo
	{
		bool found;				//! if no such nick
		std::string nick;			//! user's nickname
		std::string user;			//! user's user
		std::string host;			//! hostname
		std::string realname;			//! realname
		std::vector<std::string> channels;	//! channels
	};

	struct Identity {
		std::string m_name;			//! identity name
		std::string m_nickname;			//! user nickname
		std::string m_username;			//! IRC client user
		std::string m_realname;			//! user real name
		std::string m_ctcpversion;		//! CTCP version

		Identity()
		{
			m_name = "__irccd__";
			m_nickname = "irccd";
			m_username = "irccd";
			m_realname = "IRC Client Daemon";
			m_ctcpversion = "IRC Client Daemon (dev)";
		}
	};

	typedef std::unordered_map<std::string, std::vector<std::string>> NameList;
	typedef std::unordered_map<std::string, WhoisInfo> WhoisList;

private:
	// IRC thread
	irc_callbacks_t m_callbacks;		//! callbacks for libircclient
	std::thread m_thread;			//! server's thread
	IrcSession m_session;			//! libircclient session
	bool m_threadStarted;			//! thread's status

	// For deferred events
	NameList m_nameLists;			//! channels names to receive
	WhoisList m_whoisLists;			//! list of whois

	/**
	 * Initialize callbacks.
	 */
	void init();

protected:
	Info m_info;				//! server info
	Identity m_identity;			//! identity to use
	Options m_options;			//! some options

	/**
	 * Default constructor. Should not be used.
	 */
	Server();

public:	
	/**
	 * Convert the s context to a shared_ptr<Server>.
	 *
	 * @param s the session
	 * @return the shared_ptr.
	 */
	static std::shared_ptr<Server> toServer(irc_session_t *s);

	/**
	 * Better constructor with information and options.
	 *
	 * @param info the host info
	 * @param identity the identity
	 * @param options some options
	 */
	Server(const Info &info, const Identity &identity, const Options &options);

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
	NameList & getNameLists();

	/**
	 * Get the whois lists to build /whois reporting.
	 *
	 * @return the current list
	 */
	WhoisList & getWhoisLists();

	/**
	 * Get the server information.
	 *
	 * @return the server information
	 */
	const Info & getInfo() const;

	/**
	 * Get the identity used for that server
	 *
	 * @return the identity
	 */
	const Identity & getIdentity() const;

	/**
	 * Get the server options.
	 *
	 * @return the options
	 */
	const Options & getOptions() const;

	/**
	 * Get all channels that will be auto joined
	 *
	 * @return the list of channels
	 */
	const std::vector<Channel> & getChannels() const;

	/**
	 * Add a channel that the server will connect to when the
	 * connection is complete.
	 *
	 * @param name the channel name
	 * @param password an optional channel password
	 */
	void addChannel(const std::string &name, const std::string &password = "");

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
	bool hasPrefix(const std::string &nick);

	/**
	 * Remove a channel from the server list.
	 *
	 * @param name the channel name
	 */
	void removeChannel(const std::string &name);

	/**
	 * Start listening on that server, this will create a thread that
	 * can be stopped with stopConnection();
	 *
	 * @see stopConnection
	 */
	void startConnection();

	/**
	 * Stop the connection on that server.
	 */
	void stopConnection();

	/* ------------------------------------------------
	 * IRC commands
	 * ------------------------------------------------ */

	/**
	 * Send a notice to a public channel.
	 *
	 * @param channel the target channel
	 * @param message the message to send
	 */
	virtual void cnotice(const std::string &channel, const std::string &message);

	/**
	 * Invite someone to a channel.
	 *
	 * @param target the target nickname
	 * @param channel the channel
	 */
	virtual void invite(const std::string &target, const std::string &channel);

	/**
	 * Join a channel.
	 *
	 * @param channel the channel name
	 * @param password an optional password
	 */
	virtual void join(const std::string &name, const std::string &password = "");

	/**
	 * Kick someone from a channel.
	 *
	 * @param name the nick name
	 * @param channel the channel from
	 * @param reason an optional reason
	 */
	virtual void kick(const std::string &name, const std::string &channel, const std::string &reason = "");

	/**
	 * Send a CTCP ACTION known as /me.
	 *
	 * @param target the nickname or channel
	 * @param message the message to send
	 */
	virtual void me(const std::string &target, const std::string &message);

	/**
	 * Change the channel mode.
	 *
	 * @param channel the target channel
	 * @param mode the mode
	 */
	virtual void mode(const std::string &channel, const std::string &mode);

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
	virtual void notice(const std::string &nickname, const std::string &message);

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
	virtual void query(const std::string &who, const std::string &message);

	/**
	 * Say something to a channel or to a nickname.
	 *
	 * @param target the nickname or channel
	 * @param message the message to send
	 */
	virtual void say(const std::string &target, const std::string &message);

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
	virtual void topic(const std::string &channel, const std::string &topic);

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

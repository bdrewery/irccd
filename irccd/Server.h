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

#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <libircclient.h>

namespace irccd {

struct Identity {
	std::string m_name;		/*! identity name */
	std::string m_nickname;		/*! user nickname */
	std::string m_username;		/*! IRC client user */
	std::string m_realname;		/*! user real name */
	std::string m_ctcpversion;	/*! CTCP version */

	Identity(void)
	{
		m_name = "__irccd__";
		m_nickname = "irccd";
		m_username = "irccd";
		m_realname = "IRC Client Daemon";
		m_ctcpversion = "IRC Client Daemon (dev)";
	}

	~Identity(void)
	{
	}
};

class IrcDeleter {
public:
	void operator()(irc_session_t *s)
	{
		irc_destroy_session(s);
	}
};

/**
 * Server class, each class define a server that irccd
 * can connect to
 */
class Server {
public:
	struct Channel {
		std::string m_name;		/*! channel name */
		std::string m_password;		/*! channel optional password */
	};

private:
	// Some options
	std::string m_commandChar;		/*! command token */
	bool m_joinInvite;			/*! auto join on invites */
	bool m_ctcpReply;			//! auto reply CTCP
	std::vector<Channel> m_channels;	/*! list of channels */

	// Connection settings
	Identity m_identity;			/*! identity to use */
	std::string m_name;			/*! server's name */
	std::string m_host;			/*! hostname */
	unsigned m_port;			/*! server's port */
	std::string m_password;			/*! optional server password */

	// IRC thread
	irc_callbacks_t m_callbacks;		//! callbacks for libircclient
	std::thread m_thread;			/*! server's thread */
	std::unique_ptr<irc_session_t, IrcDeleter> m_session;
	bool m_threadStarted;			/*! thread's status */

public:
	Server();

	Server(Server &&src);

	virtual ~Server();

	/**
	 * Get the command character.
	 *
	 * @return the command character
	 */
	const std::string & getCommandChar() const;

	/**
	 * Set the command character.
	 *
	 * @param commandChar the command character
	 */
	void setCommandChar(const std::string &commandChar);

	/**
	 * Tell if we should join channel on /invite commands
	 *
	 * @return true if we should
	 */
	bool autoJoinInvite() const;

	/**
	 * Set the join invite mode
	 *
	 * @param joinInvite the mode
	 */
	void setJoinInvite(bool joinInvite);

	/**
	 * Set if we should auto reply to ctcp request.
	 *
	 * @param autoCtcpReply true if we should
	 */
	void setAutoCtcpReply(bool autoCtcpReply);

	/**
	 * Tell if we should auto reply to ctcp request.
	 *
	 * @return true if we should
	 */
	bool autoCtcpReply() const;

	/**
	 * Get all channels that will be auto joined
	 *
	 * @return the list of channels
	 */
	const std::vector<Channel> & getChannels();

	/**
	 * Get the identity used for that server
	 *
	 * @return the identity
	 */
	Identity & getIdentity();

	/**
	 * Get the server instance name. This is only used
	 * to reference a server from irccd controllers.
	 *
	 * @return the name
	 */
	const std::string & getName() const;

	/**
	 * Get the server host name.
	 *
	 * @return the hostname
	 */
	const std::string & getHost() const;

	/**
	 * Get the port connection.
	 *
	 * @return the port used
	 */
	unsigned getPort() const;

	/**
	 * Set connections settings.
	 *
	 * @param name the server resource name
	 * @param host the hostname
	 * @param port the port
	 * @param ssl true if SSL is required
	 * @param password an optional password
	 */
	void setConnection(const std::string &name, const std::string &host,
			   unsigned port, bool ssl = false, const std::string &password = "");

	/**
	 * Set the user identity.
	 *
	 * @param identity the identity to use
	 */
	void setIdentity(const Identity &identity);

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
};

} // !irccd

#endif // !_SERVER_H_

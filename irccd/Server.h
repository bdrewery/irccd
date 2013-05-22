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
		m_nickname = "irccd";
		m_username = "irccd";
		m_realname = "IRC Client Daemon";
		m_ctcpversion = "IRC Client Daemon (dev)";
	}

	~Identity(void)
	{
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
	std::vector<Channel> m_channels;	/*! list of channels */

	// Connection settings
	Identity m_identity;			/*! identity to use */
	std::string m_name;			/*! server's name */
	std::string m_host;			/*! hostname */
	unsigned m_port;			/*! server's port */
	std::string m_password;			/*! optional server password */

	// IRC thread
	std::thread m_thread;			/*! server's thread */
	bool m_threadStarted;			/*! thread's status */
	irc_session_t *m_session;		/*! libircclient session */

public:
	Server(void);
	~Server(void);

	/**
	 * Get the command character.
	 *
	 * @return the command character
	 */
	const std::string & getCommandChar(void) const;

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
	bool getJoinInvite(void) const;

	/**
	 * Set the join invite mode
	 *
	 * @param joinInvite the mode
	 */
	void setJoinInvite(bool joinInvite);

	/**
	 * Get all channels that will be auto joined
	 *
	 * @return the list of channels
	 */
	const std::vector<Channel> & getChannels(void);

	/**
	 * Get the identity used for that server
	 *
	 * @return the identity
	 */
	const Identity & getIdentity(void) const;

	/**
	 * Get the server instance name. This is only used
	 * to reference a server from irccd controllers.
	 *
	 * @return the name
	 */
	const std::string & getName(void) const;

	/**
	 * Get the server host name.
	 *
	 * @return the hostname
	 */
	const std::string & getHost(void) const;

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
	 * Start listening on that server, this will create a thread that
	 * can be stopped with stopConnection();
	 *
	 * @see stopConnection
	 */
	void startConnection(void);

	/**
	 * Stop the connection on that server.
	 */
	void stopConnection(void);

	/* ------------------------------------------------
	 * IRC commands
	 * ------------------------------------------------ */

	/**
	 * Join a channel.
	 *
	 * @param channel the channel name
	 * @param password an optional password
	 */
	void join(const std::string &name, const std::string &password = "");

	/**
	 * Kick someone from a channel.
	 *
	 * @param name the nick name
	 * @param channel the channel from
	 * @param reason an optional reason
	 */
	void kick(const std::string &name, const std::string &channel, const std::string &reason = "");

	/**
	 * Send a CTCP ACTION known as /me.
	 *
	 * @param target the nickname or channel
	 * @param message the message to send
	 */
	void me(const std::string &target, const std::string &message);

	/**
	 * Change your nickname.
	 *
	 * @param nick the new nickname
	 */
	void nick(const std::string &nick);

	/**
	 * Leave a channel.
	 *
	 * @param channel the channel to leave
	 */
	void part(const std::string &channel);

	/**
	 * Say something to a channel or to a nickname.
	 *
	 * @param target the nickname or channel
	 * @param message the message to send
	 */
	void say(const std::string &target, const std::string &message);

	/**
	 * Change a channel topic.
	 *
	 * @param channel the channel target
	 * @param topic the new topic
	 */
	void topic(const std::string &channel, const std::string &topic);
};

} // !irccd

#endif // !_SERVER_H_

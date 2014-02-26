/*
 * IrcSession.h -- libircclient wrapper
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

#ifndef _IRC_SESSION_H_
#define _IRC_SESSION_H_

#include <memory>

#include <libircclient.h>
#include <libirc_rfcnumeric.h>

namespace irccd {

class Server;

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
	 * Convert the s context to a shared_ptr<Server>.
	 *
	 * @param s the session
	 * @return the shared_ptr.
	 */
	static std::shared_ptr<Server> toServer(irc_session_t *);

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

	/**
	 * Connect to the server.
	 *
	 * @param server the server
	 */
	void connect(std::shared_ptr<Server> server);

	/**
	 * Run forever.
	 */
	void run();

	/**
	 * Send a notice to a public channel.
	 *
	 * @param channel the target channel
	 * @param message the message to send
	 */
	void cnotice(const std::string &channel,
		     const std::string &message);

	/**
	 * Invite someone to a channel.
	 *
	 * @param target the target nickname
	 * @param channel the channel
	 */
	void invite(const std::string &target,
		    const std::string &channel);

	/**
	 * Join a channel.
	 *
	 * @param channel the channel name
	 * @param password an optional password
	 */
	void join(const std::string &channel,
		  const std::string &password);

	/**
	 * Kick someone from a channel.
	 *
	 * @param name the nick name
	 * @param channel the channel from
	 * @param reason an optional reason
	 */
	void kick(const std::string &name,
		  const std::string &channel,
		  const std::string &reason);

	/**
	 * Send a CTCP ACTION known as /me.
	 *
	 * @param target the nickname or channel
	 * @param message the message to send
	 */
	void me(const std::string &target,
		const std::string &message);

	/**
	 * Change the channel mode.
	 *
	 * @param channel the target channel
	 * @param mode the mode
	 */
	void mode(const std::string &channel,
		  const std::string &mode);

	/**
	 * Get the list of names as a deferred call.
	 *
	 * @param channel which channel
	 * @param plugin the plugin to call on end of list
	 * @param ref the function reference
	 */
	void names(const std::string &channel);

	/**
	 * Change your nickname.
	 *
	 * @param nick the new nickname
	 */
	void nick(const std::string &newnick);

	/**
	 * Send a notice to someone.
	 *
	 * @param nickname the target nickname
	 * @param message the message
	 */
	void notice(const std::string &target,
		    const std::string &message);

	/**
	 * Leave a channel.
	 *
	 * @param channel the channel to leave
	 * @param reason an optional reason
	 */
	void part(const std::string &channel,
		  const std::string &reason);

	/**
	 * Send a query message.
	 *
	 * @param who the target nickname
	 * @param message the message
	 */
	void query(const std::string &target,
		   const std::string &message);

	/**
	 * Say something to a channel or to a nickname.
	 *
	 * @param target the nickname or channel
	 * @param message the message to send
	 */
	void say(const std::string &channel,
		 const std::string &message);

	/**
	 * Change a channel topic.
	 *
	 * @param channel the channel target
	 * @param topic the new topic
	 */
	void topic(const std::string &channel,
		   const std::string &topic);

	/**
	 * Change your own user mode.
	 *
	 * @param mode the mode
	 */
	void umode(const std::string &mode);

	/**
	 * Get the whois information from a user.
	 *
	 * @param target the nickname target
	 */
	void whois(const std::string &target);

	/**
	 * Send a raw message, no need to finish with \r\n.
	 *
	 * @param raw the raw message
	 */
	void send(const std::string &raw);

	void disconnect();
};

} // !irccd

#endif // !_IRC_SESSION_H_

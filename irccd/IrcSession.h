/*
 * IrcSession.h -- libircclient wrapper
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

#ifndef _IRCCD_IRC_SESSION_H_
#define _IRCCD_IRC_SESSION_H_

/**
 * @file IrcSession.h
 * @brief Wrapper for libircclient
 */

#include <memory>

#include <libircclient.h>
#include <libirc_rfcnumeric.h>

namespace irccd {

class Server;

/**
 * @class IrcSession
 * @brief Wrapper for irc_session_t
 */
class IrcSession {
private:
	//using Ptr = std::unique_ptr<irc_session_t, void (*)(irc_session_t *)>;

	std::unique_ptr<irc_session_t, void (*)(irc_session_t *)> m_handle;

	/**
	 * Call a libircclient function with its parameters, if the function
	 * returns an error, we check if the event queue is fulled, in that
	 * case we return false.
	 *
	 * @param func the function
	 * @param args the arguments
	 * @return true if the message was sent
	 */
	template <typename Func, typename... Args>
	bool call(Func func, Args&&... args)
	{
		int ret = func(m_handle.get(), std::forward<Args>(args)...);

		if (ret != 0 && irc_errno(m_handle.get()) == LIBIRC_ERR_NOMEM)
			return false;

		return true;
	}

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
	 * @return true if the message was sent
	 */
	bool cnotice(const std::string &channel, const std::string &message);

	/**
	 * Invite someone to a channel.
	 *
	 * @param target the target nickname
	 * @param channel the channel
	 * @return true if the message was sent
	 */
	bool invite(const std::string &target, const std::string &channel);

	/**
	 * Join a channel.
	 *
	 * @param channel the channel name
	 * @param password an optional password
	 * @return true if the message was sent
	 */
	bool join(const std::string &channel, const std::string &password);

	/**
	 * Kick someone from a channel.
	 *
	 * @param name the nick name
	 * @param channel the channel from
	 * @param reason an optional reason
	 * @return true if the message was sent
	 */
	bool kick(const std::string &name, const std::string &channel, const std::string &reason);

	/**
	 * Send a CTCP ACTION known as /me.
	 *
	 * @param target the nickname or channel
	 * @param message the message to send
	 * @return true if the message was sent
	 */
	bool me(const std::string &target, const std::string &message);

	/**
	 * Change the channel mode.
	 *
	 * @param channel the target channel
	 * @param mode the mode
	 * @return true if the message was sent
	 */
	bool mode(const std::string &channel, const std::string &mode);

	/**
	 * Get the list of names as a deferred call.
	 *
	 * @param channel which channel
	 * @return true if the message was sent
	 */
	bool names(const std::string &channel);

	/**
	 * Change your nickname.
	 *
	 * @param newnick the new nickname
	 * @return true if the message was sent
	 */
	bool nick(const std::string &newnick);

	/**
	 * Send a notice to someone.
	 *
	 * @param target the target nickname
	 * @param message the message
	 * @return true if the message was sent
	 */
	bool notice(const std::string &target, const std::string &message);

	/**
	 * Leave a channel.
	 *
	 * @param channel the channel to leave
	 * @param reason an optional reason
	 * @return true if the message was sent
	 */
	bool part(const std::string &channel, const std::string &reason);

	/**
	 * Say something to a channel or to a nickname.
	 *
	 * @param target the nickname or channel
	 * @param message the message to send
	 * @return true if the message was sent
	 */
	bool say(const std::string &target, const std::string &message);

	/**
	 * Change a channel topic.
	 *
	 * @param channel the channel target
	 * @param topic the new topic
	 * @return true if the message was sent
	 */
	bool topic(const std::string &channel, const std::string &topic);

	/**
	 * Change your own user mode.
	 *
	 * @param mode the mode
	 * @return true if the message was sent
	 */
	bool umode(const std::string &mode);

	/**
	 * Get the whois information from a user.
	 *
	 * @param target the nickname target
	 * @return true if the message was sent
	 */
	bool whois(const std::string &target);

	/**
	 * Send a raw message, no need to finish with \\r\\n.
	 *
	 * @param raw the raw message
	 * @return true if the message was sent
	 */
	bool send(const std::string &raw);

	/**
	 * Disconnect the session.
	 */
	void disconnect();
};

} // !irccd

#endif // !_IRCCD_IRC_SESSION_H_

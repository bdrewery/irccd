/*
 * Server.h -- a IRC server to connect to
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

#ifndef _IRCCD_SERVER_H_
#define _IRCCD_SERVER_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include <libircclient.h>

#include <IrccdConfig.h>
#include <Logger.h>
#include <Signals.h>

#include "ServerState.h"

namespace irccd {

class ServerIdentity {
private:
	std::string m_name;		//!< identity name
	std::string m_nickname;		//!< nickname to show
	std::string m_username;		//!< username to use for connection
	std::string m_realname;		//!< the full real name
	std::string m_ctcpversion;	//!< the CTCP version to define

public:
	ServerIdentity(std::string name = "irccd",
		       std::string nickname = "irccd",
		       std::string username = "irccd",
		       std::string realname = "IRC Client Daemon",
		       std::string ctcpversion = "IRC Client Daemon")
		: m_name(std::move(name))
		, m_nickname(std::move(nickname))
		, m_username(std::move(username))
		, m_realname(std::move(realname))
		, m_ctcpversion(std::move(ctcpversion))
	{
	}

	inline const std::string &name() const noexcept
	{
		return m_name;
	}

	inline const std::string &nickname() const noexcept
	{
		return m_nickname;
	}

	inline void setNickname(std::string nickname) noexcept
	{
		m_nickname = std::move(nickname);
	}

	inline const std::string &username() const noexcept
	{
		return m_username;
	}

	inline const std::string &realname() const noexcept
	{
		return m_realname;
	}

	inline const std::string &ctcpversion() const noexcept
	{
		return m_ctcpversion;
	}
};

/**
 * @class ServerChannel
 * @brief A channel to join with an optional password
 */
class ServerChannel {
public:
	std::string name;		//!< the channel to join
	std::string password;		//!< the optional password
};

/**
 * List of channels.
 */
using ServerChannels = std::vector<ServerChannel>;

/**
 * @enum ServerMode
 * @brief Prefixes for nicknames
 */
enum class ServerMode {
	Creator		= 'O',			//!< channel creator
	HalfOperator	= 'h',			//!< half operator
	Operator	= 'o',			//!< channel operator
	Protection	= 'a',			//!< unkillable
	Voiced		= 'v'			//!< voice power
};

/**
 * @class ServerWhois
 * @brief Describe a whois information
 *
 * This is provided when whois command was requested.
 */
class ServerWhois {
public:
	bool found{false};			//!< if no such nick
	std::string nick;			//!< user's nickname
	std::string user;			//!< user's user
	std::string host;			//!< hostname
	std::string realname;			//!< realname
	std::vector<std::string> channels;	//!< the channels where the user is
};

/**
 * @class ServerInfo
 * @brief Server information
 *
 * This class contains everything needed to connect to a server.
 */
class ServerInfo {
public:
	std::string name;			//!< server's name
	std::string host;			//!< hostname
	std::string password;			//!< optional server password
	uint16_t port{6667};			//!< server's port
	bool ipv6{false};			//!< use IPv6?
	bool ssl{false};			//!< use SSL?
	bool sslverify{false};			//!< verify SSL?
};

/**
 * @class ServerSettings
 * @brief Contains settings to tweak the server
 *
 * This class contains additional settings that tweaks the
 * server operations.
 */
class ServerSettings {
public:
	ServerChannels channels;	//!< list of channel to join
	std::string command{"!"};	//!< the command character to trigger plugin command
	int recotries{3};		//!< number of tries to reconnect before giving up
	int recotimeout{30};		//!< number of seconds to wait before trying to connect
	int recocurrent{1};		//!< number of tries tested
	bool autorejoin{false};		//!< auto rejoin after a kick?
};

/**
 * Deferred command to send to the server.
 *
 * If the command returns true, it has been correctly buffered for outgoing
 * and removed from the queue.
 */
using ServerCommand = std::function<bool ()>;

/**
 * @class Server
 * @brief The class that connect to a IRC server
 *
 * The server is a class that stores callbacks which will be called on IRC
 * events. It is the lowest part of the connection to a server, it can be used
 * directly by the user to connect to a server.
 *
 * It is primarily designed to be added in the ServerManager which will dispatch
 * these events automatically in the Irccd event queue.
 *
 * Only functions marked as "Thread safe" are actually thread safe, otherwise
 * please read the comment for explanation.
 *
 * All handlers must not be changed if the server has been installed into a
 * ServerManager.
 *
 * <h3>The hierarchy / association diagram</h3>
 *
 * @code
 *                              m_state: std::unique_ptr<ServerState>
 * +---------------+               +---------------+
 * | Server        |---------------| ServerState   |
 * +---------------+             1 +---------------+
 *         |
 *         |
 *       1 | m_queue: std::queue<std::unique_ptr<ServerCommand>>
 * +---------------+
 * | ServerCommand |
 * +---------------+
 * @endcode
 *
 * <h3>The activity between threads</h3>
 *
 * When you use ServerManager to control servers, the activity has the following
 * workflow:
 *
 * @code
 *    Main loop          ServerManager (thread)                 Server                 Other threads
 *        |                    |                                   |                         |
 *        |                    | For all servers, do:              |                         |
 *        |                    v                                   |<------------------------|
 *        |            +---------------+                           |     cnotice(), message(), etc.
 *        |            | Update states |                           |         ^ Enqueue a command, lock for m_queue.
 *        |            +---------------+                           |
 * +---------------+           |      ^ Call Server::update        |          The user must not use any of the other functions.
 * | Wait          |           v                                   |
 * +---------------+   +---------------+                           |
 *         |           | Flush queue   |                           |
 *         |           +---------------+                           |
 *         |                   |      ^ Call Server::flush         |
 *         |                   v                                   |
 *         |           +---------------+                   +---------------+
 *         |           | Prepare       |------------------>| prepare()     |
 *         |           +---------------+                   +---------------+
 *         |                   |      ^ Call Server::prepare       |      ^ May change the m_state and reset session.
 *         |                   v                                   |
 *         |           +---------------+                   +---------------+
 *         |           | Process I/O   |------------------>| process()     |
 *         |           +---------------+                   +---------------+
 *         |                   |      ^ Call Server::process       |      ^ May call server callbacks.
 *         |                   |                                   |
 *         |<------------------|<----------------------------------|
 *            ^ Enqueue the event    ^
 *                                   | Notify with m_on* appropriate function
 * @endcode
 *
 * <h3>Processing the server</h3>
 *
 * To process the server, use the following functions:
 *
 * - update(), not thread safe, should only be used in the loop controlling the
 *   server. This function will change the state if needed.
 * - flush(), thread safe, this function will flush the queue of commands if
 *   possible. Should be used in the server loop.
 * - prepare(), not thread safe, this function will fill the appropriate fd_set
 *   and may also switch the server state if needed.
 * - process(), not thread safe, this function should be used after the select()
 *   call to dispatch the incoming/outgoing data.
 *
 * <h3>The server states</h3>
 *
 * The server state <strong>MUST</strong> must only use:
 *
 * - next()
 * - info()
 * - settings()
 *
 * For more information, see also ServerState.h
 *
 * <h3>The server commands</h3>
 *
 * The server contains several commands that are used to send data to the IRC
 * server,
 *
 * They try to send as much as possible data, they enqueue'ed to the server
 * using one of the user commands like cnotice(), message(), etc.
 *
 * They are then called via the flush() function and should never be called
 * anywhere else because it depends on m_session.
 *
 * For more information, see also ServerCommand.h
 */
class Server {
public:
#if defined(WITH_JS)
	/**
	 * Object name for JS API.
	 */
	static constexpr const char *JsName{"Server"};
#endif

	/* ------------------------------------------------
	 * Signals
	 * ------------------------------------------------ */

	/**
	 * ServerEvent: onConnect
	 * ------------------------------------------------
	 *
	 * Triggered when the server is successfully connected.
	 */
	Signal<> onConnect;
	

	/**
	 * ServerEvent: onChannelNotice
	 * ------------------------------------------------
	 *
	 * Triggered when a notice has been sent on a channel.
	 *
	 * Arguments:
	 * - the origin (the nickname who has sent the notice)
	 * - the channel name
	 * - the notice message
	 */
	Signal<std::string, std::string, std::string> onChannelNotice;

	/**
	 * ServerEvent: onInvite
	 * ------------------------------------------------
	 *
	 * Triggered when an invite has been sent to you (the bot).
	 *
	 * Arguments:
	 * - the origin
	 * - the channel
	 * - your nickname
	 */
	Signal<std::string, std::string, std::string> onInvite;

	/**
	 * ServerEvent: onJoin
	 * ------------------------------------------------
	 *
	 * Triggered when a user has joined the channel, it also includes you.
	 *
	 * Arguments:
	 * - the origin (may be you)
	 * - the channel
	 */
	Signal<std::string, std::string> onJoin;

	/**
	 * ServerEvent: onKick
	 * ------------------------------------------------
	 *
	 * Triggered when someone has been kicked from a channel.
	 *
	 * Arguments:
	 * - the origin
	 * - the channel
	 * - the target who has been kicked
	 * - the optional reason
	 */
	Signal<std::string, std::string, std::string, std::string> onKick;

	/**
	 * ServerEvent: onMessage
	 * ------------------------------------------------
	 *
	 * Triggered when a message on a channel has been sent.
	 *
	 * Arguments:
	 * - the origin
	 * - the channel
	 * - the message
	 */
	Signal<std::string, std::string, std::string> onMessage;

	/**
	 * ServerEvent: onMe
	 * ------------------------------------------------
	 *
	 * Triggered on a CTCP Action.
	 *
	 * This is both used in a channel and in a private message so the target
	 * may be a channel or your nickname.
	 *
	 * Arguments:
	 * - the origin
	 * - the target
	 * - the message
	 */
	Signal<std::string, std::string, std::string> onMe;

	/**
	 * ServerEvent: onMode
	 * ------------------------------------------------
	 *
	 * Triggered when someone changed the channel mode.
	 *
	 * Arguments:
	 * - the origin
	 * - the channel
	 * - the mode
	 * - the optional mode argument
	 */
	Signal<std::string, std::string, std::string, std::string> onMode;

	/**
	 * ServerEvent: onNick
	 * ------------------------------------------------
	 *
	 * Triggered when someone changed its nickname, it also includes you.
	 *
	 * Arguments:
	 * - the old nickname (may be you)
	 * - the new nickname
	 */
	Signal<std::string, std::string> onNick;

	/**
	 * ServerEvent: onNotice
	 * ------------------------------------------------
	 *
	 * Triggered when someone has sent a notice to you.
	 *
	 * Arguments:
	 * - the origin
	 * - the notice message
	 */
	Signal<std::string, std::string> onNotice;

	/**
	 * ServerEvent: onPart
	 * ------------------------------------------------
	 *
	 * Triggered when someone has left the channel.
	 *
	 * Arguments:
	 * - the origin
	 * - the channel that the nickname has left
	 * - the optional reason
	 */
	Signal<std::string, std::string, std::string> onPart;

	/**
	 * ServerEvent: onQuery
	 * ------------------------------------------------
	 *
	 * Triggered when someone has sent you a private message.
	 *
	 * Arguments:
	 * - the origin
	 * - the message
	 */
	Signal<std::string, std::string> onQuery;

	/**
	 * ServerEvent: onTopic
	 * ------------------------------------------------
	 *
	 * Triggered when someone changed the channel topic.
	 *
	 * Arguments:
	 * - the origin
	 * - the channel
	 * - the new topic
	 */
	Signal<std::string, std::string, std::string> onTopic;

	/**
	 * ServerEvent: onUserMode
	 * ------------------------------------------------
	 *
	 * Triggered when the server changed your user mode.
	 *
	 * Arguments:
	 * - the origin
	 * - the mode (e.g +i)
	 */
	Signal<std::string, std::string> onUserMode;

private:
	using Session = std::unique_ptr<irc_session_t, void (*)(irc_session_t *)>;
	using Mutex = std::mutex;
	using Queue = std::queue<ServerCommand>;

private:
	ServerInfo m_info;
	ServerSettings m_settings;
	ServerIdentity m_identity;
	Session m_session;
	ServerState m_state;
	ServerState m_next;
	Queue m_queue;
	mutable Mutex m_mutex;

	void handleChannel(const char *, const char **) noexcept;
	void handleChannelNotice(const char *, const char **) noexcept;
	void handleConnect(const char *, const char **) noexcept;
	void handleCtcpAction(const char *, const char **) noexcept;
	void handleInvite(const char *, const char **) noexcept;
	void handleJoin(const char *, const char **) noexcept;
	void handleKick(const char *, const char **) noexcept;
	void handleMode(const char *, const char **) noexcept;
	void handleNick(const char *, const char **) noexcept;
	void handleNotice(const char *, const char **) noexcept;
	void handlePart(const char *, const char **) noexcept;
	void handleQuery(const char *, const char **) noexcept;
	void handleTopic(const char *, const char **) noexcept;
	void handleUserMode(const char *, const char **) noexcept;

	/*
	 * Return a string even if s is nullptr.
	 */
	inline std::string strify(const char *s)
	{
		return (s == nullptr) ? "" : std::string(s);
	}

	/*
	 * Add a command safely
	 */
	void addCommand(ServerCommand command)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		m_queue.push(std::move(command));
	}

public:
	/**
	 * Construct a server.
	 *
	 * @param info the information
	 * @param identity the identity
	 * @param settings the settings
	 */
	Server(ServerInfo info, ServerIdentity identity = {}, ServerSettings settings = {});

	/**
	 * Destructor. Close the connection if needed.
	 */
	virtual ~Server();

	/**
	 * Set the next state to be used. This function is thread safe because
	 * the server manager may set the next state to the current state.
	 *
	 * If the server is installed into the ServerManager, it is called
	 * automatically.
	 *
	 * @param type the new state type
	 * @warning Not thread-safe
	 */
	inline void next(ServerState::Type type)
	{
		m_next = std::move(ServerState(type));
	}

	/**
	 * Switch to next state if it has.
	 *
	 * If the server is installed into the ServerManager, it is called
	 * automatically.
	 *
	 * @warning Not thread-safe
	 */
	inline void update() noexcept
	{
		if (m_next.type() != ServerState::Undefined) {
			Logger::debug() << "server: switching to state: ";

			switch (m_next.type()) {
			case ServerState::Connecting:
				Logger::debug() << "\"Connecting\"" << std::endl;
				break;
			case ServerState::Connected:
				Logger::debug() << "\"Connected\"" << std::endl;
				break;
			case ServerState::Disconnected:
				Logger::debug() << "\"Disconnected\"" << std::endl;
				break;
			case ServerState::Dead:
				Logger::debug() << "\"Dead\"" << std::endl;
				break;
			default:
				break;
			}

			m_state = std::move(m_next);
			m_next = ServerState::Undefined;
		}
	}

	/**
	 * Request to disconnect. This function does not notify the
	 * ServerService.
	 *
	 * @see Irccd::serverDisconnect
	 * @note Thread-safe
	 */
	inline void disconnect() noexcept
	{
		using namespace std::placeholders;

		std::lock_guard<std::mutex> lock(m_mutex);

		irc_disconnect(m_session.get());
		next(ServerState::Type::Dead);
	}

	/**
	 * Asks for a reconnection. This function does not notify the
	 * ServerService.
	 *
	 * @see Irccd::serverReconnect
	 * @note Thread-safe
	 */
	inline void reconnect() noexcept
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		irc_disconnect(m_session.get());
		next(ServerState::Type::Connecting);
	}

	/**
	 * Flush the pending commands if possible. This function will send
	 * as much as possible commands.
	 *
	 * If the server is installed into the ServerManager, it is called
	 * automatically.
	 *
	 * @note Thread-safe
	 */
	void flush() noexcept;

	/**
	 * Prepare the IRC Session to the socket.
	 *
	 * If the server is installed into the ServerManager, it is called
	 * automatically.
	 *
	 * @warning Not thread-safe
	 */
	inline void prepare(fd_set &setinput, fd_set &setoutput, int &maxfd) noexcept
	{
		m_state.prepare(*this, setinput, setoutput, maxfd);
	}

	/**
	 * Process incoming/outgoing data after selection.
	 *
	 * If the server is installed into the ServerManager, it is called
	 * automatically.
	 *
	 * @param setinput
	 * @param setoutput
	 * @throw any exception that have been throw from user functions
	 * @warning Not thread-safe
	 */
	inline void process(fd_set &setinput, fd_set &setoutput)
	{
		irc_process_select_descriptors(m_session.get(), &setinput, &setoutput);
	}

	/**
	 * Get the server information.
	 *
	 * @warning This overload should not be used by the user, it is required to
	 *          update the nickname.
	 * @return the server information
	 */
	inline ServerInfo &info() noexcept
	{
		return m_info;
	}

	/**
	 * Get the server information.
	 *
	 * @return the server information
	 */
	inline const ServerInfo &info() const noexcept
	{
		return m_info;
	}

	/**
	 * Get the server settings.
	 *
	 * @warning This overload should not be used by the user, it is required to
	 *          update the reconnection information.
	 * @return the settings
	 */
	inline ServerSettings &settings() noexcept
	{
		return m_settings;
	}

	/**
	 * Get the server settings.
	 *
	 * @return the settings
	 */
	inline const ServerSettings &settings() const noexcept
	{
		return m_settings;
	}

	/**
	 * Get the identity.
	 *
	 * @return the identity
	 */
	inline ServerIdentity &identity() noexcept
	{
		return m_identity;
	}

	/**
	 * Overloaded function
	 *
	 * @return the identity
	 */
	inline const ServerIdentity &identity() const noexcept
	{
		return m_identity;
	}

	/**
	 * Get the current state identifier. Should not be used by user code.
	 *
	 * @note Thread-safe but the state may change just after the call
	 */
	inline ServerState::Type type() const noexcept
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		return m_state.type();
	}

	/**
	 * Get the libircclient session.
	 *
	 * @warning Do not use this function, it is only required for ServerState's
	 * @return the session
	 */
	inline irc_session_t *session() noexcept
	{
		return m_session.get();
	}

	/**
	 * Send a channel notice.
	 *
	 * @param channel the channel
	 * @param message message notice
	 * @note Thread-safe
	 */
	inline void cnotice(std::string channel, std::string message) noexcept
	{
		addCommand([=] () {
			return irc_cmd_notice(this->m_session.get(), channel.c_str(), message.c_str()) == 0;
		});
	}

	/**
	 * Invite a user to a channel.
	 *
	 * @param target the target nickname
	 * @param channel the channel
	 * @note Thread-safe
	 */
	inline void invite(std::string target, std::string channel) noexcept
	{
		addCommand([=] () {
			return irc_cmd_invite(this->m_session.get(), target.c_str(), channel.c_str()) == 0;
		});
	}

	/**
	 * Join a channel, the password is optional and can be kept empty.
	 *
	 * @param channel the channel to join
	 * @param password the optional password
	 * @note Thread-safe
	 */
	inline void join(std::string channel, std::string password = "") noexcept
	{
		addCommand([=] () {
			const char *ptr = password.empty() ? nullptr : password.c_str();

			return irc_cmd_join(this->m_session.get(), channel.c_str(), ptr) == 0;
		});
	}

	/**
	 * Kick someone from the channel. Please be sure to have the rights
	 * on that channel because errors won't be reported.
	 *
	 * @param target the target to kick
	 * @param channel from which channel
	 * @param reason the optional reason
	 * @note Thread-safe
	 */
	inline void kick(std::string target, std::string channel, std::string reason = "") noexcept
	{
		addCommand([=] () {
			return irc_cmd_kick(this->m_session.get(), target.c_str(), channel.c_str(), reason.c_str()) == 0;
		});
	}

	/**
	 * Send a CTCP Action as known as /me. The target may be either a
	 * channel or a nickname.
	 *
	 * @param target the nickname or the channel
	 * @param message the message
	 * @note Thread-safe
	 */
	inline void me(std::string target, std::string message)
	{
		addCommand([=] () {
			return irc_cmd_me(m_session.get(), target.c_str(), message.c_str()) == 0;
		});
	}

	/**
	 * Send a message to the specified target or channel.
	 *
	 * @param target the target
	 * @param message the message
	 * @note Thread-safe
	 */
	inline void message(std::string target, std::string message)
	{
		addCommand([=] () {
			return irc_cmd_msg(m_session.get(), target.c_str(), message.c_str()) == 0;
		});
	}

	/**
	 * Change the channel mode.
	 *
	 * @param channel the channel
	 * @param mode the new mode
	 * @note Thread-safe
	 */
	inline void mode(std::string channel, std::string mode)
	{
		addCommand([=] () {
			return irc_cmd_channel_mode(m_session.get(), channel.c_str(), mode.c_str()) == 0;
		});
	}

	/**
	 * Request the list of names.
	 *
	 * @param channel the channel
	 * @note Thread-safe
	 */
	inline void names(std::string channel)
	{
		addCommand([=] () {
			return irc_cmd_names(m_session.get(), channel.c_str()) == 0;
		});
	}

	/**
	 * Change your nickname.
	 *
	 * @param newnick the new nickname to use
	 * @note Thread-safe
	 */
	inline void nick(std::string newnick)
	{
		addCommand([=] () {
			return irc_cmd_nick(m_session.get(), newnick.c_str()) == 0;
		});
	}

	/**
	 * Send a private notice.
	 *
	 * @param target the target
	 * @param message the notice message
	 * @note Thread-safe
	 */
	inline void notice(std::string target, std::string message)
	{
		addCommand([=] () {
			return irc_cmd_notice(m_session.get(), target.c_str(), message.c_str()) == 0;
		});
	}

	/**
	 * Part from a channel.
	 *
	 * Please note that the reason is not supported on all servers so if
	 * you want portability, don't provide it.
	 *
	 * @param channel the channel to leave
	 * @param reason the optional reason
	 * @note Thread-safe
	 */
	inline void part(std::string channel, std::string /*reason = ""*/)
	{
		addCommand([=] () {
			return irc_cmd_part(m_session.get(), channel.c_str()) == 0;
		});
	}

	/**
	 * Send a raw message to the IRC server. You don't need to add
	 * message terminators.
	 *
	 * @warning Use this function with care
	 * @param raw the raw message (without \r\n\r\n)
	 * @note Thread-safe
	 */
	inline void send(std::string raw)
	{
		addCommand([=] () {
			return irc_send_raw(m_session.get(), raw.c_str()) == 0;
		});
	}

	/**
	 * Change the channel topic.
	 *
	 * @param channel the channel
	 * @param topic the desired topic
	 * @note Thread-safe
	 */
	inline void topic(std::string channel, std::string topic)
	{
		addCommand([=] () {
			return irc_cmd_topic(m_session.get(), channel.c_str(), topic.c_str()) == 0;
		});
	}

	/**
	 * Change your user mode.
	 *
	 * @param mode the mode
	 * @note Thread-safe
	 */
	inline void umode(std::string mode)
	{
		addCommand([=] () {
			return irc_cmd_user_mode(m_session.get(), mode.c_str()) == 0;
		});
	}

	/**
	 * Request for whois information.
	 *
	 * @param target the target nickname
	 * @note Thread-safe
	 */
	inline void whois(std::string target)
	{
		addCommand([=] () {
			return irc_cmd_whois(this->m_session.get(), target.c_str()) == 0;
		});
	}
};

} // !irccd

#if 0

/**
 * Map for IRC prefixes to the character.
 */
using IrcPrefixes	= std::map<IrcChanNickMode, char>;

class Server : public std::enable_shared_from_this<Server> {
public:
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
	using WhoisList	= std::unordered_map<std::string, IrcWhois>;

private:
	NameList	m_nameLists;		//!< channels names to receive
	WhoisList	m_whoisLists;		//!< list of whois

public:
	void extractPrefixes(const std::string &line);
	NameList &nameLists();
	WhoisList &whoisLists();
};

} // !irccd

#endif

#endif // !_IRCCD_SERVER_H_

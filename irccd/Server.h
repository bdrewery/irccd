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

#include "Identity.h"
#include "ServerState.h"
#include "ServerCommand.h"

#include "servercommand/ChannelNotice.h"
#include "servercommand/Invite.h"
#include "servercommand/Join.h"
#include "servercommand/Kick.h"
#include "servercommand/Me.h"
#include "servercommand/Message.h"
#include "servercommand/Mode.h"
#include "servercommand/Names.h"
#include "servercommand/Nick.h"
#include "servercommand/Notice.h"
#include "servercommand/Part.h"
#include "servercommand/Send.h"
#include "servercommand/Topic.h"
#include "servercommand/UserMode.h"
#include "servercommand/Whois.h"

namespace irccd {

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
	uint16_t port{0};			//!< server's port
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
	/**
	 * ServerEvent: onConnect
	 * ------------------------------------------------
	 *
	 * Triggered when the server is successfully connected.
	 */
	using OnConnect = std::function<void ()>;

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
	using OnChannelNotice = std::function<void (std::string, std::string, std::string)>;

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
	using OnInvite = std::function<void (std::string, std::string, std::string)>;

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
	using OnJoin = std::function<void (std::string, std::string)>;

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
	using OnKick = std::function<void (std::string, std::string, std::string, std::string)>;

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
	using OnMessage = std::function<void (std::string, std::string, std::string)>;

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
	using OnMe = std::function<void (std::string, std::string, std::string)>;

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
	using OnMode = std::function<void (std::string, std::string, std::string, std::string)>;

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
	using OnNick = std::function<void (std::string, std::string)>;

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
	using OnNotice = std::function<void (std::string, std::string)>;

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
	using OnPart = std::function<void (std::string, std::string, std::string)>;

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
	using OnQuery = std::function<void (std::string, std::string)>;

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
	using OnTopic = std::function<void (std::string, std::string, std::string)>;

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
	using OnUserMode = std::function<void (std::string, std::string)>;

private:
	using Session = std::unique_ptr<irc_session_t, void (*)(irc_session_t *)>;
	using State = std::unique_ptr<ServerState>;
	using Mutex = std::mutex;
	using Queue = std::queue<std::unique_ptr<ServerCommand>>;

private:
	ServerInfo m_info;
	ServerSettings m_settings;
	Identity m_identity;
	Session m_session;
	State m_state;
	State m_next;
	Queue m_queue;
	mutable Mutex m_mutex;

	// All events
	OnConnect m_onConnect;
	OnChannelNotice m_onChannelNotice;
	OnInvite m_onInvite;
	OnJoin m_onJoin;
	OnKick m_onKick;
	OnMessage m_onMessage;
	OnMe m_onMe;
	OnMode m_onMode;
	OnNick m_onNick;
	OnNotice m_onNotice;
	OnPart m_onPart;
	OnQuery m_onQuery;
	OnTopic m_onTopic;
	OnUserMode m_onUserMode;

private:
	/*
	 * Wrappers for libircclient callbacks to our std::functions, the
	 * irc_callbacks_t structure is filled with lambdas that call
	 * these appropriate functions.
	 *
	 * The signatures do not match the irc_event_callback_t function
	 * because we have discarded useless parameter.
	 */
	template <typename Function, typename... Args>
	inline void wrapHandler(Function &function, Args&&... args) noexcept
	{
		if (function) {
			function(std::forward<Args>(args)...);
		}
	}

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
	template <typename Command, typename... Args>
	void addCommand(Args&&... args)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		m_queue.push(std::make_unique<Command>(std::forward<Args>(args)...));
	}

public:
	/**
	 * Construct a server.
	 *
	 * @param info the information
	 * @param identity the identity
	 * @param settings the settings
	 */
	Server(ServerInfo info, Identity identity = {}, ServerSettings settings = {});

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
	 * @warning Not thread-safe
	 */
	template <typename State, typename... Args>
	inline void next(Args&&... args)
	{
		m_next = std::make_unique<State>(std::forward<Args>(args)...);
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
		if (m_next) {
			m_state = std::move(m_next);
		}
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
		m_state->prepare(*this, setinput, setoutput, maxfd);
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
	inline Identity &identity() noexcept
	{
		return m_identity;
	}

	/**
	 * Overloaded function
	 *
	 * @return the identity
	 */
	inline const Identity &identity() const noexcept
	{
		return m_identity;
	}

	/**
	 * Get the current state identifier. Should not be used by user code.
	 *
	 * @note Thread-safe but the state may change just after the call
	 */
	inline int state() const noexcept
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		return m_state->state();
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
	 * Set the onConnect handler.
	 *
	 * @param func the function
	 */
	inline void onConnect(OnConnect func) noexcept
	{
		m_onConnect = std::move(func);
	}

	/**
	 * Set the onChannelNotice handler.
	 *
	 * @param func the function
	 */
	inline void onChannelNotice(OnChannelNotice func) noexcept
	{
		m_onChannelNotice = std::move(func);
	}

	/**
	 * Set the onInvite handler.
	 *
	 * @param func the function
	 */
	inline void onInvite(OnInvite func) noexcept
	{
		m_onInvite = std::move(func);
	}

	/**
	 * Set the onJoin handler.
	 *
	 * @param func the function
	 */
	inline void onJoin(OnJoin func) noexcept
	{
		m_onJoin = std::move(func);
	}

	/**
	 * Set the onKick handler.
	 *
	 * @param func the function
	 */
	inline void onKick(OnKick func) noexcept
	{
		m_onKick = std::move(func);
	}

	/**
	 * Set the onMessage handler.
	 *
	 * @param func the function
	 */
	inline void onMessage(OnMessage func) noexcept
	{
		m_onMessage = std::move(func);
	}

	/**
	 * Set the onMe handler.
	 *
	 * @param func the function
	 */
	inline void onMe(OnMe func) noexcept
	{
		m_onMe = std::move(func);
	}

	/**
	 * Set the onMode handler.
	 *
	 * @param func the function
	 */
	inline void onMode(OnMode func) noexcept
	{
		m_onMode = std::move(func);
	}

	/**
	 * Set the onNick handler.
	 *
	 * @param func the function
	 */
	inline void onNick(OnNick func) noexcept
	{
		m_onNick = std::move(func);
	}

	/**
	 * Set the onNotice handler.
	 *
	 * @param func the function
	 */
	inline void onNotice(OnNotice func) noexcept
	{
		m_onNotice = std::move(func);
	}

	/**
	 * Set the onPart handler.
	 *
	 * @param func the function
	 */
	inline void onPart(OnPart func) noexcept
	{
		m_onPart = std::move(func);
	}

	/**
	 * Set the onQuery handler.
	 *
	 * @param func the function
	 */
	inline void onQuery(OnQuery func) noexcept
	{
		m_onQuery = std::move(func);
	}

	/**
	 * Set the onTopic handler.
	 *
	 * @param func the function
	 */
	inline void onTopic(OnTopic func) noexcept
	{
		m_onTopic = std::move(func);
	}

	/**
	 * Set the onUserMode handler.
	 *
	 * @param func the function
	 */
	inline void onUserMode(OnUserMode func) noexcept
	{
		m_onUserMode = std::move(func);
	}

	/**
	 * Send a channel notice.
	 *
	 * @param channel the channel
	 * @param message message notice
	 * @note Thread-safe
	 */
	inline void cnotice(std::string channel, std::string message)
	{
		addCommand<command::ChannelNotice>(*this, std::move(channel), std::move(message));
	}

	/**
	 * Invite a user to a channel.
	 *
	 * @param target the target nickname
	 * @param channel the channel
	 * @note Thread-safe
	 */
	inline void invite(std::string target, std::string channel)
	{
		addCommand<command::Invite>(*this, std::move(target), std::move(channel));
	}

	/**
	 * Join a channel, the password is optional and can be kept empty.
	 *
	 * @param channel the channel to join
	 * @param password the optional password
	 * @note Thread-safe
	 */
	inline void join(std::string channel, std::string password = "")
	{
		addCommand<command::Join>(*this, std::move(channel), std::move(password));
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
	inline void kick(std::string target, std::string channel, std::string reason = "")
	{
		addCommand<command::Kick>(*this, std::move(target), std::move(channel), std::move(reason));
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
		addCommand<command::Me>(*this, std::move(target), std::move(message));
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
		addCommand<command::Mode>(*this, std::move(channel), std::move(mode));
	}

	/**
	 * Request the list of names.
	 *
	 * @param channel the channel
	 * @note Thread-safe
	 */
	inline void names(std::string channel)
	{
		addCommand<command::Names>(*this, std::move(channel));
	}

	/**
	 * Change your nickname.
	 *
	 * @param newnick the new nickname to use
	 * @note Thread-safe
	 */
	inline void nick(std::string newnick)
	{
		addCommand<command::Nick>(*this, std::move(newnick));
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
		addCommand<command::Notice>(*this, std::move(target), std::move(message));
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
	inline void part(std::string channel, std::string reason = "")
	{
		addCommand<command::Part>(*this, std::move(channel), std::move(reason));
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
		addCommand<command::Message>(*this, std::move(target), std::move(message));
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
		addCommand<command::Send>(*this, std::move(raw));
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
		addCommand<command::Topic>(*this, std::move(channel), std::move(topic));
	}

	/**
	 * Change your user mode.
	 *
	 * @param mode the mode
	 * @note Thread-safe
	 */
	inline void umode(std::string mode)
	{
		addCommand<command::UserMode>(*this, std::move(mode));
	}

	/**
	 * Request for whois information.
	 *
	 * @param target the target nickname
	 * @note Thread-safe
	 */
	inline void whois(std::string target)
	{
		addCommand<command::Whois>(*this, std::move(target));
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

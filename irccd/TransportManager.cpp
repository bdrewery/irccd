#include "Json.h"
#include "SocketListener.h"
#include "TransportManager.h"

#include "transportcommand/ChannelNotice.h"
#include "transportcommand/Connect.h"
#include "transportcommand/Disconnect.h"
#include "transportcommand/Invite.h"

namespace irccd {

using namespace transport;

/* --------------------------------------------------------
 * Transport events
 * -------------------------------------------------------- */

/*
 * Send a channel notice
 * --------------------------------------------------------
 *
 * Send a channel notice to the specified channel.
 *
 * {
 *   "command": "cnotice",
 *   "server: "the server name",
 *   "channel": "name",
 *   "message": "the message"
 * }
 */
void TransportManager::cnotice(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	m_onEvent(std::make_unique<ChannelNotice>(
		client,
		want(object, "server").toString(),
		want(object, "channel").toString(),
		want(object, "message").toString()
	));
}

/*
 * Connect to a server
 * --------------------------------------------------------
 *
 * Connect to a server.
 *
 * There are no default argument, everything must be set.
 *
 * {
 *   "command": "connect",
 *   "name": "server ident",
 *   "host": "server host",
 *   "port": 6667,
 *   "ssl": true,
 *   "ssl-verify": true
 * }
 *
 * Responses:
 *   - Error if a server with that name already exists
 */
void TransportManager::connect(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	m_onEvent(std::make_unique<Connect>(
		client,
		want(object, "name").toString(),
		want(object, "host").toString(),
		want(object, "port").toInteger(),
		want(object, "ssl").isTrue(),
		want(object, "ssl-verify").isTrue()
	));
}

/*
 * Disconnect a server
 * --------------------------------------------------------
 *
 * Disconnect from a server.
 *
 * {
 *   "command": "disconnect",
 *   "server: "the server name"
 * }
 *
 * Responses:
 *   - Error if the server does not exist
 */
void TransportManager::disconnect(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	m_onEvent(std::make_unique<Disconnect>(
		client,
		want(object, "server").toString()
	));
}

/*
 * Invite someone
 * --------------------------------------------------------
 *
 * Invite someone to the specified channel.
 *
 * {
 *   "command": "invite",
 *   "server: "the server name",
 *   "target": "the nickname",
 *   "channel": "the channel"
 * }
 */
void TransportManager::invite(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	m_onEvent(std::make_unique<Invite>(
		client,
		want(object, "server").toString(),
		want(object, "target").toString(),
		want(object, "channel").toString()
	));
}

/*
 * Join a channel
 * --------------------------------------------------------
 *
 * Join a channel, you may add an optional password.
 *
 * {
 *   "command": "join",
 *   "server: "the server name",
 *   "channel": "channel name",
 *   "password": "the password"		(Optional)
 * }
 */
void TransportManager::join(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &)
{
}

/*
 * Kick someone from a channel
 * --------------------------------------------------------
 *
 * Kick a target from a channel.
 *
 * {
 *   "command": "kick",
 *   "server: "the server name",
 *   "target": "the nickname",
 *   "channel": "the channel",
 *   "reason": "the optional reason"	(Optional)
 * }
 */
void TransportManager::kick(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &)
{
}

/*
 * Load a plugin.
 * --------------------------------------------------------
 *
 * Load a plugin not already loaded by Irccd.
 *
 * 1. By relative path (searched through all Irccd plugin directories)
 *
 * {
 *   "command": "load",
 *   "name": "plugin"
 * }
 *
 * 2. By path, if not absolute, it is relative to the irccd current working directory.
 *
 * {
 *   "command": "load",
 *   "path": "/opt/irccd/plugins/crazygame.js"
 * }
 *
 * Responses:
 *   - Error if the plugin failed to load or was not found
 */
void TransportManager::load(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &)
{
}

/*
 * Send a CTCP Action
 * --------------------------------------------------------
 *
 * Send a CTCP action knows as /me.
 *
 * {
 *   "command": "me",
 *   "server: "the server name",
 *   "channel": "the channel",
 *   "message": "the message"		(Optional)
 * }
 */
void TransportManager::me(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &)
{
}

/*
 * Change the channel mode
 * --------------------------------------------------------
 *
 * Change the channel mode.
 *
 * {
 *   "command": "mode",
 *   "server: "the server name",
 *   "channel": "channel",
 *   "mode": "mode and its arguments"
 * }
 */
void TransportManager::mode(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &)
{
}

/**
 * Send a notice
 * --------------------------------------------------------
 *
 * Send a notice to a target.
 *
 * {
 *   "command": "notice",
 *   "server: "the server name",
 *   "target": "the nickname",
 *   "message": "the message"
 * }
 */
void TransportManager::notice(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &)
{
}

/*
 * Change the bot nickname
 * --------------------------------------------------------
 *
 * Change the daemon nickname.
 *
 * {
 *   "command": "nick",
 *   "server: "the server name",
 *   "nickname": "the new nickname"
 * }
 */
void TransportManager::nick(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &)
{
}

/*
 * Part from a channel
 * --------------------------------------------------------
 *
 * Leaves a channel. May add an optional reason but it does not work
 * for every servers.
 *
 * {
 *   "command": "part",
 *   "server: "the server name",
 *   "channel": "the channel name",
 *   "reason": "the reason"		(Optional)
 * }
 */
void TransportManager::part(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &)
{
}

/*
 * Force reconnection of a server
 * --------------------------------------------------------
 *
 * May be used to force the reconnexion of a server if Irccd didn't catch
 * the disconnection.
 *
 * {
 *   "command": "reconnect",
 *   "server: "the server name",
 *   "name": "the server name"
 * }
 *
 * Responses:
 *   - Error if the server does not exist
 */
void TransportManager::reconnect(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &)
{
}

/*
 * Reload a plugin
 * --------------------------------------------------------
 *
 * Reload the plugin by name, invokes the onReload function, does not unload and
 * load the plugin.
 *
 * {
 *   "command": "reload",
 *   "name": "crazygame"
 * }
 *
 * Responses:
 *   - Error if the plugin does not exists
 */
void TransportManager::reload(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &)
{
}

/*
 * Say something to a target
 * --------------------------------------------------------
 *
 * Send a message to a target which can be a nickname or a channel.
 *
 * {
 *   "command": "say",
 *   "server: "the server name",
 *   "target": "channel or nickname",
 *   "message": "The message"
 * }
 */
void TransportManager::say(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &)
{
}

/*
 * Change a channel topic
 * --------------------------------------------------------
 *
 * Change the topic on the specified channel.
 *
 * {
 *   "command": "topic",
 *   "server: "the server name",
 *   "channel": "the channel name",
 *   "topic": "the new topic"		(Optional)
 * }
 */
void TransportManager::topic(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &)
{
}

/*
 * Set the irccd user mode
 * --------------------------------------------------------
 *
 * Change your irccd user mode for the specified server.
 *
 * {
 *   "command": "umode",
 *   "server: "the server name",
 *   "mode": "the mode"
 * }
 */
void TransportManager::umode(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &)
{
}

/*
 * Unload a plugin
 * --------------------------------------------------------
 *
 * Unload a plugin by its name. Also invokes the onUnload function
 * before removing it.
 *
 * {
 *   "command": "unload",
 *   "name": "crazygame"
 * }
 *
 * Responses:
 *   - Error if the plugin does not exists
 */
void TransportManager::unload(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &)
{
}

/* --------------------------------------------------------
 * TransportClient slots
 * -------------------------------------------------------- */

void TransportManager::onMessage(const std::shared_ptr<TransportClientAbstract> &client, const std::string &message)
{
	try {
		JsonDocument document(message);

		if (!document.isObject()) {
			client->send("{ \"error\": \"Not an object\" }");
		} else {
			JsonObject object = document.toObject();

			if (!object.contains("command")) {
				client->send("{ \"error\": \"Invalid message\" }");
			} else if (m_commandMap.count(object["command"].toString()) == 0) {
				client->send("{ \"error\": \"Invalid command\" }");
			} else {
				(this->*m_commandMap.at(object["command"].toString()))(client, object);
			}
		}
	} catch (const std::exception &error) {
		client->send("{ \"error\": \"" + std::string(error.what()) + "\" }");
	}
}

void TransportManager::onWrite()
{
	static constexpr char command = Reload;
	static constexpr int size = sizeof (char);

	std::lock_guard<std::mutex> lock(m_mutex);

	// Signal select() to reload its set.
	m_signal.sendto(&command, size, m_signalAddress);
}

void TransportManager::onDie(const std::shared_ptr<TransportClientAbstract> &client)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	// TODO: debug message

	m_clients.erase(client->socket());
}

/* --------------------------------------------------------
 * Private helpers
 * -------------------------------------------------------- */

JsonValue TransportManager::want(const JsonObject &object, const std::string &key) const
{
	if (!object.contains(key)) {
		throw std::runtime_error("missing `" + key + "' property");
	}

	return object[key];
}

JsonValue TransportManager::optional(const JsonObject &object, const std::string &key, const JsonValue &def) const
{
	return (object.contains(key)) ? object[key] : def;
}

bool TransportManager::isTransport(const Socket &s) const noexcept
{
	return m_transports.count(s) > 0;
}

void TransportManager::accept(const Socket &s)
{
	using namespace std;
	using namespace std::placeholders;

	std::shared_ptr<TransportClientAbstract> client = m_transports.at(s)->accept();

	// TODO: debug log

	client->onComplete(bind(&TransportManager::onMessage, this, client, _1));
	client->onWrite(bind(&TransportManager::onWrite, this));
	client->onDie(bind(&TransportManager::onDie, this, client));

	// Add for listening
	std::lock_guard<std::mutex> lock(m_mutex);

	m_clients.emplace(client->socket(), std::move(client));
}

void TransportManager::process(const Socket &s, int direction)
{
	/*
	 * Do not lock, the client function may already call onWrite
	 * which will lock.
	 */
	m_clients.at(s)->process(direction);
}

void TransportManager::run() noexcept
{
	SocketListener listener;

	while (m_running) {
		try {
			listener.clear();
			listener.set(m_signal, SocketListener::Read);

			for (auto &ts : m_transports) {
				listener.set(ts.second->socket(), SocketListener::Read);
			}
			for (auto &tc : m_clients) {
				listener.set(tc.second->socket(), SocketListener::Read);

				if (tc.second->hasOutput()) {
					listener.set(tc.second->socket(), SocketListener::Write);
				}
			}

			SocketStatus status = listener.select(1000);

			if (status.socket == m_signal) {
				static char command;
				static int size = sizeof (char);
				static SocketAddress address;

				std::lock_guard<std::mutex> lock(m_mutex);

				m_signal.recvfrom(&command, size, address);

				switch (command) {
				case Stop:
					m_running = false;
				case Reload:
				default:
					break;
				}
			} else if (isTransport(status.socket)) {
				accept(status.socket);
			} else {
				process(status.socket, status.direction);
			}
		} catch (const SocketError &ex) {
			// TODO: log
		}
	}
}

TransportManager::TransportManager()
#if defined(_WIN32)
	: m_signal(AF_INET, 0)
#else
	: m_signal(AF_LOCAL, 0)
#endif
	, m_commandMap{
		{ "cnotice",	&TransportManager::cnotice	},
		{ "connect",	&TransportManager::connect	},
		{ "disconnect",	&TransportManager::disconnect	},
		{ "invite",	&TransportManager::invite	},
		{ "join",	&TransportManager::join		},
		{ "kick",	&TransportManager::kick		},
		{ "load",	&TransportManager::load		},
		{ "me",		&TransportManager::me		},
		{ "mode",	&TransportManager::mode		},
		{ "notice",	&TransportManager::notice	},
		{ "nick",	&TransportManager::nick		},
		{ "part",	&TransportManager::part		},
		{ "reconnect",	&TransportManager::reconnect	},
		{ "reload",	&TransportManager::reload	},
		{ "say",	&TransportManager::say		},
		{ "topic",	&TransportManager::topic	},
		{ "unload",	&TransportManager::reload	}
	}
{
	m_signal.set(SOL_SOCKET, SO_REUSEADDR, 1);

#if defined(_WIN32)
	m_signal.bind(address::Internet("127.0.0.1", 0, AF_INET));

	// Get the port
	auto port = ntohs(reinterpret_cast<const sockaddr_in &>(m_signal.address().address()).sin_port);
	m_signalAddress = address::Internet("127.0.0.1", port, AF_INET);
#else
	char path[] = "/tmp/.irccd.sock";

	m_signalAddress = address::Unix(path, true);
	m_signal.bind(m_signalAddress);
#endif
}

TransportManager::~TransportManager()
{
	stop();

	m_signal.close();

#if !defined(_WIN32)
	// Also remove the file at exit
	remove("/tmp/.irccd.sock");
#endif
}

void TransportManager::stop()
{
	if (m_running) {
		static constexpr char command = Stop;
		static constexpr int size = sizeof (char);

		/*
		 * Try to tell the thread to stop by sending the appropriate
		 * stop command. If it succeed, the select will stops
		 * immediately and there will be no lag.
		 *
		 * If it fails, stop manually the thread, it will requires
		 * to wait that the listener timeout.
		 */
		try {
			std::lock_guard<std::mutex> lock(m_mutex);

			m_signal.sendto(&command, size, m_signalAddress);
		} catch (const std::exception &) {
			m_running = false;

		}

		try {
			m_thread.join();
		} catch (...) { }

		m_transports.clear();
		m_clients.clear();
	}
}

} // !irccd

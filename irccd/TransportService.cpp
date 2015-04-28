/*
 * TransportManager.cpp -- maintain transport I/O
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

#include <Json.h>
#include <SocketListener.h>
#include <Util.h>

#include "TransportCommand.h"
#include "TransportService.h"

namespace irccd {

using namespace std;
using namespace std::string_literals;

/* --------------------------------------------------------
 * Transport events
 * -------------------------------------------------------- */

void TransportService::handleChannelNotice(const shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	string server = want(object, "server").toString();
	string channel = want(object, "channel").toString();
	string message = want(object, "message").toString();
	string ident = Util::join({"cnotice"s, server, channel, message});

	m_onEvent(TransportCommand(m_irccd, move(ident), move(client), &TransportCommand::cnotice, server, channel, message));
}

void TransportService::handleConnect(const shared_ptr<TransportClientAbstract> &, const JsonObject &)
{
#if 0
	m_onEvent(std::make_unique<Connect>(
		client,
		want(object, "name").toString(),
		want(object, "host").toString(),
		want(object, "port").toInteger(),
		want(object, "ssl").isTrue(),
		want(object, "ssl-verify").isTrue()
	));
#endif
}

void TransportService::handleDisconnect(const shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	string server = want(object, "server").toString();
	string ident = Util::join({"disconnect"s, server});

	m_onEvent(TransportCommand(m_irccd, move(ident), move(client), &TransportCommand::disconnect, server));
}

void TransportService::handleInvite(const shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	string server = want(object, "server").toString();
	string target = want(object, "target").toString();
	string channel = want(object, "channel").toString();
	string ident = Util::join({"invite"s, server, target, channel});

	m_onEvent(TransportCommand(m_irccd, move(ident), move(client), &TransportCommand::invite, server, target, channel));
}

void TransportService::handleJoin(const shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	string server = want(object, "server").toString();
	string channel = want(object, "channel").toString();
	string password = optional(object, "password", "").toString();
	string ident = Util::join({"join"s, server, channel, password});

	m_onEvent(TransportCommand(m_irccd, move(ident), move(client), &TransportCommand::join, server, channel, password));
}

void TransportService::handleKick(const shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	string server = want(object, "server").toString();
	string target = want(object, "target").toString();
	string channel = want(object, "channel").toString();
	string reason = optional(object, "reason", "").toString();
	string ident = Util::join({"kick"s, server, target, channel, reason});

	m_onEvent(TransportCommand(m_irccd, move(ident), move(client), &TransportCommand::kick, server, target, channel, reason));
}

void TransportService::handleLoad(const shared_ptr<TransportClientAbstract> &, const JsonObject &)
{
#if 0
	if (object.contains("name")) {
		m_onEvent(std::make_unique<Load>(client, want(object, "name").toString(), true));
	} else if (object.contains("path")) {
		m_onEvent(std::make_unique<Load>(client, want(object, "path").toString(), false));
	} else {
		client->error("load command requires `path' or `name' property");
	}
#endif
}

void TransportService::handleMe(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	string server = want(object, "server").toString();
	string channel = want(object, "channel").toString();
	string message = optional(object, "message", "").toString();
	string ident = Util::join({"me"s, server, channel, message});

	m_onEvent(TransportCommand(m_irccd, move(ident), move(client), &TransportCommand::me, server, channel, message));
}

void TransportService::handleMessage(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	string server = want(object, "server").toString();
	string target = want(object, "target").toString();
	string message = optional(object, "message", "").toString();
	string ident = Util::join({"message"s, server, target, message});

	m_onEvent(TransportCommand(m_irccd, move(ident), move(client), &TransportCommand::message, server, target, message));
}

void TransportService::handleMode(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	string server = want(object, "server").toString();
	string channel = want(object, "channel").toString();
	string mode = want(object, "mode").toString();
	string ident = Util::join({"mode"s, server, channel, mode});

	m_onEvent(TransportCommand(m_irccd, move(ident), move(client), &TransportCommand::mode, server, channel, mode));
}

void TransportService::handleNick(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	string server = want(object, "server").toString();
	string nickname = want(object, "nickname").toString();
	string ident = Util::join({"nick"s, server, nickname});

	m_onEvent(TransportCommand(m_irccd, move(ident), move(client), &TransportCommand::nick, server, nickname));
}

void TransportService::handleNotice(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	string server = want(object, "server").toString();
	string target = want(object, "target").toString();
	string message = want(object, "message").toString();
	string ident = Util::join({"notice"s, server, target, message});

	m_onEvent(TransportCommand(m_irccd, move(ident), move(client), &TransportCommand::notice, server, target, message));
}

void TransportService::handlePart(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	string server = want(object, "server").toString();
	string channel = want(object, "channel").toString();
	string reason = optional(object, "reason", "").toString();
	string ident = Util::join({"part"s, server, channel, reason});

	m_onEvent(TransportCommand(m_irccd, move(ident), move(client), &TransportCommand::part, server, channel, reason));
}

void TransportService::handleReconnect(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	string server = optional(object, "server", "").toString();
	string ident = Util::join({"reconnect"s, server});

	m_onEvent(TransportCommand(m_irccd, move(ident), move(client), &TransportCommand::reconnect, server));
}

void TransportService::handleReload(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	string plugin = want(object, "plugin").toString();
	string ident = Util::join({"reload"s, plugin});

	m_onEvent(TransportCommand(m_irccd, move(ident), move(client), &TransportCommand::reload, plugin));
}

void TransportService::handleTopic(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	string server = want(object, "server").toString();
	string channel = want(object, "channel").toString();
	string topic = want(object, "topic").toString();
	string ident = Util::join({"topic"s, server, channel, topic});

	m_onEvent(TransportCommand(m_irccd, move(ident), move(client), &TransportCommand::topic, server, channel, topic));
}

void TransportService::handleUserMode(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	string server = want(object, "server").toString();
	string mode = want(object, "mode").toString();
	string ident = Util::join({"umode"s, server, mode});

	m_onEvent(TransportCommand(m_irccd, move(ident), move(client), &TransportCommand::umode, server, mode));
}

void TransportService::handleUnload(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	string plugin = want(object, "plugin").toString();
	string ident = Util::join({"unload"s, plugin});

	m_onEvent(TransportCommand(m_irccd, move(ident), move(client), &TransportCommand::unload, plugin));
}

/* --------------------------------------------------------
 * TransportService slots from TransportClient signals
 * -------------------------------------------------------- */

void TransportService::onMessage(const std::shared_ptr<TransportClientAbstract> &client, const std::string &message)
{
	try {
		JsonDocument document(message);

		if (!document.isObject()) {
			client->error("Invalid JSon command");
		} else {
			JsonObject object = document.toObject();

			if (!object.contains("command")) {
				client->error("Invalid message");
			} else if (m_commandMap.count(object["command"].toString()) == 0) {
				client->error("Invalid command");
			} else {
				(this->*m_commandMap.at(object["command"].toString()))(client, object);
			}
		}
	} catch (const std::exception &error) {
		client->error(error.what());
	}
}

void TransportService::onWrite()
{
	Service::reload();
}

void TransportService::onDie(const std::shared_ptr<TransportClientAbstract> &client)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	Logger::debug() << "transport: client disconnected" << std::endl;

	m_clients.erase(client->socket());
}

/* --------------------------------------------------------
 * Private helpers
 * -------------------------------------------------------- */

JsonValue TransportService::want(const JsonObject &object, const std::string &key) const
{
	if (!object.contains(key)) {
		throw std::runtime_error("missing `" + key + "' property");
	}

	return object[key];
}

JsonValue TransportService::optional(const JsonObject &object, const std::string &key, const JsonValue &def) const
{
	return (object.contains(key)) ? object[key] : def;
}

bool TransportService::isTransport(const Socket &s) const noexcept
{
	return m_transports.count(s) > 0;
}

void TransportService::accept(const Socket &s)
{
	using namespace std;
	using namespace std::placeholders;

	std::shared_ptr<TransportClientAbstract> client = m_transports.at(s)->accept();

	Logger::debug() << "transport: new client" << std::endl;

	client->setOnComplete(bind(&TransportService::onMessage, this, client, _1));
	client->setOnWrite(bind(&TransportService::onWrite, this));
	client->setOnDie(bind(&TransportService::onDie, this, client));

	// Add for listening
	std::lock_guard<std::mutex> lock(m_mutex);

	m_clients.emplace(client->socket(), std::move(client));
}

void TransportService::process(const Socket &s, int direction)
{
	/*
	 * Do not lock, the client function may already call onWrite
	 * which will lock.
	 */
	m_clients.at(s)->process(direction);
}

void TransportService::run()
{
	SocketListener listener;

	while (isRunning()) {
		try {
			listener.clear();
			listener.set(socket(), SocketListener::Read);

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

			/*
			 * TODO: Add better handling of reload so that we don't
			 * clear the SocketListener at each iteration.
			 */
			if (isService(status.socket)) {
				(void)action();
				continue;
			}

			if (isTransport(status.socket)) {
				accept(status.socket);
			} else {
				process(status.socket, status.direction);
			}
		} catch (const SocketError &ex) {
			if (ex.code() != SocketError::Timeout) {
				Logger::debug() << "transport: error: " << ex.what() << std::endl;
			}
		}
	}
}

TransportService::TransportService(Irccd &irccd)
	: Service("transport", "/tmp/._irccd_ts.sock")
	, m_irccd(irccd)
	, m_commandMap{
		{ "cnotice",	&TransportService::handleChannelNotice	},
		{ "connect",	&TransportService::handleConnect	},
		{ "disconnect",	&TransportService::handleDisconnect	},
		{ "invite",	&TransportService::handleInvite		},
		{ "join",	&TransportService::handleJoin		},
		{ "kick",	&TransportService::handleKick		},
		{ "load",	&TransportService::handleLoad		},
		{ "me",		&TransportService::handleMe		},
		{ "message",	&TransportService::handleMessage	},
		{ "mode",	&TransportService::handleMode		},
		{ "nick",	&TransportService::handleNick		},
		{ "notice",	&TransportService::handleNotice		},
		{ "part",	&TransportService::handlePart		},
		{ "reconnect",	&TransportService::handleReconnect	},
		{ "reload",	&TransportService::handleReload		},
		{ "topic",	&TransportService::handleTopic		},
		{ "umode",	&TransportService::handleUserMode	},
		{ "unload",	&TransportService::handleUnload		}
	}
{
}

void TransportService::stop()
{
	Service::stop();

	m_transports.clear();
	m_clients.clear();
}

void TransportService::broadcast(const std::string &msg)
{
	assert(isRunning());

	/* Protect clients while broadcasting */
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		for (auto &tc : m_clients) {
			tc.second->send(msg, false);
		}
	}

	Service::reload();
}

} // !irccd

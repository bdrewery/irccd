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

#include "Json.h"
#include "SocketListener.h"
#include "TransportService.h"

#include "transportcommand/ChannelNotice.h"
#include "transportcommand/Connect.h"
#include "transportcommand/Disconnect.h"
#include "transportcommand/Invite.h"
#include "transportcommand/Join.h"
#include "transportcommand/Kick.h"
#include "transportcommand/Load.h"
#include "transportcommand/Me.h"
#include "transportcommand/Mode.h"
#include "transportcommand/Nick.h"
#include "transportcommand/Notice.h"
#include "transportcommand/Part.h"
#include "transportcommand/Reconnect.h"
#include "transportcommand/Reload.h"
#include "transportcommand/Say.h"
#include "transportcommand/Topic.h"
#include "transportcommand/Unload.h"
#include "transportcommand/UserMode.h"

namespace irccd {

using namespace transport;

/* --------------------------------------------------------
 * Transport events
 * -------------------------------------------------------- */

void TransportService::cnotice(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	m_onEvent(std::make_unique<ChannelNotice>(
		client,
		want(object, "server").toString(),
		want(object, "channel").toString(),
		want(object, "message").toString()
	));
}

void TransportService::connect(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
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

void TransportService::disconnect(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	m_onEvent(std::make_unique<Disconnect>(client, want(object, "server").toString()));
}


void TransportService::invite(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	m_onEvent(std::make_unique<Invite>(
		client,
		want(object, "server").toString(),
		want(object, "target").toString(),
		want(object, "channel").toString()
	));
}

void TransportService::join(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	m_onEvent(std::make_unique<Join>(
		client,
		want(object, "server").toString(),
		want(object, "channel").toString(),
		optional(object, "password", "").toString()
	));
}

void TransportService::kick(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	m_onEvent(std::make_unique<Kick>(
		client,
		want(object, "server").toString(),
		want(object, "target").toString(),
		want(object, "channel").toString(),
		optional(object, "reason", "").toString()
	));
}

void TransportService::load(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	if (object.contains("name")) {
		m_onEvent(std::make_unique<Load>(client, want(object, "name").toString(), true));
	} else if (object.contains("path")) {
		m_onEvent(std::make_unique<Load>(client, want(object, "path").toString(), false));
	} else {
		client->error("load command requires `path' or `name' property");
	}
}

void TransportService::me(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	m_onEvent(std::make_unique<Me>(
		client,
		want(object, "server").toString(),
		want(object, "channel").toString(),
		optional(object, "message", "").toString()
	));
}

void TransportService::mode(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	m_onEvent(std::make_unique<Mode>(
		client,
		want(object, "server").toString(),
		want(object, "channel").toString(),
		want(object, "mode").toString()
	));
}

void TransportService::nick(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	m_onEvent(std::make_unique<Nick>(
		client,
		want(object, "server").toString(),
		want(object, "nickname").toString()
	));
}

void TransportService::notice(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	m_onEvent(std::make_unique<Notice>(
		client,
		want(object, "server").toString(),
		want(object, "target").toString(),
		want(object, "message").toString()
	));
}

void TransportService::part(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	m_onEvent(std::make_unique<Part>(
		client,
		want(object, "server").toString(),
		want(object, "channel").toString(),
		optional(object, "reason", "").toString()
	));
}

void TransportService::reconnect(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	m_onEvent(std::make_unique<Reconnect>(client, optional(object, "server", "").toString()));
}

void TransportService::reload(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	m_onEvent(std::make_unique<transport::Reload>(client, want(object, "plugin").toString()));
}

void TransportService::say(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	m_onEvent(std::make_unique<Say>(
		client,
		want(object, "server").toString(),
		want(object, "target").toString(),
		optional(object, "message", "").toString()
	));
}

void TransportService::topic(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	m_onEvent(std::make_unique<Topic>(
		client,
		want(object, "server").toString(),
		want(object, "channel").toString(),
		want(object, "topic").toString()
	));
}

void TransportService::umode(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	m_onEvent(std::make_unique<UserMode>(
		client,
		want(object, "server").toString(),
		want(object, "mode").toString()
	));
}

void TransportService::unload(const std::shared_ptr<TransportClientAbstract> &client, const JsonObject &object)
{
	m_onEvent(std::make_unique<Unload>(client, want(object, "plugin").toString()));
}

/* --------------------------------------------------------
 * TransportClient slots
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

TransportService::TransportService()
	: Service("transport", "/tmp/._irccd_ts.sock")
	, m_commandMap{
		{ "cnotice",	&TransportService::cnotice	},
		{ "connect",	&TransportService::connect	},
		{ "disconnect",	&TransportService::disconnect	},
		{ "invite",	&TransportService::invite	},
		{ "join",	&TransportService::join		},
		{ "kick",	&TransportService::kick		},
		{ "load",	&TransportService::load		},
		{ "me",		&TransportService::me		},
		{ "mode",	&TransportService::mode		},
		{ "nick",	&TransportService::nick		},
		{ "notice",	&TransportService::notice	},
		{ "part",	&TransportService::part		},
		{ "reconnect",	&TransportService::reconnect	},
		{ "reload",	&TransportService::reload	},
		{ "say",	&TransportService::say		},
		{ "topic",	&TransportService::topic	},
		{ "umode",	&TransportService::umode	},
		{ "unload",	&TransportService::unload	}
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

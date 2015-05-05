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

#include <SocketListener.h>
#include <Util.h>

#include "Irccd.h"
#include "TransportCommand.h"
#include "TransportService.h"

namespace irccd {

using namespace std;
using namespace placeholders;
using namespace string_literals;

/* --------------------------------------------------------
 * Transport events
 * -------------------------------------------------------- */

void TransportService::handleChannelNotice(shared_ptr<TransportClientAbstract> client, string server, string channel, string message) const
{
	auto ident = Util::join({"cnotice"s, server, channel, message});
	auto function = [=] (Irccd &irccd) {
		irccd.serverFind(server)->cnotice(move(channel), move(message));
	};

	onCommand({move(client), move(ident), move(function)});
}

void TransportService::handleConnect(std::shared_ptr<TransportClientAbstract> client, ServerInfo info, ServerIdentity identity, ServerSettings settings) const
{
	auto ident = Util::join({"connect"s, info.name, info.host, to_string(info.port)});
	auto function = [=] (Irccd &irccd) {
		irccd.serverAdd(move(info), move(identity), move(settings));
	};

	onCommand({move(client), move(ident), move(function)});
}

void TransportService::handleDisconnect(shared_ptr<TransportClientAbstract> client, string server) const
{
	auto ident = Util::join({"disconnect"s, server});
	auto function = [=] (Irccd &irccd) {
		irccd.serverDisconnect(move(server));
	};

	onCommand({move(client), move(ident), move(function)});
}

void TransportService::handleInvite(shared_ptr<TransportClientAbstract> client, string server, string target, string channel) const
{
	auto ident = Util::join({"invite"s, server, target, channel});
	auto function = [=] (Irccd &irccd) {
		irccd.serverFind(server)->invite(move(target), move(channel));
	};

	onCommand({move(client), move(ident), move(function)});
}

void TransportService::handleJoin(shared_ptr<TransportClientAbstract> client, string server, string channel, string password) const
{
	auto ident = Util::join({"join"s, server, channel, password});
	auto function = [=] (Irccd &irccd) {
		irccd.serverFind(server)->join(move(channel), move(password));
	};

	onCommand({move(client), move(ident), move(function)});
}

void TransportService::handleKick(shared_ptr<TransportClientAbstract> client, string server, string target, string channel, string reason) const
{
	auto ident = Util::join({"kick"s, server, target, channel, reason});
	auto function = [=] (Irccd &irccd) {
		irccd.serverFind(server)->kick(move(target), move(channel), move(reason));
	};

	onCommand({move(client), move(ident), move(function)});
}

void TransportService::handleLoad(shared_ptr<TransportClientAbstract> client, string plugin) const
{
	auto ident = Util::join({"load"s,  plugin});
	auto function = [=] (Irccd &irccd) {
		irccd.pluginLoad(move(plugin));
	};

	onCommand({move(client), move(ident), move(function)});
}

void TransportService::handleMe(shared_ptr<TransportClientAbstract> client, string server, string target, string message) const
{
	auto ident = Util::join({"me"s, server, target, message});
	auto function = [=] (Irccd &irccd) {
		irccd.serverFind(server)->me(move(target), move(message));
	};

	onCommand({move(client), move(ident), move(function)});
}

void TransportService::handleMessage(shared_ptr<TransportClientAbstract> client, string server, string target, string message) const
{
	auto ident = Util::join({"message"s, server, target, message});
	auto function = [=] (Irccd &irccd) {
		irccd.serverFind(server)->message(move(target), move(message));
	};

	onCommand({move(client), move(ident), move(function)});
}

void TransportService::handleMode(shared_ptr<TransportClientAbstract> client, string server, string channel, string mode) const
{
	auto ident = Util::join({"mode"s, server, channel, mode});
	auto function = [=] (Irccd &irccd) {
		irccd.serverFind(server)->mode(move(channel), move(mode));
	};

	onCommand({move(client), move(ident), move(function)});
}

void TransportService::handleNick(shared_ptr<TransportClientAbstract> client, string server, string nickname) const
{
	auto ident = Util::join({"nick"s, server, nickname});
	auto function = [=] (Irccd &irccd) {
		irccd.serverFind(server)->nick(move(nickname));
	};

	onCommand({move(client), move(ident), move(function)});
}

void TransportService::handleNotice(shared_ptr<TransportClientAbstract> client, string server, string target, string message) const
{
	auto ident = Util::join({"notice"s, server, target, message});
	auto function = [=] (Irccd &irccd) {
		irccd.serverFind(server)->notice(move(target), move(message));
	};

	onCommand({move(client), move(ident), move(function)});
}

void TransportService::handlePart(shared_ptr<TransportClientAbstract> client, string server, string channel, string reason) const
{
	auto ident = Util::join({"part"s, server, channel, reason});
	auto function = [=] (Irccd &irccd) {
		irccd.serverFind(server)->part(move(channel), move(reason));
	};

	onCommand({move(client), move(ident), move(function)});
}

void TransportService::handleReconnect(shared_ptr<TransportClientAbstract> client, string server) const
{
	auto ident = Util::join({"reconnect"s, server});
	auto function = [=] (Irccd &irccd) {
		irccd.serverReconnect()
	};

	onCommand({move(client), move(ident), move(function)});
}

void TransportService::handleReload(shared_ptr<TransportClientAbstract> client, string plugin) const
{
	auto ident = Util::join({"reload"s, plugin});
	auto function = [=] (Irccd &irccd) {
		irccd.pluginReload(move(plugin));
	};

	onCommand({move(client), move(ident), move(function)});
}

void TransportService::handleTopic(shared_ptr<TransportClientAbstract> client, string server, string channel, string topic) const
{
	auto ident = Util::join({"topic"s, server, channel, topic});
	auto function = [=] (Irccd &irccd) {
		irccd.serverFind(server)->topic(move(channel), move(topic));
	};

	onCommand({move(client), move(ident), move(function)});
}

void TransportService::handleUnload(shared_ptr<TransportClientAbstract> client, string plugin) const
{
	auto ident = Util::join({"unload"s, plugin});
	auto function = [=] (Irccd &irccd) {
		irccd.pluginUnload(move(plugin));
	};

	onCommand({move(client), move(ident), move(function)});
}

void TransportService::handleUserMode(shared_ptr<TransportClientAbstract> client, string server, string mode) const
{
	auto ident = Util::join({"umode"s, server, mode});
	auto function = [=] (Irccd &irccd) {
		irccd.serverFind(server)->umode(move(mode));
	};

	onCommand({move(client), move(ident), move(function)});
}

void TransportService::handleOnWrite()
{
	reload();
}

void TransportService::handleOnDie(const shared_ptr<TransportClientAbstract> &client)
{
	lock_guard<mutex> lock(m_mutex);

	Logger::debug() << "transport: client disconnected" << endl;

	m_clients.erase(client->socket());
}

/* --------------------------------------------------------
 * Private helpers
 * -------------------------------------------------------- */

bool TransportService::isTransport(const Socket &s) const noexcept
{
	return m_transports.count(s) > 0;
}

void TransportService::accept(const Socket &s)
{
	using namespace std;
	using namespace placeholders;

	shared_ptr<TransportClientAbstract> client = m_transports.at(s)->accept();

	Logger::debug() << "transport: new client" << endl;

	// TODO: add all slots here
	client->onChannelNotice.connect(bind(&handleChannelNotice, this, client, _1, _2, _3));
	client->onConnect.connect(bind(&handleConnect, this, client, _1, _2, _3));
	client->onDisconnect.connect(bind(&handleDisconnect, this, client, _1));
	client->onInvite.connect(bind(&handleInvite, this, client, _1, _2, _3));
	client->onJoin.connect(bind(&handleJoin, this, client, _1, _2, _3));
	client->onKick.connect(bind(&handleKick, this, client, _1, _2, _3, _4));
	client->onLoad.connect(bind(&handleLoad, this, client, _1));
	client->onMe.connect(bind(&handleMe, this, client, _1, _2, _3));
	client->onMessage.connect(bind(&handleMessage, this, client, _1, _2, _3));
	client->onMode.connect(bind(&handleMode, this, client, _1, _2, _3));
	client->onNick.connect(bind(&handleNick, this, client, _1, _2));
	client->onNotice.connect(bind(&handleNotice, this, client, _1, _2, _3));
	client->onPart.connect(bind(&handlePart, this, client, _1, _2, _3));
	client->onReconnect.connect(bind(&handleReconnect, this, client, _1));
	client->onReload.connect(bind(&handleReload, this, client, _1));
	client->onTopic.connect(bind(&handleTopic, this, client, _1, _2, _3));
	client->onUnload.connect(bind(&handleUnload, this, client, _1));
	client->onUserMode.connect(bind(&handleUserMode, this, client, _1, _2));
	client->onWrite.connect(bind(&handleOnWrite, this));
	client->onDie.connect(bind(&handleOnDie, this, client));

	// Add for listening
	lock_guard<mutex> lock(m_mutex);

	m_clients.emplace(client->socket(), move(client));
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
		// TODO: do not rebuild the listener at each iteration.
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
				Logger::debug() << "transport: error: " << ex.what() << endl;
			}
		}
	}
}

TransportService::TransportService()
	: Service("transport", "/tmp/._irccd_ts.sock")
{
}

void TransportService::stop()
{
	Service::stop();

	m_transports.clear();
	m_clients.clear();
}

void TransportService::broadcast(const string &msg)
{
	assert(isRunning());

	/* Protect clients while broadcasting */
	{
		lock_guard<mutex> lock(m_mutex);

		for (auto &tc : m_clients) {
			tc.second->send(msg, false);
		}
	}

	Service::reload();
}

} // !irccd

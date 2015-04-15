/*
 * ServerService.cpp -- processes servers automatically
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

#include <Logger.h>
#include <Socket.h>

#include "ServerService.h"

#include "serverevent/ChannelNotice.h"
#include "serverevent/Connect.h"
#include "serverevent/Invite.h"
#include "serverevent/Join.h"
#include "serverevent/Kick.h"
#include "serverevent/Me.h"
#include "serverevent/Message.h"
#include "serverevent/Mode.h"
#include "serverevent/Names.h"
#include "serverevent/Nick.h"
#include "serverevent/Notice.h"
#include "serverevent/Part.h"
#include "serverevent/Query.h"
#include "serverevent/Topic.h"
#include "serverevent/UserMode.h"
#include "serverevent/Whois.h"

using namespace std::string_literals;

namespace irccd {

using namespace event;

void ServerService::run()
{
	while (isRunning()) {
		fd_set setinput;
		fd_set setoutput;
		int max = 0;

		FD_ZERO(&setinput);
		FD_ZERO(&setoutput);

		// Add service socket
		FD_SET(socket().handle(), &setinput);

		if (socket().handle() > static_cast<Socket::Handle>(max)) {
			max = socket().handle();
		}

		// Protect the list of servers while preparing the set.
		{
			std::lock_guard<std::mutex> lock(m_mutex);

			// 1. Update server states and flush their pending commands
			for (auto &pair : m_servers) {
				pair.second->update();
				pair.second->flush();
			}

			// 2. Remove dead servers
			for (auto it = m_servers.begin(); it != m_servers.end(); ) {
				if (it->second->state() == ServerState::Dead) {
					Logger::debug() << "server " << it->second->info().name << ": destroyed" << std::endl;
					it = m_servers.erase(it);
				} else {
					++ it;
				}
			}

			// 3. Update the sets
			for (auto &pair : m_servers) {
				pair.second->prepare(setinput, setoutput, max);
			}
		}

		// 4. Do the selection
		struct timeval tv;

		tv.tv_sec = 0;
		tv.tv_usec = 250000;

		int error = select(max + 1, &setinput, &setoutput, nullptr, &tv);

		/* Skip on error */
		if (error < 0) {
			Logger::warning() << "irccd: " << Socket::syserror() << std::endl;
			continue;
		}

		/* Skip if it's the service socket */
		if (FD_ISSET(socket().handle(), &setinput)) {
			(void)action();
			continue;
		}

		// Protect the set while processing the sessions.
		std::lock_guard<std::mutex> lock(m_mutex);

		for (auto &pair : m_servers) {
			pair.second->process(setinput, setoutput);
		}
	}
}

void ServerService::onChannelNotice(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string notice)
{
	Logger::debug() << "server " << server->info().name << ": onChannelNotice: "
			<< "origin=" << origin << ", channel=" << channel <<", notice=" << notice << std::endl;

	m_onEvent(std::make_unique<ChannelNotice>(
		std::move(server),
		std::move(origin),
		std::move(channel),
		std::move(notice)
	));
}

void ServerService::onConnect(std::shared_ptr<Server> server)
{
	Logger::debug() << "server " << server->info().name << ": onConnect" << std::endl;

	m_onEvent(std::make_unique<Connect>(std::move(server)));
}

void ServerService::onInvite(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string target)
{
	Logger::debug() << "server " << server->info().name << ": onInvite: "
			<< "origin=" << origin << ", channel=" << channel << ", target=" << target << std::endl;

	m_onEvent(std::make_unique<Invite>(
		std::move(server),
		std::move(origin),
		std::move(channel)
	));
}

void ServerService::onJoin(std::shared_ptr<Server> server, std::string origin, std::string channel)
{
	Logger::debug() << "server " << server->info().name << ": onJoin: "
			<< "origin=" << origin << ", channel=" << channel << std::endl;

	m_onEvent(std::make_unique<Join>(
		std::move(server),
		std::move(origin),
		std::move(channel)
	));
}

void ServerService::onKick(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string target, std::string reason)
{
	Logger::debug() << "server " << server->info().name << ": onKick: "
			<< "origin=" << origin << ", channel=" << channel << ", target=" << target << ", reason=" << reason << std::endl;

	m_onEvent(std::make_unique<Kick>(
		std::move(server),
		std::move(origin),
		std::move(channel),
		std::move(target),
		std::move(reason)
	));
}

void ServerService::onMessage(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string message)
{
	Logger::debug() << "server " << server->info().name << ": onMessage: "
			<< "origin=" << origin << ", channel=" << channel << ", message=" << message << std::endl;

	m_onEvent(std::make_unique<Message>(
		std::move(server),
		std::move(origin),
		std::move(channel),
		std::move(message)
	));
}

void ServerService::onMe(std::shared_ptr<Server> server, std::string origin, std::string target, std::string message)
{
	Logger::debug() << "server " << server->info().name << ": onMe: "
			<< "origin=" << origin << ", target=" << target << ", message=" << message << std::endl;

	m_onEvent(std::make_unique<Me>(
		std::move(server),
		std::move(origin),
		std::move(target),
		std::move(message)
	));
}

void ServerService::onMode(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string mode, std::string arg)
{
	Logger::debug() << "server " << server->info().name << ": onMode: "
			<< "origin=" << origin << ", channel=" << channel << ", mode=" << mode << ", argument=" << arg << std::endl;

	m_onEvent(std::make_unique<Mode>(
		std::move(server),
		std::move(origin),
		std::move(channel),
		std::move(mode),
		std::move(arg)
	));
}

void ServerService::onNick(std::shared_ptr<Server> server, std::string origin, std::string nickname)
{
	Logger::debug() << "server " << server->info().name << ": onNick: "
			<< "origin=" << origin << ", nickname=" << nickname << std::endl;

	m_onEvent(std::make_unique<Nick>(
		std::move(server),
		std::move(origin),
		std::move(nickname)
	));
}

void ServerService::onNotice(std::shared_ptr<Server> server, std::string origin, std::string message)
{
	Logger::debug() << "server " << server->info().name << ": onNotice: "
			<< "origin=" << origin << ", message=" << message << std::endl;

	m_onEvent(std::make_unique<Notice>(
		std::move(server),
		std::move(origin),
		std::move(message)
	));
}

void ServerService::onPart(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string reason)
{
	Logger::debug() << "server " << server->info().name << ": onPart: "
			<< "origin=" << origin << ", channel=" << channel << ", reason=" << reason << std::endl;

	m_onEvent(std::make_unique<Part>(
		std::move(server),
		std::move(origin),
		std::move(channel),
		std::move(reason)
	));
}

void ServerService::onQuery(std::shared_ptr<Server> server, std::string origin, std::string message)
{
	Logger::debug() << "server " << server->info().name << ": onQuery: "
			<< "origin=" << origin << ", message=" << message << std::endl;

	m_onEvent(std::make_unique<Query>(
		std::move(server),
		std::move(origin),
		std::move(message)
	));
}

void ServerService::onTopic(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string topic)
{
	Logger::debug() << "server " << server->info().name << ": onTopic: "
			<< "origin=" << origin << ", channel=" << channel << ", topic=" << topic << std::endl;

	m_onEvent(std::make_unique<Topic>(
		std::move(server),
		std::move(origin),
		std::move(channel),
		std::move(topic)
	));
}

void ServerService::onUserMode(std::shared_ptr<Server> server, std::string origin, std::string mode)
{
	Logger::debug() << "server " << server->info().name << ": onUserMode: "
			<< "origin=" << origin << ", mode=" << mode << std::endl;

	m_onEvent(std::make_unique<UserMode>(
		std::move(server),
		std::move(origin),
		std::move(mode)
	));
}

ServerService::ServerService()
	: Service("server", "/tmp/._irccd_sv.sock")
{
}

std::shared_ptr<Server> ServerService::find(const std::string &name) const
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_servers.count(name) == 0) {
		throw std::invalid_argument("server "s + name + " not found");
	}

	return m_servers.at(name);
}

} // !irccd

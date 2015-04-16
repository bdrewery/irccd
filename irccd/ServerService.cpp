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

#include <sstream>

#include <Json.h>
#include <Logger.h>
#include <Socket.h>

#if defined(WITH_JS)
#  include "Plugin.h"
#endif

#include "ServerEvent.h"
#include "ServerService.h"

using namespace std::string_literals;

namespace irccd {

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

	std::ostringstream json;

	json << "{"
	     << "\"event\":\"ChannelNotice\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"channel\":\"" << JsonValue::escape(channel) << "\","
	     << "\"notice\":\"" << JsonValue::escape(notice) << "\""
	     << "}";

	m_onEvent(ServerEvent("onChannelNotice", json.str(), server, origin, channel, [=] (Plugin &plugin) {
#if defined(WITH_JS)
		plugin.onChannelNotice(server, origin, channel, notice);
#else
		(void)plugin;
#endif
	}));
}

void ServerService::onConnect(std::shared_ptr<Server> server)
{
	Logger::debug() << "server " << server->info().name << ": onConnect" << std::endl;

	std::ostringstream json;

	json << "{"
	     << "\"event\":\"onConnect\","
	     << "\"server\":\"" << server->info().name << "\""
	     << "}";

	m_onEvent(ServerEvent("onConnect", json.str(), server, "", "", [=] (Plugin &plugin) {
#if defined(WITH_JS)
		plugin.onConnect(server);
#else
		(void)plugin;
#endif
	}));
}

void ServerService::onInvite(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string target)
{
	Logger::debug() << "server " << server->info().name << ": onInvite: "
			<< "origin=" << origin << ", channel=" << channel << ", target=" << target << std::endl;

	std::ostringstream json;

	json << "{"
	     << "\"event\":\"onInvite\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"channel\":\"" << JsonValue::escape(channel) << "\""
	     << "}";

	m_onEvent(ServerEvent("onInvite", json.str(), server, origin, channel, [=] (Plugin &plugin) {
#if defined(WITH_JS)
		plugin.onInvite(server, origin, channel);
#else
		(void)plugin;
#endif
	}));
}

void ServerService::onJoin(std::shared_ptr<Server> server, std::string origin, std::string channel)
{
	Logger::debug() << "server " << server->info().name << ": onJoin: "
			<< "origin=" << origin << ", channel=" << channel << std::endl;

	std::ostringstream json;

	json << "{"
	     << "\"event\":\"onJoin\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"channel\":\"" << JsonValue::escape(channel) << "\""
	     << "}";

	m_onEvent(ServerEvent("onJoin", json.str(), server, origin, channel, [=] (Plugin &plugin) {
#if defined(WITH_JS)
		plugin.onJoin(server, origin, channel);
#else
		(void)plugin;
#endif
	}));
}

void ServerService::onKick(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string target, std::string reason)
{
	Logger::debug() << "server " << server->info().name << ": onKick: "
			<< "origin=" << origin << ", channel=" << channel << ", target=" << target << ", reason=" << reason << std::endl;

	std::ostringstream json;

	json << "{"
	     << "\"event\":\"onKick\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"channel\":\"" << JsonValue::escape(channel) << "\","
	     << "\"target\":\"" << JsonValue::escape(target) << "\","
	     << "\"reason\":\"" << JsonValue::escape(reason) << "\""
	     << "}";

	m_onEvent(ServerEvent("onKick", json.str(), server, origin, channel, [=] (Plugin &plugin) {
#if defined(WITH_JS)
		plugin.onKick(server, origin, channel, target, reason);
#else
		(void)plugin;
#endif
	}));
}

void ServerService::onMessage(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string message)
{
	Logger::debug() << "server " << server->info().name << ": onMessage: "
			<< "origin=" << origin << ", channel=" << channel << ", message=" << message << std::endl;

	std::ostringstream json;

	json << "{"
	     << "\"event\":\"Message\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"channel\":\"" << JsonValue::escape(channel) << "\","
	     << "\"message\":\"" << JsonValue::escape(message) << "\""
	     << "}";

	// TODO: handle onMessage/onCommand
}

void ServerService::onMe(std::shared_ptr<Server> server, std::string origin, std::string target, std::string message)
{
	Logger::debug() << "server " << server->info().name << ": onMe: "
			<< "origin=" << origin << ", target=" << target << ", message=" << message << std::endl;

	std::ostringstream json;

	json << "{"
	     << "\"event\":\"Me\","
	     << "\"server\":\"" << server->info().name << "\""
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"target\":\"" << JsonValue::escape(target) << "\","
	     << "\"message\":\"" << JsonValue::escape(message) << "\""
	     << "}";

	m_onEvent(ServerEvent("onMe", json.str(), server, origin, target, [=] (Plugin &plugin) {
#if defined(WITH_JS)
		plugin.onMe(server, origin, target, message);
#else
		(void)plugin;
#endif
	}));
}

void ServerService::onMode(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string mode, std::string arg)
{
	Logger::debug() << "server " << server->info().name << ": onMode: "
			<< "origin=" << origin << ", channel=" << channel << ", mode=" << mode << ", argument=" << arg << std::endl;

	std::ostringstream json;

	json << "{"
	     << "\"event\":\"Mode\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"mode\":\"" << JsonValue::escape(mode) << "\","
	     << "\"argument\":\"" << JsonValue::escape(arg) << "\""
	     << "}";

	m_onEvent(ServerEvent("onMode", json.str(), server, origin, channel, [=] (Plugin &plugin) {
#if defined(WITH_JS)
		plugin.onMode(server, origin, channel, mode, arg);
#else
		(void)plugin;
#endif
	}));
}

void ServerService::onNick(std::shared_ptr<Server> server, std::string origin, std::string nickname)
{
	Logger::debug() << "server " << server->info().name << ": onNick: "
			<< "origin=" << origin << ", nickname=" << nickname << std::endl;

	std::ostringstream json;

	json << "{"
	     << "\"event\":\"onNick\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"old\":\"" << origin << "\","
	     << "\"new\":\"" << nickname << "\""
	     << "}";

	m_onEvent(ServerEvent("onNick", json.str(), server, origin, "", [=] (Plugin &plugin) {
#if defined(WITH_JS)
		plugin.onNick(server, origin, nickname);
#else
		(void)plugin;
#endif
	}));
}

void ServerService::onNotice(std::shared_ptr<Server> server, std::string origin, std::string message)
{
	Logger::debug() << "server " << server->info().name << ": onNotice: "
			<< "origin=" << origin << ", message=" << message << std::endl;

	std::ostringstream json;

	json << "{"
	     << "\"event\":\"onNotice\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"notice\":\"" << JsonValue::escape(message) << "\""
	     << "}";

	m_onEvent(ServerEvent("onNotice", json.str(), server, origin, /* channel */ "", [=] (Plugin &plugin) {
#if defined(WITH_JS)
		plugin.onNotice(server, origin, message);
#else
		(void)plugin;
#endif
	}));
}

void ServerService::onPart(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string reason)
{
	Logger::debug() << "server " << server->info().name << ": onPart: "
			<< "origin=" << origin << ", channel=" << channel << ", reason=" << reason << std::endl;

	std::ostringstream json;

	json << "{"
	     << "\"event\":\"Part\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"channel\":\"" << JsonValue::escape(channel) << "\","
	     << "\"reason\":\"" << JsonValue::escape(reason) << "\""
	     << "}";

	m_onEvent(ServerEvent("onPart", json.str(), server, origin, channel, [=] (Plugin &plugin) {
#if defined(WITH_JS)
		plugin.onPart(server, origin, channel, reason);
#else
		(void)plugin;
#endif
	}));
}

void ServerService::onQuery(std::shared_ptr<Server> server, std::string origin, std::string message)
{
	Logger::debug() << "server " << server->info().name << ": onQuery: "
			<< "origin=" << origin << ", message=" << message << std::endl;

	// TODO: like onMessage
}

void ServerService::onTopic(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string topic)
{
	Logger::debug() << "server " << server->info().name << ": onTopic: "
			<< "origin=" << origin << ", channel=" << channel << ", topic=" << topic << std::endl;

	std::ostringstream json;

	json << "{"
	     << "\"event\":\"Topic\""
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"channel\":\"" << JsonValue::escape(channel) << "\","
	     << "\"topic\":\"" << JsonValue::escape(topic) << "\""
	     << "}";

	m_onEvent(ServerEvent("onTopic", json.str(), server, origin, channel, [=] (Plugin &plugin) {
#if defined(WITH_JS)
		plugin.onTopic(server, origin, channel, topic);
#else
		(void)plugin;
#endif
	}));
}

void ServerService::onUserMode(std::shared_ptr<Server> server, std::string origin, std::string mode)
{
	Logger::debug() << "server " << server->info().name << ": onUserMode: "
			<< "origin=" << origin << ", mode=" << mode << std::endl;

	std::ostringstream json;

	json << "{"
	     << "\"event\":\"UserMode\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"mode\":\"" << JsonValue::escape(mode) << "\""
	     << "}";

	m_onEvent(ServerEvent("onUserMode", json.str(), server, origin, "", [=] (Plugin &plugin) {
#if defined(WITH_JS)
		plugin.onUserMode(server, origin, mode);
#else
		(void)plugin;
#endif
	}));
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

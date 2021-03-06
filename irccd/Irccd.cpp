/*
 * Irccd.cpp -- main irccd class
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

#include <algorithm>
#include <cassert>
#include <stdexcept>

#include <Filesystem.h>
#include <Logger.h>
#include <Util.h>

#include "Irccd.h"

using namespace std;
using namespace std::placeholders;
using namespace std::string_literals;

using namespace irccd::address;

namespace irccd {

#if defined(WITH_JS)

ServerMessagePair Irccd::parseMessage(string message, Server &server, Plugin &plugin)
{
	string cc = server.settings().command;
	string name = plugin.info().name;
	string result = message;
	bool iscommand = false;

	// handle special commands "!<plugin> command"
	if (cc.length() > 0) {
		auto pos = result.find_first_of(" \t");
		auto fullcommand = cc + name;

		/*
		 * If the message that comes is "!foo" without spaces we
		 * compare the command char + the plugin name. If there
		 * is a space, we check until we find a space, if not
		 * typing "!foo123123" will trigger foo plugin.
		 */
		if (pos == string::npos) {
			iscommand = result == fullcommand;
		} else {
			iscommand = result.length() >= fullcommand.length() &&
			    result.compare(0, pos, fullcommand) == 0;
		}

		if (iscommand) {
			/*
			 * If no space is found we just set the message to "" otherwise
			 * the plugin name will be passed through onCommand
			 */
			if (pos == string::npos) {
				result = "";
			} else {
				result = message.substr(pos + 1);
			}
		}
	}

	return ServerMessagePair{result, ((iscommand) ? ServerMessageType::Command : ServerMessageType::Message)};
}

#endif

void Irccd::dispatch()
{
	/*
	 * Make a copy because the events can add other events while we are iterating it. Also lock because the timers
	 * may alter these events too.
	 */
	vector<Event> copy;

	{
		lock_guard<mutex> lock{m_mutex};

		copy = move(m_events);

		/* Clear for safety */
		m_events.clear();
	}

	if (copy.size() > 0) {
		Logger::debug() << "irccd: dispatching " << copy.size() << " event(s)" << endl;
	}

	for (auto &ev : copy) {
		ev();
	}
}

void Irccd::process(fd_set &setinput, fd_set &setoutput)
{
	/* 1. May be IPC */
	if (FD_ISSET(m_socketServer.handle(), &setinput)) {
		try {
			(void)m_socketServer.recv(8);
		} catch (const std::exception &) {
			// TODO: think what we can do here
		}
	}

	/* 2. Check for transport clients */
	for (auto &pair : m_lookupTransportClients) {
		pair.second->sync(setinput, setoutput);
	}

	/* 3. Check for transport servers */
	for (auto &pair : m_lookupTransportServers) {
		if (FD_ISSET(pair.second->socket().handle(), &setinput)) {
			Logger::debug() << "transport: new client connected" << endl;

			auto client = pair.second->accept();

			m_lookupTransportClients.emplace(client->socket().handle(), move(client));
		}
	}

	/* 3. Check for servers */
	for (auto &pair : m_servers) {
		pair.second->sync(setinput, setoutput);
	}
}

void Irccd::exec()
{
	fd_set setinput;
	fd_set setoutput;
	auto max = m_socketServer.handle();
	auto set = [&] (fd_set &set, SocketAbstract::Handle handle) {
		FD_SET(handle, &set);

		if (handle > max) {
			max = handle;
		}
	};

	FD_ZERO(&setinput);
	FD_ZERO(&setoutput);

	/* 1. Add master socket */
	FD_SET(m_socketServer.handle(), &setinput);

	/* 2. Add servers */
	for (auto &pair : m_servers) {
		pair.second->update();
		pair.second->prepare(setinput, setoutput, max);
	}

	/* 3. Add transports clients */
	for (auto &pair : m_lookupTransportClients) {
		set(setinput, pair.first);

		if (pair.second->hasOutput()) {
			set(setoutput, pair.first);
		}
	}

	/* 4. Add transport servers */
	for (auto &pair : m_lookupTransportServers) {
		set(setinput, pair.first);
	}

	// 4. Do the selection
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 250000;

	int error = select(max + 1, &setinput, &setoutput, nullptr, &tv);

	/* Skip anyway */
	if (!m_running) {
		return;
	}

	/* Skip on error */
	if (error < 0 && errno != EINTR) {
		Logger::warning() << "irccd: " << SocketAbstract::syserror() << std::endl;
		return;
	}

	process(setinput, setoutput);
	dispatch();
}

void Irccd::addTransportEvent(shared_ptr<TransportClientAbstract> tc, Event ev)  noexcept
{
	addEvent([=] () {
		try {
			ev();
		} catch (const std::exception &ex) {
			tc->error(ex.what());
		}
	});
}

void Irccd::addServerEvent(ServerEvent event) noexcept
{
	addEvent([=] () {
		for (auto &pair : m_plugins) {
			auto name = event.name(*pair.second);

			// TODO: match rules here
			(void)name;

			event.exec(*pair.second);
		}
	});

	/* Asynchronous send */
	for (auto &pair : m_lookupTransportClients) {
		pair.second->send(event.json);
	}
}

Irccd::Irccd()
	: m_socketServer{AF_INET, 0}
	, m_socketClient{AF_INET, 0}
{
	/* Create temporary socket */
	SocketTcp<Ipv4> master{AF_INET, 0};

	master.set(SOL_SOCKET, SO_REUSEADDR, 1);
	master.bind(Ipv4{"*", 0});
	master.listen(1);

	Ipv4 local = master.getsockname();

	m_socketClient.connect(Ipv4{"127.0.0.1", local.port()});
	m_socketServer = master.accept();
	m_socketClient.setBlockMode(false);
}

void Irccd::addEvent(Event ev) noexcept
{
	lock_guard<std::mutex> lock{m_mutex};

	m_events.push_back(std::move(ev));

	/* Silently discard */
	try {
		m_socketClient.send(" ");
	} catch (...) {
	}
}

void Irccd::addServer(shared_ptr<Server> server) noexcept
{
	server->onChannelNotice.connect(bind(&Irccd::handleServerOnChannelNotice, this, server, _1, _2, _3));
	server->onConnect.connect(bind(&Irccd::handleServerOnConnect, this, server));
	server->onInvite.connect(bind(&Irccd::handleServerOnInvite, this, server, _1, _2, _3));
	server->onJoin.connect(bind(&Irccd::handleServerOnJoin, this, server, _1, _2));
	server->onKick.connect(bind(&Irccd::handleServerOnKick, this, server, _1, _2, _3, _4));
	server->onMessage.connect(bind(&Irccd::handleServerOnMessage, this, server, _1, _2, _3));
	server->onMe.connect(bind(&Irccd::handleServerOnMe, this, server, _1, _2, _3));
	server->onMode.connect(bind(&Irccd::handleServerOnMode, this, server, _1, _2, _3, _4));
	server->onNick.connect(bind(&Irccd::handleServerOnNick, this, server, _1, _2));
	server->onNotice.connect(bind(&Irccd::handleServerOnNotice, this, server, _1, _2));
	server->onPart.connect(bind(&Irccd::handleServerOnPart, this, server, _1, _2, _3));
	server->onQuery.connect(bind(&Irccd::handleServerOnQuery, this, server, _1, _2));
	server->onTopic.connect(bind(&Irccd::handleServerOnTopic, this, server, _1, _2, _3));
	server->onUserMode.connect(bind(&Irccd::handleServerOnUserMode, this, server, _1, _2));

	m_servers.emplace(server->info().name, move(server));
}

void Irccd::addTransport(std::shared_ptr<TransportServerAbstract> ts)
{
	Logger::info() << "transport: listening on " << ts->info() << endl;

	m_lookupTransportServers.emplace(ts->socket().handle(), move(ts));
}

shared_ptr<Plugin> Irccd::findPlugin(const string &name) const
{
	auto it = m_plugins.find(name);

	if (it == m_plugins.end()) {
		throw std::out_of_range("plugin "s + name + " not found"s);
	}

	return it->second;
}

#if defined(WITH_JS)

void Irccd::loadPlugin(string path)
{
	shared_ptr<Plugin> plugin;
	string name;

	if (Filesystem::isRelative(path)) {
		name = path;

		/*
		 * Iterate over all plugin directories and try to load, we emit
		 * a warning for all failures but do not break unless we have
		 * found a plugin working.
		 */
		for (const string &dir : Util::pathsPlugins()) {
			string fullpath = dir + Filesystem::Separator + path + ".js";

			try {
				Logger::info() << "plugin " << name << ": trying " << fullpath << endl;

				plugin = make_shared<Plugin>(path, fullpath, m_pluginConf[name]);
				break;
			} catch (const exception &ex) {
				Logger::info() << "plugin " << name << ": " << fullpath << ": " << ex.what() << endl;
			}
		}
	} else {
		/* Extract name from the path */
		string::size_type first = path.rfind(Filesystem::Separator);
		string::size_type last = path.rfind(".js");

		if (first == string::npos || last == string::npos) {
			throw invalid_argument("unable to extract plugin name");
		}

		name = path.substr(first, last);

		Logger::info() << "plugin " << name << ": trying " << path << endl;

		try {
			plugin = make_shared<Plugin>(move(name), move(path), m_pluginConf[name]);
		} catch (const exception &ex) {
			Logger::info() << "plugin " << name << ": error: " << ex.what() << endl;
		}
	}

	if (!plugin) {
		Logger::warning() << "plugin " << name << ": unable to find suitable plugin" << endl;
		return;
	}

	/*
	 * These signals will be called from the Timer thread.
	 */
	plugin->onTimerSignal.connect(bind(&Irccd::handleTimerSignal, this, plugin, _1));
	plugin->onTimerEnd.connect(bind(&Irccd::handleTimerEnd, this, plugin, _1));
	plugin->onLoad();

	m_plugins.emplace(plugin->info().name, move(plugin));
}

#endif // !WITH_JS

void Irccd::handleServerOnChannelNotice(shared_ptr<Server> server, string origin, string channel, string notice)
{
	Logger::debug() << "server " << server->info().name << ": onChannelNotice: "
			<< "origin=" << origin << ", channel=" << channel <<", notice=" << notice << endl;

	ostringstream json;

	json << "{"
	     << "\"event\":\"channelNotice\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"channel\":\"" << JsonValue::escape(channel) << "\","
	     << "\"notice\":\"" << JsonValue::escape(notice) << "\""
	     << "}";

	addServerEvent({server->info().name, origin, channel, json.str(),
		[=] (Plugin &) -> string {
			return "onChannelNotice";
		},
		[=] (Plugin &plugin) {
			plugin.onChannelNotice(move(server), move(origin), move(channel), move(notice));
		}
	});
}

void Irccd::handleServerOnConnect(shared_ptr<Server> server)
{
	Logger::debug() << "server " << server->info().name << ": onConnect" << endl;

	ostringstream json;

	json << "{"
	     << "\"event\":\"onConnect\","
	     << "\"server\":\"" << server->info().name << "\""
	     << "}";

	addServerEvent({server->info().name, /* origin */ "", /* channel */ "", json.str(),
		[=] (Plugin &) -> string {
			return "onConnect";
		},
		[=] (Plugin &plugin) {
			plugin.onConnect(move(server));
		}
	});
}

void Irccd::handleServerOnInvite(shared_ptr<Server> server, string origin, string channel, string target)
{
	Logger::debug() << "server " << server->info().name << ": onInvite: "
			<< "origin=" << origin << ", channel=" << channel << ", target=" << target << endl;

	ostringstream json;

	json << "{"
	     << "\"event\":\"onInvite\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"channel\":\"" << JsonValue::escape(channel) << "\""
	     << "}";

	addServerEvent({server->info().name, origin, channel, json.str(),
		[=] (Plugin &) -> string {
			return "onInvite";
		},
		[=] (Plugin &plugin) {
			plugin.onInvite(move(server), move(origin), move(channel));
		}
	});
}

void Irccd::handleServerOnJoin(shared_ptr<Server> server, string origin, string channel)
{
	Logger::debug() << "server " << server->info().name << ": onJoin: "
			<< "origin=" << origin << ", channel=" << channel << endl;

	ostringstream json;

	json << "{"
	     << "\"event\":\"onJoin\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"channel\":\"" << JsonValue::escape(channel) << "\""
	     << "}";

	addServerEvent({server->info().name, origin, channel, json.str(),
		[=] (Plugin &) -> string {
			return "onJoin";
		},
		[=] (Plugin &plugin) {
			plugin.onInvite(move(server), move(origin), move(channel));
		}
	});
}

void Irccd::handleServerOnKick(shared_ptr<Server> server, string origin, string channel, string target, string reason)
{
	Logger::debug() << "server " << server->info().name << ": onKick: "
			<< "origin=" << origin << ", channel=" << channel << ", target=" << target << ", reason=" << reason << endl;

	ostringstream json;

	json << "{"
	     << "\"event\":\"onKick\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"channel\":\"" << JsonValue::escape(channel) << "\","
	     << "\"target\":\"" << JsonValue::escape(target) << "\","
	     << "\"reason\":\"" << JsonValue::escape(reason) << "\""
	     << "}";

	addServerEvent({server->info().name, origin, channel, json.str(),
		[=] (Plugin &) -> string {
			return "onKick";
		},
		[=] (Plugin &plugin) {
			plugin.onKick(move(server), move(origin), move(channel), move(target), move(reason));
		}
	});
}

void Irccd::handleServerOnMessage(shared_ptr<Server> server, string origin, string channel, string message)
{
	Logger::debug() << "server " << server->info().name << ": onMessage: "
			<< "origin=" << origin << ", channel=" << channel << ", message=" << message << endl;

	ostringstream json;

	json << "{"
	     << "\"event\":\"Message\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"channel\":\"" << JsonValue::escape(channel) << "\","
	     << "\"message\":\"" << JsonValue::escape(message) << "\""
	     << "}";

	addServerEvent({server->info().name, origin, channel, json.str(),
		[=] (Plugin &plugin) -> string {
			return parseMessage(message, *server, plugin).second == ServerMessageType::Command ? "onCommand" : "onMessage";
		},
		[=] (Plugin &plugin) {
			ServerMessagePair pack = parseMessage(message, *server, plugin);

			if (pack.second == ServerMessageType::Command) {
				plugin.onCommand(move(server), move(channel), move(origin), move(message));
			} else {
				plugin.onMessage(move(server), move(channel), move(origin), move(message));
			}
		}
	});
}

void Irccd::handleServerOnMe(shared_ptr<Server> server, string origin, string target, string message)
{
	Logger::debug() << "server " << server->info().name << ": onMe: "
			<< "origin=" << origin << ", target=" << target << ", message=" << message << endl;

	ostringstream json;

	json << "{"
	     << "\"event\":\"onMe\","
	     << "\"server\":\"" << server->info().name << "\""
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"target\":\"" << JsonValue::escape(target) << "\","
	     << "\"message\":\"" << JsonValue::escape(message) << "\""
	     << "}";

	addServerEvent({server->info().name, origin, target, json.str(),
		[=] (Plugin &) -> string {
			return "onMe";
		},
		[=] (Plugin &plugin) {
			plugin.onMe(move(server), move(origin), move(target), move(message));
		}
	});
}

void Irccd::handleServerOnMode(shared_ptr<Server> server, string origin, string channel, string mode, string arg)
{
	Logger::debug() << "server " << server->info().name << ": onMode: "
			<< "origin=" << origin << ", channel=" << channel << ", mode=" << mode << ", argument=" << arg << endl;

	ostringstream json;

	json << "{"
	     << "\"event\":\"Mode\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"mode\":\"" << JsonValue::escape(mode) << "\","
	     << "\"argument\":\"" << JsonValue::escape(arg) << "\""
	     << "}";

	addServerEvent({server->info().name, origin, channel, json.str(),
		[=] (Plugin &) -> string {
			return "onMode";
		},
		[=] (Plugin &plugin) {
			plugin.onMode(move(server), move(origin), move(channel), move(mode), move(arg));
		}
	});
}

void Irccd::handleServerOnNick(shared_ptr<Server> server, string origin, string nickname)
{
	Logger::debug() << "server " << server->info().name << ": onNick: "
			<< "origin=" << origin << ", nickname=" << nickname << endl;

	ostringstream json;

	json << "{"
	     << "\"event\":\"onNick\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"old\":\"" << origin << "\","
	     << "\"new\":\"" << nickname << "\""
	     << "}";

	addServerEvent({server->info().name, origin, /* channel */ "", json.str(),
		[=] (Plugin &) -> string {
			return "onNick";
		},
		[=] (Plugin &plugin) {
			plugin.onNick(move(server), move(origin), move(nickname));
		}
	});
}

void Irccd::handleServerOnNotice(shared_ptr<Server> server, string origin, string message)
{
	Logger::debug() << "server " << server->info().name << ": onNotice: "
			<< "origin=" << origin << ", message=" << message << endl;

	ostringstream json;

	json << "{"
	     << "\"event\":\"onNotice\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"notice\":\"" << JsonValue::escape(message) << "\""
	     << "}";

	addServerEvent({server->info().name, origin, /* channel */ "", json.str(),
		[=] (Plugin &) -> string {
			return "onNotice";
		},
		[=] (Plugin &plugin) {
			plugin.onNotice(move(server), move(origin), move(message));
		}
	});
}

void Irccd::handleServerOnPart(shared_ptr<Server> server, string origin, string channel, string reason)
{
	Logger::debug() << "server " << server->info().name << ": onPart: "
			<< "origin=" << origin << ", channel=" << channel << ", reason=" << reason << endl;

	ostringstream json;

	json << "{"
	     << "\"event\":\"Part\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"channel\":\"" << JsonValue::escape(channel) << "\","
	     << "\"reason\":\"" << JsonValue::escape(reason) << "\""
	     << "}";

	addServerEvent({server->info().name, origin, channel, json.str(),
		[=] (Plugin &) -> string {
			return "onPart";
		},
		[=] (Plugin &plugin) {
			plugin.onPart(move(server), move(origin), move(channel), move(reason));
		}
	});
}

void Irccd::handleServerOnQuery(shared_ptr<Server> server, string origin, string message)
{
	Logger::debug() << "server " << server->info().name << ": onQuery: "
			<< "origin=" << origin << ", message=" << message << endl;

	ostringstream json;

	json << "{"
	     << "\"event\":\"query\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"message\":\"" << JsonValue::escape(message) << "\""
	     << "}";

	addServerEvent({server->info().name, origin, /* channel */ "", json.str(),
		[=] (Plugin &plugin) -> string {
			return parseMessage(message, *server, plugin).second == ServerMessageType::Command ? "onQueryCommand" : "onQuery";
		},
		[=] (Plugin &plugin) {
			ServerMessagePair pack = parseMessage(message, *server, plugin);

			if (pack.second == ServerMessageType::Command) {
				plugin.onQueryCommand(move(server), move(origin), move(message));
			} else {
				plugin.onQuery(move(server), move(origin), move(message));
			}
		}
	});
}

void Irccd::handleServerOnTopic(shared_ptr<Server> server, string origin, string channel, string topic)
{
	Logger::debug() << "server " << server->info().name << ": onTopic: "
			<< "origin=" << origin << ", channel=" << channel << ", topic=" << topic << endl;

	ostringstream json;

	json << "{"
	     << "\"event\":\"Topic\""
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"channel\":\"" << JsonValue::escape(channel) << "\","
	     << "\"topic\":\"" << JsonValue::escape(topic) << "\""
	     << "}";

	addServerEvent({server->info().name, origin, channel, json.str(),
		[=] (Plugin &) -> string {
			return "onTopic";
		},
		[=] (Plugin &plugin) {
			plugin.onTopic(move(server), move(origin), move(channel), move(topic));
		}
	});
}

void Irccd::handleServerOnUserMode(shared_ptr<Server> server, string origin, string mode)
{
	Logger::debug() << "server " << server->info().name << ": onUserMode: "
			<< "origin=" << origin << ", mode=" << mode << endl;

	ostringstream json;

	json << "{"
	     << "\"event\":\"UserMode\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"mode\":\"" << JsonValue::escape(mode) << "\""
	     << "}";

	addServerEvent({server->info().name, origin, /* channel */ "", json.str(),
		[=] (Plugin &) -> string {
			return "onUserMode";
		},
		[=] (Plugin &plugin) {
			plugin.onUserMode(move(server), move(origin), move(mode));
		}
	});
}

/* ---------------------------------------------------------
 * Transport management
 * --------------------------------------------------------- */

void Irccd::handleTransportChannelNotice(shared_ptr<TransportClientAbstract> tc, string server, string channel, string message)
{
	addTransportEvent(tc, [=] () {
		findServer(server)->cnotice(move(channel), move(message));
	});
}

void Irccd::handleTransportConnect()
{
	// TODO
}

void Irccd::handleTransportDisconnect(shared_ptr<TransportClientAbstract> tc, string server)
{
	addTransportEvent(tc, [=] () {
		findServer(server)->disconnect();
	});
}

void Irccd::handleTransportInvite(shared_ptr<TransportClientAbstract> tc, string server, string target, string channel)
{
	addTransportEvent(tc, [=] () {
		findServer(server)->invite(move(target), move(channel));
	});
}

void Irccd::handleTransportJoin(shared_ptr<TransportClientAbstract> tc, string server, string channel, string password)
{
	addTransportEvent(tc, [=] () {
		findServer(server)->join(move(channel), move(password));
	});
}

void Irccd::handleTransportKick(shared_ptr<TransportClientAbstract> tc, string server, string target, string channel, string reason)
{
	addTransportEvent(tc, [=] () {
		findServer(server)->kick(move(target), move(channel), move(reason));
	});
}

void Irccd::handleTransportMe(shared_ptr<TransportClientAbstract> tc, string server, string channel, string message)
{
	addTransportEvent(tc, [=] () {
		findServer(server)->me(move(channel), move(message));
	});
}

void Irccd::handleTransportMessage(shared_ptr<TransportClientAbstract> tc, string server, string channel, string message)
{
	addTransportEvent(tc, [=] () {
		findServer(server)->message(move(channel), move(message));
	});
}

void Irccd::handleTransportMode(shared_ptr<TransportClientAbstract> tc, string server, string channel, string mode)
{
	addTransportEvent(tc, [=] () {
		findServer(server)->mode(move(channel), move(mode));
	});
}

void Irccd::handleTransportNick(shared_ptr<TransportClientAbstract> tc, string server, string nickname)
{
	addTransportEvent(tc, [=] () {
		findServer(server)->nick(move(nickname));
	});
}

void Irccd::handleTransportNotice(shared_ptr<TransportClientAbstract> tc, string server, string target, string message)
{
	addTransportEvent(tc, [=] () {
		findServer(server)->notice(move(target), move(message));
	});
}

void Irccd::handleTransportPart(shared_ptr<TransportClientAbstract> tc, string server, string channel, string reason)
{
	addTransportEvent(tc, [=] () {
		findServer(server)->part(move(channel), move(reason));
	});
}

void Irccd::handleTransportReconnect(shared_ptr<TransportClientAbstract> tc, string server)
{
	addTransportEvent(tc, [=] () {
		// TODO
	});

	(void)server;
}

void Irccd::handleTransportReload(shared_ptr<TransportClientAbstract> tc, string plugin)
{
	addTransportEvent(tc, [=] () {
		// TODO
	});

	(void)plugin;
}

void Irccd::handleTransportTopic(shared_ptr<TransportClientAbstract> tc, string server, string channel, string topic)
{
	addTransportEvent(tc, [=] () {
		findServer(server)->topic(move(channel), move(topic));
	});
}

void Irccd::handleTransportUnload(shared_ptr<TransportClientAbstract> tc, string plugin)
{
	addTransportEvent(tc, [=] () {
		// TODO
	});

	(void)plugin;
}

void Irccd::handleTransportUserMode(shared_ptr<TransportClientAbstract> tc, string server, string mode)
{
	addTransportEvent(tc, [=] () {
		findServer(server)->umode(move(mode));
	});
}

/* --------------------------------------------------------
 * Timer slots
 * -------------------------------------------------------- */

#if defined(WITH_JS)

void Irccd::handleTimerSignal(std::shared_ptr<Plugin> plugin, std::shared_ptr<Timer> timer)
{
	addEvent([this, plugin, timer] () {
		duk_context *ctx = plugin->context();

		dukx_assert_begin(ctx);
		duk_push_global_object(ctx);
		duk_get_prop_string(ctx, -1, "\xff" "irccd-timers");
		duk_push_pointer(ctx, timer.get());
		duk_get_prop(ctx, -2);

		if (duk_pcall(ctx, 0) != 0) {
			Logger::warning() << "plugin " << plugin->info().name
					  << "failed to call timer: " << duk_safe_to_string(ctx, -1) << std::endl;
		}

		duk_pop(ctx);
		duk_pop_2(ctx);
		dukx_assert_equals(ctx);
	});
}

void Irccd::handleTimerEnd(std::shared_ptr<Plugin> plugin, std::shared_ptr<Timer> timer)
{
	addEvent([this, plugin, timer] () {
		plugin->timerRemove(timer);
	});
}

#endif

void Irccd::run()
{
	while (m_running) {
		exec();
	}
}

void Irccd::stop()
{
	Logger::debug() << "irccd: requesting to stop now" << std::endl;

	m_running = false;
}

atomic<bool> Irccd::m_running{true};

} // !irccd

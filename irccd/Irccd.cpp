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

Irccd::~Irccd()
{
}

/* ---------------------------------------------------------
 * Event management
 * --------------------------------------------------------- */

void Irccd::addServerEvent(string, string, string, string, string json, function<void (Plugin &)> ev) noexcept
{
	addEvent([=] () {
		for (auto &pair : m_plugins) {
			// TODO: match rules here
			ev(*pair.second);
		}
	});

	/* Asynchronous send */
	for (auto &pair : m_lookupTransportClients) {
		pair.second->send(json);
	}
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

/* ---------------------------------------------------------
 * Server management
 * --------------------------------------------------------- */

void Irccd::handleServerOnChannelNotice(shared_ptr<Server> server, string origin, string channel, string notice)
{
	Logger::debug() << "server " << server->info().name << ": onChannelNotice: "
			<< "origin=" << origin << ", channel=" << channel <<", notice=" << notice << endl;

	ostringstream json;

	json << "{"
	     << "\"event\":\"ChannelNotice\","
	     << "\"server\":\"" << server->info().name << "\","
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"channel\":\"" << JsonValue::escape(channel) << "\","
	     << "\"notice\":\"" << JsonValue::escape(notice) << "\""
	     << "}";

	addServerEvent("onChannelNotice", json.str(), server->info().name, origin, channel, [=] (Plugin &plugin) {
		plugin.onChannelNotice(server, origin, channel, notice);
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

	addServerEvent("onConnect", json.str(), server->info().name, "", "", [=] (Plugin &plugin) {
		plugin.onConnect(move(server));
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

	addServerEvent("onInvite", json.str(), server->info().name, origin, channel, [=] (Plugin &plugin) {
		plugin.onInvite(move(server), move(origin), move(channel));
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

	addServerEvent("onJoin", json.str(), server->info().name, origin, channel, [=] (Plugin &plugin) {
		plugin.onJoin(move(server), move(origin), move(channel));
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

	addServerEvent("onKick", json.str(), server->info().name, origin, channel, [=] (Plugin &plugin) {
		plugin.onKick(move(server), move(origin), move(channel), move(target), move(reason));
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

	// TODO: handle onMessage/onCommand
}

void Irccd::handleServerOnMe(shared_ptr<Server> server, string origin, string target, string message)
{
	Logger::debug() << "server " << server->info().name << ": onMe: "
			<< "origin=" << origin << ", target=" << target << ", message=" << message << endl;

	ostringstream json;

	json << "{"
	     << "\"event\":\"Me\","
	     << "\"server\":\"" << server->info().name << "\""
	     << "\"origin\":\"" << JsonValue::escape(origin) << "\","
	     << "\"target\":\"" << JsonValue::escape(target) << "\","
	     << "\"message\":\"" << JsonValue::escape(message) << "\""
	     << "}";

	addServerEvent("onMe", json.str(), server->info().name, origin, target, [=] (Plugin &plugin) {
		plugin.onMe(move(server), move(origin), move(target), move(message));
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

	addServerEvent("onMode", json.str(), server->info().name, origin, channel, [=] (Plugin &plugin) {
		plugin.onMode(move(server), move(origin), move(channel), move(mode), move(arg));
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

	addServerEvent("onNick", json.str(), server->info().name, origin, "", [=] (Plugin &plugin) {
		plugin.onNick(move(server), move(origin), move(nickname));
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

	addServerEvent("onNotice", json.str(), server->info().name, origin, /* channel */ "", [=] (Plugin &plugin) {
		plugin.onNotice(move(server), move(origin), move(message));
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

	addServerEvent("onPart", json.str(), server->info().name, origin, channel, [=] (Plugin &plugin) {
		plugin.onPart(move(server), move(origin), move(channel), move(reason));
	});
}

void Irccd::handleServerOnQuery(shared_ptr<Server> server, string origin, string message)
{
	Logger::debug() << "server " << server->info().name << ": onQuery: "
			<< "origin=" << origin << ", message=" << message << endl;

	// TODO: like onMessage
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

	addServerEvent("onTopic", json.str(), server->info().name, origin, channel, [=] (Plugin &plugin) {
		plugin.onTopic(move(server), move(origin), move(channel), move(topic));
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

	addServerEvent("onUserMode", json.str(), server->info().name, origin, "", [=] (Plugin &plugin) {
		plugin.onUserMode(server, origin, mode);
	});
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

#if 0

void Irccd::load(const std::string &path, bool isrelative)
{
	// TODO
	(void)isrelative;
	(void)path;
}

#endif

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

void Irccd::addTransport(std::shared_ptr<TransportServerAbstract> ts)
{

	m_lookupTransportServers.emplace(ts->socket().handle(), move(ts));
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

#if 0
	/*
	 * These signals will be called from the Timer thread.
	 */
	plugin->onTimerSignal.connect([this, plugin] (shared_ptr<Timer> timer) {
		timerAddEvent({move(plugin), move(timer)});
	});
	plugin->onTimerEnd.connect([this, plugin] (shared_ptr<Timer> timer) {
		timerAddEvent({move(plugin), move(timer), TimerEventType::End});
	});
	plugin->onLoad();
#endif

	m_plugins.emplace(plugin->info().name, move(plugin));
}

#if 0

void Irccd::pluginUnload(const string &name)
{
	shared_ptr<Plugin> plugin = pluginFind(name);

	plugin->onUnload();

	/*
	 * Erase any element in the timer event queue that match this plugin
	 * to avoid calling the event of a plugin that is not in irccd anymore.
	 */
	for (auto it = m_timerEvents.begin(); it != m_timerEvents.end(); ) {
		if (it->plugin() == plugin) {
			it = m_timerEvents.erase(it);
		} else {
			++ it;
		}
	}

	m_plugins.erase(plugin->info().name);
}

#endif
#endif

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
			Logger::debug() << "transport: new client" << std::endl;

			auto client = pair.second->accept();

			m_lookupTransportClients.emplace(client->socket().handle(), move(client));
		}
	}

	/* 3. Check for servers */
	for (auto &pair : m_servers) {
		pair.second->sync(setinput, setoutput);
	}
}

void Irccd::dispatch()
{
	Logger::debug() << "irccd: dispatching " << m_events.size() << " event(s)" << std::endl;

	/*
	 * Make a copy because the events can add other events while we are iterating it. Also lock because the timers
	 * may alter these events too.
	 */
	vector<Event> copy;

	{
		lock_guard<mutex> lock(m_mutex);

		copy = move(m_events);
	}

	/* Clear for safety */
	m_events.clear();

	for (auto &ev : copy) {
		ev();
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

	/* Skip on error */
	if (error < 0) {
		Logger::warning() << "irccd: " << SocketAbstract::syserror() << std::endl;
		return;
	}

	process(setinput, setoutput);
	dispatch();
}

void Irccd::run()
{
	while (m_running) {
		exec();
	}
}

void Irccd::stop()
{
	m_running = false;
	m_condition.notify_one();
}

atomic<bool> Irccd::m_running{true};
condition_variable Irccd::m_condition;

} // !irccd

#if 0

using namespace literals;

namespace irccd {

Irccd::Irccd()
	: m_running(true)
	, m_foreground(false)
{
}

void Irccd::initialize()
{
	ostringstream oss;

	Socket::init();
	Logger::setVerbose(false);

#if defined(WITH_LUA)
	// Add user's path
	oss << Util::pathUser() << "plugins/";
	PluginManager::instance().addPath(oss.str());

	// Add system's path
	oss.str("");
	if (!Util::isAbsolute(MODDIR))
		oss << Util::pathBase();

	oss << MODDIR << Util::DIR_SEP;
	PluginManager::instance().addPath(oss.str());
#endif
}

bool Irccd::isOverriden(char c)
{
	return m_overriden.find(c) != m_overriden.end();
}

/*
 * Order is:
 * 1. Option -c passed to the command line
 * 2. User defined, usually ~/.config/irccd/irccd.conf
 * 3. Default cmake configured path, usually /usr/local/etc/irccd.conf
 */
void Irccd::openConfig()
{
	Parser config;

	// Open requested file by command line or default
	try {
		if (!isOverriden(Options::Config)) {
			try {
				m_configPath = Util::findConfiguration("irccd.conf");
				config = Parser(m_configPath);
			} catch (const runtime_error &ex) {
				Logger::fatal(1, "irccd: %s", ex.what());
			}
		} else
			config = Parser(m_configPath);
	} catch (const runtime_error &ex) {
		Logger::fatal(1, "irccd: could not open %s, exiting", m_configPath.c_str());
	}

	Logger::log("irccd: using configuration %s", m_configPath.c_str());

	/*
	 * Order is important, load everything we can before plugins so that
	 * they can use identities, configuration and such, but servers
	 * requires plugin to be loaded.
	 */
	readGeneral(config);
	readIdentities(config);
	readRules(config);
	readListeners(config);
	readPlugins(config);

#if !defined(_WIN32)
	if (!m_foreground) {
		Logger::log("irccd: forking to background...");
		(void)daemon(0, 0);
	}
#endif

#if defined(WITH_LUA)
	/* Now, we load plugins specified by command line */
	for (auto s : m_wantedPlugins) {
		try {
			PluginManager::instance().load(s);
		} catch (const exception &error) {
			Logger::warn("irccd: %s", error.what());
		}
	}
#endif

	readServers(config);
}

void Irccd::readGeneral(const Parser &config)
{
	if (config.hasSection("general")) {
		auto general = config.getSection("general");

#if defined(WITH_LUA)
		// Extract parameters that are needed for the next
		if (general.hasOption("plugin-path"))
			PluginManager::instance().addPath(general.getOption<string>("plugin-path"));
#endif

#if !defined(_WIN32)
		if (general.hasOption("syslog"))
			Logger::setSyslog(general.getOption<bool>("syslog"));
		if (general.hasOption("foreground") && !isOverriden(Options::Foreground))
			m_foreground = general.getOption<bool>("foreground");
		if (general.hasOption("verbose") && !isOverriden(Options::Verbose))
			Logger::setVerbose(general.getOption<bool>("verbose"));

		m_uid = parse(general, "uid", false);
		m_gid = parse(general, "gid", true);

		if (setgid(m_gid) < 0)
			Logger::warn("irccd: failed to set gid to %s: %s",
			    idname(true).c_str(), strerror(errno));
		if (setuid(m_uid) < 0)
			Logger::warn("irccd: failed to set uid to %s: %s",
			    idname(false).c_str(), strerror(errno));
#endif
	}
}

#if !defined(_WIN32)

int Irccd::parse(const Section &section, const char *name, bool isgid)
{
	int result(0);

	if (!section.hasOption(name))
		return 0;

	auto value = section.getOption<string>(name);

	try {
		if (isgid) {
			auto group = getgrnam(value.c_str());
			result = (group == nullptr) ? stoi(value) : group->gr_gid;
		} else {
			auto pw = getpwnam(value.c_str());
			result = (pw == nullptr) ? stoi(value) : pw->pw_uid;
		}
	} catch (...) {
		Logger::warn("irccd: invalid %sid %s", ((isgid) ? "g" : "u"), value.c_str());
	}

	return result;
}

string Irccd::idname(bool isgid)
{
	string result;

	if (isgid) {
		auto group = getgrgid(m_gid);
		result = (group == nullptr) ? to_string(m_gid) : group->gr_name;
	} else {
		auto pw = getpwuid(m_uid);
		result = (pw == nullptr) ? to_string(m_uid) : pw->pw_name;
	}

	return result;
}

#endif

void Irccd::readPlugins(const Parser &config)
{
	// New way of loading plugins
	if (config.hasSection("plugins")) {
#if defined(WITH_LUA)
		Section section = config.getSection("plugins");

		for (auto opt : section) {
			try {
				if (opt.second.length() == 0)
					PluginManager::instance().load(opt.first);
				else
					PluginManager::instance().load(opt.second, true);
			} catch (const runtime_error &error) {
				Logger::warn("irccd: %s", error.what());
			}
		}
#else
		(void)config;
		Logger::warn("irccd: ignoring plugins, Lua support is disabled");
#endif
	}
}

void Irccd::readIdentities(const Parser &config)
{
	config.findSections("identity", [&] (const Section &s) {
		Server::Identity identity;

		try {
			identity.name = s.requireOption<string>("name");

			if (s.hasOption("nickname"))
				identity.nickname = s.getOption<string>("nickname");
			if (s.hasOption("username"))
				identity.username = s.getOption<string>("username");
			if (s.hasOption("realname"))
				identity.realname = s.getOption<string>("realname");
			if (s.hasOption("ctcp-version"))
				identity.ctcpVersion = s.getOption<string>("ctcp-version");

			Logger::log("identity: found identity %s (%s, %s, \"%s\")",
			    identity.name.c_str(),
			    identity.nickname.c_str(), identity.username.c_str(),
			    identity.realname.c_str());

			m_identities.push_back(identity);
		} catch (const out_of_range &ex) {
			Logger::log("identity: parameter %s", ex.what());
		}
	});
}

void Irccd::readRules(const Parser &config)
{
#if defined(WITH_LUA)
	using string;

	config.findSections("rule", [&] (const Section &s) {
		RuleMap servers = getList(s, "servers");
		RuleMap channels = getList(s, "channels");
		RuleMap nicknames = getList(s, "nicknames");
		RuleMap plugins = getList(s, "plugins");
		RuleMap events = getList(s, "events");
		RuleAction action{RuleAction::Accept};

		if (!s.hasOption("action")) {
			Logger::warn("rule: missing action");
			return;
		}

		auto value = s.getOption<string>("action");

		if (value == "drop") {
			action = RuleAction::Drop;
		} else if (value == "accept") {
			action = RuleAction::Accept;
		} else {
			Logger::warn("rule: invalid action value `%s`", value.c_str());
			return;
		}

		Logger::debug("rule: found rule (%s)", (action == RuleAction::Accept) ? "accept" : "drop");

		RuleManager::instance().add(Rule{servers, channels, nicknames, plugins, events, action});
	});
#else
	(void)config;
	Logger::warn("irccd: ignoring rules, Lua support is disabled");
#endif
}

void Irccd::readListeners(const Parser &config)
{
	config.findSections("listener", [&] (const Section &s) {
		try {
			string type = s.requireOption<string>("type");

			if (type == "internet")
				extractInternet(s);
			else if (type == "unix") {
#if !defined(_WIN32)
				extractUnix(s);
#else
				Logger::warn("listener: unix sockets are not supported on Windows");
#endif
			} else
				Logger::warn("listener: unknown listener type `%s'", type.c_str());
		} catch (const exception &ex) {
			Logger::warn("listener: parameter %s", ex.what());
		}
	});
}

void Irccd::extractInternet(const Section &s)
{
	vector<string> protocols;
	string address = "*", family;
	int port;
	bool ipv4 = false, ipv6 = false;

	if (s.hasOption("address"))
		address = s.getOption<string>("address");

	family = s.getOption<string>("family");
	port = s.getOption<int>("port");

	// extract list of family
	protocols = Util::split(family, " \t");

	for (auto p : protocols) {
		if (p == "ipv4")
			ipv4 = true;
		else if (p == "ipv6")
			ipv6 = true;
		else {
			Logger::warn("listener: parameter family is one of them: ipv4, ipv6");
			Logger::warn("listener: defaulting to ipv4");

			ipv4 = true;
			ipv6 = false;
		}
	}

	try {
		int reuse = 1;

		Socket inet((ipv6) ? AF_INET6 : AF_INET, SOCK_STREAM, 0);

		inet.set(SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse));
		if (ipv6) {
			int mode = !ipv4;
			inet.set(IPPROTO_IPV6, IPV6_V6ONLY, &mode, sizeof (mode));
		}

		inet.bind(BindAddressIP(address, port, (ipv6) ? AF_INET6 : AF_INET));
		inet.listen(64);

		Listener::instance().add(inet);
		Logger::log("listener: listening for clients on port %d...", port);
	} catch (const SocketError &ex) {
		Logger::warn("listener: internet socket error: %s", ex.what());
	}
}

#if !defined(_WIN32)
void Irccd::extractUnix(const Section &s)
{
	auto path = s.requireOption<string>("path");

	// First remove the dust
	if (Util::exist(path) && remove(path.c_str()) < 0) {
		Logger::warn("listener: error removing %s: %s", path.c_str(), strerror(errno));
	} else {
		try {
			Socket un(AF_UNIX, SOCK_STREAM, 0);

			un.bind(AddressUnix(path, true));
			un.listen(64);

			Listener::instance().add(un);
			Logger::log("listener: listening for clients on %s...", path.c_str());
		} catch (const SocketError &ex) {
			Logger::warn("listener: unix socket error: %s", ex.what());
		}
	}
}
#endif

void Irccd::readServers(const Parser &config)
{
	config.findSections("server", [&] (const Section &s) {
		try {
			Server::Info info;
			Server::Identity identity;
			Server::RetryInfo reco;
			unsigned options = 0;

			// Server information
			info.name = s.requireOption<string>("name");
			info.host = s.requireOption<string>("host");
			info.port = s.requireOption<int>("port");

			if (s.hasOption("command-char"))
				info.command = s.getOption<string>("command-char");

			// Some boolean options
			if (s.hasOption("ssl") && s.getOption<bool>("ssl"))
				options |= Server::OptionSsl;
			if (s.hasOption("ssl-verify") && !s.getOption<bool>("ssl-verify"))
				options |= Server::OptionSslNoVerify;
			if (s.hasOption("join-invite") && s.getOption<bool>("join-invite"))
				options |= Server::OptionJoinInvite;
			if (s.hasOption("auto-rejoin") && s.getOption<bool>("auto-rejoin"))
				options |= Server::OptionAutoRejoin;

			if (s.hasOption("password"))
				info.password = s.getOption<string>("password");

			// Identity
			if (s.hasOption("identity"))
				identity = findIdentity(s.getOption<string>("identity"));

			// Reconnection settings
			if (s.hasOption("reconnect"))
				reco.enabled = s.getOption<bool>("reconnect");
			if (s.hasOption("reconnect-tries"))
				reco.maxretries = s.getOption<int>("reconnect-tries");
			if (s.hasOption("reconnect-timeout"))
				reco.timeout = s.getOption<int>("reconnect-timeout");

			auto server = make_shared<Server>(info, identity, reco, options);
			auto &manager = ServerManager::instance();

			// Extract channels to auto join
			extractChannels(s, server);
			if (manager.has(info.name))
				Logger::warn("server %s: duplicated server", info.name.c_str());
			else
				manager.add(move(server));
		} catch (const out_of_range &ex) {
			Logger::warn("server: parameter %s", ex.what());
		}
	});
}

void Irccd::extractChannels(const Section &section, shared_ptr<Server> &server)
{
	vector<string> channels;
	string list, name, password;

	if (section.hasOption("channels")) {
		list = section.getOption<string>("channels");
		channels = Util::split(list, " \t");

		for (const auto &s : channels)
			server->addChannel(Server::toChannel(s));
	}
}

Irccd::~Irccd()
{
	Socket::finish();
}

void Irccd::override(char c)
{
	m_overriden[c] = true;
}

void Irccd::setConfigPath(const string &path)
{
	m_configPath = path;
}

void Irccd::setForeground(bool mode)
{
	m_foreground = mode;
}

const Server::Identity &Irccd::findIdentity(const string &name)
{
	/*
	 * When name is length 0 that mean user hasn't defined an identity
	 * because it's optional, we don't write an empty message error.
	 */
	if (name.length() == 0)
		return m_defaultIdentity;

	for (const auto &i : m_identities)
		if (i.name == name)
			return i;

	Logger::warn("identity: %s not found", name.c_str());

	return m_defaultIdentity;
}

} // !irccd

#endif

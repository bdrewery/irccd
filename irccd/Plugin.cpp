/*
 * Plugin.cpp -- irccd Lua plugin interface
 *
 * Copyright (c) 2013, 2014 David Demelier <markand@malikania.fr>
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
#include <sstream>
#include <stdexcept>

#include <Logger.h>
#include <Util.h>

#include "Irccd.h"
#include "Plugin.h"

namespace irccd {

/* --------------------------------------------------------
 * Plugin exception
 * -------------------------------------------------------- */

Plugin::ErrorException::ErrorException(std::string which, std::string error)
	: m_error(std::move(error))
	, m_which(std::move(which))
{
}

const std::string &Plugin::ErrorException::which() const
{
	return m_which;
}

const std::string &Plugin::ErrorException::error() const
{
	return m_error;
}

const char *Plugin::ErrorException::what() const throw()
{
	return m_error.c_str();
}

/* --------------------------------------------------------
 * private methods and members
 * -------------------------------------------------------- */

std::string Plugin::getGlobal(const std::string &name)
{
	std::string result;

	Luae::getglobal(*m_process, name);
	if (Luae::type(*m_process, -1) == LUA_TSTRING)
		result = Luae::get<std::string>(*m_process, -1);
	Luae::pop(*m_process, 1);

	return result;
}

/* --------------------------------------------------------
 * Public methods
 * -------------------------------------------------------- */

Plugin::Plugin(std::string name, std::string path)
{
	m_info.name = std::move(name);
	m_info.path = std::move(path);
	m_process = std::make_shared<Process>();
}

const std::string &Plugin::getName() const
{
	return m_info.name;
}

const std::string &Plugin::getHome() const
{
	return m_info.home;
}

lua_State *Plugin::getState()
{
	return static_cast<lua_State *>(*m_process);
}

void Plugin::open()
{
	auto lock = m_process->lock();
	auto L = static_cast<lua_State *>(*m_process);

	// Load default library as it was done by require.
	for (const auto &l : Process::luaLibs)
		Luae::require(L, l.first, l.second, true);

	// Put external modules in package.preload so user
	// will need require (modname)
	for (const auto &l : Process::irccdLibs)
		Luae::preload(L, l.first, l.second);

	try {
		Luae::dofile(L, m_info.path);
	} catch (const std::exception &error) {
		throw ErrorException(m_info.name, error.what());
	}

	// Find the home directory for the plugin
	m_info.home = Util::findPluginHome(m_info.name);

	// Extract global information
	m_info.author	= getGlobal("AUTHOR");
	m_info.comment	= getGlobal("COMMENT");
	m_info.version	= getGlobal("VERSION");
	m_info.license	= getGlobal("LICENSE");

	// Initialize the plugin name and its data
	Process::initialize(m_process, m_info);

	// Do a initial load
	call("onLoad");
}

/* --------------------------------------------------------
 * Plugin callbacks
 * -------------------------------------------------------- */

void Plugin::onCommand(std::shared_ptr<Server> server, std::string channel, std::string nick, std::string message)
{
	call("onCommand", std::move(server), std::move(channel), std::move(nick), std::move(message));
}

void Plugin::onConnect(std::shared_ptr<Server> server)
{
	call("onConnect", std::move(server));
}

void Plugin::onChannelNotice(std::shared_ptr<Server> server, std::string who, std::string channel, std::string notice)
{
	call("onChannelNotice", std::move(server), std::move(who), std::move(channel), std::move(notice));
}

void Plugin::onInvite(std::shared_ptr<Server> server, std::string channel, std::string who)
{
	call("onInvite", std::move(server), std::move(channel), std::move(who));
}

void Plugin::onJoin(std::shared_ptr<Server> server, std::string channel, std::string nickname)
{
	call("onJoin", std::move(server), std::move(channel), std::move(nickname));
}

void Plugin::onKick(std::shared_ptr<Server> server, std::string channel, std::string who, std::string kicked, std::string reason)
{
	call("onKick", std::move(server), std::move(channel), std::move(who), std::move(kicked), std::move(reason));
}

void Plugin::onLoad()
{
	call("onLoad");
}

void Plugin::onMessage(std::shared_ptr<Server> server, std::string channel, std::string nick, std::string message)
{
	call("onMessage", std::move(server), std::move(channel), std::move(nick), std::move(message));
}

void Plugin::onMe(std::shared_ptr<Server> server, std::string channel, std::string nick, std::string message)
{
	call("onMe", std::move(server), std::move(channel), std::move(nick), std::move(message));
}

void Plugin::onMode(std::shared_ptr<Server> server, std::string channel, std::string nickname, std::string mode, std::string arg)
{
	call("onMode", std::move(server), std::move(channel), std::move(nickname), std::move(mode), std::move(arg));
}

void Plugin::onNames(std::shared_ptr<Server> server, std::string channel, std::vector<std::string> list)
{
	call("onNames", std::move(server), std::move(channel), std::move(list));
}

void Plugin::onNick(std::shared_ptr<Server> server, std::string oldnick, std::string newnick)
{
	call("onNick", std::move(server), std::move(oldnick), std::move(newnick));
}

void Plugin::onNotice(std::shared_ptr<Server> server, std::string who, std::string target, std::string notice)
{
	call("onNotice", std::move(server), std::move(who), std::move(target), std::move(notice));
}

void Plugin::onPart(std::shared_ptr<Server> server, std::string channel, std::string nickname, std::string reason)
{
	call("onPart", std::move(server), std::move(channel), std::move(nickname), std::move(reason));
}

void Plugin::onQuery(std::shared_ptr<Server> server, std::string who, std::string message)
{
	call("onQuery", std::move(server), std::move(who), std::move(message));
}

void Plugin::onQueryCommand(std::shared_ptr<Server> server, std::string who, std::string message)
{
	call("onQueryCommand", std::move(server), std::move(who), std::move(message));
}

void Plugin::onReload()
{
	call("onReload");
}

void Plugin::onTopic(std::shared_ptr<Server> server, std::string channel, std::string who, std::string topic)
{
	call("onTopic", std::move(server), std::move(channel), std::move(who), std::move(topic));
}

void Plugin::onUnload()
{
	call("onUnload");
}

void Plugin::onUserMode(std::shared_ptr<Server> server, std::string who, std::string mode)
{
	call("onUserMode", std::move(server), std::move(who), std::move(mode));
}

void Plugin::onWhois(std::shared_ptr<Server> server, IrcWhois info)
{
	call("onWhois", std::move(server), std::move(info));
}

} // !irccd

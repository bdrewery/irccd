/*
 * Plugin.cpp -- irccd Lua plugin interface
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
#include <sstream>
#include <stdexcept>

/*
 * Keep this ordered like this, on Windows we get some errors saying that windows.h
 * must be included before winsock2.h
 */
#include "Server.h"
#include "Plugin.h"

namespace irccd {

std::string Plugin::global(const std::string &name) const
{
	std::string result;

	duk_push_global_object(m_context);
	duk_get_prop_string(m_context, -1, name.c_str());
	result = duk_to_string(m_context, -1);
	duk_pop_2(m_context);

	return result;
}

Plugin::Plugin(std::string name, std::string path)
{
	m_info.name = std::move(name);
	m_info.path = std::move(path);

	if (duk_peval_file(m_context, m_info.path.c_str()) != 0) {
		throw std::runtime_error(duk_safe_to_string(m_context, -1));
	}

	// Safe a reference to this
	duk_push_global_object(m_context);
	duk_push_pointer(m_context, this);
	duk_put_prop_string(m_context, -2, "\xff""\xff""plugin");
	duk_pop(m_context);
}

const PluginInfo &Plugin::info() const
{
	return m_info;
}

#if 0
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

#endif

void Plugin::onCommand(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string message)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, channel.c_str());
	duk_push_string(m_context, message.c_str());
}

void Plugin::onConnect(std::shared_ptr<Server> server)
{
	dukx_push_shared(m_context, server);
}

void Plugin::onChannelNotice(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string notice)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, channel.c_str());
	duk_push_string(m_context, notice.c_str());
}

void Plugin::onInvite(std::shared_ptr<Server> server, std::string origin, std::string channel)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, channel.c_str());
}

void Plugin::onJoin(std::shared_ptr<Server> server, std::string origin, std::string channel)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, channel.c_str());
}

void Plugin::onKick(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string target, std::string reason)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, channel.c_str());
	duk_push_string(m_context, target.c_str());
	duk_push_string(m_context, reason.c_str());
}

void Plugin::call(const char *name, int nargs)
{
	duk_push_global_object(m_context);
	duk_get_prop_string(m_context, -1, name);

	if (duk_get_type(m_context, -1) == DUK_TYPE_UNDEFINED) {
		duk_pop_2(m_context);
	} else {
		duk_remove(m_context, -2);
		duk_insert(m_context, -1 -nargs);

		if (duk_pcall(m_context, nargs) != 0) {
			printf("failed to call function: %s\n", duk_safe_to_string(m_context, -1));
		}

		duk_pop(m_context);
	}
}

void Plugin::onLoad()
{
	call("onLoad");
}

void Plugin::onMessage(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string message)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, channel.c_str());
	duk_push_string(m_context, message.c_str());
	call("onMessage", 4);
}

void Plugin::onMe(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string message)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, channel.c_str());
	duk_push_string(m_context, message.c_str());
}

void Plugin::onMode(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string mode, std::string arg)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, channel.c_str());
	duk_push_string(m_context, mode.c_str());
	duk_push_string(m_context, arg.c_str());
}

void Plugin::onNames(std::shared_ptr<Server>, std::string, std::vector<std::string>)
{
}

void Plugin::onNick(std::shared_ptr<Server> server, std::string oldnick, std::string newnick)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, oldnick.c_str());
	duk_push_string(m_context, newnick.c_str());
}

void Plugin::onNotice(std::shared_ptr<Server> server, std::string origin, std::string notice)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, notice.c_str());
}

void Plugin::onPart(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string reason)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, channel.c_str());
	duk_push_string(m_context, reason.c_str());
}

void Plugin::onQuery(std::shared_ptr<Server> server, std::string origin, std::string message)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, message.c_str());
}

void Plugin::onQueryCommand(std::shared_ptr<Server> server, std::string origin, std::string message)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, message.c_str());
}

void Plugin::onReload()
{
}

void Plugin::onTopic(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string topic)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, channel.c_str());
	duk_push_string(m_context, topic.c_str());
}

void Plugin::onUnload()
{
}

void Plugin::onUserMode(std::shared_ptr<Server> server, std::string origin, std::string mode)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, mode.c_str());
}

void Plugin::onWhois(std::shared_ptr<Server>, ServerWhois)
{
}

} // !irccd

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

#include <IrccdConfig.h>

#if defined(HAVE_STAT)
#  include <sys/stat.h>
#  include <cerrno>
#  include <cstring>
#endif

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

void Plugin::call(const char *name, int nargs)
{
	duk_push_global_object(m_context);
	duk_get_prop_string(m_context, -1, name);

	if (duk_get_type(m_context, -1) == DUK_TYPE_UNDEFINED) {
		duk_pop_2(m_context);
	} else {
		duk_remove(m_context, -2);
		duk_insert(m_context, -1 -nargs);

		// TODO, throw instead
		if (duk_pcall(m_context, nargs) != 0) {
			printf("failed to call function: %s\n", duk_safe_to_string(m_context, -1));
		}

		duk_pop(m_context);
	}
}

Plugin::Plugin(std::string name, std::string path, PluginConfig config)
	: m_config(std::move(config))
{
	m_info.name = std::move(name);
	m_info.path = std::move(path);

	/*
	 * Duktape currently emit useless warnings when a file do
	 * not exists so we do a homemade access.
	 */
#if defined(HAVE_STAT)
	struct stat st;

	if (stat(m_info.path.c_str(), &st) < 0) {
		throw std::runtime_error(std::strerror(errno));
	}
#endif

	if (duk_peval_file(m_context, m_info.path.c_str()) != 0) {
		throw std::runtime_error(duk_safe_to_string(m_context, -1));
	}

	/* Save a reference to this */
	duk_push_global_object(m_context);
	duk_push_pointer(m_context, this);
	duk_put_prop_string(m_context, -2, "\xff""\xff""plugin");
	duk_push_string(m_context, m_info.path.c_str());
	duk_put_prop_string(m_context, -2, "\xff""\xff""path");
	duk_push_string(m_context, m_info.name.c_str());
	duk_put_prop_string(m_context, -2, "\xff""\xff""name");
	duk_pop(m_context);
}

const PluginInfo &Plugin::info() const
{
	return m_info;
}

void Plugin::timerAdd(std::shared_ptr<Timer> timer) noexcept
{
	assert(m_onTimerSignal != nullptr);
	assert(m_onTimerEnd != nullptr);

	/*
	 * These signals are called from the Timer thread.
	 */
	timer->onSignal([this, timer] () {
		m_onTimerSignal(timer);
	});
	timer->onEnd([this, timer] () {
		m_onTimerEnd(timer);
	});

	m_timers.insert(std::move(timer));
}

void Plugin::onCommand(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string message)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, channel.c_str());
	duk_push_string(m_context, message.c_str());
	call("onCommand", 4);
}

void Plugin::onConnect(std::shared_ptr<Server> server)
{
	dukx_push_shared(m_context, server);
	call("onConnect", 1);
}

void Plugin::onChannelNotice(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string notice)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, channel.c_str());
	duk_push_string(m_context, notice.c_str());
	call("onChannelNotice", 4);
}

void Plugin::onInvite(std::shared_ptr<Server> server, std::string origin, std::string channel)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, channel.c_str());
	call("onInvite", 3);
}

void Plugin::onJoin(std::shared_ptr<Server> server, std::string origin, std::string channel)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, channel.c_str());
	call("onJoin", 3);
}

void Plugin::onKick(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string target, std::string reason)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, channel.c_str());
	duk_push_string(m_context, target.c_str());
	duk_push_string(m_context, reason.c_str());
	call("onKick", 5);
}

void Plugin::onLoad()
{
	duk_push_object(m_context);
	for (const auto &pair : m_config) {
		duk_push_string(m_context, pair.second.c_str());
		duk_put_prop_string(m_context, -2, pair.first.c_str());
	}

	call("onLoad", 1);
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
	call("onMe", 4);
}

void Plugin::onMode(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string mode, std::string arg)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, channel.c_str());
	duk_push_string(m_context, mode.c_str());
	duk_push_string(m_context, arg.c_str());
	call("onMode", 5);
}

void Plugin::onNames(std::shared_ptr<Server> server, std::string channel, std::vector<std::string> names)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, channel.c_str());
	duk_push_array(m_context);

	int i = 0;
	for (const std::string &s : names) {
		duk_push_string(m_context, s.c_str());
		duk_put_prop_index(m_context, -2, i++);
	}

	call("onNames", 3);
}

void Plugin::onNick(std::shared_ptr<Server> server, std::string oldnick, std::string newnick)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, oldnick.c_str());
	duk_push_string(m_context, newnick.c_str());
	call("onNick", 3);
}

void Plugin::onNotice(std::shared_ptr<Server> server, std::string origin, std::string notice)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, notice.c_str());
	call("onNotice", 3);
}

void Plugin::onPart(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string reason)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, channel.c_str());
	duk_push_string(m_context, reason.c_str());
	call("onPart", 4);
}

void Plugin::onQuery(std::shared_ptr<Server> server, std::string origin, std::string message)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, message.c_str());
	call("onQuery", 3);
}

void Plugin::onQueryCommand(std::shared_ptr<Server> server, std::string origin, std::string message)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, message.c_str());
	call("onQueryCommand", 3);
}

void Plugin::onReload()
{
	call("onReload");
}

void Plugin::onTopic(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string topic)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, channel.c_str());
	duk_push_string(m_context, topic.c_str());
	call("onTopic", 4);
}

void Plugin::onUnload()
{
	call("onUnload");
}

void Plugin::onUserMode(std::shared_ptr<Server> server, std::string origin, std::string mode)
{
	dukx_push_shared(m_context, server);
	duk_push_string(m_context, origin.c_str());
	duk_push_string(m_context, mode.c_str());
	call("onUserMode", 3);
}

void Plugin::onWhois(std::shared_ptr<Server> server, ServerWhois whois)
{
	dukx_push_shared(m_context, server);
	duk_push_object(m_context);
	duk_push_boolean(m_context, whois.found);
	duk_put_prop_string(m_context, -2, "found");
	duk_push_string(m_context, whois.nick.c_str());
	duk_put_prop_string(m_context, -2, "nickname");
	duk_push_string(m_context, whois.user.c_str());
	duk_put_prop_string(m_context, -2, "username");
	duk_push_string(m_context, whois.realname.c_str());
	duk_put_prop_string(m_context, -2, "realname");
	duk_push_string(m_context, whois.host.c_str());
	duk_put_prop_string(m_context, -2, "host");
	duk_push_array(m_context);

	int i = 0;
	for (const std::string &channel : whois.channels) {
		duk_push_string(m_context, channel.c_str());
		duk_put_prop_index(m_context, -2, i++);
	}
	duk_put_prop_string(m_context, -2, "channels");
	call("onWhois", 2);
}

} // !irccd

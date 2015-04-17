/*
 * TransportCommand.cpp -- transport command queue'ed to the main loop
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

#include "Irccd.h"
#include "TransportCommand.h"

namespace irccd {

void TransportCommand::cnotice(const std::string &server, const std::string &channel, const std::string &message)
{
	irccd->serverFind(server)->cnotice(channel, message);
}

void TransportCommand::connect()
{
	// TODO
}

void TransportCommand::disconnect(const std::string &name)
{
	irccd->serverDisconnect(name);
}

void TransportCommand::invite(const std::string &server, const std::string &target, const std::string &channel)
{
	irccd->serverFind(server)->invite(target, channel);
}

void TransportCommand::join(const std::string &server, const std::string &channel, const std::string &password)
{
	irccd->serverFind(server)->join(channel, password);
}

void TransportCommand::kick(const std::string &server, const std::string &target, const std::string &channel, const std::string &reason)
{
	irccd->serverFind(server)->kick(target, channel, reason);
}

void TransportCommand::load(const std::string &path, bool isrelative)
{
	// TODO
	(void)isrelative;
	(void)path;
}

void TransportCommand::me(const std::string &server, const std::string &channel, const std::string &message)
{
	irccd->serverFind(server)->me(channel, message);
}

void TransportCommand::message(const std::string &server, const std::string &channel, const std::string &message)
{
	irccd->serverFind(server)->message(channel, message);
}

void TransportCommand::mode(const std::string &server, const std::string &channel, const std::string &mode)
{
	irccd->serverFind(server)->mode(channel, mode);
}

void TransportCommand::nick(const std::string &server, const std::string &nickname)
{
	irccd->serverFind(server)->nick(nickname);
}

void TransportCommand::notice(const std::string &server, const std::string &target, const std::string &message)
{
	irccd->serverFind(server)->notice(target, message);
}

void TransportCommand::part(const std::string &server, const std::string &channel, const std::string &reason)
{
	irccd->serverFind(server)->part(channel, reason);
}

void TransportCommand::reconnect(const std::string &server)
{
	irccd->serverReconnect(server);
}

void TransportCommand::reload(const std::string &plugin)
{
	irccd->pluginReload(plugin);
}

void TransportCommand::topic(const std::string &server, const std::string &channel, const std::string &topic)
{
	irccd->serverFind(server)->topic(channel, topic);
}

void TransportCommand::unload(const std::string &plugin)
{
	irccd->pluginUnload(plugin);
}

void TransportCommand::umode(const std::string &server, const std::string &mode)
{
	irccd->serverFind(server)->umode(mode);
}

} // !irccd

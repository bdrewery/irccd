/*
 * Rule.cpp -- rule for server and channels
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

#include <iostream>
#include <stdexcept>

#include "Logger.h"
#include "Rule.h"

namespace irccd {

Rule::Rule(RuleMap servers, RuleMap channels, RuleMap nicknames, RuleMap plugins, RuleMap events, RuleAction action)
	: m_servers{std::move(servers)}
	, m_channels{std::move(channels)}
	, m_nicknames{std::move(nicknames)}
	, m_plugins{std::move(plugins)}
	, m_events{std::move(events)}
	, m_action{action}
{
}

bool Rule::match(const RuleMap &map, const std::string &value) const noexcept
{
	if (value.size() == 0 || map.size() == 0)
		return true;

	return map.count(value) == 1;
}

bool Rule::match(const std::string &server,
		 const std::string &channel,
		 const std::string &nick,
		 const std::string &plugin,
		 const std::string &event) const noexcept
{
#if !defined(NDEBUG)
	auto smatch =  match(m_servers, server);
	auto cmatch = match(m_channels, channel);
	auto nmatch = match(m_nicknames, nick);
	auto pmatch = match(m_plugins, plugin);
	auto ematch = match(m_events, event);

	Logger::debug("  rule candidate:");
	std::cout << "    - servers: ";
	for (const auto &s : m_servers)
		std::cout << s << " ";
	std::cout << std::endl;
	std::cout << "    - channels: ";
	for (const auto &s : m_channels)
		std::cout << s << " ";
	std::cout << std::endl;
	std::cout << "    - nicknames: ";
	for (const auto &s : m_nicknames)
		std::cout << s << " ";
	std::cout << std::endl;
	std::cout << "    - plugins: ";
	for (const auto &s : m_plugins)
		std::cout << s << " ";
	std::cout << std::endl;
	std::cout << "    - events: ";
	for (const auto &s : m_events)
		std::cout << s << " ";
	std::cout << std::endl;

	std::cout << std::boolalpha;
	std::cout << "    result: smatch:" << smatch << " "
		  << "cmatch:" << cmatch << " "
		  << "nmatch:" << nmatch << " "
		  << "pmatch:" << pmatch << " "
		  << "ematch:" << ematch << std::endl;

	if (smatch && cmatch && nmatch && pmatch && ematch) {
		std::cout << "    rule candidate match" << std::endl;
	} else {
		std::cout << "    rule candidate ignored" << std::endl;
	}

	return smatch && cmatch && nmatch && pmatch && ematch;
#else
	return match(m_servers, server) &&
	       match(m_channels, channel) &&
	       match(m_nicknames, nick) &&
	       match(m_plugins, plugin) &&
	       match(m_events, event);
#endif
}

RuleAction Rule::action() const noexcept
{
	return m_action;
}

const RuleMap &Rule::servers() const noexcept
{
	return m_servers;
}

const RuleMap &Rule::channels() const noexcept
{
	return m_channels;
}

const RuleMap &Rule::nicknames() const noexcept
{
	return m_nicknames;
}

const RuleMap &Rule::plugins() const noexcept
{
	return m_plugins;
}

const RuleMap &Rule::events() const noexcept
{
	return m_events;
}

} // !irccd

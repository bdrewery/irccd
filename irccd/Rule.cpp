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

#include "Rule.h"

namespace irccd {

/* ---------------------------------------------------------
 * RuleMatch
 * --------------------------------------------------------- */

void RuleMatch::addServer(const std::string &server, bool enabled)
{
	m_servers[server] = enabled;
}

void RuleMatch::addChannel(const std::string &channel, bool enabled)
{
	m_channels[channel] = enabled;
}

void RuleMatch::addPlugin(const std::string &plugin, bool enabled)
{
	m_plugins[plugin] = enabled;
}

bool RuleMatch::match(const std::string &server,
		      const std::string &channel,
		      const std::string &plugin) const
{
	if (m_servers.size() == 0 && m_channels.size() == 0 && m_plugins.size() == 0)
		return true;

	if (m_servers.size() > 0 && (m_servers.count(server) == 0 || !m_servers.at(server)))
		return false;
	if (m_channels.size() > 0 && (m_channels.count(channel) == 0 || !m_channels.at(channel)))
		return false;
	if (m_plugins.size() > 0 && (m_plugins.count(plugin) == 0 || !m_plugins.at(plugin)))
		return false;

	return true;
}

std::ostream &operator<<(std::ostream &out, const RuleMatch &match)
{
	out << "RuleMatch:" << std::endl;

	out << "  servers: ";
	for (const auto &s : match.m_servers)
		out << (s.second ? "" : "!") << s.first;
	out << std::endl;

	out << "  channels: ";
	for (const auto &c : match.m_channels)
		out << (c.second ? "" : "!") << c.first;
	out << std::endl;

	out << "  plugins: ";
	for (const auto &p : match.m_plugins)
		out << (p.second ? "" : "!") << p.first;

	return out;
}

/* ---------------------------------------------------------
 * RuleProperties
 * --------------------------------------------------------- */

bool RuleProperties::get(const RuleMap &map, const std::string &name, bool current) const
{
	if (map.size() == 0)
		return true;

	if (map.count(name) == 0)
		return current;

	return map.at(name);
}

void RuleProperties::setPlugin(const std::string &name, bool mode)
{
	m_setPlugins[name] = mode;
}

void RuleProperties::setEvent(const std::string &name, bool mode)
{
	m_setEvents[name] = mode;
}

bool RuleProperties::isPluginEnabled(const std::string &name, bool current) const
{
	return get(m_setPlugins, name, current);
}

bool RuleProperties::isEventEnabled(const std::string &name, bool current) const
{
	return get(m_setEvents, name, current);
}

void RuleProperties::setRecode(const std::string &from, const std::string &to)
{
	m_recode = std::make_pair(from, to);
}

const RuleRecode &RuleProperties::recode() const
{
	return m_recode;
}

std::ostream &operator<<(std::ostream &out, const RuleProperties &properties)
{
	out << "RuleProperties:" << std::endl;

	out << "  set-plugins: ";
	for (const auto &sp : properties.m_setPlugins)
		out << (sp.second ? "" : "!") << sp.first;
	out << std::endl;

	out << "  set-events: ";
	for (const auto &se : properties.m_setEvents)
		out << (se.second ? "" : "!") << se.first;
	out << std::endl;

	out << "  recode: ";
	if (properties.m_recode.first.size() == 0 && properties.m_recode.second.size() == 0)
		out << "empty";

	return out;
}

/* ---------------------------------------------------------
 * Rule
 * --------------------------------------------------------- */

Rule::Rule(const RuleMatch &match, const RuleProperties &properties)
	: m_match(match)
	, m_properties(properties)
{
}

const RuleMatch &Rule::match() const
{
	return m_match;
}

const RuleProperties &Rule::properties() const
{
	return m_properties;
}

std::ostream &operator<<(std::ostream &out, const Rule &rule)
{
	out << "Rule:" << std::endl;
	out << rule.m_match << std::endl;
	out << rule.m_properties;

	return out;
}

} // !irccd

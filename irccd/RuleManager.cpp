/*
 * RuleManager.cpp -- owner of rules and solver
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

#include <stdexcept>

#include "RuleManager.h"
#include "Logger.h"

namespace irccd {

const std::unordered_set<std::string> RuleValidEvents {
	"onMe",
	"onCommand",
	"onConnect",
	"onChannelNotice",
	"onInvite",
	"onJoin",
	"onKick",
	"onLoad",
	"onMessage",
	"onMode",
	"onNames",
	"onNick",
	"onNotice",
	"onPart",
	"onQuery",
	"onQueryCommand",
	"onReload",
	"onTopic",
	"onUnload",
	"onUserMode",
	"onWhois"
};

RuleManager RuleManager::s_instance;

RuleManager &RuleManager::instance()
{
	return s_instance;
}

void RuleManager::assertIndex(int index) const
{
	if (index < 0 || static_cast<size_t>(index) >= m_rules.size())
		throw std::out_of_range(std::to_string(index) + " is out of range");
}

int RuleManager::add(const Rule &rule, int index)
{
	Lock lk(m_lock);

	if (index < 0) {
		m_rules.push_back(rule);
		return m_rules.size() - 1;
	}

	if ((size_t)index >= m_rules.size())
		throw std::out_of_range(std::to_string(index) + " is out of range");

	m_rules.insert(m_rules.begin() + index, rule);

	return index;
}

Rule RuleManager::get(int index) const
{
	Lock lk(m_lock);

	assertIndex(index);

	return m_rules[index];
}

void RuleManager::remove(int index)
{
	Lock lk(m_lock);

	assertIndex(index);

	m_rules.erase(m_rules.begin() + index);
}

unsigned RuleManager::count() const
{
	Lock lk(m_lock);

	return static_cast<unsigned>(m_rules.size());
}

void RuleManager::enable(int index)
{
	Lock lk(m_lock);

	assertIndex(index);

	m_rules[index].enable();
}

void RuleManager::disable(int index)
{
	Lock lk(m_lock);

	assertIndex(index);

	m_rules[index].disable();
}

RuleResult RuleManager::solve(const std::string &server,
			      const std::string &channel,
			      const std::string &event,
			      const std::string &plugin) const
{
	Lock lk(m_lock);

	RuleResult result;

	if (RuleValidEvents.count(event) == 0)
		throw std::invalid_argument(event + " is not a valid event");

	try {
		for (const auto &r : m_rules) {
			// Skip disabled rules
			if (!r.isEnabled())
				continue;

			const auto &match	= r.match();
			const auto &properties	= r.properties();

			/*
			 * 1. Check first if the match is applied.
			 */
			if (!match.match(server, channel, plugin))
				continue;

			result.enabled = properties.isPluginEnabled(plugin, result.enabled)
			    && properties.isEventEnabled(event, result.enabled);
		}
	} catch (const std::invalid_argument &error) {
		Logger::warn("rule: %s", error.what());
		result.enabled = false;
	}

	return result;
}

void RuleManager::clear()
{
	Lock lk(m_lock);

	m_rules.clear();
}

} // !irccd

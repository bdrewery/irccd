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

void RuleManager::add(const Rule &rule)
{
	m_rules.push_back(rule);
}

RuleResult RuleManager::solve(const std::string &server,
			      const std::string &channel,
			      const std::string &event,
			      const std::string &plugin) const
{
	RuleResult result;

	if (RuleValidEvents.count(event) == 0)
		throw std::invalid_argument(event + " is not a valid event");

	printf("==== Checking for %s, %s, %s, %s\n", server.c_str(), channel.c_str(), event.c_str(), plugin.c_str());

	try {
		int i = 0;
		printf("size: %d\n", m_rules.size());
		for (const auto &r : m_rules) {
			const auto &match	= r.match();
			const auto &properties	= r.properties();

			printf("rule: #%d\n", ++i);
			/*
			 * 1. Check first if the match is applied.
			 */
			if (!match.match(server, channel, plugin))
				continue;

			printf("before: %d\n", result.enabled);
			result.enabled = properties.isPluginEnabled(plugin, result.enabled)
			    && properties.isEventEnabled(event, result.enabled);
			printf("after: %d\n", result.enabled);
		}
	} catch (const std::invalid_argument &error) {
		Logger::warn("rules: %s", error.what());
		result.enabled = false;
	}

	return result;
}

void RuleManager::clear()
{
	m_rules.clear();
}

} // !irccd

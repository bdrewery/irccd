/*
 * Rule.h -- rule for server and channels
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

#ifndef _RULE_H_
#define _RULE_H_

/**
 * @file Rule.h
 * @brief Rule description
 */

#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>

namespace irccd {

using RuleMap = std::unordered_set<std::string>;

enum class RuleAction {
	Accept,
	Drop
};

class Rule final {
private:
	RuleMap m_servers;
	RuleMap m_channels;
	RuleMap m_nicknames;
	RuleMap m_plugins;
	RuleMap m_events;
	RuleAction m_action{RuleAction::Accept};

	/*
	 * Check if a map contains the value and return true if it is
	 * or return true if value is empty (which means applicable)
	 */
	bool match(const RuleMap &map, const std::string &value) const noexcept;

public:
	Rule(RuleMap servers = RuleMap{},
	     RuleMap channels = RuleMap{},
	     RuleMap nicknames = RuleMap{},
	     RuleMap plugins = RuleMap{},
	     RuleMap events = RuleMap{},
	     RuleAction = RuleAction::Accept);

	/**
	 * Check if that rule apply for the given criterias.
	 *
	 * @param server
	 * @param channel
	 * @param nick
	 * @param plugin
	 * @param event
	 * @return
	 */
	bool match(const std::string &server,
		   const std::string &channel,
		   const std::string &nick,
		   const std::string &plugin,
		   const std::string &event) const noexcept;

	RuleAction action() const noexcept;
	const RuleMap &servers() const noexcept;
	const RuleMap &channels() const noexcept;
	const RuleMap &nicknames() const noexcept;
	const RuleMap &plugins() const noexcept;
	const RuleMap &events() const noexcept;
};

} // !irccd

#endif // !_RULE_H_

/*
 * Rule.h -- rule for server and channels
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

/**
 * List of criterias.
 */
using RuleMap = std::unordered_set<std::string>;

/**
 * @enum RuleAction
 * @brief Rule action
 */
enum class RuleAction {
	Accept,			//!< The event is accepted (default)
	Drop			//!< The event is dropped
};

/**
 * @class Rule
 * @brief Manage rule to activate or deactive events.
 */
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
	/**
	 * Rule constructor.
	 *
	 * @param servers the server list
	 * @param channels the channels
	 * @param nicknames the nicknames
	 * @param plugins the plugins
	 * @param events the events
	 * @param action the rule action
	 */
	Rule(RuleMap servers = RuleMap{},
	     RuleMap channels = RuleMap{},
	     RuleMap nicknames = RuleMap{},
	     RuleMap plugins = RuleMap{},
	     RuleMap events = RuleMap{},
	     RuleAction action = RuleAction::Accept);

	/**
	 * Check if that rule apply for the given criterias.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param nick the origin
	 * @param plugin the plugin
	 * @param event the event
	 * @return true if match
	 */
	bool match(const std::string &server,
		   const std::string &channel,
		   const std::string &nick,
		   const std::string &plugin,
		   const std::string &event) const noexcept;

	/**
	 * Get the action.
	 *
	 * @return the action
	 */
	RuleAction action() const noexcept;

	/**
	 * Get the servers.
	 *
	 * @return the servers
	 */
	const RuleMap &servers() const noexcept;

	/**
	 * Get the channels.
	 *
	 * @return the channels
	 */
	const RuleMap &channels() const noexcept;

	/**
	 * Get the nicknames.
	 *
	 * @return the nicknames
	 */
	const RuleMap &nicknames() const noexcept;

	/**
	 * Get the plugins.
	 *
	 * @return the plugins
	 */
	const RuleMap &plugins() const noexcept;

	/**
	 * Get the events.
	 *
	 * @return the events
	 */
	const RuleMap &events() const noexcept;
};

} // !irccd

#endif // !_RULE_H_

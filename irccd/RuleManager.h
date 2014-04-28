/*
 * RuleManager.h -- owner of rules and solver
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

#ifndef _RULE_MANAGER_H_
#define _RULE_MANAGER_H_

/**
 * @file RuleManager.h
 * @brief Owner of rules and solver
 */

#include <vector>
#include <unordered_set>
#include <mutex>

#include "Rule.h"

namespace irccd {

/**
 * List of all valid events.
 */
extern const std::unordered_set<std::string> RuleValidEvents;

/**
 * @struct RuleResult
 * @brief The result
 */
struct RuleResult {
	bool		enabled = true;		//!< true if the event must be called
	RuleRecode	recode;			//!< the recode information
};

/**
 * @class RuleManager
 * @brief Owner of rules and solver
 *
 * All functions are thread safe.
 */
class RuleManager {
private:
	using Lock		= std::lock_guard<std::mutex>;

	static RuleManager	s_instance;

	std::vector<Rule>	m_rules;
	mutable std::mutex	m_lock;

	RuleManager() = default;
	RuleManager(const RuleManager &) = delete;
	RuleManager &operator=(const RuleManager &) = delete;

public:
	/**
	 * Get the rule manager instance.
	 *
	 * @return the manager
	 */
	static RuleManager &instance();

	/**
	 * Append a rule to the end.
	 *
	 * @param rule the rule to append
	 */
	void add(const Rule &rule);

	/**
	 * Check the result of a plugin and event. We first make
	 * the assumption that everything is valid and iterate over
	 * all the rules to check if the rule disable the previous
	 * one.
	 *
	 * @param server the server name
	 * @param channel the channel
	 * @param event the event name
	 * @param plugin the plugin name
	 * @return the rule result
	 */
	RuleResult solve(const std::string &server,
			 const std::string &channel,
			 const std::string &event,
			 const std::string &plugin) const;

	/**
	 * Remove all rules.
	 */
	void clear();
};

} // !irccd

#endif // !_RULE_MANAGER_H_

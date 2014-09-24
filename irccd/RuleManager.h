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

#include <Singleton.h>

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
	std::string	encoding;		//!< the recode information
};

/**
 * @class RuleManager
 * @brief Owner of rules and solver
 *
 * All functions are thread safe.
 */
class RuleManager final : public Singleton<RuleManager> {
private:
	SINGLETON(RuleManager);

	using Lock		= std::lock_guard<std::mutex>;

	std::vector<Rule>	m_rules;
	mutable std::mutex	m_lock;

	RuleManager() = default;
	RuleManager(const RuleManager &) = delete;
	RuleManager(RuleManager &&) = delete;
	RuleManager &operator=(const RuleManager &) = delete;
	RuleManager &operator=(RuleManager &&) = delete;

	void assertIndex(int index) const;

public:
	/**
	 * Append a rule to the end.
	 *
	 * @param rule the rule to append
	 * @param index the index where to insert
	 * @return the inserted index
	 * @throw std::out_of_range if index is out of bounds
	 */
	int add(const Rule &rule, int index = -1);

	/**
	 * Get a copy of a rule.
	 *
	 * @param index the index
	 * @return the rule
	 * @throw std::out_of_range if index is out of bounds
	 */
	Rule get(int index) const;

	/**
	 * Remove an existing rule.
	 *
	 * @param index the index
	 * @throw std::out_of_range if index is out of bounds
	 */
	void remove(int index);

	/**
	 * Get the number of rules in the manager.
	 *
	 * @return the count.
	 */
	unsigned count() const;

	/**
	 * Enable a rule.
	 *
	 * @param index the index
	 * @throw std::out_of_range if index is out of bounds
	 */
	void enable(int index);

	/**
	 * Disable a rule.
	 *
	 * @param index the index
	 * @throw std::out_of_range if index is out of bounds
	 */
	void disable(int index);

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

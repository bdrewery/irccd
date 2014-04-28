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
#include <unordered_map>
#include <utility>

namespace irccd {

/**
 * Map of param to bools. User may explicitly disable an object (e.g server,
 * plugin and such).
 */
using RuleMap		= std::unordered_map<std::string, bool>;

/**
 * The recode information.
 */
using RuleRecode	= std::pair<std::string, std::string>;

/**
 * @class RuleMatch
 * @brief Rule matching arguments
 *
 * Rule matching class. This describe the matchings arguments by servers,
 * channels and plugins.
 *
 * All are optionals.
 */
class RuleMatch {
private:
	RuleMap		m_servers;		//!< list of server to apply
	RuleMap		m_channels;		//!< list of channels to apply
	RuleMap		m_plugins;		//!< list of plugins

public:
	/**
	 * Add a server.
	 *
	 * @param server the server
	 * @param enabled true to enable
	 */
	void addServer(const std::string &server, bool enabled = true);

	/**
	 * Add a channel.
	 *
	 * @param channel the channel
	 * @param enabled true to enable
	 */
	void addChannel(const std::string &channel, bool enabled = true);

	/**
	 * Add a plugin.
	 *
	 * @param plugin the plugin
	 * @param enabled true to enable
	 */
	void addPlugin(const std::string &plugin, bool enabled = true);

	/**
	 * Check if the parameters match the rule.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param plugin the plugin name
	 */
	bool match(const std::string &server,
		   const std::string &channel,
		   const std::string &plugin) const;

	/**
	 * Convert to string.
	 *
	 * @param out the out stream
	 * @param match the rule match
	 * @return out
	 */
	friend std::ostream &operator<<(std::ostream &out, const RuleMatch &match);
};

/**
 * @class RuleProperties
 * @brief Settings for a matched rule
 *
 * Some settings to apply on the matched rule.
 */
class RuleProperties {
private:
	RuleMap		m_setPlugins;		//!< plugin to enable / disable
	RuleMap		m_setEvents;		//!< event to enable / disable
	RuleRecode	m_recode;		//!< reencoding settings

	bool get(const RuleMap &map, const std::string &name, bool current) const;

public:
	/**
	 * Enable or disable a plugin for the rule.
	 *
	 * @param plugin the plugin
	 * @param mode true to (re)enable
	 */
	void setPlugin(const std::string &plugin, bool mode = true);

	/**
	 * Enable or disable an event for the rule.
	 *
	 * @param event the event
	 * @param mode true to (re)enable
	 */
	void setEvent(const std::string &event, bool mode = true);

	/**
	 * Check if the plugin is enabled. If it is not set, current is
	 * returned.
	 *
	 * @param plugin the plugin
	 * @param current the current rule match
	 * @return true, false or current
	 */
	bool isPluginEnabled(const std::string &plugin, bool current) const;

	/**
	 * Check if the event is enabled. If it is not set, current is
	 * returned.
	 *
	 * @param event the event
	 * @param current the current rule match
	 * @return true, false or current
	 */
	bool isEventEnabled(const std::string &event, bool current) const;
	
	/**
	 * Set the recode.
	 *
	 * @param from the incoming encoding
	 * @param to the desired encoding for plugins
	 */
	void setRecode(const std::string &from, const std::string &to = "");

	/**
	 * Get the recode info. Empty strings means default.
	 *
	 * @return the recode
	 */
	const RuleRecode &recode() const;

	/**
	 * Convert to string.
	 *
	 * @param out the out stream
	 * @param properties the properties
	 * @return out
	 */
	friend std::ostream &operator<<(std::ostream &out, const RuleProperties &properties);
};

/**
 * @class Rule
 * @brief Rule description
 *
 * Owns a rule match description and its properties.
 */
class Rule {
private:
	bool		m_enabled = true;
	RuleMatch	m_match;
	RuleProperties	m_properties;

public:
	/**
	 * Constructor.
	 *
	 * @param match the rule match
	 * @param properties the properties
	 */
	Rule(const RuleMatch &match, const RuleProperties &properties, bool enabled = true);

	/**
	 * Enable the rule.
	 */
	void enable();

	/**
	 * Disable the rule
	 */
	void disable();

	/**
	 * Tell if the rule is enabled.
	 *
	 * @return true if enabled.
	 */
	bool isEnabled() const;

	/**
	 * Get the rule match.
	 *
	 * @return the rule match
	 */
	const RuleMatch &match() const;

	/**
	 * Get the rule properties.
	 *
	 * @return the rule properties
	 */
	const RuleProperties &properties() const;

	/**
	 * Convert to string.
	 *
	 * @param out the out stream
	 * @param rule the rule
	 * @return out
	 */
	friend std::ostream &operator<<(std::ostream &out, const Rule &rule);
};

} // !irccd

#endif // !_RULE_H_

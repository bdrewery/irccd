/*
 * DefCall.h -- deferred plugin function call
 *
 * Copyright (c) 2013 David Demelier <markand@malikania.fr>
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

#ifndef _DEF_CALL_H_
#define _DEF_CALL_H_

#include "Server.h"
#include "Plugin.h"

namespace irccd {

/**
 * @class DefCall
 * @brief Deferred call class.
 *
 * This class is used to call deferred functions.
 */
class DefCall {
private:
	IrcEventType m_type;		//! which type of event
	Plugin::Ptr m_plugin;		//! for which plugin
	int m_ref;			//! function reference

	/**
	 * Call the function that is pushed on the stack, also remove it from
	 * the registry.
	 *
	 * @param nparams the number of parameters passed
	 * @throw Plugin::ErrorException on error
	 */
	void call(int nparams = 0);

public:
	/**
	 * Default constructor.
	 */
	DefCall() = default;

	/**
	 */
	DefCall(IrcEventType type, Plugin::Ptr plugin, int ref);

	/**
	 * Get the deferred IRC event type.
	 *
	 * @return the type
	 */
	IrcEventType type() const;

	/**
	 * Names listing.
	 *
	 * @param users the user list
	 */
	void onNames(const std::vector<std::string> &users);

	/**
	 * Whois information.
	 *
	 * @param params the parameters
	 */
	void onWhois(const std::vector<std::string> &params);

	/**
	 * Test the DeferredCall equality.
	 *
	 * @param c1 the object to test
	 * @return true on equality
	 */
	bool operator==(const DefCall &c1);
};

} // !irccd

#endif // !_DEF_CALL_H_

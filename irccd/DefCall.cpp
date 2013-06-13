/*
 * DefCall.cpp -- deferred plugin function call
 *
 * Copyright (c) 2011, 2012, 2013 David Demelier <markand@malikania.fr>
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

#include "DefCall.h"

using namespace irccd;
using namespace std;

DefCall::DefCall()
{
}

DefCall::DefCall(IrcEventType type, shared_ptr<Plugin> plugin, int ref)
	: m_type(type)
	, m_plugin(plugin)
	, m_ref(ref)
{
}

IrcEventType DefCall::type() const
{
	return m_type;
}

void DefCall::onNames(const vector<string> users)
{
	m_plugin->getState().rawget(LUA_REGISTRYINDEX, m_ref);
	m_plugin->getState().createtable(users.size(), users.size());

	for (size_t i = 0; i < users.size(); ++i) {
		m_plugin->getState().push(users[i]);
		m_plugin->getState().rawset(-2, i + 1);
	}
	
	bool result = m_plugin->getState().pcall(1, 0, 0);
	m_plugin->getState().unref(LUA_REGISTRYINDEX, m_ref);
	
	if (!result)
		throw Plugin::ErrorException(m_plugin->getState().getError());
}

bool DefCall::operator==(const DefCall &c1)
{
	return m_type == c1.m_type &&
	    m_plugin == c1.m_plugin &&
	    m_ref == c1.m_ref;
}
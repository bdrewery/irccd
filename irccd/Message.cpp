/*
 * Message.cpp -- message handler for clients
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

#include "Message.h"

Message::Message(const Message &m)
{
	m_data.str(m.m_data.str());
}

bool Message::isFinished(const std::string &data, std::string &ret)
{
	std::size_t pos;
	std::string tmp;

	m_data << data;
	tmp = m_data.str();

	if ((pos = tmp.find_first_of("\n")) == std::string::npos)
		return false;

	// Remove the '\n'	
	tmp.erase(pos);
	ret = tmp;

	return true;
}

Message &Message::operator=(const Message &m)
{
	m_data.str(m.m_data.str());

	return *this;
}

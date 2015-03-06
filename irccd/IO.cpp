/*
 * IO.cpp -- general outgoint / incoming message
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

#include <IrccdConfig.h>

#include "IO.h"
#include "Logger.h"

#if defined(WITH_LIBICONV)
#  include "Converter.h"
#endif

namespace irccd {

IO::IO(std::string serverName, std::string targetName)
	: m_serverName(std::move(serverName))
	, m_targetName(std::move(targetName))
{
}

std::string IO::tryEncodeFull(const std::string &from, const std::string &to, const std::string &input) const
{
#if defined(WITH_LIBICONV)
	try {
		return Converter::convert(from.c_str(), to.c_str(), input);
	} catch (const std::exception &error) {
		Logger::warn("rule: encoding failure: %s", error.what());
	}

	// Return the input string as a callback
	return input;
#else
	(void)from;
	(void)to;

	return input;
#endif
}

const std::string &IO::server() const
{
	return m_serverName;
}

const std::string &IO::target() const
{
	return m_targetName;
}

void IO::encode(const std::string &encoding)
{
	m_mustEncode = true;
	m_encoding = encoding;
}

bool IO::empty() const
{
	return m_serverName.size() == 0 && m_targetName.size() == 0;
}

} // !irccd

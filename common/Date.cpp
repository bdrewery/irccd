/*
 * Date.cpp -- date and time manipulation
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

#include "Date.h"

namespace irccd {

Date::Date()
{
	m_timestamp = time(NULL);
}

Date::Date(time_t timestamp)
{
	m_timestamp = timestamp;
}

time_t Date::getTimestamp() const
{
	return m_timestamp;
}

std::string Date::format(const std::string &format) const
{
	char buffer[512];
	struct tm *tm;

	tm = localtime(&m_timestamp);
	strftime(buffer, sizeof (buffer), format.c_str(), tm);

	return std::string(buffer);
}

bool operator==(const Date &d1, const Date &d2)
{
	return d1.getTimestamp() == d2.getTimestamp();
}

bool operator<=(const Date &d1, const Date &d2)
{
	return d1.getTimestamp() <= d2.getTimestamp();
}

} // !irccd

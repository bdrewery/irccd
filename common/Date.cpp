/*
 * Date.cpp -- date and time manipulation
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

#include "Date.h"

using namespace irccd;
using namespace std;


Date::Date(void)
{
	m_timestamp = time(NULL);
	m_tm = *localtime(&m_timestamp);
}

Date::Date(time_t timestamp)
{
	m_timestamp = time(&timestamp);
	m_tm = *localtime(&m_timestamp);
}

Date::~Date(void)
{
}

string Date::format(const string &format)
{
	char buffer[512];

	strftime(buffer, sizeof (buffer), format.c_str(), &m_tm);

	return string(buffer);
}

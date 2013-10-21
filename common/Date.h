/*
 * Date.h -- date and time manipulation
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

#ifndef _DATE_H_
#define _DATE_H_

#include <cstdint>
#include <ctime>
#include <string>

namespace irccd
{

struct Date
{
	time_t m_timestamp;		//! time epoch

	Date();
	Date(time_t timestamp);
	~Date();

	/**
	 * Get the timestamp.
	 *
	 * @return the timestamp
	 */
	time_t getTimestamp() const;

	/**
	 * Format the current that in the specified format,
	 * see strftime(3) for patterns.
	 *
	 * @param format the format
	 * @return the date formated
	 */
	std::string format(const std::string &format);
};

bool operator==(const Date &, const Date &);

bool operator<=(const Date &, const Date &);

} // !irccd

#endif // !_DATE_H_

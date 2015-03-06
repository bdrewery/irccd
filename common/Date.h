/*
 * Date.h -- date and time manipulation
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

#ifndef _IRCCD_DATE_H_
#define _IRCCD_DATE_H_

/**
 * @file Date.h
 * @brief Date management
 */

#include <cstdint>
#include <ctime>
#include <string>

namespace irccd {

/**
 * @class Date
 * @brief Date management
 */
class Date {
private:
	time_t m_timestamp;		//! time epoch

public:
	/**
	 * Default constructor to now.
	 */
	Date();

	/**
	 * Constructor with specific timestamp.
	 *
	 * @param timestamp the timestamp
	 */
	Date(time_t timestamp);

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

/**
 * Test equality.
 *
 * @param d1 the first date
 * @param d2 the second date
 * @return true if equals
 */
bool operator==(const Date &d1, const Date &d2);

/**
 * Less or equal operator.
 *
 * @param d1 the first date
 * @param d2 the second date
 * @return true if d1 is less or equals than d2
 */
bool operator<=(const Date &d1, const Date &d2);

} // !irccd

#endif // !_IRCCD_DATE_H_

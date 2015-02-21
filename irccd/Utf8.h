/*
 * Utf8.h -- UTF-8 to UCS-4 conversions
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

#ifndef _UTF8_H_
#define _UTF8_H_

/**
 * @file Utf8.h
 * @brief UTF-8 to UCS-4 conversions
 */

#include <cstdint>
#include <stdexcept>
#include <string>

namespace irccd {

/**
 * @class Utf8
 * @brief Conversion between UTF-8 and UCS-4
 */
class Utf8 {
private:
	static void encode(uint32_t point, char res[5]);
	static void decode(uint32_t &c, const char *res);

public:
	/**
	 * Get the number of bytes for the first multi byte character from a
	 * utf-8 string.
	 *
	 * @param c the first multi byte character
	 * @return the number of bytes [1-4] or -1 on invalid
	 */
	static int8_t nbytesUtf8(uint8_t c);

	/**
	 * Get the number of bytes for the unicode point.
	 *
	 * @param point the unicode point
	 * @return the number of bytes [1-4] or -1 on invalid
	 */
	static int8_t nbytesPoint(uint32_t point);

	/**
	 * Get real number of character in a string.
	 *
	 * @param str the string
	 * @return the length
	 * @throw std::invalid_argument on invalid sequence
	 */
	static size_t length(const std::string &str);

	/**
	 * Convert a UCS-4 string to UTF-8 string.
	 *
	 * @param array the UCS-4 string
	 * @return the UTF-8 string
	 * @throw std::invalid_argument on invalid sequence
	 */
	static std::string toutf8(const std::u32string &array);

	/**
	 * Convert a UTF-8 string to UCS-4 string.
	 *
	 * @param str the UTF-8 string
	 * @return the UCS-4 string
	 * @throw std::invalid_argument on invalid sequence
	 */
	static std::u32string toucs(const std::string &str);

	/**
	 * Check if the unicode character is space.
	 *
	 * @param c the character
	 * @return true if space
	 */
	static bool isspace(uint32_t c);

	/**
	 * Check if the unicode character is digit.
	 *
	 * @param c the character
	 * @return true if digit
	 */
	static bool isdigit(uint32_t c);

	/**
	 * Check if the unicode character is letter.
	 *
	 * @param c the character
	 * @return true if letter
	 */
	static bool isletter(uint32_t c);

	/**
	 * Check if the unicode character is upper case.
	 *
	 * @param c the character
	 * @return true if upper case
	 */
	static bool isupper(uint32_t c);

	/**
	 * Check if the unicode character is lower case.
	 *
	 * @param c the character
	 * @return true if lower case
	 */
	static bool islower(uint32_t c);

	/**
	 * Check if the unicode character is title case.
	 *
	 * @param c the character
	 * @return true if title case
	 */
	static bool istitle(uint32_t c);

	/**
	 * Convert to upper case.
	 *
	 * @param c the character
	 * @return the upper case character
	 */
	static uint32_t toupper(uint32_t c);

	/**
	 * Convert to lower case.
	 *
	 * @param c the character
	 * @return the lower case character
	 */
	static uint32_t tolower(uint32_t c);

	/**
	 * Convert to title case.
	 *
	 * @param c the character
	 * @return the title case character
	 */
	static uint32_t totitle(uint32_t c);

	/**
	 * Convert the UTF-8 string to upper case.
	 *
	 * @param str the str
	 * @return the upper case string
	 */
	static inline std::string toupper(const std::string &str)
	{
		return toutf8(toupper(toucs(str)));
	}

	/**
	 * Convert the UCS-4 string to upper case.
	 *
	 * @param str the str
	 * @return the upper case string
	 */
	static inline std::u32string toupper(const std::u32string &str)
	{
		auto copy = str;

		for (size_t i = 0; i < str.size(); ++i)
			copy[i] = toupper(str[i]);

		return copy;
	}

	/**
	 * Convert the UTF-8 string to lower case.
	 *
	 * @param str the str
	 * @return the lower case string
	 */
	static inline std::string tolower(const std::string &str)
	{
		return toutf8(tolower(toucs(str)));
	}

	/**
	 * Convert the UCS-4 string to lower case.
	 *
	 * @param str the str
	 * @return the lower case string
	 */
	static inline std::u32string tolower(const std::u32string &str)
	{
		auto copy = str;

		for (size_t i = 0; i < str.size(); ++i)
			copy[i] = tolower(str[i]);

		return copy;
	}
};

} // !irccd

#endif // !_UTF8_H_

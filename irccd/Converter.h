/*
 * Converter.h -- iconv based converter
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

#ifndef _CONVERTER_H_
#define _CONVERTER_H_

/**
 * @file Converter.h
 * @brief Converter using libiconv
 */

#include <string>

namespace irccd {

/**
 * @class Converter
 * @brief Convert string between different encodings
 */
class Converter {
public:
	/**
	 * Convert the string into a different encoding.
	 *
	 * @param from the from encoding
	 * @param to the destination encoding
	 * @param input the string to convert
	 * @return the converted string
	 * @throw std::invalid_argument on invalid sequence
	 */
	static std::string convert(const char *from,
				   const char *to,
				   const std::string &input);
};

} // !irccd

#endif // !_CONVERTER_H_

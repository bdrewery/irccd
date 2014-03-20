/*
 * Utf8.cpp -- UTF-8 to UCS-4 conversions
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

#include "Utf8.h"

namespace irccd {

void Utf8::encode(uint32_t c, char res[5])
{
	switch (nbytesPoint(c)) {
	case 1:
		res[0] = c;
		res[1] = '\0';
		break;
	case 2:
		res[0] = 0xC0 | ((c >> 6)  & 0x1F);
		res[1] = 0x80 | (c & 0x3F);
		res[2] = '\0';
		break;
	case 3:
		res[0] = 0xE0 | ((c >> 12) & 0xF );
		res[1] = 0x80 | ((c >> 6)  & 0x3F);
		res[2] = 0x80 | (c & 0x3F);
		res[3] = '\0';
		break;
	case 4:
		res[0] = 0xF0 | ((c >> 18) & 0x7 );
		res[1] = 0x80 | ((c >> 12) & 0x3F);
		res[2] = 0x80 | ((c >> 6)  & 0x3F);
		res[3] = 0x80 | (c & 0x3F);
		res[4] = '\0';
		break;
	default:
		break;
	}
}

void Utf8::decode(uint32_t &c, const char *res)
{
	switch (nbytesUtf8(res[0])) {
	case 1:
		c = res[0];
		break;
	case 2:
		c = res[0] & 0x1F;
		c = (c << 6) | (res[1] & 0x3F);
		break;
	case 3:
		c = res[0] & 0x1F;
		c = (c << 6) | (res[1] & 0x3F);
		c = (c << 6) | (res[2] & 0x3F);
		break;
	case 4:
		c = res[0] & 0x1F;
		c = (c << 6) | (res[1] & 0x3F);
		c = (c << 6) | (res[2] & 0x3F);
		c = (c << 6) | (res[3] & 0x3F);
		break;
	default:
		break;
	}
}

int8_t Utf8::nbytesUtf8(uint8_t c)
{
	if (c <= 0x7F)
		return 1;
	if ((c & 0xE0) == 0xC0)
		return 2;
	if ((c & 0xF0) == 0xE0)
		return 3;
	if ((c & 0xF8) == 0xF0)
		return 4;

	return -1;
}

int8_t Utf8::nbytesPoint(uint32_t c)
{
	if (c <= 0x7F)
		return 1;
	if (c <= 0x7FF)
		return 2;
	if (c <= 0xFFFF)
		return 3;
	if (c <= 0x1FFFFF)
		return 4;

	return -1;
}

size_t Utf8::length(const std::string &str)
{
	size_t total = 0;

	for (size_t i = 0; i < str.size(); ) {
		auto size = nbytesUtf8(str[i]);

		if (size < 0)
			throw std::invalid_argument("invalid sequence");

		total ++;
		i += size;
	}

	return total;
}

std::string Utf8::toutf8(const std::u32string &array)
{
	std::string res;

	for (size_t i = 0; i < array.size(); ++i) {
		char tmp[5];
		auto size = nbytesPoint(array[i]);

		if (size < 0)
			throw std::invalid_argument("invalid sequence");

		encode(array[i], tmp);
		res.insert(res.length(), tmp);
	}

	return res;
}

std::u32string Utf8::toucs(const std::string &str)
{
	std::u32string res;

	for (size_t i = 0; i < str.size(); ) {
		uint32_t point;
		auto size = nbytesUtf8(str[i]);

		if (size < 0)
			throw std::invalid_argument("invalid sequence");

		decode(point, str.data() + i);
		res.push_back(point);
		i += size;
	}

	return res;
}

} // !irccd
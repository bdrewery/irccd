/*
 * Converter.cpp -- iconv based converter
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

#include <cerrno>
#include <string>
#include <iterator>
#include <memory>
#include <string>
#include <stdexcept>
#include <vector>

#include <iconv.h>

#include "Converter.h"

/**
 * @struct Deleter
 * @brief The iconv descriptor deleter
 */
struct Deleter {
	/**
	 * Destroy the iconv descriptor.
	 *
	 * @param desc the descriptor
	 */
	void operator()(iconv_t desc)
	{
		iconv_close(desc);
	}
};

using Iconv = std::unique_ptr<std::remove_pointer<iconv_t>::type, Deleter>;

std::string Converter::convert(const char *from,
			       const char *to,
			       const std::string &input)
{
	// No conversion if from and to are identical
	if (std::strcmp(from, to) == 0)
		return input;

	// Try to open the conversion descriptor
	auto cd = iconv_open(to, from);

	if (cd == (iconv_t)-1)
		throw std::invalid_argument(std::strerror(errno));

	Iconv cv(cd);
	std::size_t insize(input.size());
	std::size_t outsize(insize);
	std::vector<char> result(insize + 1);

	auto *b = &input[0];
	auto *p = &result[0];

	while (insize > 0) {
		/* Convert */
		auto r = iconv(cv.get(), &b, &insize, &p, &outsize);

		if (r == (size_t)-1) {
			switch (errno) {
			case EBADF:
			case EILSEQ:
			case EINVAL:
				throw std::invalid_argument(std::strerror(errno));
			case E2BIG:
				/*
				 * Here, we need to reallocate more data because the output
				 * string may need more space.
				 *
				 * We use 16 as an optimistic value.
				 */

				result.reserve(result.size() + 16 + 1);
				p = &result[result.size()];
				outsize += 16;
			default:
				break;
			}
		}

	}

	return std::string(&result[0], (p - &result[0]));
}

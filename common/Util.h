/*
 * Util.h -- some utilities
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

#ifndef _UTIL_H_
#define _UTIL_H_

#include <string>
#include <vector>

namespace irccd {

class Util {
public:
	/**
	 * Split a string by delimiters.
	 *
	 * @param list the string to split
	 * @param delimiter a list of delimiters
	 * @param max max number of split
	 * @return a list of string splitted
	 */
	static std::vector<std::string> split(const std::string &list, const std::string &delimiter, int max = -1);

	/**
	 * Get the XDG config directory that is usually
	 * ~/.config/.
	 *
	 * @return a string to the directory
	 */
	static std::string configDirectory(void);

	/**
	 * Return the path at XDG_CONFIG_HOME or default
	 * if not found.
	 *
	 * @param filename the config filename
	 * @return the final string ready for use
	 */
	static std::string configFilePath(const std::string filename);
};

} // !irccd

#endif // !_UTIL_H_

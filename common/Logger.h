/*
 * Logger.h -- common logger routines
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

#include <cstdio>
#include <cstdarg>
#include <string>

#ifndef _LOGGER_H_
#define _LOGGER_H_

namespace irccd {

class Logger {
private:
	static bool m_verbose;

	static void printFile(FILE *fp, std::string fmt, va_list ap);

public:
	/**
	 * Enable or disable the verbosity of further
	 * log() calls.
	 *
	 * @see log
	 * @param verbose enable or disable
	 */
	static void setVerbose(bool verbose);

	/**
	 * Log an additional verbose message.
	 *
	 * @param fmt format
	 * @param ... arguments
	 */
	static void log(std::string fmt, ...);

	/**
	 * Write a warning message, it will be printed
	 * whenever the verbosity is set to false.
	 *
	 * @param fmt format
	 * @param ... arguments
	 */
	static void warn(std::string fmt, ...);

	/**
	 * Write a message used as debugging, messages will
	 * only be shown if the application has been build with
	 * DEBUG macro set.
	 *
	 * @param fmt format
	 * @param ... arguments
	 */
	static void debug(std::string fmt, ...);
};

} // !irccd

#endif // !_LOGGER_H_

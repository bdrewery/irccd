/*
 * Logger.h -- common logger routines
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

#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <string>

namespace irccd {

class Logger {
private:
	static bool m_verbose;

#if !defined(_WIN32)
	static bool m_syslog;
	static bool m_syslogLoaded;
#endif

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

#if !defined(_WIN32)
	/**
	 * Tells to use syslog. Only on Unix.
	 *
	 * @param syslog enable or disable
	 */
	static void setSyslog(bool syslog);
#endif

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
	 * Write a message and then exits with a specific error code.
	 *
	 * @param code the code to exit
	 * @param fmt the format
	 * @param ... arguments
	 */
	static void fatal(int code, std::string fmt, ...);

	/**
	 * Write a message used as debugging, messages will
	 * only be shown if the application has been build with
	 * without NDEBUG macro set.
	 *
	 * @param fmt format
	 * @param ... arguments
	 */
	static void debug(std::string fmt, ...);
};

} // !irccd

#endif // !_LOGGER_H_

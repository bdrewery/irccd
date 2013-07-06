/*
 * Logger.cpp -- common logger routines
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

#if !defined(_WIN32)
#  include <syslog.h>
#endif

#include "Logger.h"

using namespace irccd;
using namespace std;

bool Logger::m_verbose = true;

#if !defined(_WIN32)

bool Logger::m_syslog = false;
bool Logger::m_syslogLoaded = false;

#endif

void Logger::printFile(FILE *fp, string fmt, va_list ap)
{
#if !defined(_WIN32)
	if (m_syslog) {
		int priority = (fp == stdin) ? LOG_INFO : LOG_WARNING;
		vsyslog(priority, fmt.c_str(), ap);
	} else {
#else
		vfprintf(fp, fmt.c_str(), ap);
		fputc('\n', fp);
#endif

#if !defined(_WIN32)
	}
#endif
}

void Logger::setVerbose(bool verbose)
{
	Logger::m_verbose = verbose;
}

#if !defined(_WIN32)

void Logger::setSyslog(bool syslog)
{
	if (!syslog && Logger::m_syslogLoaded) {
		closelog();
		Logger::m_syslogLoaded = false;
	} else if (syslog && !Logger::m_syslogLoaded) {
		openlog("irccd", LOG_PID, 0);
		Logger::m_syslogLoaded = true;
	}
		
	Logger::m_syslog = syslog;
}

#endif

void Logger::log(string fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (Logger::m_verbose)
		Logger::printFile(stdout, fmt, ap);
	va_end(ap);
}

void Logger::warn(string fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	Logger::printFile(stderr, fmt, ap);
	va_end(ap);
}

void Logger::debug(string fmt, ...)
{
#if defined(DEBUG)
	va_list ap;

	va_start(ap, fmt);
	Logger::printFile(stdout, fmt, ap);
	va_end(ap);
#else
	(void)fmt;
#endif
}

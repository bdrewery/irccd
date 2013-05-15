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

#include "Logger.h"

using namespace irccd;
using namespace std;

bool Logger::m_verbose = true;

void Logger::printFile(FILE *fp, const std::string &fmt, va_list ap)
{
	vfprintf(fp, fmt.c_str(), ap);
	fputc('\n', fp);
}

void Logger::setVerbose(bool verbose)
{
	Logger::m_verbose = verbose;
}

void Logger::log(const string &fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (Logger::m_verbose)
		Logger::printFile(stdout, fmt, ap);
	va_end(ap);
}

void Logger::warn(const string &fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	Logger::printFile(stderr, fmt, ap);
	va_end(ap);
}

void Logger::debug(const string &fmt, ...)
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

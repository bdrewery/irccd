/*
 * Logger.h -- irccd logging
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

#ifndef _IRCCD_LOGGER_H_
#define _IRCCD_LOGGER_H_

#include <fstream>
#include <iostream>
#include <memory>
#include <utility>

namespace irccd {

/**
 * @class LoggerBase
 *
 * Base class for logging output.
 */
class LoggerBase : public std::ostream {
public:
	LoggerBase(std::streambuf *buf);
	virtual ~LoggerBase() = default;
};

/**
 * @class LoggerConsole
 * @brief Logger implementation for console output
 */
class LoggerConsole : public LoggerBase {
public:
	LoggerConsole(const std::ostream &stream = std::cout);
};

/**
 * @class LoggerFile
 * @brief Output to a file
 */
class LoggerFile : public LoggerBase {
private:
	std::filebuf m_buffer;

public:
	LoggerFile(const std::string &path);
};

/**
 * @class LoggerSilent
 * @brief Use to disable logs
 *
 * Useful for unit tests when some classes may emits log.
 */
class LoggerSilent : public LoggerBase {
private:
	class Fake : public std::streambuf {
	} m_buffer;

public:
	LoggerSilent();
};

class Logger {
private:
	static std::unique_ptr<LoggerBase> m_output;
	static std::unique_ptr<LoggerBase> m_error;
	static bool m_verbose;

public:
	template <typename T, typename... Args>
	static inline void setStandard(Args&&... args)
	{
		m_output = std::make_unique<T>(std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	static inline void setError(Args&&... args)
	{
		m_error = std::make_unique<T>(std::forward<Args>(args)...);
	}

	static void setVerbose(bool mode) noexcept;

	static std::ostream &info() noexcept;

	static std::ostream &warning() noexcept;

	static std::ostream &error() noexcept;

	static std::ostream &debug() noexcept;
};

} // !irccd

#endif // !_IRCCD_LOGGER_H_

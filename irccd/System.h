/*
 * System.h -- platform dependent functions for system inspection
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

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

/**
 * @file System.h
 * @brief System dependant functions
 */

#include <cstdint>
#include <string>

namespace irccd {

/**
 * @class System
 * @brief System dependant operations
 */
class System {
public:
	/**
	 * Get the system name.
	 *
	 * @return the name
	 */
	static std::string name();

	/**
	 * Get the system version.
	 *
	 * @return the version
	 */
	static std::string version();

	/**
	 * Get the number of seconds elapsed since the boottime.
	 *
	 * @return the number of seconds
	 */
	static uint64_t uptime();

	/**
	 * Get the milliseconds elapsed since the application
	 * startup.
	 *
	 * @return the milliseconds
	 */
	static uint64_t ticks();

	/**
	 * Get an environment variable.
	 *
	 * @return the value or empty string
	 */
	static std::string env(const std::string &var);

	/**
	 * Get home directory usually /home/foo
	 *
	 * @return the home directory
	 */
	static std::string home();
};

} // !irccd

#endif // !_SYSTEM_H_

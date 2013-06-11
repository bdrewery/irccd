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

#include <cstdint>
#include <exception>
#include <string>
#include <vector>

#include <config.h>

namespace irccd {

class Util {
public:
	class ErrorException : public std::exception {
		std::string m_error;

	public:
		ErrorException();
		ErrorException(const std::string &error);
		~ErrorException() throw();

		virtual const char * what() const throw();
	};

	/**
	 * Directory separator, / on unix, \ on Windows.
	 */
	static const char DIR_SEP;

	/**
	 * Get the basename of a file path, that is, remove
	 * all parent path from it.
	 *
	 * @param path the full file path
	 * @return the file name
	 */
	static std::string basename(const std::string &path);

	/**
	 * Get the XDG config directory that is usually
	 * ~/.config/irccd. On Windows, it is C:\Users\Foo\irccd.
	 * This add a trailing slash.
	 *
	 * @return a string to the directory
	 */
	static std::string configDirectory();

	/**
	 * Return the path at the config path or default
	 * if not found.
	 *
	 * @param filename the config filename
	 * @return the final string ready for use
	 */
	static std::string configFilePath(const std::string &filename);

	/**
	 * Wrapper around dirname(3) for portability. Returns the parent
	 * directory of the file
	 *
	 * @param file the filename to check
	 * @return the parent directory
	 */
	static std::string dirname(const std::string &file);

	/**
	 * Tell if a specified file or directory exists
	 *
	 * @param path the file / directory to check
	 * @return true on success
	 */
	static bool exist(const std::string &path);

	/**
	 * Get home directory usually /home/foo
	 *
	 * @return the home directory
	 */
	static std::string getHome(void);

	/**
	 * Get the milliseconds elapsed since the application
	 * startup.
	 *
	 * @return the milliseconds
	 */
	static uint64_t getTicks();

	/**
	 * Create a directory.
	 *
	 * @param dir the directory path
	 * @param mode the mode
	 */
	static void mkdir(const std::string &dir, int mode);

	/**
	 * Get the user plugin directory. This add a trailing slash.
	 *
	 * @return a string to the plugin path
	 */
	static std::string pluginDirectory();

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
	 * Sleep for milli seconds.
	 *
	 * @param msec the number of milli seconds
	 */
	static void usleep(int msec);
};

} // !irccd

#endif // !_UTIL_H_

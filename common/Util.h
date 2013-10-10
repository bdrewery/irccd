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
#include <functional>
#include <string>
#include <vector>

#include <config.h>

namespace irccd
{

class Util
{
public:
	class ErrorException : public std::exception
	{
	private:
		std::string m_error;

	public:
		ErrorException();
		ErrorException(const std::string &error);
		~ErrorException() throw();

		virtual const char * what() const throw();
	};

	enum Hints
	{
		HintLocal	= (1 << 0),	//! ./
		HintSystem	= (1 << 1),	//! $PREFIX/etc/irccd
		HintUser	= (1 << 2),	//! ~/.config/irccd
		HintAll		= HintLocal |
				  HintSystem |
				  HintUser
	};

	typedef std::function<bool (const std::string &)> ConfigFinder;

private:
	/**
	 * Get the installation prefix or installation directory. Also append
	 * the path to it.
	 *
	 * @param path what to append
	 * @return the final path
	 */
	static std::string pathBase(const std::string &path);

	/**
	 * Get the local path to the user append the path at the end.
	 *
	 * @param path what to append
	 * @return the final path
	 */
	static std::string pathUser(const std::string &path);

public:
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
	static std::string baseName(const std::string &path);

	/**
	 * Get the system config path, usually /usr/local/etc on Unix
	 * and the install path for Windows.
	 *
	 * @return the path
	 */
	static inline std::string configSystem()
	{
		return pathBase(ETCDIR) + DIR_SEP;
	}

	/**
	 * Get the user config directory, on Unix it will be
	 * XDG_CONFIG_HOME or ~/.config/home
	 *
	 * @return the path
	 */
	static inline std::string configUser()
	{
		return pathUser("") + DIR_SEP;
	}

	/**
	 * Return the plugin system path, usually /usr/local/share/irccd/plugins
	 * on Unix and the installation path + plugins on Windows.
	 *
	 * @return the path
	 */
	static inline std::string pluginPathSystem()
	{
		return pathBase(MODDIR) + DIR_SEP;
	}

	/**
	 * Return the default user plugin path.
	 *
	 * @return the path
	 */
	static inline std::string pluginPathUser()
	{
		return pathUser("/plugins") + DIR_SEP;
	}

	/**
	 * Wrapper around dirname(3) for portability. Returns the parent
	 * directory of the file
	 *
	 * @param file the filename to check
	 * @return the parent directory
	 */
	static std::string dirName(const std::string &file);

	/**
	 * Tell if a specified file or directory exists
	 *
	 * @param path the file / directory to check
	 * @return true on success
	 */
	static bool exist(const std::string &path);

	/**
	 * Tells if the user has access to a file.
	 *
	 * @param path the path
	 * @return true if has
	 */
	static bool hasAccess(const std::string &path);

	/**
	 * Get home directory usually /home/foo
	 *
	 * @return the home directory
	 */
	static std::string getHome();

	/**
	 * Get the milliseconds elapsed since the application
	 * startup.
	 *
	 * @return the milliseconds
	 */
	static uint64_t getTicks();

	/**
	 * This function will try to open the file specified by name
	 * prepended by hints paths.
	 *
	 * The function called must return true if it has successfully opened the
	 * file.
	 *
	 * This function may exits if no file was found.
	 *
	 * The order is (if hints enabled):
	 *
	 * 1. HintUser
	 * 2. HintLocal
	 * 3. HintSystem
	 *
	 * @param name the file name
	 * @param hints which paths to seek
	 * @param func the function to call
	 */
	static void findConfig(const std::string &name,
			       Hints hints,
			       ConfigFinder func);

	/**
	 * Create a directory.
	 *
	 * @param dir the directory path
	 * @param mode the mode
	 */
	static void mkdir(const std::string &dir, int mode);

	/**
	 * Split a string by delimiters.
	 *
	 * @param list the string to split
	 * @param delimiter a list of delimiters
	 * @param max max number of split
	 * @return a list of string splitted
	 */
	static std::vector<std::string> split(const std::string &list,
					      const std::string &delimiter,
					      int max = -1);

	/**
	 * Sleep for milli seconds.
	 *
	 * @param msec the number of milli seconds
	 */
	static void usleep(int msec);
};

} // !irccd

#endif // !_UTIL_H_

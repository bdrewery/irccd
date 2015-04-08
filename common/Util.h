/*
 * Util.h -- some utilities
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

#ifndef _IRCCD_UTIL_H_
#define _IRCCD_UTIL_H_

/**
 * @file Util.h
 * @brief Utilities
 */

#include <cstdint>
#include <ctime>
#include <exception>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace irccd {

/**
 * @class Util
 * @brief Some utilities
 */
class Util {
public:
	/**
	 * @enum Flags
	 * @brief Flags for \ref convert
	 */
	enum Flags {
		ConvertEnv	= (1 << 0),	//!< Convert ${} environnement
		ConvertHome	= (1 << 1),	//!< Convert ~ as home
		ConvertDate	= (1 << 2),	//!< Convert % as strftime
	};

	/**
	 * @struct Args
	 * @brief Arguments for \ref convert
	 */
	struct Args {
		/**
		 * Map of keywords.
		 */
		using Keywords	= std::unordered_map<char, std::string>;

		std::time_t	timestamp;	//!< timestamp date
		Keywords	keywords;	//!< keywords

		/**
		 * Default constructor, use the current date.
		 */
		Args()
			: timestamp(std::time(nullptr))
		{
		}
	};

public:
	/**
	 * Directory separator, / on unix, \ on Windows.
	 *
	 * @deprecated Use Filesystem::Separator instead
	 */
	static const char DIR_SEP;

	/**
	 * Get the installation prefix or installation directory. Also append
	 * the path to it.
	 *
	 * @param path what to append
	 * @return the final path
	 */
	static std::string pathBase(const std::string &path = "");

	/**
	 * Get the local path to the user append the path at the end.
	 *
	 * @param path what to append
	 * @return the final path
	 */
	static std::string pathUser(const std::string &path = "");

	/**
	 * Get the basename of a file path, that is, remove
	 * all parent path from it.
	 *
	 * @param path the full file path
	 * @return the file name
	 * @deprecated Use Filesystem::baseName instead
	 */
	static std::string baseName(const std::string &path);

	/**
	 * Wrapper around dirname(3) for portability. Returns the parent
	 * directory of the file
	 *
	 * @param file the filename to check
	 * @return the parent directory
	 * @deprecated Use Filesystem::dirName instead
	 */
	static std::string dirName(const std::string &file);

	/**
	 * Find a configuration file, only for irccd.conf or
	 * irccdctl.conf. Order is:
	 *
	 * 1. User's config
	 * 2. System
	 *
	 * Example:
	 * 	~/.config/irccd || C:/Users/jean/irccd
	 * 	/usr/local/etc/ || Path/To/Irccd/etc
	 *
	 * @param filename the filename to append
	 * @return the found path
	 * @throw ErrorException if none found
	 */
	static std::string findConfiguration(const std::string &filename);

	/**
	 * Find the plugin home directory. Order is:
	 *
	 * 1. User's config
	 * 2. System
	 *
	 * Example:
	 * 	~/.config/irccd/name || C:/Users/jean/irccd/name
	 * 	/usr/local/etc/irccd/name || Path/To/Irccd/etc/irccd/name
	 *
	 * @param name the plugin name
	 * @return the found path or system one
	 */
	static std::string findPluginHome(const std::string &name);

	/**
	 * Tell if a specified file or directory exists
	 *
	 * @param path the file / directory to check
	 * @return true on success
	 * @deprecated Use Filesystem::exists instead
	 */
	static bool exist(const std::string &path);

	/**
	 * Tells if the path is absolute.
	 *
	 * @param path the path
	 * @return true if it is
	 * @deprecated Use Filesystem::isAbsolute instead
	 */
	static bool isAbsolute(const std::string &path);

	/**
	 * Tells if the user has access to a file.
	 *
	 * @param path the path
	 * @return true if has
	 * @deprecated Don't use this function
	 */
	static bool hasAccess(const std::string &path);

	/**
	 * Create a directory.
	 *
	 * @param dir the directory path
	 * @param mode the mode
	 * @deprecated Use Filesystem::mkdir instead
	 */
	static void mkdir(const std::string &dir, int mode);

	/**
	 * Convert a string from a common patterns, date, environnement
	 * and such.
	 *
	 * @param line the line to convert
	 * @param args the args
	 * @param flags the flags
	 * @return the new string
	 */
	static std::string convert(const std::string &line,
				   const Args &args,
				   int flags = 0);

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
	 * Remove leading and trailing spaces.
	 *
	 * @param str the string
	 * @return the removed white spaces
	 */
	static std::string strip(const std::string &str);
};

/**
 * @class DefinedOption
 * @brief Helper to determine if an option has been set at command line
 */
class DefinedOption {
private:
	std::string m_value;
	bool m_defined{false};

	/* Not needed */
	DefinedOption(DefinedOption &&) = delete;
	DefinedOption &operator=(DefinedOption &&) = delete;
	DefinedOption(const DefinedOption &) = delete;
	DefinedOption &operator=(const DefinedOption &) = delete;

public:
	/**
	 * Default constructor, option not defined.
	 */
	DefinedOption() = default;

	/**
	 * Tell if the option is defined.
	 */
	inline operator bool() const noexcept
	{
		return m_defined;
	}

	/**
	 * Tell if the option is defined.
	 */
	inline bool operator!() const noexcept
	{
		return !m_defined;
	}

	/**
	 * Tell if the option is defined.
	 *
	 * @return true if defined
	 */
	inline bool isDefined() const noexcept
	{
		return m_defined;
	}

	/**
	 * Get the defined value.
	 *
	 * @return the value
	 * @throw std::invalid_argument if the option was not defined
	 */
	inline std::string value() const
	{
		if (!m_defined) {
			throw std::invalid_argument("option not defined");
		}

		return m_value;
	}

	/**
	 * Assign a value, the option will be defined.
	 *
	 * @param value the value
	 * @return *this
	 */
	inline DefinedOption &operator=(std::string value) {
		m_value = std::move(value);
		m_defined = true;

		return *this;
	}
};

using DefinedOptions = std::unordered_map<std::string, DefinedOption>;

} // !irccd

#endif // !_IRCCD_UTIL_H_

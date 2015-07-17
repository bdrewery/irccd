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

#include <ctime>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace irccd {

/**
 * @class Util
 * @brief Some utilities
 */
class Util {
private:
	static bool m_programPathDefined;
	static std::string m_programPath;

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

	static std::string programPath();

	/* User paths */
	static std::string pathConfigUser();
	static std::string pathDataUser();
	static std::string pathCacheUser();
	static std::string pathPluginUser();

public:
	/**
	 * This function must be called before running irccd.
	 *
	 * It use system dependant program path lookup if available and
	 * fallbacks to the path given as argument if any failure was
	 * encoutered.
	 *
	 * @param path the path to the executable (argv[0])
	 */
	static void setProgramPath(const std::string &path);

	static std::vector<std::string> pathsBinaries();
	static std::vector<std::string> pathsConfig();
	static std::vector<std::string> pathsData();
	static std::vector<std::string> pathsCache();
	static std::vector<std::string> pathsPlugins();

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
	static std::string strip(std::string str);

	/**
	 * Join values by a separator and return a string.
	 *
	 * @param first the first iterator
	 * @param last the last iterator
	 * @param delim the optional delimiter
	 */
	template <typename InputIt>
	static std::string join(InputIt first, InputIt last, char delim = ':')
	{
		std::ostringstream oss;

		if (first != last) {
			oss << *first;

			while (++first != last) {
				oss << delim << *first;
			}
		}

		return oss.str();
	}

	/**
	 * Convenient overload.
	 *
	 * @param list the initializer list
	 * @param delim the delimiter
	 * @return the string
	 */
	template <typename T>
	static inline std::string join(std::initializer_list<T> list, char delim = ':')
	{
		return join(list.begin(), list.end(), delim);
	}

	/**
	 * Server and identities must have strict names. This function can
	 * be used to ensure that they are valid.
	 *
	 * @param name the identifier name
	 * @return true if is valid
	 */
	static inline bool isIdentifierValid(const std::string &name)
	{
		std::regex regex("[A-Za-z0-9-_]+");
		std::smatch match;

		return std::regex_match(name, match, regex);
	}
};

} // !irccd

#endif // !_IRCCD_UTIL_H_

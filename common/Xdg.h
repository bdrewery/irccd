/*
 * Xdg.h -- XDG directory specifications
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

#ifndef _IRCCD_XDG_H_
#define _IRCCD_XDG_H_

/**
 * @file Xdg.h
 * @brief Read XDG standard specifications
 */

#include <vector>
#include <string>

namespace irccd {

/**
 * @class Xdg
 * @brief XDG specifications
 *
 * Read and get XDG directories. This file contains exports thingies so it can
 * compiles successfully on Windows but its usage is discouraged.
 */
class Xdg {
public:
	/**
	 * list of directories.
	 */
	using List	= std::vector<std::string>;

private:
	std::string	m_configHome;
	std::string	m_dataHome;
	std::string	m_cacheHome;
	std::string	m_runtimeDir;
	List		m_configDirs;
	List		m_dataDirs;

public:
	/**
	 * Open an xdg instance and load directories.
	 *
	 * @throw std::runtime_error on failures
	 */
	Xdg();

	/**
	 * Get the config directory. ${XDG_CONFIG_HOME} or ${HOME}/.config
	 *
	 * @return the config directory
	 */
	const std::string &configHome() const noexcept;

	/**
	 * Get the data directory. ${XDG_DATA_HOME} or ${HOME}/.local/share
	 *
	 * @return the data directory
	 */
	const std::string &dataHome() const noexcept;

	/**
	 * Get the cache directory. ${XDG_CACHE_HOME} or ${HOME}/.cache
	 *
	 * @return the cache directory
	 */
	const std::string &cacheHome() const noexcept;

	/**
	 * Get the runtime directory. ${XDG_RUNTIME_DIR} must be set,
	 * if not, it throws an exception.
	 *
	 * The XDG standard says that application should handle XDG_RUNTIME_DIR by
	 * themselves.
	 *
	 * @return the runtime directory
	 * @throw std::runtime_error on error
	 */
	const std::string &runtimeDir() const;

	/**
	 * Get the standard config directories. ${XDG_CONFIG_DIRS} or { "/etc/xdg" }
	 *
	 * @return the list of config directories
	 */
	const List &configDirs() const noexcept;

	/**
	 * Get the data directories. ${XDG_DATA_DIRS} or { "/usr/local/share", "/usr/share" }
	 *
	 * @return the list of data directories
	 */
	const List &dataDirs() const noexcept;
};

} // !irccd

#endif // !_IRCCD_XDG_H_

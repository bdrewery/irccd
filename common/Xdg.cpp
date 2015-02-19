/*
 * Xdg.cpp -- XDG directory specifications
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

#include <cstdlib>
#include <stdexcept>
#include <sstream>

#include "Xdg.h"

namespace irccd {

namespace {

bool isabsolute(const std::string &path)
{
	return path.length() > 0 && path[0] == '/';
}

std::vector<std::string> split(const std::string &arg)
{
	std::stringstream iss(arg);
	std::string item;
	std::vector<std::string> elems;

	while (std::getline(iss, item, ':'))
		if (isabsolute(item))
			elems.push_back(item);

	return elems;
}

std::string envOrHome(const std::string &var, const std::string &repl)
{
	auto value = getenv(var.c_str());

	if (value == nullptr || !isabsolute(value)) {
		auto home = getenv("HOME");

		if (home == nullptr)
			throw std::runtime_error("could not get home directory");

		return std::string(home) + "/" + repl;
	}

	return value;
}

std::vector<std::string> listOrDefaults(const std::string &var, const std::vector<std::string> &list)
{
	auto value = getenv(var.c_str());

	if (!value)
		return list;

	// No valid item at all? Use defaults
	auto result = split(value);

	return (result.size() == 0) ? list : result;
}

} // !namespace

Xdg::Xdg()
{
	m_configHome	= envOrHome("XDG_CONFIG_HOME", ".config");
	m_dataHome	= envOrHome("XDG_DATA_HOME", ".local/share");
	m_cacheHome	= envOrHome("XDG_CACHE_HOME", ".cache");

	m_configDirs	= listOrDefaults("XDG_CONFIG_DIRS", { "/etc/xdg" });
	m_dataDirs	= listOrDefaults("XDG_DATA_DIRS", { "/usr/local/share", "/usr/share" });

	/*
	 * Runtime directory is a special case and does not have a replacement, the
	 * application should manage this by itself.
	 */
	auto runtime = getenv("XDG_RUNTIME_DIR");
	if (runtime && isabsolute(runtime))
		m_runtimeDir = runtime;
}

const std::string &Xdg::configHome() const noexcept
{
	return m_configHome;
}

const std::string &Xdg::dataHome() const noexcept
{
	return m_dataHome;
}

const std::string &Xdg::cacheHome() const noexcept
{
	return m_cacheHome;
}

const std::string &Xdg::runtimeDir() const
{
	if (m_runtimeDir.size() == 0)
		throw std::runtime_error("XDG_RUNTIME_DIR is not set");

	return m_runtimeDir;
}

const Xdg::List &Xdg::configDirs() const noexcept
{
	return m_configDirs;
}

const Xdg::List &Xdg::dataDirs() const noexcept
{
	return m_dataDirs;
}

} // !irccd

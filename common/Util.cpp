/*
 * Util.cpp -- some utilities
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

#include <algorithm>
#include <cassert>
#include <cstring>
#include <sstream>
#include <stdexcept>

#include <IrccdConfig.h>

#if defined(IRCCD_SYSTEM_WINDOWS)
#  include <Windows.h>
#  include <Shlobj.h>
#else
#  include <Xdg.h>
#endif

#include "Logger.h"
#include "Util.h"
#include "Filesystem.h"

namespace irccd {

bool		Util::m_programPathDefined{false};
std::string	Util::m_programPath;

/*
 * programPath
 * ---------------------------------------------------------
 *
 * This function is used to determine the executable path. It is available and used when the project has been built
 * in the relocatable manner.
 *
 * That will be used to determine the application data, cache and such directories by getting back to the parent
 * directory from where the executable is found.
 *
 * On system that does not support that, we just get the executable path from the argv[0] when starting, the main
 * must **absolutely** calls setProgramPath before.
 */
#if defined(IRCCD_SYSTEM_WINDOWS)

std::string Util::programPath()
{
	std::string result;
	char exepath[MAX_PATH];

	if (!GetModuleFileNameA(NULL, exepath, sizeof (exepath))) {
		throw std::runtime_error("GetModuleFileName error");
	}

	result = Filesystem::dirName(exepath);

	std::string::size_type pos = result.rfind(WITH_BINDIR);
	if (pos != std::string::npos) {
		result.erase(pos);
		result += Filesystem::Separator;
	}

	return result;
}

#else

/*
 * TODO: add support for more systems here.
 *
 * - Linux
 * - FreeBSD
 * - NetBSD
 * - OpenBSD
 */

std::string systemProgramPath()
{
	throw std::runtime_error("unsupported");
}

#endif

/*
 * pathConfigUser
 * ---------------------------------------------------------
 *
 * Get the path directory to the user configuration. Example:
 *
 * Unix:
 *
 * XDG_CONFIG_HOME/irccd
 * HOME/.config/irccd
 *
 * Windows:
 *
 * CSIDL_PROFILE
 *
 */
std::string Util::pathConfigUser()
{
	std::ostringstream oss;

#if defined(IRCCD_SYSTEM_WINDOWS)
	char path[MAX_PATH];

	if (SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path) != S_OK)
		oss << "";
	else {
		oss << path;
		oss << "\\irccd\\";
	}
#else
	try {
		Xdg xdg;

		oss << xdg.configHome();
		oss << "/irccd/";
	} catch (const std::exception &) {
		const char *home = getenv("HOME");

		if (home != nullptr)
			oss << home;

		oss << "/.config/irccd/";
	}
#endif

	return oss.str();
}

std::string Util::pathDataUser()
{
	std::ostringstream oss;

#if defined(IRCCD_SYSTEM_WINDOWS)
	// TODO
#else
	try {
		Xdg xdg;

		oss << xdg.dataHome();
		oss << "/irccd/";
	} catch (const std::exception &) {
		const char *home = getenv("HOME");

		if (home != nullptr)
			oss << home;

		oss << "/.local/share/irccd/";
	}
#endif

	return oss.str();
}

std::string Util::pathCacheUser()
{
	return "";
}

void Util::setProgramPath(const std::string &path)
{
	m_programPathDefined = true;

	try {
		m_programPath = systemProgramPath();
	} catch (const std::exception &) {
		/* Fallback using argv[0] */
		m_programPath = Filesystem::dirName(path);

		std::string::size_type pos = m_programPath.rfind(WITH_BINDIR);
		if (pos != std::string::npos) {
			m_programPath.erase(pos);
		}

		/* Now the path may end with / or \ */
		if (m_programPath.length() > 0 && m_programPath[m_programPath.length() - 1] == Filesystem::Separator) {
			m_programPath.pop_back();
		}
	}
}

std::vector<std::string> Util::pathsBinaries()
{
	return {m_programPath + Filesystem::Separator + WITH_BINDIR};
}

std::vector<std::string> Util::pathsConfig()
{
	std::vector<std::string> paths;

	paths.push_back(pathConfigUser());
	paths.push_back(m_programPath + Filesystem::Separator + WITH_CONFDIR);

	return paths;
}

std::vector<std::string> Util::pathsData()
{
	std::vector<std::string> paths;

	paths.push_back(pathDataUser());
	paths.push_back(m_programPath + Filesystem::Separator + WITH_DATADIR);

	return paths;
}

std::vector<std::string> Util::pathsCache()
{
	std::vector<std::string> paths;

	paths.push_back(pathCacheUser());

	return paths;
}

std::vector<std::string> Util::pathsPlugins()
{
	std::vector<std::string> paths;

	/* Always current directory first */
	paths.push_back(Filesystem::cwd());
	paths.push_back(pathDataUser() + "plugins");
	paths.push_back(m_programPath + Filesystem::Separator + WITH_DATADIR + Filesystem::Separator + "plugins");

	return paths;
}

std::string Util::convert(const std::string &line, const Args &args, int flags)
{
	// TODO: please clean up this function.
	auto copy(line);
	auto &kw(args.keywords);

	for (size_t i = 0; i < copy.size(); ++i) {
		char tok;

		if (copy[i] == '#') {
			if (i >= copy.size() - 1)
				continue;

			// Keywords
			if (copy[i + 1] == '#') {
				copy.erase(i, 1);
				continue;
			}

			if (kw.count(copy[i + 1]) > 0) {
				tok = copy[i + 1];
				copy.erase(i, 2);		// erase '#'
				copy.insert(i, kw.at(tok));	// replace
				i += kw.at(tok).size();
			}
		} else if (copy[i] == '$' && (flags & ConvertEnv)) {
			if (i >= copy.size() - 1)
				continue;

			if (copy[i + 1] != '{')
				continue;

			auto pos = copy.find('}');

			if (pos == std::string::npos)
				continue;

			auto name = copy.substr(i + 2, pos - (i + 2));
			auto value = std::getenv(name.c_str());

			copy.erase(i, pos - (i - 1));

			if (value != nullptr) {
				copy.insert(i, value);
				i += strlen(value);
			}
		} else if (copy[i] == '~' && (flags & ConvertHome)) {
			auto value = std::getenv("HOME");

			copy.erase(i, 1);

			if (value != nullptr) {
				copy.insert(i, value);
				i += strlen(value);
			}
		}
	}

	if (flags & ConvertDate) {
		auto tm = *std::localtime(&args.timestamp);

		auto tmp = new char[copy.size() + 2];

		std::strftime(tmp, copy.size() + 2, copy.c_str(), &tm);
		copy = tmp;

		delete [] tmp;
	}

	return copy;
}

std::vector<std::string> Util::split(const std::string &list,
				     const std::string &delimiter,
				     int max)
{
	std::vector<std::string> result;
	size_t next = -1, current;
	int count = 1;
	bool finished = false;

	do {
		std::string val;

		current = next + 1;
		next = list.find_first_of(delimiter, current);

		// split max, get until the end
		if (max >= 0 && count++ >= max) {
			val = list.substr(current, std::string::npos);
			finished = true;
		} else {
			val = list.substr(current, next - current);
			finished = next == std::string::npos;
		}

		result.push_back(val);
	} while (!finished);

	return result;
}

std::string Util::strip(std::string str)
{
	auto test = [] (char c) { return !std::isspace(c); };

	str.erase(str.begin(), std::find_if(str.begin(), str.end(), test));
	str.erase(std::find_if(str.rbegin(), str.rend(), test).base(), str.end());

	return str;
}

} // !irccd

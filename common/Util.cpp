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
#include <sstream>
#include <stdexcept>

#include <IrccdConfig.h>

#if defined(IRCCD_SYSTEM_WINDOWS)
#  include <Windows.h>
#  include <Shlobj.h>
#endif

#include "Logger.h"
#include "Util.h"
#include "Filesystem.h"

namespace irccd {

bool Util::m_programPathDefined{false};
std::string Util::m_programPath;

namespace {

#if defined(IRCCD_SYSTEM_WINDOWS)

std::string systemProgramPath()
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

} // !namespace

void Util::setProgramPath(const std::string &path)
{
	m_programPathDefined = true;

	try {
		m_programPath = systemProgramPath();
	} catch (const std::exception &ex) {
		Logger::debug() << getprogname() << ": failed to get executable path: " << ex.what() << std::endl;

		/* Fallback using argv[0] */
		m_programPath = Filesystem::dirName(path);

		std::string::size_type pos = m_programPath.rfind(WITH_BINDIR);
		if (pos != std::string::npos) {
			m_programPath.erase(pos);
		}

		m_programPath += Filesystem::Separator;
	}

}

std::string Util::pathUser(const std::string &append)
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

		// append default path.
		oss << "/.config/irccd/";
	}
#endif

	oss << append;

	return oss.str();
}

std::string Util::findConfiguration(const std::string &)
{
#if 0
	std::ostringstream oss;
	std::string fpath;

	// 1. User first
	oss << pathUser() << filename;
	fpath = oss.str();

	Logger::info() << getprogname() << ": checking for " << fpath << std::endl;
	if (hasAccess(fpath))
		return fpath;

	// 2. Base + ETCDIR + filename
	oss.str("");

	if (!isAbsolute(ETCDIR))
		oss << pathBase();

	oss << ETCDIR << Util::DIR_SEP << filename;
	fpath = oss.str();

	Logger::info() << getprogname() << ": checking for " << fpath << std::endl;
	if (hasAccess(fpath))
		return fpath;

	// Failure
	oss.str("");
	oss << "could not find configuration file for " << filename;

	throw std::runtime_error(oss.str());
#endif
	return "";
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

std::string Util::path(Directory directory)
{
	assert(m_programPathDefined);

	switch (directory) {
	case Binary:
		return m_programPath + Filesystem::Separator + WITH_BINDIR;
	case Config:
		return m_programPath + Filesystem::Separator + WITH_BINDIR;
	case Plugins:
		return m_programPath + Filesystem::Separator + WITH_BINDIR;
	default:
		break;
	}

	throw std::invalid_argument("unknown directory");
}

} // !irccd

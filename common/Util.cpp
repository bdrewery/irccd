/*
 * Util.cpp -- some utilities
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

#include <cerrno>
#include <cstring>
#include <fstream>
#include <sstream>

#if defined(_WIN32)
#  include <sys/timeb.h>
#  include <direct.h>
#  include <windows.h>
#  include <shlobj.h>

#  define _MKDIR(p, x)	::_mkdir(p)
#else
#  include <libgen.h>

// This is libxdg-basedir
#  include <basedir.h>

#  define _MKDIR(p, x)	::mkdir(p, x)
#endif

#include <sys/stat.h>

#include "config.h"
#include "Util.h"
#include "Logger.h"

namespace irccd {

/* --------------------------------------------------------
 * Util class (private functions)
 * -------------------------------------------------------- */

std::string Util::pathBase(const std::string &append)
{
	std::ostringstream oss;

#if defined(_WIN32)
	std::string base;
	char exepath[512];
	size_t pos;

	/*
	 * Window is more complicated case as we don't know in advance
	 * where irccd is installed, so we get the current process path
	 * and removes its bin/ suffix.
	 */
	GetModuleFileNameA(NULL, exepath, sizeof (exepath));
	base = Util::dirName(exepath);
	pos = base.find("bin");
	if (pos != std::string::npos)
		base.erase(pos);
	
	oss << base << "\\";
#else
	oss << PREFIX << "/";
#endif

	oss << append;

	return oss.str();
}

std::string Util::pathUser(const std::string &append)
{
	std::ostringstream oss;

#if defined(_WIN32)
	char path[MAX_PATH];

	if (SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path) != S_OK)
		oss << "";
	else {
		oss << path;
		oss << "\\irccd\\";
	}
#else
	xdgHandle handle;

	if ((xdgInitHandle(&handle)) == nullptr) {
		const char *home = getenv("HOME");

		if (home != nullptr)
			oss << home;

		// append default path.
		oss << "/.config/irccd/";
	} else {
		oss << xdgConfigHome(&handle);
		oss << "/irccd/";

		xdgWipeHandle(&handle);
	}

#endif

	oss << append;

	return oss.str();
}

/* --------------------------------------------------------
 * Util class (public functions)
 * -------------------------------------------------------- */

#if defined(_WIN32)
	const char Util::DIR_SEP = '\\';
#else
	const char Util::DIR_SEP = '/';
#endif

std::string Util::baseName(const std::string &path)
{
#if defined(_WIN32) || defined(_MSC_VER)
	std::string copy = path;
	size_t pos;

	pos = copy.find_last_of('\\');
	if (pos == std::string::npos)
		pos = copy.find_last_of('/');

	if (pos == std::string::npos)
		return copy;

	return copy.substr(pos + 1);
#else
	char *copy = strdup(path.c_str());
	std::string ret;

	if (copy == NULL)
		return "";

	ret = std::string(::basename(copy));
	free(copy);

	return ret;
#endif
}

std::string Util::findConfiguration(const std::string &filename)
{
	std::ostringstream oss;
	std::string fpath;

	// 1. User first
	oss << pathUser() << filename;
	fpath = oss.str();

	Logger::log("%s: checking for %s", getprogname(), fpath.c_str());
	if (hasAccess(fpath))
		return fpath;

	// 2. Base + ETCDIR + filename
	oss.str("");

	if (!isAbsolute(ETCDIR))
		oss << pathBase();

	oss << ETCDIR << Util::DIR_SEP << filename;
	fpath = oss.str();

	Logger::log("%s: checking for %s", getprogname(), fpath.c_str());
	if (hasAccess(fpath))
		return fpath;

	// Failure
	oss.str("");
	oss << "could not find configuration file for " << filename;

	throw std::runtime_error(oss.str());
}

std::string Util::findPluginHome(const std::string &name)
{
	std::ostringstream oss;
	std::string fpath;

	// 1. User first
	oss << pathUser() << name;
	fpath = oss.str();

	if (hasAccess(fpath))
		return fpath;

	// 2. Base + ETCDIR + "irccd" + name
	oss.str("");

	if (!isAbsolute(ETCDIR))
		oss << pathBase();

	oss << ETCDIR << Util::DIR_SEP << "irccd" << Util::DIR_SEP;
	oss << name;
	fpath = oss.str();

	/*
	 * We returns the system path so that plugins can just check
	 * the error code of opening files and such wich a real path
	 * and not ""
	 */

	return fpath;
}

std::string Util::dirName(const std::string &file)
{
#if defined(_WIN32) || defined(_MSC_VER)
	std::string copy = file;
	std::size_t pos;

	pos = copy.find_last_of('\\');
	if (pos == std::string::npos)
		pos = copy.find_last_of('/');

	if (pos == std::string::npos)
		return copy;

	return copy.substr(0, pos);
#else
	char *copy = strdup(file.c_str());
	std::string ret;

	if (copy == NULL)
		return "";

	ret = std::string(::dirname(copy));
	free(copy);

	return ret;
#endif
}

bool Util::exist(const std::string &path)
{
	struct stat st;

	return (stat(path.c_str(), &st) != -1);
}

bool Util::isAbsolute(const std::string &path)
{
	return path.length() > 0 && path[0] == Util::DIR_SEP;
}

bool Util::hasAccess(const std::string &path)
{
	std::ifstream file;
	bool ret;

	file.open(path);
	ret = file.is_open();
	file.close();

	return ret;
}

void Util::mkdir(const std::string &dir, int mode)
{
	std::ostringstream oss;

	oss << "mkdir: ";

	for (size_t i = 0; i < dir.length(); ++i) {
		if (dir[i] != '/')
			continue;

		std::string part = dir.substr(0, i);
		if (part.length() <= 0 || exist(part))
			continue;

		if (_MKDIR(part.c_str(), mode) == -1) {
			oss << part << ": " << strerror(errno);
			throw std::runtime_error(oss.str());
		}
	}

	// Last part
	if (_MKDIR(dir.c_str(), mode) == -1) {
		oss << dir << ": " << strerror(errno);
		throw std::runtime_error(oss.str());
	}
}

std::string Util::convert(const std::string &line, const Args &args, int flags)
{
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

std::string Util::strip(const std::string &str)
{
	auto copy = str;
	auto test = [] (char c) { return !std::isspace(c); };

	copy.erase(copy.begin(), std::find_if(copy.begin(), copy.end(), test));
	copy.erase(std::find_if(copy.rbegin(), copy.rend(), test).base(), copy.end());

	return copy;
}

} // !irccd

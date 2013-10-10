/*
 * Util.cpp -- some utilities
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

#include <cerrno>
#include <cstring>
#include <sstream>

#if defined(_WIN32)
#  include <sys/timeb.h>
#  include <direct.h>
#  include <windows.h>
#  include <shlobj.h>

#  define _MKDIR(p, x)	::_mkdir(p)
#else
#  include <sys/time.h>
#  include <libgen.h>

// This is libxdg-basedir
#  include <basedir.h>

#  define _MKDIR(p, x)	::mkdir(p, x)
#endif

#include <sys/stat.h>

#include "config.h"
#include "Util.h"
#include "Logger.h"

namespace irccd
{

/* --------------------------------------------------------
 * ErrorException class
 * -------------------------------------------------------- */

Util::ErrorException::ErrorException()
{
}

Util::ErrorException::ErrorException(const std::string &error)
	: m_error(error)
{
}

Util::ErrorException::~ErrorException() throw()
{
}

const char * Util::ErrorException::what() const throw()
{
	return m_error.c_str();
}

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
		oss << "\\irccd";
	}
#else
	xdgHandle handle;

	if ((xdgInitHandle(&handle)) == nullptr) {
		const char *home = getenv("HOME");

		if (home != nullptr)
			oss << home;

		// append default path.
		oss << "/.config/irccd";

		xdgWipeHandle(&handle);
	} else {
		oss << xdgConfigHome(&handle);
		oss << "/irccd";
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

void Util::findConfig(const std::string &name,
		      Hints hints,
		      ConfigFinder func)
{
	std::vector<std::string> tried;
	std::ostringstream oss;
	std::string path;
	bool done = false;

	// 1. User ~/.config/irccd
	if (hints & Util::HintUser)
	{
		oss << configUser();
		oss << name;
		path = oss.str();

		if (!(done = func(path)))
			tried.push_back(path);
	}

	// 2. Local ./
	if (!done && (hints & Util::HintLocal))
	{
		oss.str("");
		oss << name;
		path = oss.str();

		if (!(done = func(path)))
			tried.push_back(path);
	}

	// 3. System config
	if (!done && (hints & Util::HintSystem))
	{
		oss.str("");
		oss << configSystem();
		oss << name;
		path = oss.str();

		if (!(done = func(path)))
			tried.push_back(path);
	}

	if (!done)
	{
		Logger::warn("%s: unable to find %s", getprogname(), name.c_str());

		for (auto p : tried)
			Logger::warn("%s: have tried %s", getprogname(), p.c_str());

		std::exit(1);
	}
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

std::string Util::getHome()
{
#if defined(_WIN32)
	char path[MAX_PATH];

	if (SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path) != S_OK)
		return "";

	return std::string(path);
#else
	return std::string(getenv("HOME"));
#endif
}

uint64_t Util::getTicks()
{
#if defined(_WIN32) || defined(_MSC_VER)
	_timeb tp;

	_ftime(&tp);

	return tp.time * 1000LL + tp.millitm;
#else
        struct timeval tp;

        gettimeofday(&tp, NULL);

        return tp.tv_sec * 1000LL + tp.tv_usec / 1000;
#endif
}

void Util::mkdir(const std::string &dir, int mode)
{
	std::ostringstream oss;

	oss << "mkdir: ";

	for (size_t i = 0; i < dir.length(); ++i)
	{
		if (dir[i] != '/')
			continue;

		std::string part = dir.substr(0, i);
		if (part.length() <= 0 || exist(part))
			continue;

		if (_MKDIR(part.c_str(), mode) == -1)
		{
			oss << part << ": " << strerror(errno);
			throw Util::ErrorException(oss.str());
		}
	}

	// Last part
	if (_MKDIR(dir.c_str(), mode) == -1)
	{
		oss << dir << ": " << strerror(errno);
		throw Util::ErrorException(oss.str());
	}
}

std::string Util::pluginDirectory()
{
	/*
	 * Under unix, we store local plugins under the same directory
	 * as the config, plus /plugins suffix, so it's usually:
	 *
	 * ~/.config/irccd/plugins.
	 *
	 * On windows, it reside inside the user home like :
	 * C:\Users\Simone\irccd\plugins
	 */
	std::ostringstream oss;

	oss << configUser();

#if defined(_WIN32)
	oss << "plugins\\";
#else
	oss << "plugins/";
#endif

	return oss.str();
}

std::vector<std::string> Util::split(const std::string &list,
				     const std::string &delimiter,
				     int max)
{
	std::vector<std::string> result;
	size_t next = -1, current;
	int count = 1;
	bool finished = false;

	do
	{
		std::string val;

		current = next + 1;
		next = list.find_first_of(delimiter, current);

		// split max, get until the end
		if (max >= 0 && count++ >= max)
		{
			val = list.substr(current, std::string::npos);
			finished = true;
		}
		else
		{
			val = list.substr(current, next - current);
			finished = next == std::string::npos;
		}

		result.push_back(val);
	}
	while (!finished);

	return result;
}

void Util::usleep(int msec)
{
#if defined(_WIN32)
	Sleep(msec);
#else
	::usleep(msec * 1000);
#endif
}

} // !irccd

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

#include <sstream>

#if defined(_WIN32)
#  include <sys/timeb.h>
#  include <direct.h>
#  include <windows.h>
#  include <shlobj.h>

#  define _CRT_SECURE_NO_WARNINGS
#  define mkdir(p, x)	mkdir(p)

#else
#  include <sys/time.h>
#  include <libgen.h>

#  include <basedir.h>
#endif

#include <sys/stat.h>

#include "Util.h"

using namespace irccd;
using namespace std;

Util::ErrorException::ErrorException(void)
{
}

Util::ErrorException::ErrorException(const std::string &error)
	:m_error(error)
{
}

Util::ErrorException::~ErrorException(void) throw()
{
}

const char * Util::ErrorException::what(void) const throw()
{
	return m_error.c_str();
}

vector<string> Util::split(const string &list, const string &delimiter, int max)
{
	vector<string> result;
	size_t next = -1, current;
	int count = 1;
	bool finished = false;

	do {
		string val;

		current = next + 1;
		next = list.find_first_of(delimiter, current);

		// split max, get until the end
		if (max >= 0 && count++ >= max) {
			val = list.substr(current, string::npos);
			finished = true;
		} else {
			val = list.substr(current, next - current);
			finished = next == string::npos;
		}

		result.push_back(val);
	} while (!finished);

	return result;
}

string Util::configDirectory(void)
{
#if defined(_WIN32)
	char path[MAX_PATH];

	if (SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path ) != S_OK)
		return "";

	return string(path);
#else
	xdgHandle handle;
	ostringstream oss;

	if ((xdgInitHandle(&handle)) == nullptr) {
		const char *home = getenv("HOME");

		if (home != nullptr)
			oss << home;

		// append default path.
		oss << "/.config/irccd/";
	} else {
		oss << xdgConfigHome(&handle);
		oss << "/irccd/";
	}

	xdgWipeHandle(&handle);

	return oss.str();
#endif
}

string Util::configFilePath(const std::string &filename)
{
	ostringstream oss;

	oss << Util::configDirectory();
	oss << filename;

	return oss.str();
}

string Util::getHome(void)
{
#if defined(_WIN32)
	char path[MAX_PATH];

	if (SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path ) != S_OK)
		return "";

	return string(path);
#else
	return string(getenv("HOME"));
#endif
}

uint64_t Util::getTicks(void)
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

string Util::basename(const string &path)
{
#if defined(_WIN32) || defined(_MSC_VER)
	string copy = path;
	size_t pos;

	pos = copy.find_last_of('\\');
	if (pos == string::npos)
		pos = copy.find_last_of('/');

	if (pos == string::npos)
		return copy;

	return copy.substr(pos + 1);
#else
	return string(::basename(path.c_str()));
#endif
}

string Util::dirname(const string &file)
{
#if defined(_WIN32) || defined(_MSC_VER)
	string copy = file;
	size_t pos;

	pos = copy.find_last_of('\\');
	if (pos == string::npos)
		pos = copy.find_last_of('/');

	if (pos == string::npos)
		return copy;

	return copy.substr(0, pos);
#else
	return string(::dirname(file.c_str()));
#endif
}

bool Util::exist(const string &path)
{
	struct stat st;

	return (stat(path.c_str(), &st) != -1);
}

void Util::mkdir(const std::string &dir, int mode)
{
	ostringstream oss;

	oss << "mkdir: ";

	for (size_t i = 0; i < dir.length(); ++i) {
		if (dir[i] != '/')
			continue;

		string part = dir.substr(0, i);
		if (part.length() <= 0 || exist(part))
			continue;

		if (::mkdir(part.c_str(), mode) == -1) {
			oss << part << ": " << strerror(errno);
			throw Util::ErrorException(oss.str());
		}
	}

	// Last part
	if (::mkdir(dir.c_str(), mode) == -1) {
		oss << dir << ": " << strerror(errno);
		throw Util::ErrorException(oss.str());
	}
}

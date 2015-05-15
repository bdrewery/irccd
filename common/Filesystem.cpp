/*
 * Filesystem.cpp -- some file system operation
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

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <sstream>

#include <IrccdConfig.h>

#if defined(IRCCD_SYSTEM_WINDOWS)
#  include <direct.h>
#  include <shlwapi.h>
#else
#  include <sys/stat.h>
#  include <climits>
#  include <unistd.h>
#  include <libgen.h>
#endif

#include "Filesystem.h"

namespace irccd {

#if defined(IRCCD_SYSTEM_WINDOWS)
const char Filesystem::Separator{'\\'};
#else
const char Filesystem::Separator{'/'};
#endif

std::string Filesystem::baseName(std::string path)
{
#if defined(IRCCD_SYSTEM_WINDOWS)
	size_t pos;

	pos = path.find_last_of('\\');
	if (pos == std::string::npos)
		pos = path.find_last_of('/');

	if (pos == std::string::npos) {
		return path;
	}

	return path.substr(pos + 1);
#else
	return basename(&path[0]);
#endif
}

std::string Filesystem::dirName(std::string path)
{
#if defined(IRCCD_SYSTEM_WINDOWS)
	std::size_t pos;

	pos = path.find_last_of('\\');
	if (pos == std::string::npos) {
		pos = path.find_last_of('/');
	}

	if (pos == std::string::npos) {
		return path;
	}

	return path.substr(0, pos);
#else
	return dirname(&path[0]);
#endif
}

bool Filesystem::isAbsolute(const std::string &path) noexcept
{
#if defined(IRCCD_SYSTEM_WINDOWS)
	return !isRelative(path);
#else
	return path.size() > 0 && path[0] == '/';
#endif
}

bool Filesystem::isRelative(const std::string &path) noexcept
{
#if defined(IRCCD_SYSTEM_WINDOWS)
	return PathIsRelativeA(path.c_str());
#else
	return !isAbsolute(path);
#endif
}

bool Filesystem::exists(const std::string &path)
{
#if defined(HAVE_ACCESS)
	return access(path.c_str(), F_OK) == 0;
#elif defined(HAVE_STAT)
	struct stat st;

	return (stat(path.c_str(), &st) == 0);
#else
	// worse fallback
	std::FILE *file = std::fopen(path.c_str(), "r");
	bool result;

	if (file != nullptr) {
		result = true;
		std::fclose(file);
	} else {
		result = false;
	}

	return result;
#endif
}

void Filesystem::mkdir(const std::string &dir, int mode)
{
	std::ostringstream oss;

	oss << "mkdir: ";

	for (size_t i = 0; i < dir.length(); ++i) {
		if (dir[i] != '/')
			continue;

		std::string part = dir.substr(0, i);
		if (part.length() <= 0 || exists(part))
			continue;

#if defined(IRCCD_SYSTEM_WINDOWS)
		if (::_mkdir(part.c_str()) < 0) {
#else
		if (::mkdir(part.c_str(), mode) < 0) {
#endif
			oss << part << ": " << std::strerror(errno);
			throw std::runtime_error(oss.str());
		}
	}

	// Last part
#if defined(IRCCD_SYSTEM_WINDOWS)
	if (::_mkdir(dir.c_str()) < 0) {
#else
	if (::mkdir(dir.c_str(), mode) < 0) {
#endif
		oss << dir << ": " << std::strerror(errno);
		throw std::runtime_error(oss.str());
	}

#if defined(IRCCD_SYSTEM_WINDOWS)
	// Windows's mkdir does not use mode.
	(void)mode;
#endif
}

std::string Filesystem::cwd()
{
#if defined(IRCCD_SYSTEM_WINDOWS)
	char path[MAX_PATH];

	if (GetCurrentDirectoryA(path, sizeof (path)) < 0) {
		// TODO: format message
		throw std::runtime_error("failed to get current working directory");
	}

	return path;
#else
	char path[PATH_MAX];

	if (getcwd(path, sizeof (path)) == nullptr) {
		throw std::runtime_error(std::strerror(errno));
	}

	return path;
#endif
}

} // !irccd

/*
 * System.cpp -- platform dependent functions for system inspection
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
#include <ctime>
#include <stdexcept>

#if defined(_WIN32)
#  include <sys/types.h>
#  include <sys/timeb.h>
#  include <Windows.h>
#  include <Shlobj.h>
#else
#  include <cerrno>
#  include <cstring>
#  include <stdexcept>

#if defined(__linux__)
#  include <sys/sysinfo.h>
#endif

#  include <sys/utsname.h>
#  include <sys/time.h>
#endif

#include "IrccdConfig.h"

#include "Logger.h"
#include "System.h"

namespace irccd {

namespace {

#if defined(_WIN32)

std::string systemName()
{
	return "Windows";
}

std::string systemVersion()
{
	auto version = GetVersion();
 
	auto major = (DWORD)(LOBYTE(LOWORD(version)));
	auto minor = (DWORD)(HIBYTE(LOWORD(version)));

	return std::to_string(major) + "." + std::to_string(minor);
}

uint64_t systemUptime()
{
	return ::GetTickCount64() / 1000;
}

uint64_t systemTicks()
{
	_timeb tp;

	_ftime(&tp);

	return tp.time * 1000LL + tp.millitm;
}

std::string systemHome()
{
	char path[MAX_PATH];

	if (SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path) != S_OK)
		return "";

	return std::string(path);
}

#else

std::string systemName()
{
	struct utsname uts;

	if (uname(&uts) < 0)
		throw std::runtime_error(strerror(errno));

	return std::string(uts.sysname);
}

std::string systemVersion()
{
	struct utsname uts;

	if (uname(&uts) < 0)
		throw std::runtime_error(strerror(errno));

	return std::string(uts.release);
}

uint64_t systemUptime()
{
#if defined(__linux__)
	struct sysinfo info;

	if (sysinfo(&info) < 0)
		throw std::runtime_error(strerror(errno));

	return info.uptime;

#else
	struct timespec ts;

	if (clock_gettime(CLOCK_UPTIME, &ts) < 0)
		throw std::runtime_error(strerror(errno));

	return ts.tv_sec;
#endif
}

uint64_t systemTicks()
{
        struct timeval tp;

        gettimeofday(&tp, NULL);

        return tp.tv_sec * 1000LL + tp.tv_usec / 1000;
}

std::string systemHome()
{
	return std::string(getenv("HOME"));
}

#endif

}

std::string System::name()
{
	try {
		return systemName();
	} catch (std::runtime_error error) {
		Logger::warn("%s: %s", getprogname(), error.what());
		return "Unknown";
	}
}

std::string System::version()
{
	try {
		return systemVersion();
	} catch (std::runtime_error error) {
		Logger::warn("%s: %s", getprogname(), error.what());
		return "Unknown";
	}
}

uint64_t System::uptime()
{
	return systemUptime();
}

uint64_t System::ticks()
{
	return systemTicks();
}

std::string System::env(const std::string &var)
{
	auto value = std::getenv(var.c_str());

	if (value == nullptr)
		return "";

	return value;
}

std::string System::home()
{
	return systemHome();
}

} // !irccd

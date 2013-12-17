/*
 * System.cpp -- platform dependent functions for system inspection
 *
 * Copyright (c) 2013 David Demelier <markand@malikania.fr>
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

#if defined(_WIN32)
#  include <Windows.h>
#else
#  include <cerrno>
#  include <cstring>
#  include <stdexcept>

#  include <sys/utsname.h>
#  include <sys/time.h>
#endif

#include "config.h"

#include "Logger.h"
#include "System.h"

namespace irccd {

namespace {

#if defined(_WIN32)

std::string systemName()
{
	return ::GetTicksCount64() / 1000;
}

void systemUsleep(int milliseconds)
{
	::Sleep(milliseconds * 1000);
}

uint32_t systemTicks()
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
	struct timespec ts;

	if (clock_gettime(CLOCK_UPTIME, &ts) < 0)
		throw std::runtime_error(strerror(errno));

	return ts.tv_sec;
}

void systemUsleep(int milliseconds)
{
	::usleep(milliseconds * 1000);
}

uint32_t systemTicks()
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

void System::sleep(int seconds)
{
	return System::usleep(seconds * 1000);
}

void System::usleep(int milliseconds)
{
	return systemUsleep(milliseconds);
}

uint32_t System::ticks()
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

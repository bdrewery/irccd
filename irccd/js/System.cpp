/*
 * System.cpp -- platform dependent functions for system inspection
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

#include <cstdlib>
#include <ctime>
#include <stdexcept>

#include <IrccdConfig.h>

#if defined(IRCCD_SYSTEM_WINDOWS)
#  include <sys/types.h>
#  include <sys/timeb.h>
#  include <Windows.h>
#  include <Shlobj.h>
#else
#  include <cerrno>
#  include <cstring>
#  include <stdexcept>
#  include <ctime>

#if defined(IRCCD_SYSTEM_MAC)
#  include <sys/sysctl.h>
#endif

#if defined(IRCCD_SYSTEM_LINUX)
#  include <sys/sysinfo.h>
#endif

#  include <sys/utsname.h>
#  include <sys/time.h>
#endif

#include "Logger.h"
#include "System.h"

namespace irccd {

std::string System::name()
{
#if defined(IRCCD_SYSTEM_LINUX)
	return "Linux";
#elif defined(IRCCD_SYSTEM_WINDOWS)
	return "Windows";
#elif defined(IRCCD_SYSTEM_FREEBSD)
	return "FreeBSD";
#elif defined(IRCCD_SYSTEM_OPENBSD)
	return "OpenBSD";
#elif defined(IRCCD_SYSTEM_NETBSD)
	return "NetBSD";
#elif defined(IRCCD_SYSTEM_MAC)
	return "Mac";
#else
	return "Unknown";
#endif
}

std::string System::version()
{
#if defined(IRCCD_SYSTEM_WINDOWS)
	auto version = GetVersion();

	auto major = (DWORD)(LOBYTE(LOWORD(version)));
	auto minor = (DWORD)(HIBYTE(LOWORD(version)));

	return std::to_string(major) + "." + std::to_string(minor);
#else
	struct utsname uts;

	if (uname(&uts) < 0)
		throw std::runtime_error(strerror(errno));

	return std::string(uts.release);
#endif
}

uint64_t System::uptime()
{
#if defined(IRCCD_SYSTEM_WINDOWS)
	return ::GetTickCount64() / 1000;
#elif defined(IRCCD_SYSTEM_LINUX)
	struct sysinfo info;

	if (sysinfo(&info) < 0)
		throw std::runtime_error(strerror(errno));

	return info.uptime;
#elif defined(IRCCD_SYSTEM_MAC)
	struct timeval boottime;
	size_t length = sizeof (boottime);
	int mib[2] = { CTL_KERN, KERN_BOOTTIME };

	if (sysctl(mib, 2, &boottime, &length, nullptr, 0) < 0)
		throw std::runtime_error(strerror(errno));

	time_t bsec = boottime.tv_sec, csec = time(nullptr);

	return difftime(csec, bsec);
#else
	/* BSD */
	struct timespec ts;

	if (clock_gettime(CLOCK_UPTIME, &ts) < 0)
		throw std::runtime_error(strerror(errno));

	return ts.tv_sec;
#endif
}

uint64_t System::ticks()
{
#if defined(IRCCD_SYSTEM_WINDOWS)
	_timeb tp;

	_ftime(&tp);

	return tp.time * 1000LL + tp.millitm;
#else
	struct timeval tp;

	gettimeofday(&tp, NULL);

	return tp.tv_sec * 1000LL + tp.tv_usec / 1000;
#endif
}

std::string System::home()
{
#if defined(IRCCD_SYSTEM_WINDOWS)
	char path[MAX_PATH];

	if (SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path) != S_OK)
		return "";

	return std::string(path);
#else
	return env("HOME");
#endif
}

std::string System::env(const std::string &var)
{
	auto value = std::getenv(var.c_str());

	if (value == nullptr)
		return "";

	return value;
}

} // !irccd

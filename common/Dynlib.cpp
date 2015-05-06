/*
 * DynLib.cpp -- portable shared library loader
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

#include <stdexcept>

#if defined(_WIN32)
#  include <Windows.h>
#else
#  include <dlfcn.h>
#endif

#include "Dynlib.h"

#if defined(_WIN32)

namespace irccd {

namespace {

std::string systemError()
{
	LPSTR error = nullptr;
	std::string errmsg = "Unknown error";

	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&error, 0, NULL);

	if (error) {
		errmsg = std::string(error);
		LocalFree(error);
	}

	return errmsg;
}

}

void Dynlib::systemInit()
{
	m_handle = nullptr;
}

Dynlib::Handle Dynlib::systemLoad(const std::string &path, Policy) const
{
	Handle handle = LoadLibraryA(path.c_str());

	if (handle == nullptr)
		throw std::runtime_error(systemError());

	return handle;
}

Dynlib::Sym Dynlib::systemSym(const std::string &name)
{
	Sym sym;

	if (m_handle == nullptr)
		throw std::runtime_error("library not loaded");

	sym = GetProcAddress(m_handle, name.c_str());
	if (sym == nullptr)
		throw std::out_of_range(systemError());

	return sym;
}

void Dynlib::systemClose()
{
	if (m_handle != nullptr)
		FreeLibrary(m_handle);
}

#else

void Dynlib::systemInit()
{
	m_handle = nullptr;
}

Dynlib::Handle Dynlib::systemLoad(const std::string &path, Policy policy) const
{
	int mode = (policy == Immediately) ? RTLD_NOW : RTLD_LAZY;
	Handle handle;

	handle = dlopen(path.c_str(), mode);
	if (handle == nullptr)
		throw std::runtime_error(dlerror());

	return handle;
}

Dynlib::Sym Dynlib::systemSym(const std::string &name)
{
	Sym sym;

	if (m_handle == nullptr)
		throw std::runtime_error("library not loaded");

	sym = dlsym(m_handle, name.c_str());
	if (sym == nullptr)
		throw std::out_of_range(dlerror());

	return sym;
}

void Dynlib::systemClose()
{
	if (m_handle != nullptr)
		dlclose(m_handle);
}

#endif

Dynlib::Dynlib()
{
	systemInit();
}

Dynlib::Dynlib(const std::string &path, Policy policy)
{
	m_handle = systemLoad(path, policy);
}

Dynlib::~Dynlib()
{
	systemClose();
}

} // !irccd
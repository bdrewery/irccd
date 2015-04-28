/*
 * Directory.cpp -- open and read directories
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

#include <sstream>
#include <stdexcept>

#include "Directory.h"

#if defined(_WIN32)
#  include <Windows.h>
#else
#  include <cstring>
#  include <cerrno>

#  include <sys/types.h>
#  include <dirent.h>
#endif

namespace irccd {

#if defined(_WIN32)

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

void Directory::systemLoad(const std::string &path, int flags)
{
	std::ostringstream oss;
	HANDLE handle;
	WIN32_FIND_DATA fdata;

	oss << path << "\\*";
	handle = FindFirstFile(oss.str().c_str(), &fdata);

	if (handle == nullptr)
		throw std::runtime_error(systemError());

	do {
		DirectoryEntry entry;

		entry.name = fdata.cFileName;
		if (entry.name == "." && !(flags & Directory::Dot))
			continue;
		if (entry.name == ".." && !(flags & Directory::DotDot))
			continue;

		switch (fdata.dwFileAttributes) {
		case FILE_ATTRIBUTE_DIRECTORY:
			entry.type = DirectoryEntry::Dir;
			break;
		case FILE_ATTRIBUTE_NORMAL:
			entry.type = DirectoryEntry::File;
			break;
		case FILE_ATTRIBUTE_REPARSE_POINT:
			entry.type = DirectoryEntry::Link;
			break;
		default:
			break;
		}

		m_list.push_back(entry);
	} while (FindNextFile(handle, &fdata) != 0);

	FindClose(handle);
}

#else

void Directory::systemLoad(const std::string &path, int flags)
{
	DIR *dp;
	struct dirent *ent;

	if ((dp = opendir(path.c_str())) == nullptr)
		throw std::runtime_error(strerror(errno));

	while ((ent = readdir(dp)) != nullptr) {
		DirectoryEntry entry;

		entry.name = ent->d_name;
		if (entry.name == "." && !(flags & Directory::Dot))
			continue;
		if (entry.name == ".." && !(flags & Directory::DotDot))
			continue;

		switch (ent->d_type) {
		case DT_DIR:
			entry.type = DirectoryEntry::Dir;
			break;
		case DT_REG:
			entry.type = DirectoryEntry::File;
			break;
		case DT_LNK:
			entry.type = DirectoryEntry::Link;
			break;
		default:
			break;
		}

		m_list.push_back(entry);
	}

	closedir(dp);
}

#endif

bool operator==(const DirectoryEntry &e1, const DirectoryEntry &e2)
{
	return e1.name == e2.name && e1.type == e2.type;
}

Directory::Directory()
{
}

Directory::Directory(const std::string &path, int flags)
{
	systemLoad(path, flags);
}

Directory::List::iterator Directory::begin()
{
	return m_list.begin();
}

Directory::List::const_iterator Directory::cbegin() const
{
	return m_list.cbegin();
}

Directory::List::iterator Directory::end()
{
	return m_list.end();
}

Directory::List::const_iterator Directory::cend() const
{
	return m_list.cend();
}

int Directory::count() const
{
	return m_list.size();
}

bool operator==(const Directory &d1, const Directory &d2)
{
	return d1.m_list == d2.m_list;
}

} // !irccd

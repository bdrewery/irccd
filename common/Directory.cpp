/*
 * Directory.cpp -- open and scan directories
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

#include <cerrno>
#include <cstring>

#if defined(_WIN32)
#include <sstream>
#include <windows.h>
#else
#include <dirent.h>
#endif

#include "Directory.h"

namespace irccd
{

Directory::Directory(const std::string &path)
{
	m_path = path;
}

Directory::~Directory()
{
}

bool Directory::open(bool skipParents)
{
#if defined(_WIN32)
	std::ostringstream oss;
	WIN32_FIND_DATA fdata;
	HANDLE hd;

	oss << m_path;
	oss << "\\*";

	hd = FindFirstFile(oss.str().c_str(), &fdata);
	if (hd == INVALID_HANDLE_VALUE) {
		m_error = "Could not open directory " + m_path;
		return false;
	}

	do {
		Entry entry;

		if (skipParents &&
		    (strcmp(fdata.cFileName, ".") == 0 || strcmp(fdata.cFileName, "..") == 0))
			continue;

		entry.m_isDirectory = (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? true : false;
		entry.m_name = fdata.cFileName;

		m_entries.push_back(entry);
	} while (FindNextFile(hd, &fdata) != 0);

	FindClose(hd);

	return true;
	
#else
	DIR *dir;
	struct dirent *dp;

	if ((dir = opendir(m_path.c_str())) == nullptr) {
		m_error = strerror(errno);
		return false;
	}

	while ((dp = readdir(dir)) != nullptr) {
		Entry entry;

		if (skipParents &&
		    (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0))
			continue;

		entry.m_name = dp->d_name;
		entry.m_isDirectory = (dp->d_type == DT_DIR);

		m_entries.push_back(entry);
	}

	closedir(dir);

	return true;
#endif
}

const std::string & Directory::getPath() const
{
	return m_path;
}

const std::string & Directory::getError() const
{
	return m_error;
}

const std::vector<Entry> & Directory::getEntries() const
{
	return m_entries;
}

bool operator==(const Entry &e1, const Entry &e2)
{
	return e1.m_name == e2.m_name &&
	    e1.m_isDirectory == e2.m_isDirectory;
}

bool operator==(const Directory &d1, const Directory &d2)
{
	return d1.getPath() == d2.getPath() &&
	    d1.getEntries() == d2.getEntries();
}

} // !irccd

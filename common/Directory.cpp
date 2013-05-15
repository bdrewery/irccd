/*
 * Directory.cpp -- open and scan directories
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

#include <dirent.h>

#include "Directory.h"

using namespace irccd;
using namespace std;

Directory::Directory(const std::string &path)
{
	m_path = path;
}

Directory::~Directory(void)
{
}

bool Directory::open(bool skipParents)
{
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
}

const std::string & Directory::getError(void) const
{
	return m_error;
}

const std::vector<Entry> & Directory::getEntries(void) const
{
	return m_entries;
}

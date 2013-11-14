/*
 * Directory.h -- open and scan directories
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

#ifndef _DIRECTORY_H_
#define _DIRECTORY_H_

#include <string>
#include <vector>

namespace irccd
{

struct Entry
{
	std::string m_name;
	bool m_isDirectory;
};

class Directory
{
private:
	std::string m_path;
	std::string m_error;
	std::vector<Entry> m_entries;

public:
	Directory(const std::string &path);
	~Directory();

	/**
	 * Open the directory.
	 *
	 * @param skipParents skip "." and ".."
	 * @return true on success
	 */
	bool open(bool skipParents = false);

	/**
	 * Get the directory path
	 *
	 * @return the path
	 */
	const std::string & getPath() const;

	/**
	 * Get the error
	 *
	 * @return the error
	 */
	const std::string & getError() const;

	/**
	 * Get the list of entries found.
	 *
	 * @return the list
	 */
	const std::vector<Entry> & getEntries() const;
};

bool operator==(const Entry &e1, const Entry &e2);

bool operator==(const Directory &d1, const Directory &d2);

} // !irccd

#endif // !_DIRECTORY_H_

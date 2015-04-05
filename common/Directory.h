/*
 * Directory.h -- open and read directories
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

#ifndef _DIRECTORY_H_
#define _DIRECTORY_H_

#include <cstddef>
#include <string>
#include <vector>

namespace irccd {

/**
 * @class Entry
 * @brief entry in the directory list
 */
class DirectoryEntry {
public:
	/**
	 * @enum Type
	 * @brief Describe the type of an entry
	 */
	enum Type {
		Unknown		= 0,
		File,
		Dir,
		Link
	};

	std::string	name;		//! name of entry (base name)
	Type		type{Unknown};	//! type of file

	friend bool operator==(const DirectoryEntry &e1, const DirectoryEntry &e2);
};

/**
 * @class Directory
 * @brief class to manipulate directories
 *
 * This class allow the user to iterate directories in a for range based
 * loop using iterators.
 */
class Directory {
public:
	/**
	 * @enum Flags
	 * @brief optional flags to read directories
	 */
	enum Flags {
		Dot	= (1 << 0),	//!< If set, lists "." too
		DotDot	= (1 << 1)	//!< If set, lists ".." too
	};

	using List = std::vector<DirectoryEntry>;

	// C++ Container compatibility
	using value_type	= List::value_type;
	using iterator		= List::iterator;
	using const_iterator	= List::const_iterator;

private:
	List m_list;

	void systemLoad(const std::string &path, int flags);

public:
	/**
	 * Default constructor, does nothing.
	 */
	Directory();

	/**
	 * Open a directory and read all its content.
	 * @param path the path
	 * @param flags the optional flags
	 */
	Directory(const std::string &path, int flags = 0);

	/**
	 * Return an iterator the beginning.
	 *
	 * @return the iterator
	 */
	List::iterator begin();

	/**
	 * Return a const iterator the beginning.
	 *
	 * @return the iterator
	 */
	List::const_iterator cbegin() const;

	/**
	 * Return an iterator to past the end.
	 *
	 * @return the iterator
	 */
	List::iterator end();

	/**
	 * Return a const iterator to past the end.
	 *
	 * @return the iterator
	 */
	List::const_iterator cend() const;

	/**
	 * Get the number of entries in the directory.
	 *
	 * @return the number
	 */
	int count() const;

	friend bool operator==(const Directory &d1, const Directory &d2);
};

} // !irccd

#endif // !_DIRECTORY_H_

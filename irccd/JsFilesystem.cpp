/*
 * JsFilesystem.cpp -- filesystem operations for irccd JS API
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
#include <cstring>
#include <fstream>
#include <regex>
#include <stdexcept>
#include <string>

#include <IrccdConfig.h>

#if defined(HAVE_SYS_STAT_H) && defined(HAVE_STAT)
#  include <sys/stat.h>
#endif

#include <Directory.h>
#include <Filesystem.h>

#include "Js.h"

namespace irccd {

namespace {

/* --------------------------------------------------------
 * File utilities
 * -------------------------------------------------------- */

/*
 * File object that is used by File constructor.
 */
class File {
public:
	enum {
		Output,
		Input
	};

protected:
	std::string m_path;
	std::fstream m_stream;
	int m_type;

public:
	inline File(std::string path, std::fstream::openmode mode, int type)
		: m_path(std::move(path))
		, m_stream(m_path, mode)
		, m_type(type)
	{
		if (!m_stream.is_open()) {
			throw std::runtime_error(std::strerror(errno));
		}
	}

	inline const std::string &path() const noexcept
	{
		return m_path;
	}

	inline int type() const noexcept
	{
		return m_type;
	}

	inline void seek(std::fstream::off_type amount, std::fstream::seekdir dir)
	{
		m_stream.seekg(amount, dir);

		if (!m_stream) {
			throw std::runtime_error(std::strerror(errno));
		}
	}

	inline unsigned tell()
	{
		unsigned pos = static_cast<unsigned>(m_stream.tellg());

		if (!m_stream) {
			throw std::runtime_error(std::strerror(errno));
		}

		return pos;
	}

	bool readline(std::string &result)
	{
		std::getline(m_stream, result);

		if (m_stream.eof()) {
			return false;
		}

		if (!m_stream) {
			throw std::runtime_error(std::strerror(errno));
		}

		return true;
	}

	std::string read(int amount)
	{
		if (!m_stream) {
			throw std::runtime_error(std::strerror(errno));
		}

		// amount set to negative means everything
		if (amount < 0) {
			return std::string(std::istreambuf_iterator<char>(m_stream), std::istreambuf_iterator<char>());
		}

		std::string result;

		result.resize(amount);
		m_stream.read(&result[0], amount);

		if (!m_stream)  {
			throw std::runtime_error(std::strerror(errno));
		}

		result.resize(m_stream.gcount());

		return result;
	}

	void write(const std::string &data)
	{
		if (!m_stream) {
			throw std::runtime_error(std::strerror(errno));
		}

		m_stream.write(data.c_str(), data.size());

		if (!m_stream) {
			throw std::runtime_error(std::strerror(errno));
		}
	}
};

#if defined(HAVE_STAT)

/*
 * Push the struct stat as an object to JS.
 */
duk_ret_t filePushStat(duk_context *ctx, const struct stat &st)
{
	duk_push_object(ctx);

#if defined(HAVE_STAT_ST_ATIME)
	duk_push_int(ctx, st.st_atime);
	duk_put_prop_string(ctx, -2, "atime");
#endif
#if defined(HAVE_STAT_ST_BLKSIZE)
	duk_push_int(ctx, st.st_blksize);
	duk_put_prop_string(ctx, -2, "blksize");
#endif
#if defined(HAVE_STAT_ST_BLOCKS)
	duk_push_int(ctx, st.st_blocks);
	duk_put_prop_string(ctx, -2, "blocks");
#endif
#if defined(HAVE_STAT_ST_CTIME)
	duk_push_int(ctx, st.st_ctime);
	duk_put_prop_string(ctx, -2, "ctime");
#endif
#if defined(HAVE_STAT_ST_DEV)
	duk_push_int(ctx, st.st_dev);
	duk_put_prop_string(ctx, -2, "dev");
#endif
#if defined(HAVE_STAT_ST_GID)
	duk_push_int(ctx, st.st_gid);
	duk_put_prop_string(ctx, -2, "gid");
#endif
#if defined(HAVE_STAT_ST_INO)
	duk_push_int(ctx, st.st_ino);
	duk_put_prop_string(ctx, -2, "ino");
#endif
#if defined(HAVE_STAT_ST_MODE)
	duk_push_int(ctx, st.st_mode);
	duk_put_prop_string(ctx, -2, "mode");
#endif
#if defined(HAVE_STAT_ST_MTIME)
	duk_push_int(ctx, st.st_mtime);
	duk_put_prop_string(ctx, -2, "mtime");
#endif
#if defined(HAVE_STAT_ST_NLINK)
	duk_push_int(ctx, st.st_nlink);
	duk_put_prop_string(ctx, -2, "nlink");
#endif
#if defined(HAVE_STAT_ST_RDEV)
	duk_push_int(ctx, st.st_rdev);
	duk_put_prop_string(ctx, -2, "rdev");
#endif
#if defined(HAVE_STAT_ST_SIZE)
	duk_push_int(ctx, st.st_size);
	duk_put_prop_string(ctx, -2, "size");
#endif
#if defined(HAVE_STAT_ST_UID)
	duk_push_int(ctx, st.st_uid);
	duk_put_prop_string(ctx, -2, "uid");
#endif

	return 1;
}

#endif // !HAVE_STAT

/* --------------------------------------------------------
 * Directory utilities
 * -------------------------------------------------------- */

/*
 * Find an entry recursively (or not) in a directory using a predicate
 * which can be used to test for regular expression, equality.
 *
 * Do not use this function directly, use:
 *
 * - directoryFindName
 * - directoryFindRegex
 */
template <typename Pred>
std::string directoryFindPath(const std::string &base, const std::string &destination, bool recursive, Pred pred)
{
	/*
	 * For performance reason, we first iterate over all entries that are
	 * not directories to avoid going deeper recursively if the requested
	 * file is in the current directory.
	 */
	Directory directory(base);

	for (const DirectoryEntry &entry : directory) {
		if (entry.type != DirectoryEntry::Dir && pred(entry.name)) {
			return destination + entry.name;
		}
	}

	if (!recursive) {
		throw std::out_of_range("entry not found");
	}

	for (const DirectoryEntry &entry : directory) {
		std::string path;

		if (entry.type == DirectoryEntry::Dir) {
			path = directoryFindPath(base + entry.name + Filesystem::Separator,
						destination + entry.name + Filesystem::Separator,
						true, pred);
		}

		if (!path.empty()) {
			return path;
		}
	}

	throw std::out_of_range("entry not found");
}

/*
 * Helper for finding by equality.
 */
std::string directoryFindName(std::string base, const std::string &pattern, bool recursive, const std::string &destination = "")
{
	if (base.size() > 0 && base.back() != Filesystem::Separator) {
		base.push_back(Filesystem::Separator);
	}

	return directoryFindPath(base, destination, recursive, [&] (const std::string &entryname) -> bool {
		return pattern == entryname;
	});
}

/*
 * Helper for finding by regular expression
 */
std::string directoryFindRegex(std::string base, std::string pattern, bool recursive, const std::string &destination = "")
{
	if (base.size() > 0 && base.back() != Filesystem::Separator) {
		base.push_back(Filesystem::Separator);
	}

	// Duktape keeps leading and trailing '/' remove them if any.
	if (pattern.size() > 0 && pattern.front() == '/') {
		pattern.erase(0, 1);
	}
	if (pattern.size() > 0 && pattern.back() == '/') {
		pattern.erase(pattern.length() - 1, 1);
	}

	std::regex regexp(pattern, std::regex::ECMAScript);
	std::smatch smatch;

	return directoryFindPath(base, destination, recursive, [&] (const std::string &entryname) -> bool {
		return std::regex_match(entryname, smatch, regexp);
	});
}

/*
 * Get the path stored in the directory object.
 */
const char *directoryPath(duk_context *ctx)
{
	const char *path;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "path");
	path = duk_to_string(ctx, -1);
	duk_pop_2(ctx);

	return path;
}

/*
 * Generic find function for:
 *
 * - Directory.find
 * - Directory.prototype.find
 */
duk_ret_t directoryFind(duk_context *ctx, const char *base, int beginindex)
{
	bool recursive = false;

	if (duk_get_top(ctx) == beginindex + 2) {
		recursive = duk_require_boolean(ctx, beginindex + 1);
	}

	try {
		std::string path;

		if (duk_is_string(ctx, beginindex)) {
			path = directoryFindName(base, duk_to_string(ctx, beginindex), recursive);
		} else if (duk_is_object(ctx, beginindex)) {
			path = directoryFindRegex(base, duk_to_string(ctx, beginindex), recursive);
		} else {
			dukx_throw(ctx, -1, "pattern must be a string or a regex expression");
		}

		if (path.empty()) {
			return 0;
		}

		duk_push_string(ctx, path.c_str());
	} catch (const std::exception &ex) {
		dukx_throw(ctx, -1, ex.what());
	}

	return 1;
}

/*
 * Generic find function for:
 *
 * - Directory.find
 * - Directory.prototype.find
 */
duk_ret_t directoryRemove(duk_context *ctx, const std::string &path, int beginindex)
{
	bool recursive = false;

	if (duk_get_top(ctx) == beginindex + 1) {
		recursive = duk_require_boolean(ctx, beginindex);
	}
	if (!recursive) {
		::remove(path.c_str());
	} else {
		try {
			Directory directory(path);

			for (const DirectoryEntry &entry : directory) {
				if (entry.type == DirectoryEntry::Dir) {
					(void)directoryRemove(ctx, path + Filesystem::Separator + entry.name, true);
				} else {
					::remove((path + Filesystem::Separator + entry.name).c_str());
				}
			}

			::remove(path.c_str());
		} catch (const std::exception &ex) {
			// TODO: put the error in a log.
		}
	}

	return 0;
}

/* --------------------------------------------------------
 * File methods
 * -------------------------------------------------------- */

/*
 * Method: File.basename()
 * --------------------------------------------------------
 *
 * Synonym of File.basename(path) but with the path from the file.
 *
 * Returns:
 *   The base file name
 */
duk_ret_t File_prototype_basename(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<File>(ctx, [&] (File &file) {
		duk_push_string(ctx, Filesystem::baseName(file.path()).c_str());
	});
	dukx_assert_end(ctx, 1);

	return 1;
}

/*
 * Method: File.dirname()
 * --------------------------------------------------------
 *
 * Synonym of File.dirname(path) but with the path from the file.
 *
 * Returns:
 *   The base directory name
 */
duk_ret_t File_prototype_dirname(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<File>(ctx, [&] (File &file) {
		duk_push_string(ctx, Filesystem::dirName(file.path()).c_str());
	});
	dukx_assert_end(ctx, 1);

	return 1;
}

/*
 * Method: File.read(amount)
 * --------------------------------------------------------
 *
 * Read the specified amount of characters or the whole file.
 *
 * Arguments:
 *   - amount, the amount of characters or -1 to read all (default: -1)
 *
 * Returns:
 *   - The string
 *
 * Throws:
 *   - Any exception on error
 */
duk_ret_t File_prototype_read(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<File>(ctx, [&] (File &file) {
		if (file.type() == File::Output) {
			dukx_throw(ctx, -1, "file is opened for writing");
		}

		int amount = -1;

		if (duk_get_top(ctx) > 0) {
			amount = duk_require_int(ctx, 0);
		}

		try {
			duk_push_string(ctx, file.read(amount).c_str());
		} catch (const std::exception &ex) {
			dukx_throw(ctx, -1, ex.what());
		}
	});
	dukx_assert_end(ctx, 1);

	return 1;
}

/*
 * Method: File.readline()
 * --------------------------------------------------------
 *
 * Read the next line available.
 *
 * Returns:
 *   - The next line or undefined if eof
 * Throws:
 *   - Any exception on error
 */
duk_ret_t File_prototype_readline(duk_context *ctx)
{
	dukx_with_this<File>(ctx, [&] (File &file) {
		try {
			std::string str;

			file.readline(str);
			duk_push_string(ctx, str.c_str());
		} catch (const std::exception &ex) {
			dukx_throw(ctx, errno, std::strerror(errno));
		}
	});

	return 1;
}

/*
 * Method: File.remove()
 * --------------------------------------------------------
 *
 * Synonym of File.remove(path) but with the path from the file.
 *
 * Throws:
 *   - Any exception on error
 */
duk_ret_t File_prototype_remove(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<File>(ctx, [&] (File &file) {
		if (remove(file.path().c_str()) < 0) {
			dukx_throw_syserror(ctx, errno);
		}
	});
	dukx_assert_equals(ctx);

	return 0;
}

/*
 * Method: File.seek(type, amount)
 * --------------------------------------------------------
 *
 * Sets the position in the file.
 *
 * Arguments:
 *   - type, the type of setting (File.SeekSet, File.SeekCur, File.SeekSet)
 *   - amount, the new offset
 * Throws:
 *   - Any exception on error
 */
duk_ret_t File_prototype_seek(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<File>(ctx, [&] (File &file) {
		int type = duk_require_int(ctx, 0);
		int amount = duk_require_int(ctx, 1);

		try {
			file.seek(static_cast<std::fstream::off_type>(amount),
				  static_cast<std::fstream::seekdir>(type));
		} catch (const std::exception &ex) {
			dukx_throw(ctx, -1, ex.what());
		}
	});
	dukx_assert_equals(ctx);

	return 0;
}

#if defined(HAVE_STAT)

/*
 * Method: File.stat() [optional]
 * --------------------------------------------------------
 *
 * Synonym of File.stat(path) but with the path from the file.
 *
 * Returns:
 *   - The stat information
 * Throws:
 *   - Any exception on error
 */
duk_ret_t File_prototype_stat(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<File>(ctx, [&] (File &file) {
		struct stat st;

		if (stat(file.path().c_str(), &st) < 0) {
			dukx_throw_syserror(ctx, errno);
		}

		(void)filePushStat(ctx, st);
	});
	dukx_assert_end(ctx, 1);

	return 1;
}

#endif // !HAVE_STAT

/*
 * Method: File.tell()
 * --------------------------------------------------------
 *
 * Get the actual position in the file.
 *
 * Returns:
 *   - The position
 * Throws:
 *   - Any exception on error
 */
duk_ret_t File_prototype_tell(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<File>(ctx, [&] (File &file) {
		try {
			duk_push_int(ctx, file.tell());
		} catch (const std::exception &ex) {
			dukx_throw(ctx, -1, ex.what());
		}
	});
	dukx_assert_end(ctx, 1);

	return 1;
}

/*
 * Method: File.write(data)
 * --------------------------------------------------------
 *
 * Write some characters to the file.
 *
 * Arguments:
 *   - data, the character to write
 * Throws:
 *   - Any exception on error
 */
duk_ret_t File_prototype_write(duk_context *ctx)
{
	const char *data = duk_require_string(ctx, 0);

	dukx_assert_begin(ctx);
	dukx_with_this<File>(ctx, [&] (File &file) {
		if (file.type() == File::Input) {
			dukx_throw(ctx, -1, "file is opened for reading");
		}

		try {
			file.write(data);
		} catch (const std::exception &ex) {
			dukx_throw(ctx, -1, ex.what());
		}
	});
	dukx_assert_equals(ctx);

	return 0;
}

constexpr const duk_function_list_entry fileMethods[] = {
	{ "basename",	File_prototype_basename,	0	},
	{ "dirname",	File_prototype_dirname,		0	},
	{ "read",	File_prototype_read,		1	},
	{ "readline",	File_prototype_readline,	0	},
	{ "remove",	File_prototype_remove,		0	},
	{ "seek",	File_prototype_seek,		2	},
#if defined(HAVE_STAT)
	{ "stat",	File_prototype_stat,		0	},
#endif
	{ "tell",	File_prototype_tell,		0	},
	{ "write",	File_prototype_write,		1	},
	{ nullptr,	nullptr,			0	}
};

/* --------------------------------------------------------
 * File "static" functions
 * -------------------------------------------------------- */

/*
 * Function: fs.File(path, mode) [constructor]
 * --------------------------------------------------------
 *
 * Open a file specified by path with the specified mode.
 *
 * Arguments:
 *   - path, the path to the file
 *   - mode, the mode, can be "r" "w" or "a"
 * Throws:
 *   - Any exception on error
 */
duk_ret_t File_File(duk_context *ctx)
{
	if (!duk_is_constructor_call(ctx)) {
		return 0;
	}

	const char *path = duk_require_string(ctx, 0);
	const char *modestring = duk_require_string(ctx, 1);

	std::fstream::openmode mode = static_cast<std::fstream::openmode>(0);

	for (const char *p = modestring; *p != '\0'; ++p) {
		if (*p == 'w') {
			mode |= std::fstream::out;
		} else if (*p == 'r') {
			mode |= std::fstream::in;
		} else if (*p == 'a') {
			mode |= (std::fstream::app);
		}
	}

	if (((mode & std::fstream::out) || (mode & std::fstream::app)) && (mode & std::fstream::in)) {
		dukx_throw(ctx, -1, "can not open for both reading and writing");
	}

	duk_push_this(ctx);

	try {
		if (mode & std::fstream::out) {
			dukx_set_class<File>(ctx, new File(path, mode, File::Output));
		} else {
			dukx_set_class<File>(ctx, new File(path, mode, File::Input));
		}
	} catch (...) {
		duk_pop(ctx);
		dukx_throw_syserror(ctx, errno);
	}

	duk_pop(ctx);

	return 0;
}

/*
 * Function: fs.File.basename(path)
 * --------------------------------------------------------
 *
 * Return the file basename as specified in basename(3) C function.
 *
 * Arguments:
 *   - path, the path to the file
 * Returns:
 *   - the base name
 */
duk_ret_t File_basename(duk_context *ctx)
{
	duk_push_string(ctx, Filesystem::baseName(duk_require_string(ctx, 0)).c_str());

	return 1;
}

/*
 * Function: fs.File.dirname(path)
 * --------------------------------------------------------
 *
 * Return the file directory name as specified in `dirname(3)` C function.
 *
 * Arguments:
 *   - path, the path to the file
 * Returns:
 *   - the directory name
 */
duk_ret_t File_dirname(duk_context *ctx)
{
	duk_push_string(ctx, Filesystem::dirName(duk_require_string(ctx, 0)).c_str());

	return 1;
}

/*
 * Function: fs.File.exists(path)
 * --------------------------------------------------------
 *
 * Check if the file exists.
 *
 * Arguments:
 *   - path, the path to the file
 * Returns:
 *   - true if exists
 * Throws:
 *   - Any exception if we don't have access
 */
duk_ret_t File_exists(duk_context *ctx)
{
	duk_push_boolean(ctx, Filesystem::exists(duk_require_string(ctx, 0)));

	return 1;
}

/*
 * function fs.File.remove(path)
 * --------------------------------------------------------
 *
 * Remove the file at the specified path.
 *
 * Arguments:
 *   - path, the path to the file
 * Throws:
 *   - Any exception on error
 */
duk_ret_t File_remove(duk_context *ctx)
{
	if (remove(duk_require_string(ctx, 0)) < 0) {
		dukx_throw_syserror(ctx, errno);
	}

	return 0;
}

#if defined(HAVE_STAT)

/*
 * function fs.File.stat(path) [optional]
 * --------------------------------------------------------
 *
 * Get file information at the specified path.
 *
 * Arguments:
 *   - path, the path to the file
 * Returns:
 *   - the stats information
 * Throws:
 *   - Any exception on error
 */
duk_ret_t File_stat(duk_context *ctx)
{
	const char *path = duk_require_string(ctx, 0);
	struct stat st;

	if (stat(path, &st) < 0) {
		dukx_throw_syserror(ctx, errno);
	}

	return filePushStat(ctx, st);
}

#endif // !HAVE_STAT

constexpr const duk_function_list_entry fileFunctions[] = {
	{ "basename",	File_basename,	1			},
	{ "dirname",	File_dirname,	1			},
	{ "exists",	File_exists,	1			},
	{ "remove",	File_remove,	1			},
#if defined(HAVE_STAT)
	{ "stat",	File_stat,	1			},
#endif
	{ nullptr,	nullptr,	0			}
};

constexpr const duk_number_list_entry fileConstants[] = {
	{ "SeekCur",	static_cast<int>(std::fstream::cur)	},
	{ "SeekEnd",	static_cast<int>(std::fstream::end)	},
	{ "SeekSet",	static_cast<int>(std::fstream::beg)	},
	{ nullptr,	0					}
};

/* --------------------------------------------------------
 * Directory object
 * -------------------------------------------------------- */

/*
 * Method: Directory.find(pattern, recursive)
 * --------------------------------------------------------
 *
 * Synonym of Directory.find(path, pattern, recursive) but the path is taken
 * from the directory object.
 *
 * Arguments:
 *   - pattern, the regular expression or file name
 *   - recursive, set to true to search recursively (default: false)
 * Returns:
 *   - the path to the file or undefined on errors or not found
 */
duk_ret_t Directory_prototype_find(duk_context *ctx)
{
	return directoryFind(ctx, directoryPath(ctx), 0);
}

/*
 * Method: Directory.remove(recursive)
 * --------------------------------------------------------
 *
 * Synonym of Directory.remove(recursive) but the path is taken from the
 * directory object.
 *
 * Arguments:
 *   - recursive, recursively or not (default: false)
 * Throws:
 *   Any exception on error
 */
duk_ret_t Directory_prototype_remove(duk_context *ctx)
{
	return directoryRemove(ctx, directoryPath(ctx), 0);
}

constexpr const duk_function_list_entry directoryMethods[] = {
	{ "find",		Directory_prototype_find,	DUK_VARARGS	},
	{ "remove",		Directory_prototype_remove,	1		},
	{ nullptr,		nullptr,			0		}
};

/* --------------------------------------------------------
 * Directory "static" functions
 * -------------------------------------------------------- */

/*
 * Function: fs.Directory(path, flags) [constructor]
 * --------------------------------------------------------
 *
 * Opens and read the directory at the specified path.
 *
 * Arguments:
 *   - path, the path to the directory
 *   - flags, the optional flags (default: 0)
 * Throws:
 *   - Any exception on error
 */
duk_ret_t Directory_Directory(duk_context *ctx)
{
	if (!duk_is_constructor_call(ctx)) {
		return 0;
	}

	const char *path = duk_require_string(ctx, 0);
	int flags = 0;

	if (duk_get_top(ctx) > 1) {
		flags = duk_require_int(ctx, 1);
	}

	try {
		Directory directory(path, flags);

		duk_push_this(ctx);
		duk_push_string(ctx, "count");
		duk_push_int(ctx, directory.count());
		duk_def_prop(ctx, -3, DUK_DEFPROP_ENUMERABLE | DUK_DEFPROP_HAVE_VALUE);
		duk_push_string(ctx, "path");
		duk_push_string(ctx, path);
		duk_def_prop(ctx, -3, DUK_DEFPROP_ENUMERABLE | DUK_DEFPROP_HAVE_VALUE);

		// add entries
		duk_push_string(ctx, "entries");
		duk_push_array(ctx);

		int i = 0;
		for (const DirectoryEntry &entry : directory) {
			duk_push_object(ctx);
			duk_push_string(ctx, entry.name.c_str());
			duk_put_prop_string(ctx, -2, "name");
			duk_push_int(ctx, static_cast<int>(entry.type));
			duk_put_prop_string(ctx, -2, "type");
			duk_put_prop_index(ctx, -2, i++);
		}

		duk_def_prop(ctx, -3, DUK_DEFPROP_ENUMERABLE | DUK_DEFPROP_HAVE_VALUE);
	} catch (const std::exception &ex) {
		dukx_throw(ctx, -1, ex.what());
	}

	return 0;
}

/*
 * Function: fs.Directory.find(path, pattern, recursive)
 * --------------------------------------------------------
 *
 * Find an entry by a pattern or a regular expression.
 *
 * Arguments:
 *   - path, the base path
 *   - pattern, the regular expression or file name
 *   - recursive, set to true to search recursively (default: false)
 * Returns:
 *   - the path to the file or undefined on errors or not found
 */
duk_ret_t Directory_find(duk_context *ctx)
{
	return directoryFind(ctx, duk_require_string(ctx, 0), 1);
}

/*
 * Function: fs.Directory.remove(path, recursive)
 * --------------------------------------------------------
 *
 * Remove the directory optionally recursively.
 *
 * Arguments:
 *   - path, the path to the directory
 *   - recursive, recursively or not (default: false)
 * Throws:
 *   Any exception on error
 */
duk_ret_t Directory_remove(duk_context *ctx)
{
	return directoryRemove(ctx, duk_require_string(ctx, 0), 1);
}

/*
 * Function: fs.Directory.mkdir(path, mode = 0700)
 * --------------------------------------------------------
 *
 * Create a directory specified by path. It will created needed subdirectories
 * just like you have invoked mkdir -p.
 *
 * Arguments:
 *   - path, the path to the directory
 *   - mode, the mode, not available on all platforms
 * Throws:
 *   - Any exception on error
 */
duk_ret_t Directory_mkdir(duk_context *ctx)
{
	const char *path = duk_require_string(ctx, 0);
	int mode = 0700;

	if (duk_get_top(ctx) == 2) {
		mode = duk_require_int(ctx, 1);
	}

	try {
		Filesystem::mkdir(path, mode);
	} catch (const std::exception &ex) {
		dukx_throw(ctx, -1, ex.what());
	}

	return 0;
}

constexpr const duk_function_list_entry directoryFunctions[] = {
	{ "find",		Directory_find,		DUK_VARARGS		},
	{ "mkdir",		Directory_mkdir,	DUK_VARARGS		},
	{ "remove",		Directory_remove,	DUK_VARARGS		},
	{ nullptr,		nullptr,		0			}
};

constexpr const duk_number_list_entry directoryConstants[] = {
	{ "Dot",		static_cast<int>(Directory::Dot)		},
	{ "DotDot",		static_cast<int>(Directory::DotDot)		},
	{ "TypeUnknown",	static_cast<int>(DirectoryEntry::Unknown)	},
	{ "TypeDir",		static_cast<int>(DirectoryEntry::Dir)		},
	{ "TypeFile",		static_cast<int>(DirectoryEntry::File)		},
	{ "TypeLink",		static_cast<int>(DirectoryEntry::Link)		},
	{ nullptr, 		0						}
};

} // !namespace

/* --------------------------------------------------------
 * Module function
 * -------------------------------------------------------- */

duk_ret_t dukopen_filesystem(duk_context *ctx) noexcept
{
	duk_push_object(ctx);

	// irccd.fs.File
	duk_push_c_function(ctx, File_File, 2);
	duk_put_function_list(ctx, -1, fileFunctions);
	duk_put_number_list(ctx, -1, fileConstants);
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, fileMethods);
	duk_put_prop_string(ctx, -2, "prototype");
	duk_put_prop_string(ctx, -2, "File");

	// irccd.fs.Directory
	char separator[] = { Filesystem::Separator, '\0' };

	duk_push_c_function(ctx, Directory_Directory, DUK_VARARGS);
	duk_put_function_list(ctx, -1, directoryFunctions);
	duk_put_number_list(ctx, -1, directoryConstants);
	duk_push_string(ctx, "Separator");
	duk_push_string(ctx, separator);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, directoryMethods);
	duk_put_prop_string(ctx, -2, "prototype");
	duk_put_prop_string(ctx, -2, "Directory");

	return 1;
}

} // !irccd

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
	inline File(std::string path, std::fstream::openmode mode, int type) noexcept
		: m_path(std::move(path))
		, m_stream(m_path, mode)
		, m_type(type)
	{
		if (!m_stream.is_open()) {
			throw std::runtime_error(std::strerror(errno));
		}
	}

	virtual ~File() = default;

	inline const std::string &path() const noexcept
	{
		return m_path;
	}

	inline int type() const noexcept
	{
		return m_type;
	}

	inline unsigned tell()
	{
		unsigned pos = static_cast<unsigned>(m_stream.tellg());

		if (!m_stream) {
			throw std::runtime_error(std::strerror(errno));
		}

		return pos;
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

int pushStat(duk_context *ctx, const struct stat &st)
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
File_prototype_basename(duk_context *ctx)
{
	dukx_with_this<File>(ctx, [&] (File *file) {
		duk_push_string(ctx, Filesystem::baseName(file->path()).c_str());
	});

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
File_prototype_dirname(duk_context *ctx)
{
	dukx_with_this<File>(ctx, [&] (File *file) {
		duk_push_string(ctx, Filesystem::dirName(file->path()).c_str());
	});

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
File_prototype_read(duk_context *ctx)
{
	dukx_with_this<File>(ctx, [&] (File *file) {
		if (file->type() == File::Output) {
			dukx_throw(ctx, -1, "file is opened for writing");
		}

		int amount = -1;

		if (duk_get_top(ctx) > 0) {
			amount = duk_require_int(ctx, 0);
		}

		try {
			duk_push_string(ctx, file->read(amount).c_str());
		} catch (const std::exception &ex) {
			dukx_throw(ctx, -1, ex.what());
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
File_prototype_remove(duk_context *ctx)
{
	dukx_with_this<File>(ctx, [&] (File *file) {
		if (remove(file->path().c_str()) < 0) {
			dukx_throw_syserror(ctx, errno);
		}
	});

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
File_prototype_seek(duk_context *)
{
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
File_prototype_stat(duk_context *ctx)
{
	dukx_with_this<File>(ctx, [&] (File *file) {
		struct stat st;
	
		if (stat(file->path().c_str(), &st) < 0) {
			dukx_throw_syserror(ctx, errno);
		}
	
		(void)pushStat(ctx, st);
	});

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
File_prototype_tell(duk_context *ctx)
{
	dukx_with_this<File>(ctx, [&] (File *file) {
		try {
			duk_push_int(ctx, file->tell());
		} catch (const std::exception &ex) {
			dukx_throw(ctx, -1, ex.what());
		}
	});

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
File_prototype_write(duk_context *ctx)
{
	const char *data = duk_require_string(ctx, 0);

	dukx_with_this<File>(ctx, [&] (File *file) {
		if (file->type() == File::Input) {
			dukx_throw(ctx, -1, "file is opened for reading");
		}

		try {
			file->write(data);
		} catch (const std::exception &ex) {
			dukx_throw(ctx, -1, ex.what());
		}
	});

	return 0;
}

constexpr const duk_function_list_entry fileMethods[] = {
	{ "basename",	File_prototype_basename,	0	},
	{ "dirname",	File_prototype_dirname,		0	},
	{ "read",	File_prototype_read,		1	},
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
 *
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

	std::ios_base::openmode mode = static_cast<std::ios_base::openmode>(0);

	for (const char *p = modestring; *p != '\0'; ++p) {
		if (*p == 'w') {
			mode |= std::ios_base::out;
		} else if (*p == 'r') {
			mode |= std::ios_base::in;
		} else if (*p == 'a') {
			mode |= (std::ios_base::app);
		}
	}

	if (((mode & std::ios_base::out) || (mode & std::ios_base::app)) && (mode & std::ios_base::in)) {
		dukx_throw(ctx, -1, "can not open for both reading and writing");
	}

	duk_push_this(ctx);

	try {
		if (mode & std::ios_base::out) {
			dukx_set_class<File>(ctx, new File(path, mode, File::Output));
		} else {
			dukx_set_class<File>(ctx, new File(path, mode, File::Input));
		}
	} catch (const std::exception &ex) {
		duk_pop(ctx);
		dukx_throw_syserror(ctx, errno);
	}

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
 *   - the basename
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
 * Return the file basename as specified in dirname(3) C function.
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

	return pushStat(ctx, st);
}

#endif // !HAVE_STAT

constexpr const duk_function_list_entry fileFunctions[] = {
	{ "basename",	File_basename,	1			},
	{ "dirname",	File_dirname,	1			},
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
 * Directory methods
 * -------------------------------------------------------- */

/*
 * Method: Directory.find(pattern, recursive)
 * --------------------------------------------------------
 *
 * TODO
 */
duk_ret_t Directory_prototype_find(duk_context *)
{
	return 0;
}

/*
 * Method: Directory.mkdir(recursive)
 * --------------------------------------------------------
 *
 * TODO
 */
duk_ret_t Directory_prototype_mkdir(duk_context *)
{
	return 0;
}

/*
 * Method: Directory.remove(recursive)
 * --------------------------------------------------------
 *
 * TODO
 */
duk_ret_t Directory_prototype_remove(duk_context *)
{
	return 0;
}

constexpr const duk_function_list_entry directoryMethods[] = {
	{ "find",		Directory_prototype_find,	1	},
	{ "mkdir",		Directory_prototype_mkdir,	2	},
	{ "remove",		Directory_prototype_remove,	1	},
	{ nullptr,		nullptr,			0	}
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
		duk_push_string(ctx, path);
		duk_put_prop_string(ctx, -2, "\xff""\xff" "path");
		duk_push_string(ctx, "count");
		duk_push_int(ctx, directory.count());
		duk_def_prop(ctx, -3, DUK_DEFPROP_ENUMERABLE | DUK_DEFPROP_HAVE_VALUE);

		// add entries
		duk_push_array(ctx);

		int i = 0;
		for (const Directory::Entry &entry : directory) {
			duk_push_object(ctx);
			duk_push_string(ctx, entry.name.c_str());
			duk_put_prop_string(ctx, -2, "name");
			duk_push_int(ctx, static_cast<int>(entry.type));
			duk_put_prop_string(ctx, -2, "type");
			duk_put_prop_index(ctx, -2, i++);
		}

		duk_put_prop_string(ctx, -2, "entries");
	} catch (const std::exception &ex) {
		dukx_throw(ctx, -1, ex.what());
	}

	return 0;
}

/*
 * Function: fs.Directory.find(path, pattern, recursive)
 * --------------------------------------------------------
 *
 * TODO
 */
duk_ret_t Directory_find(duk_context *)
{
#if 0
	const char *base = duk_require_string(ctx, 0);
	const char *pattern = duk_require_string(ctx, 1);
#endif

	return 0;
}

/*
 * Function: fs.Directory.remove(path, recursive)
 * --------------------------------------------------------
 *
 * TODO
 */
duk_ret_t Directory_remove(duk_context *)
{
	return 0;
}

/*
 * Function: fs.Directory.mkdir(path, recursive)
 * --------------------------------------------------------
 *
 * TODO
 */
duk_ret_t Directory_mkdir(duk_context *)
{
	return 0;
}

constexpr const duk_function_list_entry directoryFunctions[] = {
	{ "find",		Directory_find,		2		},
	{ "remove",		Directory_remove,	DUK_VARARGS	},
	{ "mkdir",		Directory_mkdir,	2		},
	{ nullptr,		nullptr,		0		}
};

constexpr const duk_number_list_entry directoryConstants[] = {
	{ "NoDot",		static_cast<int>(Directory::NotDot)	},
	{ "NoDotDot",		static_cast<int>(Directory::NotDotDot)	},
	{ "TypeUnknown",	static_cast<int>(Directory::Unknown)	},
	{ "TypeDir",		static_cast<int>(Directory::Dir)	},
	{ "TypeFile",		static_cast<int>(Directory::File)	},
	{ "TypeLink",		static_cast<int>(Directory::Link)	},
	{ nullptr, 		0					}
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
	duk_push_c_function(ctx, Directory_Directory, DUK_VARARGS);
	duk_put_function_list(ctx, -1, directoryFunctions);
	duk_put_number_list(ctx, -1, directoryConstants);
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, directoryMethods);
	duk_put_prop_string(ctx, -2, "prototype");
	duk_put_prop_string(ctx, -2, "Directory");

	return 1;
}

} // !irccd

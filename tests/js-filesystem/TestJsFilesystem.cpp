/*
 * TestJsFilesystem.cpp -- test irccd filesystem JS API
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

#include <fstream>
#include <sstream>

#include <gtest/gtest.h>

#include <IrccdConfig.h>

#include <Filesystem.h>
#include <js/Js.h>

using namespace irccd;

class TestJsFilesystem : public testing::Test {
protected:
	duk_context *m_ctx;

public:
	TestJsFilesystem()
	{
		m_ctx = duk_create_heap_default();

		duk_push_c_function(m_ctx, dukopen_filesystem, 0);
		duk_call(m_ctx, 0);
		duk_put_global_string(m_ctx, "fs");

		// set global BINARY dir
		duk_push_string(m_ctx, BINARY);
		duk_put_global_string(m_ctx, "BINARY");
	}

	~TestJsFilesystem()
	{
		duk_destroy_heap(m_ctx);
	}

	void checkSymbol(const std::string &name, const std::string &type)
	{
		std::ostringstream oss;
		std::string cmd;

		oss << "typeof (" << name << ") === \"" << type << "\"";
		cmd = oss.str();

		if (duk_peval_string(m_ctx, cmd.c_str())) {
			std::string msg = duk_safe_to_string(m_ctx, -1);

			duk_pop(m_ctx);

			FAIL() << "Error in command: " << msg;
		} else {
			bool result = duk_to_boolean(m_ctx, -1);
			duk_pop(m_ctx);

			if (!result) {
				FAIL() << "Missing symbol: " << name;
			}
		}
	}

	void execute(const std::string &cmd)
	{
		if (duk_peval_string(m_ctx, cmd.c_str())) {
			std::string msg = duk_safe_to_string(m_ctx, -1);

			duk_pop(m_ctx);

			FAIL() << "Error in command: " << msg;
		}
	}
};

TEST_F(TestJsFilesystem, symbols)
{
	// File functions
	checkSymbol("fs.File", "function");
	checkSymbol("fs.File.basename", "function");
	checkSymbol("fs.File.dirname", "function");
	checkSymbol("fs.File.exists", "function");
	checkSymbol("fs.File.remove", "function");
#if defined(HAVE_STAT)
	checkSymbol("fs.File.stat", "function");
#endif

	// File object
	checkSymbol("fs.File.prototype.basename", "function");
	checkSymbol("fs.File.prototype.dirname", "function");
	checkSymbol("fs.File.prototype.read", "function");
	checkSymbol("fs.File.prototype.remove", "function");
	checkSymbol("fs.File.prototype.seek", "function");
#if defined(HAVE_STAT)
	checkSymbol("fs.File.prototype.stat", "function");
#endif
	checkSymbol("fs.File.prototype.tell", "function");
	checkSymbol("fs.File.prototype.write", "function");

	// File constants
	checkSymbol("fs.File.SeekSet", "number");
	checkSymbol("fs.File.SeekCur", "number");
	checkSymbol("fs.File.SeekEnd", "number");

	// Directory functions
	checkSymbol("fs.Directory.find", "function");
	checkSymbol("fs.Directory.mkdir", "function");
	checkSymbol("fs.Directory.remove", "function");

	// Directory object
	checkSymbol("fs.Directory.prototype.find", "function");
	checkSymbol("fs.Directory.prototype.mkdir", "function");
	checkSymbol("fs.Directory.prototype.remove", "function");

	// Directory constants
	checkSymbol("fs.Directory.Dot", "number");
	checkSymbol("fs.Directory.DotDot", "number");
	checkSymbol("fs.Directory.TypeUnknown", "number");
	checkSymbol("fs.Directory.TypeDir", "number");
	checkSymbol("fs.Directory.TypeFile", "number");
	checkSymbol("fs.Directory.TypeLink", "number");
	checkSymbol("fs.Directory.Separator", "string");
}

TEST_F(TestJsFilesystem, basename)
{
	execute("fs.File.basename(\"/usr/local/etc/irccd.conf\");");

	ASSERT_EQ(DUK_TYPE_STRING, duk_get_type(m_ctx, -1));
	ASSERT_STREQ("irccd.conf", duk_get_string(m_ctx, -1));
}

TEST_F(TestJsFilesystem, dirname)
{
	execute("fs.File.dirname(\"/usr/local/etc/irccd.conf\");");

	ASSERT_EQ(DUK_TYPE_STRING, duk_get_type(m_ctx, -1));
	ASSERT_STREQ("/usr/local/etc", duk_get_string(m_ctx, -1));
}

TEST_F(TestJsFilesystem, exists)
{
	execute("fs.File.exists(BINARY + \"/testassets/file.txt\")");

	ASSERT_EQ(DUK_TYPE_BOOLEAN, duk_get_type(m_ctx, -1));
	ASSERT_TRUE(duk_to_boolean(m_ctx, -1));
}

TEST_F(TestJsFilesystem, notExists)
{
	execute("fs.File.exists(BINARY + \"file_does_not_exist\")");

	ASSERT_EQ(DUK_TYPE_BOOLEAN, duk_get_type(m_ctx, -1));
	ASSERT_FALSE(duk_to_boolean(m_ctx, -1));
}

TEST_F(TestJsFilesystem, remove)
{
	// First create a dummy file
	{
		std::ofstream out("test-js-fs.remove");
	}

	execute("fs.File.remove(\"test-js-fs.remove\");");

	std::ifstream in("test-js-fs.remove");

	ASSERT_FALSE(in.is_open());
}

TEST_F(TestJsFilesystem, methodBasename)
{
	execute("(new fs.File(BINARY + \"/testassets/level-1/file-1.txt\", \"r\")).basename()");

	ASSERT_EQ(DUK_TYPE_STRING, duk_get_type(m_ctx, -1));
	ASSERT_STREQ("file-1.txt", duk_get_string(m_ctx, -1));
}

TEST_F(TestJsFilesystem, methodDirname)
{
	std::string directory = std::string(BINARY) + "/testassets/level-1";

	execute("(new fs.File(BINARY + \"/testassets/level-1/file-1.txt\", \"r\")).dirname()");

	ASSERT_EQ(DUK_TYPE_STRING, duk_get_type(m_ctx, -1));
	ASSERT_EQ(directory, duk_get_string(m_ctx, -1));
}

TEST_F(TestJsFilesystem, methodSeek1)
{
	execute(
		"var f = new fs.File(BINARY + \"/testassets/file.txt\", \"r\");"
		"f.seek(fs.File.SeekSet, 4);"
		"f.read(1);"
	);

	ASSERT_EQ(DUK_TYPE_STRING, duk_get_type(m_ctx, -1));
	ASSERT_STREQ(".", duk_to_string(m_ctx, -1));
}

TEST_F(TestJsFilesystem, methodSeek2)
{
	execute(
		"var f = new fs.File(BINARY + \"/testassets/file.txt\", \"r\");"
		"f.seek(fs.File.SeekSet, 2);"
		"f.seek(fs.File.SeekCur, 2);"
		"f.read(1);"
	);

	ASSERT_EQ(DUK_TYPE_STRING, duk_get_type(m_ctx, -1));
	ASSERT_STREQ(".", duk_to_string(m_ctx, -1));
}

TEST_F(TestJsFilesystem, methodSeek3)
{
	execute(
		"var f = new fs.File(BINARY + \"/testassets/file.txt\", \"r\");"
		"f.seek(fs.File.SeekEnd, -2);"
		"f.read(1);"
	);

	ASSERT_EQ(DUK_TYPE_STRING, duk_get_type(m_ctx, -1));
	ASSERT_STREQ("x", duk_to_string(m_ctx, -1));
}

TEST_F(TestJsFilesystem, directoryCount)
{
	execute("(new fs.Directory(BINARY + \"/testassets/level-1\")).count");

	ASSERT_EQ(DUK_TYPE_NUMBER, duk_get_type(m_ctx, -1));
	ASSERT_EQ(2, duk_get_int(m_ctx, -1));
}

TEST_F(TestJsFilesystem, directoryCount2)
{
	execute("(new fs.Directory(BINARY + \"/testassets/level-1\", fs.Directory.Dot)).count");

	ASSERT_EQ(DUK_TYPE_NUMBER, duk_get_type(m_ctx, -1));
	ASSERT_EQ(3, duk_get_int(m_ctx, -1));
}

TEST_F(TestJsFilesystem, directoryCount3)
{
	execute("(new fs.Directory(BINARY + \"/testassets/level-1\", fs.Directory.Dot | fs.Directory.DotDot)).count");

	ASSERT_EQ(DUK_TYPE_NUMBER, duk_get_type(m_ctx, -1));
	ASSERT_EQ(4, duk_get_int(m_ctx, -1));
}

TEST_F(TestJsFilesystem, directoryFind1)
{
	// Not recursive
	execute("fs.Directory.find(BINARY + \"/testassets\", \"file.txt\", false)");

	ASSERT_EQ(DUK_TYPE_STRING, duk_get_type(m_ctx, -1));
	ASSERT_STREQ("file.txt", duk_to_string(m_ctx, -1));
}

TEST_F(TestJsFilesystem, directoryFind2)
{
	execute("fs.Directory.find(BINARY + \"/testassets\", \"file-1.txt\", true)");

	ASSERT_EQ(DUK_TYPE_STRING, duk_get_type(m_ctx, -1));
	ASSERT_STREQ("level-1/file-1.txt", duk_to_string(m_ctx, -1));
}

TEST_F(TestJsFilesystem, directoryFind3)
{
	// Like directoryFind2 but using a regex
	execute("fs.Directory.find(BINARY + \"/testassets/level-1/level-2\", /^file-[0-9]\\.txt$/, true)");

	ASSERT_EQ(DUK_TYPE_STRING, duk_get_type(m_ctx, -1));
	ASSERT_STREQ("file-2.txt", duk_to_string(m_ctx, -1));
}

TEST_F(TestJsFilesystem, directoryMkdir)
{
	execute("fs.Directory.mkdir(BINARY + \"/testassets/tmpdir\")");

	ASSERT_TRUE(Filesystem::exists(std::string(BINARY) + "/testassets/tmpdir"));

	if (::remove(BINARY "/testassets/tmpdir") < 0) {
		FAIL() << "Failed to remove testassets/tmpdir directory";
	}
}

TEST_F(TestJsFilesystem, directoryRemove1)
{
	// not recursive
	execute(
		"fs.Directory.mkdir(BINARY + \"/testassets/tmpdir\");"
		"fs.Directory.remove(BINARY + \"/testassets/tmpdir\", false);"
	);

	ASSERT_FALSE(Filesystem::exists(BINARY "/testassets/tmpdir"));
}

TEST_F(TestJsFilesystem, directoryRemove2)
{
	execute(
		"fs.Directory.mkdir(BINARY + \"/testassets/tmpdir1/tmpdir2\");"
		"fs.Directory.remove(BINARY + \"/testassets/tmpdir1\", true);"
	);

	ASSERT_FALSE(Filesystem::exists(BINARY "/testassets/tmpdir1"));
}

TEST_F(TestJsFilesystem, directoryMethodFind1)
{
	// Not recursive
	execute("(new fs.Directory(BINARY + \"/testassets\")).find(\"file.txt\", false)");

	ASSERT_EQ(DUK_TYPE_STRING, duk_get_type(m_ctx, -1));
	ASSERT_STREQ("file.txt", duk_to_string(m_ctx, -1));
}

TEST_F(TestJsFilesystem, directoryMethodFind2)
{
	execute("(new fs.Directory(BINARY + \"/testassets\")).find(\"file-1.txt\", true)");

	ASSERT_EQ(DUK_TYPE_STRING, duk_get_type(m_ctx, -1));
	ASSERT_STREQ("level-1/file-1.txt", duk_to_string(m_ctx, -1));
}

TEST_F(TestJsFilesystem, directoryMethodFind3)
{
	// Like directoryFind2 but using a regex
	execute("(new fs.Directory(BINARY + \"/testassets/level-1/level-2\")).find(/^file-[0-9]\\.txt$/, true)");

	ASSERT_EQ(DUK_TYPE_STRING, duk_get_type(m_ctx, -1));
	ASSERT_STREQ("file-2.txt", duk_to_string(m_ctx, -1));
}

TEST_F(TestJsFilesystem, directoryMethodMkdir)
{
#if 0
	execute("(new Directory.mkdir(BINARY + \"/testassets/\")");

	ASSERT_TRUE(Filesystem::exists(std::string(BINARY) + "/testassets/tmpdir"));

	if (::remove(BINARY "/testassets/tmpdir") < 0) {
		FAIL() << "Failed to remove testassets/tmpdir directory";
	}
#endif
}

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}

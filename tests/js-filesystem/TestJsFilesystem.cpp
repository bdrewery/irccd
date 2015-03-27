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
	checkSymbol("fs.File", "function");
	checkSymbol("fs.File.basename", "function");
	checkSymbol("fs.File.dirname", "function");
	checkSymbol("fs.File.remove", "function");
#if defined(HAVE_STAT)
	checkSymbol("fs.File.stat", "function");
#endif
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
	checkSymbol("fs.File.SeekSet", "number");
	checkSymbol("fs.File.SeekCur", "number");
	checkSymbol("fs.File.SeekEnd", "number");

	// TODO: fs.Directory.
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
#if 0
	// TODO: this need to create a directory
	execute("var f = new fs.File(\"/usr/local/etc/irccd.conf\", \"r\"); f.basename();");

	ASSERT_EQ(DUK_TYPE_STRING, duk_get_type(m_ctx, -1));
	ASSERT_STREQ("irccd.conf", duk_get_string(m_ctx, -1));
#endif
}

TEST_F(TestJsFilesystem, methodDirname)
{
#if 0
	execute("var f = new fs.File(\"/usr/local/etc/irccd.conf\", \"r\"); f.dirname();");

	ASSERT_EQ(DUK_TYPE_STRING, duk_get_type(m_ctx, -1));
	ASSERT_STREQ("/usr/local/etc", duk_get_string(m_ctx, -1));
#endif
}

TEST_F(TestJsFilesystem, directoryCount)
{
#if 0
	execute("var d = new fs.Directory(\".\"); for (var i = 0; i < d.count; ++i) { print(d.entries[i].name); }");

	int i = duk_get_int(m_ctx, -1);

	printf("count: %d\n", i);
#endif
}

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}

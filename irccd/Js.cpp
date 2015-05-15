/*
 * Js.cpp -- JS API for irccd and Duktape helpers
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

#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <unordered_map>

#include <IrccdConfig.h>

#include <Filesystem.h>
#include <Util.h>

#include "Js.h"

using namespace std::string_literals;

namespace irccd {

/* --------------------------------------------------------
 * JsDuktape
 * -------------------------------------------------------- */

JsDuktape &JsDuktape::self(duk_context *ctx) noexcept
{
	dukx_assert_begin(ctx);
	duk_get_global_string(ctx, "\xff""\xff""irccd-js-instance");
	JsDuktape &instance = *static_cast<JsDuktape *>(duk_to_pointer(ctx, -1));
	duk_pop(ctx);
	dukx_assert_equals(ctx);

	return instance;
}

std::string JsDuktape::parent(JsDuktape &ctx) noexcept
{
	dukx_assert_begin(ctx);
	duk_get_global_string(ctx, "\xff""\xff""parent");
	std::string path = duk_to_string(ctx, -1);
	duk_pop(ctx);
	dukx_assert_equals(ctx);

	return path;
}

void JsDuktape::loadFunction(JsDuktape &ctx, duk_c_function fn)
{
	dukx_assert_begin(ctx);
	duk_push_c_function(ctx, fn, 1);
	duk_call(ctx, 0);
	dukx_assert_end(ctx, 1);
}

void JsDuktape::loadPlain(JsDuktape &ctx, const std::string &path)
{
	std::ifstream file(path, std::ifstream::in);
	std::string content(std::istreambuf_iterator<char>(file.rdbuf()), std::istreambuf_iterator<char>());

	dukx_assert_begin(ctx);
	duk_push_string(ctx, content.c_str());
	dukx_assert_end(ctx, 1);
}

#if defined(WITH_JS_EXTENSION)

void JsDuktape::loadNative(JsDuktape &ctx, std::string ident, const std::string &path)
{
	using Load = duk_ret_t (*)(duk_context *ctx);

	/* Build the load string dukopen_foo */
	std::string base = Filesystem::baseName(ident);

	dukx_assert_begin(ctx);
	try {
		auto dso = std::make_unique<Dynlib>(path);
		auto load = dso->sym<Load>("dukopen_"s + base);

		load(ctx);

		ctx.m_modules.push_back(std::move(dso));
	} catch (const std::exception &ex) {
		dukx_throw(ctx, -1, "failed to load: "s + ex.what());
	}
	dukx_assert_end(ctx, 1);
}

#endif

duk_ret_t JsDuktape::modSearch(duk_context *ctx)
{
	static const std::unordered_map<std::string, duk_c_function> modules{
		{ "irccd.fs",		dukopen_filesystem	},
		{ "irccd.logger",	dukopen_logger		},
		{ "irccd.timer",	dukopen_timer		},
		{ "irccd.server",	dukopen_server		},
		{ "irccd.system",	dukopen_system		},
		{ "irccd.unicode",	dukopen_unicode		},
		{ "irccd.util",		dukopen_util		}
	};

	auto id = duk_require_string(ctx, 0);
	auto it = modules.find(id);
	bool extract = true;

	if (it != modules.end()) {
		loadFunction(self(ctx), it->second);
	} else {
		/* First, check from the parent plugin directory */
		std::string path = parent(self(ctx)) + Filesystem::Separator + std::string(id);
		std::string jspath = path + ".js";
		std::string nativepath = path + WITH_JS_EXTENSION;

		if (Filesystem::exists(jspath)) {
			loadPlain(self(ctx), jspath);
			extract = false;
		}
/*
 * TODO: can't be used unless we split irccd into libraries
 */
#if 0
#if defined(WITH_JS_EXTENSION)
		else if (Filesystem::exists(nativepath)) {
			loadNative(self(ctx), id, nativepath);
		}
#endif
#endif
	}

	if (extract) {
		/* Now the returned table contains things to export */
		duk_enum(ctx, -1, DUK_ENUM_INCLUDE_NONENUMERABLE);
		while (duk_next(ctx, -1, 1)) {
			duk_put_prop(ctx, 2);
		}
	}

	return 1;
}

duk_ret_t JsDuktape::use(duk_context *)
{
#if 0
	auto module = jsLoad(ctx);

	/* Call and verify */
	duk_push_global_object(ctx);
	module->load(ctx);

	if (!duk_get_type(ctx, -1)) {
		dukx_throw(ctx, -1, "module does not export anything");
	}

	/* Enumerate and set global */
	duk_enum(ctx, -1, DUK_ENUM_INCLUDE_NONENUMERABLE);

	while (duk_next(ctx, -1, 1)) {
		duk_put_prop(ctx, -5);
	}

	jsSelf(ctx).m_modules.emplace(module->name(), std::move(module));
#endif

	return 0;
}

JsDuktape::JsDuktape()
	: std::unique_ptr<duk_context, void (*)(duk_context *)>(duk_create_heap_default(), duk_destroy_heap)
{
	dukx_assert_begin(get());

	/* Save a reference to this */
	duk_push_global_object(get());
	duk_push_pointer(get(), this);
	duk_put_prop_string(get(), -2, "\xff""\xff" "irccd-js-instance");
	duk_pop(get());

	/* Set our "using" keyword */
	duk_push_c_function(get(), &JsDuktape::use, 1);
	duk_put_global_string(get(), "using");

	/* Set Duktape.modSearch */
	duk_get_global_string(get(), "Duktape");
	duk_push_c_function(get(), &JsDuktape::modSearch, 4);
	duk_put_prop_string(get(), -2, "modSearch");
	duk_pop(get());

#if 0
	/* Disable alert, print */
	duk_push_undefined(get());
	duk_put_global_string(get(), "alert");
	duk_push_undefined(get());
	duk_put_global_string(get(), "print");
#endif

	/* This is needed for timers */
	duk_push_global_object(get());
	duk_push_object(get());
	duk_put_prop_string(get(), -2, "\xff" "irccd-timers");
	duk_pop(get());

	/* This is needed for storing prototypes */
	duk_push_global_object(get());
	duk_push_object(get());
	duk_put_prop_string(get(), -2, "\xff" "irccd-proto");
	duk_pop(get());

	/* This is the server object, allocated from here */
	dukpreload_server(get());

	dukx_assert_equals(get());
}

void dukx_throw_syserror(duk_context *ctx, int code)
{
	duk_push_object(ctx);
	duk_push_int(ctx, code);
	duk_put_prop_string(ctx, -2, "code");
	duk_push_string(ctx, std::strerror(code));
	duk_put_prop_string(ctx, -2, "message");
	duk_throw(ctx);
}

void dukx_throw(duk_context *ctx, int code, const std::string &msg)
{
	duk_push_object(ctx);
	duk_push_int(ctx, code);
	duk_put_prop_string(ctx, -2, "code");
	duk_push_string(ctx, msg.c_str());
	duk_put_prop_string(ctx, -2, "message");
	duk_throw(ctx);
}

JsError dukx_get_error(duk_context *ctx)
{
	JsError error;

	dukx_assert_begin(ctx);
	duk_get_prop_string(ctx, -1, "name");
	error.name = duk_to_string(ctx, -1);
	duk_get_prop_string(ctx, -2, "message");
	error.error = duk_to_string(ctx, -1);
	duk_get_prop_string(ctx, -3, "fileName");
	error.source = duk_to_string(ctx, -1);
	duk_get_prop_string(ctx, -4, "lineNumber");
	error.lineNumber = duk_to_int(ctx, -1);
	duk_get_prop_string(ctx, -5, "stack");
	error.stack = duk_to_string(ctx, -1);
	duk_pop_n(ctx, 5);
	dukx_assert_equals(ctx);

	return error;
}

} // !irccd

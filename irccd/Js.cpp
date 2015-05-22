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
	duk_get_global_string(ctx, "\xff""\xff""irccd-parent");
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

void JsDuktape::loadLocal(JsDuktape &ctx, const std::string &path)
{
	std::ifstream file(path, std::ifstream::in);

	if (!file) {
		duk_push_error_object(ctx, DUK_ERR_TYPE_ERROR, "Module not found: %s", path.c_str());
		duk_throw(ctx);
	}

	std::string content(std::istreambuf_iterator<char>(file.rdbuf()), std::istreambuf_iterator<char>());

	dukx_assert_begin(ctx);
	duk_push_string(ctx, content.c_str());
	dukx_assert_end(ctx, 1);
}

#if 0
void JsDuktape::loadPlugin(JsDuktape &ctx, const std::string &path)
{
	// TODO
}
#endif

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
		duk_error(ctx, DUK_ERR_ERROR, "failed to load native module: %s",  ex.what());
	}
	dukx_assert_end(ctx, 1);
}

#endif

/*
 * Duktape.modSearch
 *
 * This function is only used when searching local files (.e.g require("./api")),
 * so it only supports .js files.
 */
duk_ret_t JsDuktape::modSearch(duk_context *ctx)
{
	const auto id = duk_require_string(ctx, 0);
	const auto path = parent(self(ctx));

	loadLocal(self(ctx), path + Filesystem::Separator + std::string(id) + ".js");

	return 1;
}

/*
 * Local require: require("./file")
 *
 * This function use the real Duktape's require implementation with the
 * associated Duktape.modSearch function to recursively 
 */
void JsDuktape::requireLocal(JsDuktape &ctx, const std::string &name)
{
	duk_get_global_string(ctx, "\xff""\xff""Duktape-require");
	duk_push_string(ctx, name.c_str());
	duk_call(ctx, 1);
}

/*
 * Plugin require: require(":plugin-name")
 *
 * This is the function to load API from a plugin. The plugin must be loaded
 * otherwise an exception is thrown.
 */
void JsDuktape::requirePlugin(JsDuktape &ctx, const std::string &name)
{
	// TODO: implement when plugin API export is ready.
	(void)ctx;
	(void)name;
}

std::stack<std::string> JsDuktape::m_paths;

/*
 * Global require: require("foo")
 *
 * This is also the one that is called when loading irccd modules, in the form
 * require("irccd.foo"), otherwise, the path is specified like in C,
 * require("foo/bar").
 */
void JsDuktape::requireGlobal(JsDuktape &ctx, const std::string &name)
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

	auto it = modules.find(name);
	if (it != modules.end()) {
		loadFunction(self(ctx), it->second);
	} else {
		// TODO: search for global .js and .<ext>
	}
}

/*
 * Require is modified to understand different formats:
 *
 * require("foo") -> search for native/plain foo in irccd directories
 * require("./foo") -> search for foo.js locally to the current module
 * require(":foo") -> import foo plugin API
 */
duk_ret_t JsDuktape::require(duk_context *ctx)
{
	const char *path = duk_require_string(ctx, 0);

	if (path[0] == '.' && path[1] == '/') {
		requireLocal(self(ctx), path + 2);
	} else if (path[0] == ':') {
		requirePlugin(self(ctx), path + 1);
	} else {
		requireGlobal(self(ctx), path);
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

JsDuktape::JsDuktape(const std::string &path)
	: std::unique_ptr<duk_context, void (*)(duk_context *)>(duk_create_heap_default(), duk_destroy_heap)
{
	dukx_assert_begin(get());

	/* Set the parent path */
	duk_push_string(get(), path.c_str());
	duk_put_global_string(get(), "\xff""\xff""irccd-parent");

	/* Save a reference to this */
	duk_push_global_object(get());
	duk_push_pointer(get(), this);
	duk_put_prop_string(get(), -2, "\xff""\xff" "irccd-js-instance");
	duk_pop(get());

	/* Set our "using" keyword */
	duk_push_c_function(get(), &JsDuktape::use, 1);
	duk_put_global_string(get(), "using");

	/* Replace the "require" function, but save it to reuse it */
	duk_get_global_string(get(), "require");
	duk_put_global_string(get(), "\xff""\xff""Duktape-require");
	duk_push_c_function(get(), &JsDuktape::require, 1);
	duk_put_global_string(get(), "require");

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

	/* SystemError (dummy) */
	duk_push_c_function(get(), [] (duk_context *) -> duk_ret_t { return 0; }, 0);
	duk_get_global_string(get(), "Error");
	duk_get_prop_string(get(), -1, "prototype");
	duk_put_prop_string(get(), -3, "prototype");
	duk_pop(get());
	duk_put_global_string(get(), "SystemError");

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

JsError dukx_error(duk_context *ctx, duk_idx_t index)
{
	JsError error;

	index = duk_normalize_index(ctx, index);

	dukx_assert_begin(ctx);
	duk_get_prop_string(ctx, index, "name");
	error.name = duk_to_string(ctx, -1);
	duk_get_prop_string(ctx, index, "message");
	error.message = duk_to_string(ctx, -1);
	duk_get_prop_string(ctx, index, "fileName");
	error.fileName = duk_to_string(ctx, -1);
	duk_get_prop_string(ctx, index, "lineNumber");
	error.lineNumber = duk_to_int(ctx, -1);
	duk_get_prop_string(ctx, index, "stack");
	error.stack = duk_to_string(ctx, -1);
	duk_pop_n(ctx, 5);
	dukx_assert_equals(ctx);

	return error;
}

} // !irccd

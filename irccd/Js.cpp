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
#include <unordered_map>

#include <IrccdConfig.h>

#include <Filesystem.h>
#include <Util.h>

#include "Js.h"

namespace irccd {

/* --------------------------------------------------------
 * JsModuleNative implementation
 * -------------------------------------------------------- */

JsModuleNative::JsModuleNative(std::string name, std::string path)
	: JsModule(std::move(name))
	, m_library(std::move(path), Dynlib::Immediately)
{
	// TODO: m_load
}

void JsModuleNative::load(duk_context *ctx)
{
	m_load(ctx);
}

/* --------------------------------------------------------
 * JsModuleIrccd implementation
 * -------------------------------------------------------- */

JsModuleIrccd::JsModuleIrccd(std::string name, duk_c_function func)
	: JsModule(std::move(name))
	, m_function(func)
{
}

void JsModuleIrccd::load(duk_context *ctx)
{
	duk_push_c_function(ctx, m_function, 1);
	duk_call(ctx, 0);
}

/* --------------------------------------------------------
 * JsModuleStandard implementation
 * -------------------------------------------------------- */

JsModuleStandard::JsModuleStandard(std::string name, std::string path)
	: JsModule(std::move(name))
	, m_path(std::move(path))
{
}

void JsModuleStandard::load(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	duk_eval_file_noresult(ctx, m_path.c_str());
	duk_get_global_string(ctx, "exports");
	dukx_assert_end(ctx, 1);
}

/* --------------------------------------------------------
 * JsDuktape
 * -------------------------------------------------------- */

JsDuktape &JsDuktape::jsSelf(duk_context *ctx) noexcept
{
	dukx_assert_begin(ctx);
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "\xff" "irccd-js-instance");
	JsDuktape &instance = *static_cast<JsDuktape *>(duk_to_pointer(ctx, -1));
	duk_pop_2(ctx);
	dukx_assert_equals(ctx);

	return instance;
}

std::string JsDuktape::jsReplace(std::string name) noexcept
{
	std::replace(name.begin(), name.end(), '.', Filesystem::Separator);

	return name;
}

std::unique_ptr<JsModule> JsDuktape::jsLoad(duk_context *ctx)
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
	
	const char *name = duk_require_string(ctx, 0);

	/*
	 * Zero, check that it's not already loaded.
	 */
	if (jsSelf(ctx).m_modules.count(name) != 0) {
		dukx_throw(ctx, -1, "module already loaded");
	}

	/*
	 * First, check if the module is a predefined irccd module.
	 */
	auto it = modules.find(name);
	if (it != modules.end()) {
		return std::make_unique<JsModuleIrccd>(name, it->second);
	}

	/*
	 * Second, check for each path if a file with the name translated to a 
	 * real path ends with .js or .suffix where the suffix
	 * depends on the system.
	 *
	 * First check for .js because the dynamic loading is not supported
	 * everywhere.
	 */

	/* Convert . to filesystem / or \ */
	std::string modname = jsReplace(name);
	std::vector<std::string> tries;

	for (const std::string &path : Util::pathsPlugins()) {
		/* path/a.b -> path/a/b */
		std::string fullpath = path + Filesystem::Separator + modname;

		/* JavaScript plugin */
		std::string jspath = fullpath + ".js";
		if (Filesystem::exists(fullpath)) {
			return std::make_unique<JsModuleStandard>(name, jspath);
		}

		tries.push_back(std::move(jspath));

#if defined(WITH_JS_EXTENSION)
		/* Dynamic loading */
		std::string nativepath = fullpath + WITH_JS_EXTENSION;
		if (Filesystem::exists(nativepath)) {
			return std::make_unique<JsModuleNative>(name, nativepath);
		}

		tries.push_back(std::move(nativepath));
#endif
	}

	dukx_throw(ctx, -1, "could not find module");

	/* dukx_throw does not return, but make the compiler happy */
	return nullptr;
}

/*
 * irccd's implementation of using()
 */
duk_ret_t JsDuktape::jsUsing(duk_context *ctx)
{
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

	return 0;
}

/*
 * irccd's implementation of require()
 */
duk_ret_t JsDuktape::jsRequire(duk_context *ctx)
{
	auto module = jsLoad(ctx);

	module->load(ctx);

	jsSelf(ctx).m_modules.emplace(module->name(), std::move(module));

	return 1;
}

JsDuktape::JsDuktape(Plugin &plugin)
	: std::unique_ptr<duk_context, void (*)(duk_context *)>(duk_create_heap_default(), duk_destroy_heap)
	, m_plugin(plugin)
{
	dukx_assert_begin(get());

	/* Save a reference to this */
	duk_push_global_object(get());
	duk_push_pointer(get(), this);
	duk_put_prop_string(get(), -2, "\xff" "irccd-js-instance");
	duk_pop(get());

	/* Set our "using" and "require" keyword */
	duk_push_c_function(get(), &JsDuktape::jsUsing, 1);
	duk_put_global_string(get(), "using");
	duk_push_c_function(get(), &JsDuktape::jsRequire, 1);
	duk_put_global_string(get(), "require");

	/* Disable alert, print */
	duk_push_undefined(get());
	duk_put_global_string(get(), "alert");
	duk_push_undefined(get());
	duk_put_global_string(get(), "print");

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

} // !irccd

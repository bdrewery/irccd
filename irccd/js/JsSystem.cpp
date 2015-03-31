/*
 * JsSystem.cpp -- system inspection for irccd JS API
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
#include <chrono>
#include <cstring>
#include <thread>

#include <IrccdConfig.h>
#include <System.h>

#include "Js.h"

namespace irccd {

namespace {

duk_ret_t System_env(duk_context *ctx)
{
	const char *key = duk_require_string(ctx, 0);
	const char *value = std::getenv(key);

	if (value == nullptr) {
		dukx_throw_syserror(ctx, errno);
	}

	duk_push_string(ctx, value);

	return 1;
}

duk_ret_t System_home(duk_context *ctx)
{
	duk_push_string(ctx, System::home().c_str());

	return 1;
}

duk_ret_t System_name(duk_context *ctx)
{
	duk_push_string(ctx, System::name().c_str());

	return 1;
}

duk_ret_t System_ticks(duk_context *ctx)
{
	duk_push_int(ctx, System::ticks());

	return 1;
}

duk_ret_t System_sleep(duk_context *ctx)
{
	std::this_thread::sleep_for(std::chrono::seconds(duk_require_int(ctx, 0)));

	return 0;
}

duk_ret_t System_usleep(duk_context *ctx)
{
	std::this_thread::sleep_for(std::chrono::microseconds(duk_require_int(ctx, 0)));

	return 0;
}

duk_ret_t System_version(duk_context *ctx)
{
	duk_push_string(ctx, System::version().c_str());

	return 1;
}

const duk_function_list_entry functions[] = {
	{ "env",	System_env,	1	},
	{ "home",	System_home,	0	},
	{ "name",	System_name,	0	},
	{ "ticks",	System_ticks,	0	},
	{ "sleep",	System_sleep,	1	},
	{ "usleep",	System_usleep,	1	},
	{ "version",	System_version,	0	},
	{ nullptr,	nullptr,	0	}
};

} // !namespace

duk_ret_t dukopen_system(duk_context *ctx) noexcept
{
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, functions);

	return 1;
}

} // !irccd

/*
 * JsLogger.cpp -- logging routines for irccd JS API
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

#include <Logger.h>

#include "Js.h"

namespace irccd {

namespace {

int print(duk_context *ctx, std::ostream &out)
{
	/*
	 * Get the message before we start printing stuff to avoid
	 * empty lines.
	 */
	const char *message = duk_require_string(ctx, 0);
	const char *pname = "todo";

	out << "plugin " << pname << ": " << message << std::endl;

	return 0;
}

duk_ret_t Logger_info(duk_context *ctx)
{
	return print(ctx, Logger::info());
}

duk_ret_t Logger_warning(duk_context *ctx)
{
	return print(ctx, Logger::warning());
}

duk_ret_t Logger_debug(duk_context *ctx)
{
	return print(ctx, Logger::debug());
}

const duk_function_list_entry loggerFunctions[] = {
	{ "info",	Logger_info,	1	},
	{ "warning",	Logger_warning,	1	},
	{ "debug",	Logger_debug,	1	},
	{ nullptr,	nullptr,	0	}
};

} // !namespace

duk_ret_t dukopen_logger(duk_context *ctx) noexcept
{
	dukx_assert_begin(ctx);
	duk_push_object(ctx);
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, loggerFunctions);
	duk_put_prop_string(ctx, -2, "Logger");
	dukx_assert_end(ctx, 1);

	return 1;
}

} // !irccd

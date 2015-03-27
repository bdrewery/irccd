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

#include "Js.h"

namespace irccd {

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

/*
 * JsUtf8.cpp -- UTF-8 manipulation for irccd JS API
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

#include "Js.h"
#include "Unicode.h"

namespace irccd {

namespace {

/*
 * Function: Unicode.isDigit(code)
 * --------------------------------------------------------
 *
 * Arguments:
 *   - code, the code point
 * Returns:
 *   - true if the code is in the digit category
 */
duk_ret_t Unicode_isDigit(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	duk_push_boolean(ctx, Unicode::isdigit(duk_require_int(ctx, 0)));
	dukx_assert_end(ctx, 1);

	return 1;
}

/*
 * Function: Unicode.isLetter(code)
 * --------------------------------------------------------
 *
 * Arguments:
 *   - code, the code point
 * Returns:
 *   - true if the code is in the letter category
 */
duk_ret_t Unicode_isLetter(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	duk_push_boolean(ctx, Unicode::isalpha(duk_require_int(ctx, 0)));
	dukx_assert_end(ctx, 1);

	return 1;
}

/*
 * Function: Unicode.isLower(code)
 * --------------------------------------------------------
 *
 * Arguments:
 *   - code, the code point
 * Returns:
 *   - true if the code is lower case
 */
duk_ret_t Unicode_isLower(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	duk_push_boolean(ctx, Unicode::islower(duk_require_int(ctx, 0)));
	dukx_assert_end(ctx, 1);

	return 1;
}

/*
 * Function: Unicode.isSpace(code)
 * --------------------------------------------------------
 *
 * Arguments:
 *   - code, the code point
 * Returns:
 *   - true if the code is in the space category
 */
duk_ret_t Unicode_isSpace(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	duk_push_boolean(ctx, Unicode::isspace(duk_require_int(ctx, 0)));
	dukx_assert_end(ctx, 1);

	return 1;
}

/*
 * Function: Unicode.isTitle(code)
 * --------------------------------------------------------
 *
 * Arguments:
 *   - code, the code point
 * Returns:
 *   - true if the code is title case
 */
duk_ret_t Unicode_isTitle(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	duk_push_boolean(ctx, Unicode::istitle(duk_require_int(ctx, 0)));
	dukx_assert_end(ctx, 1);

	return 1;
}

/*
 * Function: Unicode.isUpper(code)
 * --------------------------------------------------------
 *
 * Arguments:
 *   - code, the code point
 * Returns:
 *   - true if the code is upper case
 */
duk_ret_t Unicode_isUpper(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	duk_push_boolean(ctx, Unicode::isupper(duk_require_int(ctx, 0)));
	dukx_assert_end(ctx, 1);
	return 1;
}

const duk_function_list_entry unicodeFunctions[] = {
	{ "isDigit",		Unicode_isDigit,	1	},
	{ "isLetter",		Unicode_isLetter,	1	},
	{ "isLower",		Unicode_isLower,	1	},
	{ "isSpace",		Unicode_isSpace,	1	},
	{ "isTitle",		Unicode_isTitle,	1	},
	{ "isUpper",		Unicode_isUpper,	1	},
	{ nullptr,		nullptr,		0	}
};

} // !namespace

duk_ret_t dukopen_unicode(duk_context *ctx) noexcept
{
	dukx_assert_begin(ctx);
	duk_push_object(ctx);
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, unicodeFunctions);
	duk_put_prop_string(ctx, -2, "Unicode");
	dukx_assert_end(ctx, 1);

	return 1;
}

} // !irccd

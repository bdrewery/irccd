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

#include <Utf8.h>

#include "Js.h"

namespace irccd {

namespace {

enum class ConvertMode {
	ToUpper,
	ToLower
};

std::u32string getArray(duk_context *ctx)
{
	std::u32string str;

	dukx_assert_begin(ctx);
	duk_require_type_mask(ctx, 0, DUK_TYPE_MASK_OBJECT);
	duk_enum(ctx, 0, DUK_ENUM_ARRAY_INDICES_ONLY);

	while (duk_next(ctx, -1, 1)) {
		str.push_back(static_cast<char32_t>(duk_to_uint(ctx, -1)));
		duk_pop_2(ctx);
	}

	duk_pop(ctx);
	dukx_assert_equals(ctx);

	return str;
}

int pushArray(duk_context *ctx, std::u32string array)
{
	duk_push_array(ctx);

	for (unsigned i = 0; i < array.length(); ++i) {
		duk_push_int(ctx, array[i]);
		duk_put_prop_index(ctx, -2, i);
	}

	return 1;
}

int convert(duk_context *ctx, ConvertMode mode)
{
	/*
	 * This function can convert both UTF-32 and UTF-8 strings.
	 */
	try {
		if (duk_get_type(ctx, 0) == DUK_TYPE_OBJECT) {
			if (mode == ConvertMode::ToUpper) {
				pushArray(ctx, Utf8::toupper(getArray(ctx)));
			} else {
				pushArray(ctx, Utf8::tolower(getArray(ctx)));
			}
		} else  {
			if (mode == ConvertMode::ToUpper) {
				duk_push_string(ctx, Utf8::toupper(duk_require_string(ctx, 0)).c_str());
			} else {
				duk_push_string(ctx, Utf8::tolower(duk_require_string(ctx, 0)).c_str());
			}
		}
	} catch (const std::exception &ex) {
		dukx_throw(ctx, -1, ex.what());
	}

	return 1;
}

int Utf8_isdigit(duk_context *ctx)
{
	duk_push_boolean(ctx, Utf8::isdigit(duk_require_int(ctx, 0)));

	return 1;
}

int Utf8_isletter(duk_context *ctx)
{
	duk_push_boolean(ctx, Utf8::isletter(duk_require_int(ctx, 0)));

	return 1;
}

int Utf8_islower(duk_context *ctx)
{
	duk_push_boolean(ctx, Utf8::islower(duk_require_int(ctx, 0)));

	return 1;
}

int Utf8_isspace(duk_context *ctx)
{
	duk_push_boolean(ctx, Utf8::isspace(duk_require_int(ctx, 0)));

	return 1;
}

int Utf8_istitle(duk_context *ctx)
{
	duk_push_boolean(ctx, Utf8::istitle(duk_require_int(ctx, 0)));

	return 1;
}

int Utf8_isupper(duk_context *ctx)
{
	duk_push_boolean(ctx, Utf8::isupper(duk_require_int(ctx, 0)));

	return 1;
}

int Utf8_length(duk_context *ctx)
{
	const char *str = duk_require_string(ctx, 0);

	try {
		duk_push_int(ctx, static_cast<int>(Utf8::length(str)));
	} catch (const std::exception &error) {
		dukx_throw(ctx, -1, error.what());
	}

	return 1;
}

int Utf8_toarray(duk_context *ctx)
{
	const char *string = duk_require_string(ctx, 0);
	std::u32string array;

	try {
		array = Utf8::toucs(string);
	} catch (const std::exception &error) {
		dukx_throw(ctx, -1, error.what());
	}

	return pushArray(ctx, array);
}

int Utf8_tolower(duk_context *ctx)
{
	return convert(ctx, ConvertMode::ToLower);
}

int Utf8_tostring(duk_context *ctx)
{
	try {
		duk_push_string(ctx, Utf8::toutf8(getArray(ctx)).c_str());
	} catch (const std::exception &ex) {
		dukx_throw(ctx, -1, ex.what());
	}

	return 1;
}

int Utf8_toupper(duk_context *ctx)
{
	return convert(ctx, ConvertMode::ToUpper);
}

const duk_function_list_entry utf8Functions[] = {
	{ "isDigit",		Utf8_isdigit,	1	},
	{ "isLetter",		Utf8_isletter,	1	},
	{ "isLower",		Utf8_islower,	1	},
	{ "isSpace",		Utf8_isspace,	1	},
	{ "isTitle",		Utf8_istitle,	1	},
	{ "isUpper",		Utf8_isupper,	1	},
	{ "length",		Utf8_length,	1	},
	{ "toArray",		Utf8_toarray,	1	},
	{ "toLower",		Utf8_tolower,	1	},
	{ "toString",		Utf8_tostring,	1	},
	{ "toUpper",		Utf8_toupper,	1	},
	{ nullptr,		nullptr,	0	}
};

} // !namespace

duk_ret_t dukopen_utf8(duk_context *ctx) noexcept
{
	dukx_assert_begin(ctx);
	duk_push_object(ctx);
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, utf8Functions);
	duk_put_prop_string(ctx, -2, "Utf8");
	dukx_assert_end(ctx, 1);

	return 1;
}

} // !irccd

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
#include "Utf8.h"

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
	dukx_assert_begin(ctx);
	duk_push_array(ctx);

	for (unsigned i = 0; i < array.length(); ++i) {
		duk_push_int(ctx, array[i]);
		duk_put_prop_index(ctx, -2, i);
	}
	dukx_assert_end(ctx, 1);

	return 1;
}

int convert(duk_context *ctx, ConvertMode mode)
{
	/*
	 * This function can convert both UTF-32 and UTF-8 strings.
	 */
	dukx_assert_begin(ctx);

	try {
		if (duk_get_type(ctx, 0) == DUK_TYPE_OBJECT) {
			if (mode == ConvertMode::ToUpper) {
				pushArray(ctx, Utf8::toupper(getArray(ctx)));
			} else {
				pushArray(ctx, Utf8::tolower(getArray(ctx)));
			}
		} else if (duk_get_type(ctx, 0) == DUK_TYPE_STRING) {
			if (mode == ConvertMode::ToUpper) {
				duk_push_string(ctx, Utf8::toupper(duk_require_string(ctx, 0)).c_str());
			} else {
				duk_push_string(ctx, Utf8::tolower(duk_require_string(ctx, 0)).c_str());
			}
		} else if (duk_get_type(ctx, 0) == DUK_TYPE_NUMBER) {
			if (mode == ConvertMode::ToUpper) {
				duk_push_uint(ctx, Utf8::toupper(duk_require_uint(ctx, 0)));
			} else {
				duk_push_uint(ctx, Utf8::tolower(duk_require_uint(ctx, 0)));
			}
		} else {
			dukx_throw(ctx, -1, "invalid argument to convert");
		}
	} catch (const std::exception &ex) {
		dukx_throw(ctx, -1, ex.what());
	}
	dukx_assert_end(ctx, 1);

	return 1;
}

/*
 * Function: Unicode.isDigit(code)
 * --------------------------------------------------------
 *
 * Arguments:
 *   - code, the code point
 * Returns:
 *   - true if the code is in the digit category
 */
int Unicode_isdigit(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	duk_push_boolean(ctx, Utf8::isdigit(duk_require_int(ctx, 0)));
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
int Unicode_isletter(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	duk_push_boolean(ctx, Utf8::isletter(duk_require_int(ctx, 0)));
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
int Unicode_islower(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	duk_push_boolean(ctx, Utf8::islower(duk_require_int(ctx, 0)));
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
int Unicode_isspace(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	duk_push_boolean(ctx, Utf8::isspace(duk_require_int(ctx, 0)));
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
int Unicode_istitle(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	duk_push_boolean(ctx, Utf8::istitle(duk_require_int(ctx, 0)));
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
int Unicode_isupper(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	duk_push_boolean(ctx, Utf8::isupper(duk_require_int(ctx, 0)));
	dukx_assert_end(ctx, 1);
	return 1;
}

/*
 * Function: Unicode.length(u8string)
 * --------------------------------------------------------
 *
 * Get the real length of a UTF-8 string, return the number of characters
 * not the number of bytes.
 *
 * Arguments:
 *   - u8string, a UTF-8 string
 * Returns:
 *   - the real length
 * Throws:
 *   - Exception if the string is not a valid UTF-8 string
 */
int Unicode_length(duk_context *ctx)
{
	const char *str = duk_require_string(ctx, 0);

	dukx_assert_begin(ctx);

	try {
		duk_push_int(ctx, static_cast<int>(Utf8::length(str)));
	} catch (const std::exception &error) {
		dukx_throw(ctx, -1, error.what());
	}

	dukx_assert_end(ctx, 1);

	return 1;
}

/*
 * Function: Unicode.toUtf32(u8string)
 * --------------------------------------------------------
 *
 * Convert a UTF-8 string to a UTF-32 JavaScript array containing the chracter
 * code points.
 *
 * Arguments:
 *   - u8string, the UTF-8 string
 * Returns:
 *   - The array of unicode code points
 * Throws:
 *   - Exception if the string is not a valid UTF-8 string
 */
int Unicode_toUtf32(duk_context *ctx)
{
	const char *string = duk_require_string(ctx, 0);
	std::u32string array;

	dukx_assert_begin(ctx);

	try {
		array = Utf8::toucs(string);
	} catch (const std::exception &error) {
		dukx_throw(ctx, -1, error.what());
	}

	dukx_assert_end(ctx, 1);

	return pushArray(ctx, array);
}

/*
 * Function: Unicode.toLower(data)
 * --------------------------------------------------------
 *
 * Convert to lowercase, the data may be a unicode code point, a UTF-8
 * string or a UTF-32 array.
 *
 * Arguments:
 *   - data, the data to convert
 * Returns:
 *   - the lower case conversion
 * Throws:
 *   - Any exception on error
 */
int Unicode_tolower(duk_context *ctx)
{
	return convert(ctx, ConvertMode::ToLower);
}

/*
 * Function: Unicode.toUtf8(u32string)
 * --------------------------------------------------------
 *
 * Convert the UTF-32 array to a UTF-8 string.
 *
 * Arguments:
 *   - u32string, the UTF-32 array
 * Returns:
 *   - The UTF-8 string
 * Throws:
 *   - Any exception on error
 */
int Unicode_toUtf8(duk_context *ctx)
{
	dukx_assert_begin(ctx);

	try {
		duk_push_string(ctx, Utf8::toutf8(getArray(ctx)).c_str());
	} catch (const std::exception &ex) {
		dukx_throw(ctx, -1, ex.what());
	}

	dukx_assert_end(ctx, 1);

	return 1;
}

/*
 * Function: Unicode.toUpper(data)
 * --------------------------------------------------------
 *
 * Convert to uppercase, the data may be a unicode code point, a UTF-8
 * string or a UTF-32 array.
 *
 * Arguments:
 *   - data, the data to convert
 * Returns:
 *   - the upper case conversion
 * Throws:
 *   - Any exception on error
 */
int Unicode_toupper(duk_context *ctx)
{
	return convert(ctx, ConvertMode::ToUpper);
}

const duk_function_list_entry utf8Functions[] = {
	{ "isDigit",		Unicode_isdigit,	1	},
	{ "isLetter",		Unicode_isletter,	1	},
	{ "isLower",		Unicode_islower,	1	},
	{ "isSpace",		Unicode_isspace,	1	},
	{ "isTitle",		Unicode_istitle,	1	},
	{ "isUpper",		Unicode_isupper,	1	},
	{ "length",		Unicode_length,		1	},
	{ "toUtf32",		Unicode_toUtf32,	1	},
	{ "toLower",		Unicode_tolower,	1	},
	{ "toUtf8",		Unicode_toUtf8,		1	},
	{ "toUpper",		Unicode_toupper,	1	},
	{ nullptr,		nullptr,		0	}
};

} // !namespace

duk_ret_t dukopen_unicode(duk_context *ctx) noexcept
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

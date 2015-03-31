/*
 * Js.h -- JS API for irccd and Duktape helpers
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

#ifndef _IRCCD_JS_H_
#define _IRCCD_JS_H_

#include <memory>
#include <string>

#include <duktape.h>

namespace irccd {

/**
 * @class DukContext
 * @brief C++ Wrapper for Duktape context
 */
class DukContext : public std::unique_ptr<duk_context, void (*)(duk_context *)> {
public:
	/**
	 * Create a Duktape context prepared for irccd, it will contains the
	 * using() and require() functions specialized for irccd.
	 *
	 * @return the ready to use Duktape context
	 */
	DukContext();

	/**
	 * Convert the context to the native Duktape/C type.
	 *
	 * @return the duk_context
	 */
	inline operator duk_context *() noexcept
	{
		return get();
	}

	/**
	 * Convert the context to the native Duktape/C type.
	 *
	 * @return the duk_context
	 */
	inline operator duk_context *() const noexcept
	{
		return get();
	}
};

/**
 * Throw a javascript error object that contains the following fields:
 *
 * {
 *   code	// the system error code
 *   message	// the system error message
 * }
 *
 * @param ctx the duktape context
 * @param code the code (usually errno)
 */
void dukx_throw_syserror(duk_context *ctx, int code);

/**
 * Throw an error with a specified code and error message.
 *
 * @param ctx the context
 * @param code the code
 * @param msg the message
 */
void dukx_throw(duk_context *ctx, int code, const std::string &msg);

/**
 * Call a function with the object cast to the given type. This works
 * only if the object contains the "\xff\xff" "data" field pointer.
 *
 * The function must have the following signature:
 *	void (Type *)
 *
 * This function let the stack as it was before the call (except if the user
 * function push arguments).
 *
 * @param ctx the duktape context
 * @param func the function to call
 */
template <typename Type, typename Func>
void dukx_with_this(duk_context *ctx, Func func)
{
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xff\xff" "data");

	func(static_cast<Type *>(duk_to_pointer(ctx, -1)));

	duk_remove(ctx, 0);	// remove "this"
	duk_remove(ctx, 0);	// remove the pointer
}

/**
 * Convenient function to push a class that will be deleted by Duktape,
 * you can use dukx_with_this in the object methods.
 *
 * @class ctx the duktape context
 * @param methods the methods
 * @param ptr the the object
 */
template <typename Type>
void dukx_set_class(duk_context *ctx, Type *ptr)
{
	// deletion flag
	duk_push_false(ctx);
	duk_put_prop_string(ctx, -2, "\xff\xff" "deleted");

	// deleter function
	duk_push_c_function(ctx, [] (auto ctx) -> duk_ret_t {
		duk_get_prop_string(ctx, 0, "\xff\xff" "deleted");

		if (!duk_to_boolean(ctx, -1)) {
			duk_pop(ctx);
			duk_get_prop_string(ctx, 0, "\xff\xff" "data");
			delete static_cast<Type *>(duk_to_pointer(ctx, -1));

			duk_pop(ctx);
			duk_push_true(ctx);
			duk_put_prop_string(ctx, 0, "\xff\xff" "deleted");
		} else {
			duk_pop(ctx);
		}

		return 0;
	}, 1);
	duk_set_finalizer(ctx, -2);

	// data pointer
	duk_push_pointer(ctx, ptr);
	duk_put_prop_string(ctx, -2, "\xff\xff" "data");
}

duk_ret_t dukopen_filesystem(duk_context *ctx) noexcept;
duk_ret_t dukopen_system(duk_context *ctx) noexcept;

} // !irccd

#endif // !_IRCCD_JS_H_


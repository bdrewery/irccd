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

/**
 * @file Js.h
 * @brief Bring JavaScript support to irccd
 *
 * This file provide a wrapper around Duktape API. The class JsDuktape **must** be used where you plan to use
 * a Duktape context. It is specialized for irccd application by adding additions that are required within the irccd
 * JavaScript API.
 *
 * It's also possible to use the class Plugin instead which already embed the JsDuktape class.
 *
 * Because you will require to call a lot of Duktape API, all of these functions defined in this file follow the same
 * conventions as Duktape, they are all free functions using underscore case. They are prefixed with dukx_ though.
 */

#include <cassert>
#include <cerrno>
#include <cstring>
#include <memory>
#include <stack>
#include <string>
#include <vector>

#include <IrccdConfig.h>

#include <duktape.h>

#if defined(WITH_JS_EXTENSION)
#  include <Dynlib.h>
#endif

namespace irccd {

/**
 * @class JsError
 * @brief Error description
 *
 * This class fills the fields got in an Error object, you can get it from dukx_error.
 */
class JsError : public std::exception {
public:
	std::string name;		//!< name of error
	std::string message;		//!< error message
	std::string stack;		//!< stack if available
	std::string fileName;		//!< filename if applicable
	int lineNumber{0};		//!< line number if applicable

	/**
	 * Get the error message. This effectively returns message field.
	 *
	 * @return the message
	 */
	const char *what() const noexcept override
	{
		return message.c_str();
	}
};

/**
 * @class JsException
 * @brief Base class to use for dukx_throw
 *
 * This helper class can be used to automatically set Error fields in JavaScript exceptions.
 */
class JsException {
private:
	std::string m_name;
	std::string m_message;

public:
	/**
	 * Create the helper.
	 *
	 * @param name the name (e.g TypeError)
	 * @param message the message
	 */
	inline JsException(std::string name, std::string message) noexcept
		: m_name(std::move(name))
		, m_message(std::move(message))
	{
	}

	/**
	 * Get the error name.
	 *
	 * @return the name
	 */
	inline const std::string &name() const noexcept
	{
		return m_name;
	}

	/**
	 * Get the error message.
	 *
	 * @return the message
	 */
	inline const std::string &message() const noexcept
	{
		return m_message;
	}
};

/**
 * @class JsSystemError
 * @brief SystemError error that is usually thrown from I/O or system operations
 *
 * The SystemError inherits from Error and adds an additional errno field which contains one of the standard C++11
 * errno constants.
 */
class JsSystemError : public JsException {
private:
	int m_errno;

public:
	/**
	 * This constructor automatically use errno and std::strerror.
	 */
	inline JsSystemError()
		: JsSystemError(errno, std::strerror(errno))
	{

	}

	/**
	 * Construct the SystemError with the appropriate errno code and message.
	 *
	 * @param errn the errno number
	 * @param message the message
	 */
	inline JsSystemError(int errn, std::string message)
		: JsException("SystemError", std::move(message))
		, m_errno(errn)
	{
	}

	/**
	 * Create the exception.
	 *
	 * @param ctx the Duktape context.
	 */
	inline void create(duk_context *ctx) const
	{
		duk_get_global_string(ctx, "SystemError");
		duk_new(ctx, 0);
		duk_push_int(ctx, m_errno);
		duk_put_prop_string(ctx, -2, "errno");
	}
};

#if defined(WITH_JS_EXTENSION)
/**
 * Vector of Dynlib as pointers. It's not possible to use the exported
 * symbols when the library is closed so be sure that the object is never
 * deleted while the module is loaded.
 */
using JsModules = std::vector<std::unique_ptr<Dynlib>>;
#endif

/**
 * @class JsDuktape
 * @brief C++ Wrapper for Duktape context
 *
 * Avoid using this class directly because it needs to use global hidden
 * variables that are defined from Plugin object.
 */
class JsDuktape : public std::unique_ptr<duk_context, void (*)(duk_context *)> {
private:
#if defined(WITH_JS_EXTENSION)
	JsModules m_modules;
#endif
	/*
	 * Paths stored in a stack when loading module globally recursively.
	 */
	static std::stack<std::string> m_paths;

	/* Some helpers */
	static JsDuktape &self(duk_context *ctx) noexcept;
	static std::string parent(JsDuktape &ctx) noexcept;

	/* Loaders */
	static void loadFunction(JsDuktape &ctx, duk_c_function fn);
	static void loadLocal(JsDuktape &ctx, const std::string &path);
#if defined(WITH_JS_EXTENSION)
	static void loadNative(JsDuktape &ctx, std::string ident, const std::string &path);
#endif

	/* Require searchers */
	static void requireLocal(JsDuktape &ctx, const std::string &name);
	static void requirePlugin(JsDuktape &ctx, const std::string &name);
	static void requireGlobal(JsDuktape &ctx, const std::string &name);

	/* Duktape modifications */
	static duk_ret_t require(duk_context *);
	static duk_ret_t use(duk_context *);
	static duk_ret_t modSearch(duk_context *ctx);

	/* Move and copy forbidden */
	JsDuktape(const JsDuktape &) = delete;
	JsDuktape &operator=(const JsDuktape &) = delete;
	JsDuktape(const JsDuktape &&) = delete;
	JsDuktape &operator=(const JsDuktape &&) = delete;

public:
	/**
	 * Create a Duktape context prepared for irccd, it will contains the
	 * using() and require() functions specialized for irccd.
	 *
	 * @param path the parent directory of that context (used for require)
	 * @return the ready to use Duktape context
	 */
	JsDuktape(const std::string &path);

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

#if !defined(NDEBUG)
#define dukx_assert_begin(ctx)						\
	int _topstack = duk_get_top(ctx)
#else
#define dukx_assert_begin(ctx)
#endif

#if !defined(NDEBUG)
#define dukx_assert_equals(ctx)						\
	assert(_topstack == duk_get_top(ctx))
#else
#define dukx_assert_equals(ctx)
#endif

#if !defined(NDEBUG)
#define dukx_assert_end(ctx, count)					\
	assert(_topstack == (duk_get_top(ctx) - count))
#else
#define dukx_assert_end(ctx, count)
#endif

/**
 * Call a function with the object cast to the given type. This works
 * only if the object contains the "\xff\xff" "data" field pointer.
 *
 * The function must have the following signature:
 *	void (Type &)
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
	Type *type;

	dukx_assert_begin(ctx);
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xff\xff" "data");
	type = static_cast<Type *>(duk_to_pointer(ctx, -1));
	duk_pop_2(ctx);
	dukx_assert_equals(ctx);

	func(*type);
}

/**
 * Convenient function to push a class that will be deleted by Duktape,
 * you can use dukx_with_this in the object methods.
 *
 * This function is best used when the object is constructed *from* JavaScript
 * using function constructor.
 *
 * @class ctx the duktape context
 * @param methods the methods
 * @param ptr the the object
 */
template <typename Type>
void dukx_set_class(duk_context *ctx, Type *ptr)
{
	dukx_assert_begin(ctx);

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

	dukx_assert_equals(ctx);
}

/**
 * Similar to dukx_set_class but this function push an object instead which is
 * allocated frmo the C++ side.
 */
template <typename Type>
void dukx_push_shared(duk_context *ctx, std::shared_ptr<Type> ptr)
{
	dukx_assert_begin(ctx);

	// Object itself
	duk_push_object(ctx);

	// Set its prototype
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "\xff" "irccd-proto");
	duk_get_prop_string(ctx, -1, Type::JsName);
	duk_set_prototype(ctx, -4);
	duk_pop_2(ctx);

	// deletion flag
	duk_push_false(ctx);
	duk_put_prop_string(ctx, -2, "\xff\xff" "deleted");

	// deleter function
	duk_push_c_function(ctx, [] (auto ctx) -> duk_ret_t {
		duk_get_prop_string(ctx, 0, "\xff\xff" "deleted");

		if (!duk_to_boolean(ctx, -1)) {
			duk_pop(ctx);
			duk_get_prop_string(ctx, 0, "\xff\xff" "data");
			delete static_cast<std::shared_ptr<Type> *>(duk_to_pointer(ctx, -1));

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
	duk_push_pointer(ctx, new std::shared_ptr<Type>(ptr));
	duk_put_prop_string(ctx, -2, "\xff\xff" "data");

	dukx_assert_end(ctx, 1);
}

/**
 * Throw an exception.
 *
 * The error must have the following requirements:
 *
 * - const std::string &name() const noexcept
 * - const std::string &message() const noexcept
 * - void create(duk_context *ctx) const
 *
 * Deriving from JsException is a good idea as it already provides
 * name() and message().
 *
 * @param error the object function to throw
 * @return 0
 */
template <typename Error>
duk_ret_t dukx_throw(duk_context *ctx, const Error &error)
{
	error.create(ctx);

	duk_push_string(ctx, error.name().c_str());
	duk_put_prop_string(ctx, -2, "name");
	duk_push_string(ctx, error.message().c_str());
	duk_put_prop_string(ctx, -2, "message");
	duk_throw(ctx);

	return 0;
}

/**
 * Get the error fields from the Error object at the top of the
 * stack.
 *
 * @param ctx the context
 * @return the error object
 */
JsError dukx_error(duk_context *ctx, duk_idx_t index = -1);

/* Modules */
duk_ret_t dukopen_filesystem(duk_context *ctx) noexcept;
duk_ret_t dukopen_logger(duk_context *ctx) noexcept;
duk_ret_t dukopen_server(duk_context *ctx) noexcept;
duk_ret_t dukopen_system(duk_context *ctx) noexcept;
duk_ret_t dukopen_timer(duk_context *ctx) noexcept;
duk_ret_t dukopen_unicode(duk_context *ctx) noexcept;
duk_ret_t dukopen_util(duk_context *ctx) noexcept;

/* Preload is needed for settings up objects allocated from C++ */
void dukpreload_server(duk_context *ctx) noexcept;

} // !irccd

#endif // !_IRCCD_JS_H_


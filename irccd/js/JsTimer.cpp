/*
 * JsTimer.cpp -- timers for irccd JS API
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

#include <Plugin.h>

namespace irccd {

namespace {

duk_ret_t Timer_prototype_start(duk_context *ctx)
{
	dukx_with_this<std::shared_ptr<Timer>>(ctx, [&] (std::shared_ptr<Timer> *timer) {
		if (!(*timer)->isRunning()) {
			(*timer)->start();
		}

		return 0;
	});

	return 0;
}

duk_ret_t Timer_prototype_stop(duk_context *ctx)
{
	dukx_with_this<std::shared_ptr<Timer>>(ctx, [&] (std::shared_ptr<Timer> *timer) {
		if ((*timer)->isRunning()) {
			(*timer)->stop();
		}
	});

	return 0;
}

const duk_function_list_entry timerMethods[] = {
	{ "start",	Timer_prototype_start,	0	},
	{ "stop",	Timer_prototype_stop,	0	},
	{ nullptr,	nullptr,		0	}
};

duk_ret_t Timer_Timer(duk_context *ctx)
{
	int type = duk_require_int(ctx, 0);
	int delay = duk_require_int(ctx, 1);
	std::shared_ptr<Timer> timer = std::make_shared<Timer>(static_cast<TimerType>(type), delay);

	// Get the associated pointer
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "\xff""\xff""plugin");
	Plugin &p = *static_cast<Plugin *>(duk_to_pointer(ctx, -1));
	p.timerAdd(timer);
	duk_pop_2(ctx);

	duk_push_this(ctx);
	dukx_set_class<std::shared_ptr<Timer>>(ctx, new std::shared_ptr<Timer>(timer));
	duk_push_string(ctx, "onTimeout");
	duk_push_c_function(ctx, [] (duk_context *ctx) -> duk_ret_t {
		if (!duk_is_callable(ctx, 0)) {
			return 0;
		}

		dukx_with_this<std::shared_ptr<Timer>>(ctx, [&] (auto timer) -> duk_ret_t {
			duk_push_global_object(ctx);
			duk_get_prop_string(ctx, -1, "\xff" "irccd-timers");
			duk_push_pointer(ctx, timer->get());
			duk_dup(ctx, 0);
			duk_put_prop(ctx, -3);
			duk_pop_2(ctx);

			return 0;
		});

		return 0;
	}, 1);
	duk_def_prop(ctx, -3, DUK_DEFPROP_WRITABLE | DUK_DEFPROP_HAVE_SETTER);
	duk_pop(ctx);

	return 0;
}

const duk_number_list_entry timerType[] = {
	{ "Single",	static_cast<int>(TimerType::Single)	},
	{ "Repeat",	static_cast<int>(TimerType::Repeat)	},
	{ nullptr,	0					}
};

} // !namespace

duk_ret_t dukopen_timer(duk_context *ctx) noexcept
{
	dukx_assert_begin(ctx);
	duk_push_object(ctx);
	duk_push_c_function(ctx, Timer_Timer, 2);
	duk_put_number_list(ctx, -1, timerType);
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, timerMethods);
	duk_put_prop_string(ctx, -2, "prototype");
	duk_put_prop_string(ctx, -2, "Timer");
	dukx_assert_end(ctx, 1);

	return 1;
}

} // !irccd

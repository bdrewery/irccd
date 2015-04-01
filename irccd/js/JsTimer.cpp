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

const duk_function_list_entry timerMethods[] = {
	{ "start",	Timer_prototype_start,	0	},
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

	// Store the timer object
	duk_push_this(ctx);
	dukx_set_class<std::shared_ptr<Timer>>(ctx, new std::shared_ptr<Timer>(timer));
	duk_pop(ctx);

	return 0;
}

} // !namespace

duk_ret_t dukopen_timer(duk_context *ctx) noexcept
{
	duk_push_object(ctx);
	duk_push_c_function(ctx, Timer_Timer, 2);
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, timerMethods);
	duk_put_prop_string(ctx, -2, "prototype");
	duk_put_prop_string(ctx, -2, "Timer");

	return 1;
}

} // !irccd
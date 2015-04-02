#include <Logger.h>

#include "TimerEvent.h"

namespace irccd {

TimerEvent::TimerEvent(std::shared_ptr<Plugin> plugin, std::shared_ptr<Timer> timer, TimerEventType type) noexcept
	: m_plugin(std::move(plugin))
	, m_timer(std::move(timer))
	, m_type(type)
{
}

void TimerEvent::call() noexcept
{
	duk_context *ctx = m_plugin->context();

	dukx_assert_begin(ctx);
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "\xff" "irccd-timers");
	duk_push_pointer(ctx, m_timer.get());

	if (m_type == TimerEventType::End) {
		duk_push_undefined(ctx);
		duk_put_prop(ctx, -3);
	} else {
		duk_get_prop(ctx, -2);

		if (duk_pcall(ctx, 0) != 0) {
			Logger::warning() << "plugin " << m_plugin->info().name
					  << "failed to call timer: " << duk_safe_to_string(ctx, -1) << std::endl;
		}

		duk_pop(ctx);
	}

	duk_pop_2(ctx);
	dukx_assert_equals(ctx);
}

} // !irccd

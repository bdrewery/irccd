/*
 * TimerEvent.cpp -- timer event queue'ed to the main loop
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

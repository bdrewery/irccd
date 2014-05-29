/*
 * EventQueue.cpp -- plugin event queue
 *
 * Copyright (c) 2013, 2014 David Demelier <markand@malikania.fr>
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

#include "EventQueue.h"
#include "Plugin.h"
#include "RuleManager.h"

namespace irccd {

EventQueue::Atomic	EventQueue::alive(true);
EventQueue::Mutex	EventQueue::mutex;
EventQueue::Cond	EventQueue::cond;
EventQueue::List	EventQueue::list;
EventQueue::Thread	EventQueue::thread;

void EventQueue::routine()
{
	while (alive) {
		Ptr *event;

		{
			Lock lock(mutex);

			cond.wait(lock, [&] () -> bool {
				return !alive || list.size() > 0;
			});

			if (!alive)
				continue;

			event = &list.front();
		}
	
		Plugin::forAll([=] (Plugin::Ptr p) {
			const auto &manager = RuleManager::instance();

#if 0
			if (!call.second.empty()) {
				auto result = manager.solve(
				    call.second.server(),
				    call.second.channel(),
				    call.second.event(),
				    p->getName()
				);

				if (!result.enabled) {
					Logger::debug("rule: skip on match %s, %s, %s, %s",
					    call.second.server().c_str(),
					    call.second.channel().c_str(),
					    call.second.event().c_str(),
					    p->getName().c_str()
					);

					return;
				}

				if (result.encoding.size() > 0) {
					printf("REENCODING FROM %s\n", result.encoding.c_str());
				}
			}
#endif

			try {
				(*event)->call(*p);
			} catch (Plugin::ErrorException ex) {
				Logger::warn("plugin %s: %s", ex.which().c_str(), ex.what());
			}
		});

		{
			Lock lock(mutex);

			list.pop_front();
		}
	}
}

void EventQueue::start()
{
	alive = true;
	thread = std::thread(routine);
}

void EventQueue::stop()
{
	alive = false;
	cond.notify_one();

	try {
		thread.join();
	} catch (...) { }
}

} // !irccd

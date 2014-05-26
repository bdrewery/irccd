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
#include "RuleManager.h"

namespace irccd {

/* --------------------------------------------------------
 * EventInfo
 * -------------------------------------------------------- */

EventInfo::EventInfo(const std::string &server,
		     const std::string &channel,
		     const std::string &event)
	: m_server(server)
	, m_channel(channel)
	, m_event(event)
{
}

const std::string &EventInfo::server() const
{
	return m_server;
}

const std::string &EventInfo::channel() const
{
	return m_channel;
}

const std::string &EventInfo::event() const
{
	return m_event;
}

bool EventInfo::empty() const
{
	return m_server.size() == 0 && m_channel.size() == 0;
}

/* --------------------------------------------------------
 * EventQueue
 * -------------------------------------------------------- */

EventQueue::Atomic	EventQueue::alive(true);
EventQueue::Mutex	EventQueue::mutex;
EventQueue::Cond	EventQueue::cond;
EventQueue::Queue	EventQueue::queue;
EventQueue::Thread	EventQueue::thread;

void EventQueue::routine()
{
	while (alive) {
		Pair call;

		{
			Lock lock(mutex);

			cond.wait(lock, [&] () -> bool {
				return !alive || queue.size() > 0;
			});

			if (!alive)
				continue;

			call = queue.front();
		}

		Plugin::forAll([=] (Plugin::Ptr p) {
			const auto &manager = RuleManager::instance();

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

			try {
				call.first(*p);
			} catch (Plugin::ErrorException ex) {
				Logger::warn("plugin %s: %s", ex.which().c_str(), ex.what());
			}
		});

		{
			Lock lock(mutex);

			queue.pop();
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

void EventQueue::add(const Function &event, const EventInfo &info)
{
	Lock lock(mutex);

	queue.push(std::make_pair(event, info));
	cond.notify_one();
}

} // !irccd

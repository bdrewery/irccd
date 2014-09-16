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

void EventQueue::routine()
{
	while (m_alive) {
		Ptr *event;

		{
			Lock lock(m_mutex);

			m_cond.wait(lock, [&] () -> bool {
				return !m_alive || m_list.size() > 0;
			});

			if (!m_alive)
				continue;

			event = &m_list.front();
		}
	
		Plugin::forAll([=] (Plugin::Ptr p) {
			const auto &manager = RuleManager::instance();

			if (!(*event)->empty()) {
				auto result = manager.solve(
				    (*event)->server(),
				    (*event)->target(),
				    (*event)->name(),
				    p->getName()
				);

				if (!result.enabled) {
					Logger::debug("rule: skip on match %s, %s, %s, %s",
					    (*event)->server().c_str(),
					    (*event)->target().c_str(),
					    (*event)->name(),
					    p->getName().c_str()
					);

					return;
				}

				if (result.encoding.size() > 0) {
					(*event)->encode(result.encoding);

					Logger::debug("rule: encoding event %s from %s",
					    (*event)->name(), result.encoding.c_str());
				}
			}

			try {
				(*event)->call(*p);
			} catch (Plugin::ErrorException ex) {
				Logger::warn("plugin %s: %s", ex.which().c_str(), ex.what());
			}
		});

		{
			Lock lock(m_mutex);

			m_list.pop_front();
		}
	}
}

void EventQueue::start()
{
	m_alive = true;
	m_thread = std::thread(&EventQueue::routine, this);
}

void EventQueue::stop()
{
	m_alive = false;
	m_cond.notify_one();

	try {
		m_thread.join();

		// Close plugins
		Plugin::forAll([] (Plugin::Ptr p) {
			p->onUnload();
		});
	} catch (const std::exception &ex) {
		Logger::warn("irccd: %s", ex.what());
	}
}

} // !irccd

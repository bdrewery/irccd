/*
 * ElapsedTimer.cpp -- measure elapsed time
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

#include "ElapsedTimer.h"

using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;

namespace irccd {

ElapsedTimer::ElapsedTimer() noexcept
{
	m_last = high_resolution_clock::now();
}

void ElapsedTimer::pause() noexcept
{
	/*
	 * When we put the timer on pause, do not forget to set the already
	 * elapsed time.
	 */
	(void)elapsed();
	m_paused = true;
}

void ElapsedTimer::restart() noexcept
{
	m_paused = false;
	m_last = high_resolution_clock::now();
}

void ElapsedTimer::reset() noexcept
{
	m_elapsed = 0;
	m_last = high_resolution_clock::now();
}

unsigned ElapsedTimer::elapsed() noexcept
{
	if (!m_paused) {
		m_elapsed += duration_cast<milliseconds>(high_resolution_clock::now() - m_last).count();
		m_last = high_resolution_clock::now();
	}

	return m_elapsed;
}

} // !irccd

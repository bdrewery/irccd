/*
 * TestJsTimer.cpp -- test irccd timer JS API
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

#include <gtest/gtest.h>

#include <ElapsedTimer.h>
#include <Plugin.h>
#include <Timer.h>

using namespace irccd;
using namespace std::chrono_literals;

/* --------------------------------------------------------
 * Timer object itself
 * -------------------------------------------------------- */

TEST(Basic, single)
{
	Timer timer(TimerType::Single, 1000);
	ElapsedTimer elapsed;
	int count = 0;

	timer.onSignal.connect([&] () {
		count = elapsed.elapsed();
	});

	elapsed.reset();
	timer.start();

	std::this_thread::sleep_for(3s);

	ASSERT_TRUE(count >= 950 && count <= 1050);
}

TEST(Basic, repeat)
{
	Timer timer(TimerType::Repeat, 500);
	int max = 0;

	timer.onSignal.connect([&] () {
		max ++;
	});

	timer.start();

	// Should be at least 5
	std::this_thread::sleep_for(3s);

	ASSERT_TRUE(max >= 5);
}

/* --------------------------------------------------------
 * JS Timer API
 * -------------------------------------------------------- */

// TODO

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}

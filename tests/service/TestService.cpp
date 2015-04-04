/*
 * TestService.cpp -- test interruptible service
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

#include <chrono>

#include <gtest/gtest.h>

#include <ElapsedTimer.h>
#include <Logger.h>
#include <Service.h>
#include <SocketListener.h>

using namespace irccd;
using namespace std::chrono_literals;

class TestService : public Service {
protected:
	void run() override
	{
		// Wait for a large number of seconds
		SocketListener listener;

		try {
			listener.set(socket(), SocketListener::Read);
			auto st = listener.select(5s);

			if (isService(st.socket)) {
				(void)action();
			}
		} catch (const std::exception &ex) {
			FAIL() << ex.what();
		}
	}

public:
	TestService()
		: Service("test-service.sock")
	{
	}
};

TEST(Basic, isRunning)
{
	TestService ts;

	ts.start();
	ASSERT_TRUE(ts.isRunning());
	ts.stop();
	ASSERT_FALSE(ts.isRunning());
}

TEST(Basic, stop)
{
	// Should not take any time
	TestService ts;

	ts.start();

	ElapsedTimer timer;
	ts.stop();

	/* Should not take any longer */
	ASSERT_TRUE(timer.elapsed() <= 100);
}

int main(int argc, char **argv)
{
	// Disable logging
	Logger::setStandard<LoggerSilent>();
	Logger::setError<LoggerSilent>();
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}

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
using namespace irccd::address;

using namespace std::chrono_literals;

class TestService : public Service {
public:
	TestService()
		: Service(5000, "test-service", "/tmp/.irccd-test-service")
	{
	}
};

TEST(Basic, start)
{
	TestService ts;

	ts.start();
	ASSERT_EQ(ServiceState::Running, ts.state());
	ts.stop();
	ASSERT_EQ(ServiceState::Stopped, ts.state());
}

TEST(Basic, stop)
{
	TestService ts;

	ts.start();

	ElapsedTimer timer;
	ts.stop();

	/* Should not take any longer */
	ASSERT_TRUE(timer.elapsed() <= 100);
}

TEST(Basic, pause)
{
	TestService ts;

	ts.start();
	ASSERT_EQ(ServiceState::Running, ts.state());
	ts.pause();
	ASSERT_EQ(ServiceState::Paused, ts.state());
	ts.stop();
	ASSERT_EQ(ServiceState::Stopped, ts.state());
}

TEST(Basic, resume)
{
	TestService ts;

	ts.start();
	ASSERT_EQ(ServiceState::Running, ts.state());
	ts.pause();
	ASSERT_EQ(ServiceState::Paused, ts.state());
	ts.resume();
	ASSERT_EQ(ServiceState::Running, ts.state());
	ts.stop();
	ASSERT_EQ(ServiceState::Stopped, ts.state());
}

TEST(Basic, stopThenStart)
{
	TestService ts;

	ts.start();
	ASSERT_EQ(ServiceState::Running, ts.state());
	ts.stop();
	ASSERT_EQ(ServiceState::Stopped, ts.state());
	ts.start();
	ASSERT_EQ(ServiceState::Running, ts.state());
	ts.stop();
	ASSERT_EQ(ServiceState::Stopped, ts.state());
}

#if !defined(IRCCD_SYSTEM_WINDOWS)

TEST(Basic, connectThenStop)
{
	TestService ts;

	try {
		SocketTcp<Unix> s{AF_UNIX, 0};
		bool connected{false};

		s.bind(Unix{"connect.sock", true});
		s.listen(128);

		ts.onAcceptor.connect([&] (SocketAbstract &) {
			connected = true;
			s.accept();
		});

		ts.addAcceptor(s);
		ts.start();

		SocketTcp<Unix> client{AF_UNIX, 0};
		client.connect(Unix{"connect.sock"});

		std::this_thread::sleep_for(150ms);

		ts.stop();

		ASSERT_TRUE(connected);
	} catch (const std::exception &ex) {
		FAIL() << ex.what();
	}
}

TEST(Basic, connectPauseThenStop)
{
	TestService ts;

	try {
		SocketTcp<Unix> s{AF_UNIX, 0};
		bool connected{false};

		s.bind(Unix{"connect.sock", true});
		s.listen(128);

		ts.onAcceptor.connect([&] (SocketAbstract &) {
			connected = true;
			s.accept();
		});

		ts.addAcceptor(s);
		ts.start();

		SocketTcp<Unix> client{AF_UNIX, 0};
		client.connect(Unix{"connect.sock"});

		std::this_thread::sleep_for(150ms);

		ts.pause();
		ts.stop();

		ASSERT_TRUE(connected);
	} catch (const std::exception &ex) {
		FAIL() << ex.what();
	}
}

TEST(Basic, recv)
{
	TestService ts;

	try {
		SocketTcp<Unix> s{AF_UNIX, 0};
		bool connected{false};
		bool received{false};

		s.bind(Unix{"connect.sock", true});
		s.listen(128);

		std::unique_ptr<SocketTcp<Unix>> acceptedClient;
		ts.onAcceptor.connect([&] (SocketAbstract &) {
			connected = true;
			acceptedClient = std::make_unique<SocketTcp<Unix>>(s.accept());
		});
		ts.onIncoming.connect([&] (SocketAbstract &sc) {
			ASSERT_EQ(acceptedClient->handle(), sc.handle());

			try {
				ASSERT_EQ("Hello", acceptedClient->recv(512));
				received = true;
			} catch (const std::exception &ex) {
				FAIL() << ex.what();
			}
		});

		ts.addAcceptor(s);
		ts.start();

		SocketTcp<Unix> client{AF_UNIX, 0};
		client.connect(Unix{"connect.sock"});
		client.send("Hello");

		std::this_thread::sleep_for(150ms);

		ts.pause();
		ts.stop();

		ASSERT_TRUE(connected);
	} catch (const std::exception &ex) {
		FAIL() << ex.what();
	}
}

#else // IRCCD_SYSTEM_WINDOWS

TEST(Basic, connectThenStop)
{
	TestService ts;

	try {
		SocketTcp<Ipv4> s{AF_INET, 0};
		bool connected{false};

		s.set(SOL_SOCKET, SO_REUSEADDR, 1);
		s.bind(Ipv4{"*", 43000});
		s.listen(128);

		ts.onAcceptor.connect([&] (SocketAbstract &) {
			connected = true;
			s.accept();
		});

		ts.addAcceptor(s);
		ts.start();

		SocketTcp<Ipv4> client{AF_INET, 0};
		client.connect(Ipv4{"127.0.0.1", 43000});

		std::this_thread::sleep_for(150ms);

		ts.stop();

		ASSERT_TRUE(connected);
	} catch (const std::exception &ex) {
		FAIL() << ex.what();
	}
}

TEST(Basic, connectPauseThenStop)
{
	TestService ts;

	try {
		SocketTcp<Ipv4> s{AF_INET, 0};
		bool connected{false};

		s.set(SOL_SOCKET, SO_REUSEADDR, 1);
		s.bind(Ipv4{"*", 43000});
		s.listen(128);

		ts.onAcceptor.connect([&] (SocketAbstract &) {
			connected = true;
			s.accept();
		});

		ts.addAcceptor(s);
		ts.start();

		SocketTcp<Ipv4> client{AF_INET, 0};
		client.connect(Ipv4{"127.0.0.1", 43000});

		std::this_thread::sleep_for(150ms);

		ts.pause();
		ts.stop();

		ASSERT_TRUE(connected);
	} catch (const std::exception &ex) {
		FAIL() << ex.what();
	}
}

TEST(Basic, recv)
{
	TestService ts;

	try {
		SocketTcp<Ipv4> s{AF_INET, 0};
		bool connected{false};
		bool received{false};

		s.set(SOL_SOCKET, SO_REUSEADDR, 1);
		s.bind(Ipv4{"*", 43000});
		s.listen(128);

		std::unique_ptr<SocketTcp<Ipv4>> acceptedClient;
		ts.onAcceptor.connect([&] (SocketAbstract &) {
			connected = true;
			acceptedClient = std::make_unique<SocketTcp<Ipv4>>(s.accept());
		});
		ts.onIncoming.connect([&] (SocketAbstract &sc) {
			ASSERT_EQ(acceptedClient->handle(), sc.handle());

			try {
				ASSERT_EQ("Hello", acceptedClient->recv(512));
				received = true;
			} catch (const std::exception &ex) {
				FAIL() << ex.what();
			}
		});

		ts.addAcceptor(s);
		ts.start();

		SocketTcp<Ipv4> client{AF_INET, 0};
		client.connect(Ipv4{"127.0.0.1", 43000});
		client.send("Hello");

		std::this_thread::sleep_for(150ms);

		ts.pause();
		ts.stop();

		ASSERT_TRUE(connected);
	} catch (const std::exception &ex) {
		FAIL() << ex.what();
	}
}

#endif

int main(int argc, char **argv)
{
	SocketAbstract::initialize();

	// Disable logging
	Logger::setStandard<LoggerSilent>();
	Logger::setError<LoggerSilent>();
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}

/*
 * TestServer.cpp -- test both server commands/events
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
#include <memory>
#include <stdexcept>
#include <thread>

#include <gtest/gtest.h>

#include <IrccdConfig.h>

#include <ElapsedTimer.h>
#include <Logger.h>
#include <Server.h>
#include <Socket.h>

using namespace irccd;

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;

/*
 * For this tests you need to have an IRC server running:
 *
 * - at host WITH_TEST_IRCHOST
 * - on port WITH_TEST_IRCPORT
 *
 * You must also keep an empty channel #irccd-test where the first user
 * get operator status.
 *
 * You must also not have the following nicknames / usernames used:
 *
 * irct, pvd.
 *
 * Please wait several seconds before launching the tests because the IRC
 * server may need to re-sync before accepting the bot connection again.
 */
class ServerTest : public testing::Test {
protected:
	std::unique_ptr<Server> m_serverClient;
	std::unique_ptr<Server> m_serverIrccd;
	fd_set m_setinput;
	fd_set m_setoutput;
	int m_maxfd{0};

public:
	ServerTest()
	{
		this_thread::sleep_for(5s);

		ServerInfo info;
		ServerSettings settings;
		Identity identityClient("pvd", "pvd", "pvd");
		Identity identityIrccd("irct", "irct", "irct");

		info.name = WITH_TEST_IRCHOST;
		info.host = WITH_TEST_IRCHOST;
		info.port = WITH_TEST_IRCPORT;

		settings.recotimeout = 3;
		settings.channels = {
			{ "#irccd-test", "" }
		};

		m_serverClient = std::make_unique<Server>(info, identityClient, settings);
		m_serverIrccd = std::make_unique<Server>(info, identityIrccd, settings);
	}

	/**
	 * Do the function in maximum of the delay. If the function returns true
	 * this function stops.
	 *
	 * @param delay the maximum delay in milliseconds
	 * @param predicate must return true if the operation was successful
	 */
	template <typename Predicate>
	void timedProcess(unsigned delay, Predicate predicate)
	{
		ElapsedTimer timer;
		bool result = false;

		while (timer.elapsed() < delay && !result) {
			m_maxfd = 0;
			FD_ZERO(&m_setinput);
			FD_ZERO(&m_setoutput);

			m_serverIrccd->update();
			m_serverIrccd->flush();
			m_serverIrccd->prepare(m_setinput, m_setoutput, m_maxfd);

			// m_serverClient is optional
			if (m_serverClient) {
				m_serverClient->update();
				m_serverClient->flush();
				m_serverClient->prepare(m_setinput, m_setoutput, m_maxfd);
			}

			struct timeval tv;

			tv.tv_sec = 0;
			tv.tv_usec = 250;

			int code = select(m_maxfd + 1, &m_setinput, &m_setoutput, nullptr, &tv);
			if (code < Socket::Error) {
				FAIL() << "Error while selecting: " << Socket::syserror();
			} else if (code > 0) {
				m_serverIrccd->process(m_setinput, m_setoutput);

				if (m_serverClient) {
					m_serverClient->process(m_setinput, m_setoutput);
				}

				result = predicate();
			}

			this_thread::sleep_for(250ms);
		}

		if (!result) {
			FAIL() << "Operation timeout";
		}
	}
};

TEST_F(ServerTest, connect)
{
	// Not needed
	m_serverClient = nullptr;

	bool connected = false;

	m_serverIrccd->setOnConnect([&] () {
		connected = true;
	});

	// Let 3 seconds to connect
	timedProcess(3000, [&] () -> bool {
		return connected;
	});

	ASSERT_TRUE(connected);
}

TEST_F(ServerTest, channelNotice)
{
	std::string rorigin;
	std::string rchannel;
	std::string rmessage;

	m_serverIrccd->setOnChannelNotice([&] (auto origin, auto channel, auto message) {
		rorigin = std::move(origin);
		rchannel = std::move(channel);
		rmessage = std::move(message);
	});

	m_serverClient->setOnJoin([&] (auto, auto) {
		m_serverClient->cnotice("#irccd-test", "please don't flood");
	});

	timedProcess(10000, [&] () -> bool {
		return rorigin.compare(0, 3, "pvd") == 0 &&
		       rchannel == "#irccd-test" &&
		       rmessage == "please don't flood";
	});
}

/*
 * TODO: onInvite
 */
TEST_F(ServerTest, invite)
{
	bool joined = false;
	bool connected = false;

	bool invited = false;
	std::string rorigin;
	std::string rchannel;

	/*
	 * Step 1: wait that irccd is connected and client has joined
	 * #test-invite.
	 */
	m_serverClient->setOnJoin([&] (auto, auto channel) {
		if (channel == "#test-invite") {
			joined = true;
		}
	});
	m_serverClient->setOnConnect([&] () {
		m_serverClient->join("#test-invite");
	});
	m_serverIrccd->setOnConnect([&] () {
		connected = true;
	});
	m_serverIrccd->setOnInvite([&] (auto origin, auto channel, auto) {
		invited = true;
		rorigin = std::move(origin);
		rchannel = std::move(channel);
	});

	timedProcess(10000, [&] () -> bool {
		return connected && joined;
	});

	/*
	 * Step 2: wait that irccd receive invite event.
	 */
	m_serverClient->invite("irct", "#test-invite");
	timedProcess(10000, [&] () -> bool {
		return invited && rchannel == "#test-invite" && rorigin.compare(0, 3, "pvd") == 0;
	});
}

TEST_F(ServerTest, join)
{
	// Not needed
	m_serverClient = nullptr;

	std::string rorigin;
	std::string rchannel;

	m_serverIrccd->setOnJoin([&] (auto origin, auto channel) {
		rorigin = std::move(origin);
		rchannel = std::move(channel);
	});

	timedProcess(10000, [&] () -> bool {
		return rorigin.compare(0, 3, "irc") == 0 &&
		       rchannel == "#irccd-test";
	});
}

TEST_F(ServerTest, kick)
{
	bool joined = false;
	bool kicked = true;
	std::string rorigin;
	std::string rchannel;
	std::string rtarget;
	std::string rreason;

	/*
	 * Step 1: wait that client is connected before to gain +o.
	 */
	m_serverClient->setOnConnect([&] () {
		m_serverClient->join("#test-kick");
	});
	m_serverClient->setOnJoin([&] (auto origin, auto) {
		joined = true;

		if (origin.compare(0, 4, "irct") == 0) {
			m_serverClient->kick("irct", "#test-kick", "get out");
		}
	});

	timedProcess(10000, [&] () -> bool {
		return joined;
	});

	/*
	 * Step 2: just wait that user is being kicked.
	 */
	m_serverIrccd->join("#test-kick");
	m_serverIrccd->setOnKick([&] (auto origin, auto channel, auto target, auto reason) {
		kicked = true;
		rorigin = std::move(origin);
		rchannel = std::move(channel);
		rtarget = std::move(target);
		rreason = std::move(reason);
	});

	timedProcess(10000, [&] () -> bool {
		return rorigin.compare(0, 3, "pvd") == 0 &&
		       rtarget.compare(0, 4, "irct") == 0 &&
		       rchannel == "#test-kick" && kicked;
	});
}

TEST_F(ServerTest, message)
{
	std::string rorigin;
	std::string rchannel;
	std::string rmessage;

	m_serverIrccd->setOnMessage([&] (auto origin, auto channel, auto message) {
		rorigin = std::move(origin);
		rchannel = std::move(channel);
		rmessage = std::move(message);
	});

	m_serverClient->setOnJoin([&] (auto, auto) {
		m_serverClient->message("#irccd-test", "hello irct!");
	});

	timedProcess(10000, [&] () -> bool {
		return rorigin.compare(0, 3, "pvd") == 0 &&
		       rchannel == "#irccd-test" &&
		       rmessage == "hello irct!";
	});
}

int main(int argc, char **argv)
{
	// Disable logging
	Logger::setStandard<LoggerSilent>();
	Logger::setError<LoggerSilent>();
	Socket::initialize();
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}

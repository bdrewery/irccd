/*
 * TestTransport.cpp -- test transport management
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

#include <Logger.h>

#include <SocketListener.h>
#include <SocketTcp.h>

#include "TransportService.h"
#include "TransportCommand.h"

#define DELAY	250ms

using namespace std::literals::chrono_literals;

using namespace irccd;
using namespace address;

std::unique_ptr<TransportService> manager;
std::unique_ptr<TransportCommand> last;
std::string error;

class TransportTest : public testing::Test {
protected:
	SocketTcp m_client;
	SocketListener m_listener;

public:
	TransportTest()
		: m_client(AF_INET, 0)
	{
		m_client.connect(Internet("127.0.0.1", 25000, AF_INET));
	}

	~TransportTest()
	{
		m_client.close();

		last = nullptr;
		error = "";
	}

	void test(const std::string &str, const std::string &expected)
	{
		m_client.send(str + "\r\n\r\n");

		try {
			m_listener.set(m_client, SocketListener::Read);
			m_listener.select(DELAY);

			error = m_client.recv(512);
		} catch (...) {}

		std::this_thread::sleep_for(DELAY);

		ASSERT_TRUE(last != nullptr);
		ASSERT_EQ("", error);

		// Do not understand why ASSERT does not stop
		if (last) {
			ASSERT_EQ(expected, last->ident());
		}
	}
};

TEST_F(TransportTest, channelNotice)
{
	test(
		"{"
		  "\"command\":\"cnotice\","
		  "\"server\":\"localhost\","
		  "\"channel\":\"#staff\","
		  "\"message\":\"hello world\""
		"}",
		"cnotice:localhost:#staff:hello world"
	);
}

TEST_F(TransportTest, connect)
{
	test(
		"{"
		  "\"command\":\"connect\","
		  "\"name\":\"google\","
		  "\"host\": \"google.fr\","
		  "\"port\":6667,"
		  "\"ssl\":false,"
		  "\"ssl-verify\":true"
		"}",
		"connect:google:google.fr:6667"
	);
}

TEST_F(TransportTest, disconnect)
{
	test(
		"{"
		  "\"command\":\"disconnect\","
		  "\"server\":\"localhost\""
		"}",
		"disconnect:localhost"
	);
}

TEST_F(TransportTest, invite)
{
	test(
		"{"
		  "\"command\":\"invite\","
		  "\"server\":\"localhost\","
		  "\"target\":\"francis\","
		  "\"channel\":\"#staff\""
		"}",
		"invite:localhost:francis:#staff"
	);
}

TEST_F(TransportTest, join1)
{
	test(
		"{"
		  "\"command\":\"join\","
		  "\"server\":\"localhost\","
		  "\"channel\":\"#staff\""
		"}",
		"join:localhost:#staff:"
	);
}

TEST_F(TransportTest, join2)
{
	test(
		"{"
		  "\"command\":\"join\","
		  "\"server\":\"localhost\","
		  "\"channel\":\"#secure\","
		  "\"password\": \"abcdef\""
		"}",
		"join:localhost:#secure:abcdef"
	);
}

TEST_F(TransportTest, kick1)
{
	test(
		"{"
		  "\"command\":\"kick\","
		  "\"server\":\"localhost\","
		  "\"target\":\"jean\","
		  "\"channel\":\"#staff\""
		"}",
		"kick:localhost:jean:#staff:"
	);
}

TEST_F(TransportTest, kick2)
{
	test(
		"{"
		  "\"command\":\"kick\","
		  "\"server\":\"localhost\","
		  "\"target\":\"jean\","
		  "\"channel\":\"#staff\","
		  "\"reason\":\"bad OS\""
		"}",
		"kick:localhost:jean:#staff:bad OS"
	);
}

TEST_F(TransportTest, load)
{
	test(
		"{"
		  "\"command\":\"load\","
		  "\"plugin\":\"breakmyplugin\""
		"}",
		"load:breakmyplugin"
	);
}

TEST_F(TransportTest, me1)
{
	test(
		"{"
		  "\"command\":\"me\","
		  "\"server\":\"localhost\","
		  "\"channel\":\"#staff\""
		"}",
		"me:localhost:#staff:"
	);
}

TEST_F(TransportTest, me2)
{
	test(
		"{"
		  "\"command\":\"me\","
		  "\"server\":\"localhost\","
		  "\"channel\":\"#food\","
		  "\"message\":\"is hungry\""
		"}",
		"me:localhost:#food:is hungry"
	);
}

TEST_F(TransportTest, message1)
{
	test(
		"{"
		  "\"command\":\"message\","
		  "\"server\":\"localhost\","
		  "\"target\":\"francis\""
		"}",
		"message:localhost:francis:"
	);
}

TEST_F(TransportTest, message2)
{
	test(
		"{"
		  "\"command\":\"message\","
		  "\"server\":\"localhost\","
		  "\"target\":\"francis\","
		  "\"message\":\"lol\""
		"}",
		"message:localhost:francis:lol"
	);
}

TEST_F(TransportTest, mode)
{
	test(
		"{"
		  "\"command\":\"mode\","
		  "\"server\":\"localhost\","
		  "\"channel\":\"#staff\","
		  "\"mode\":\"+b francis\""
		"}",
		"mode:localhost:#staff:+b francis"
	);
}

TEST_F(TransportTest, notice)
{
	test(
		"{"
		  "\"command\":\"notice\","
		  "\"server\":\"localhost\","
		  "\"target\":\"francis\","
		  "\"message\":\"stop flooding\""
		"}",
		"notice:localhost:francis:stop flooding"
	);
}

TEST_F(TransportTest, part1)
{
	test(
		"{"
		  "\"command\":\"part\","
		  "\"server\":\"localhost\","
		  "\"channel\":\"#visualstudio\""
		"}",
		"part:localhost:#visualstudio:"
	);
}

TEST_F(TransportTest, part2)
{
	test(
		"{"
		  "\"command\":\"part\","
		  "\"server\":\"localhost\","
		  "\"channel\":\"#visualstudio\","
		  "\"reason\":\"too few features\""
		"}",
		"part:localhost:#visualstudio:too few features"
	);
}

TEST_F(TransportTest, reconnect1)
{
	test(
		"{"
		  "\"command\":\"reconnect\""
		"}",
		"reconnect:"
	);
}

TEST_F(TransportTest, reconnect2)
{
	test(
		"{"
		  "\"command\":\"reconnect\","
		  "\"server\":\"localhost\""
		"}",
		"reconnect:localhost"
	);
}

TEST_F(TransportTest, reload)
{
	test(
		"{"
		  "\"command\":\"reload\","
		  "\"plugin\":\"crazy\""
		"}",
		"reload:crazy"
	);
}

TEST_F(TransportTest, topic)
{
	test(
		"{"
		  "\"command\":\"topic\","
		  "\"server\":\"localhost\","
		  "\"channel\":\"#staff\","
		  "\"topic\":\"new release\""
		"}",
		"topic:localhost:#staff:new release"
	);
}

TEST_F(TransportTest, umode)
{
	test(
		"{"
		  "\"command\":\"umode\","
		  "\"server\":\"localhost\","
		  "\"mode\":\"+i\""
		"}",
		"umode:localhost:+i"
	);
}

TEST_F(TransportTest, unload)
{
	test(
		"{"
		  "\"command\":\"unload\","
		  "\"plugin\":\"crazy\""
		"}",
		"unload:crazy"
	);
}

int main(int argc, char **argv)
{
	// Disable logging
	Logger::setStandard<LoggerSilent>();
	Logger::setError<LoggerSilent>();
	testing::InitGoogleTest(&argc, argv);

	manager = std::make_unique<TransportService>();
	manager->add<TransportInet>(TransportAbstract::IPv4, 25000);
	manager->onCommand.connect([&] (TransportCommand command) {
		last = std::make_unique<TransportCommand>(std::move(command));
	});
	manager->start();

	int ret = RUN_ALL_TESTS();

	manager->stop();

	return ret;
}

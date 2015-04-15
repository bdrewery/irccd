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

#include "TransportService.h"
#include "TransportCommand.h"

#define DELAY	250ms

using namespace std::literals::chrono_literals;

using namespace irccd;
using namespace address;

std::unique_ptr<TransportService> manager;
std::unique_ptr<TransportCommand> last;

class TransportTest : public testing::Test {
protected:
	SocketTcp m_client;

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
	}
};

TEST_F(TransportTest, channelNotice)
{
	m_client.send("{ \"command\": \"cnotice\", \"server\": \"localhost\", \"channel\": \"#staff\", \"message\": \"hello world\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("cnotice:localhost:#staff:hello world", last->ident());
}

TEST_F(TransportTest, connect)
{
	m_client.send("{ \"command\": \"connect\", \"name\": \"google\", \"host\": \"google.fr\", \"port\": 6667, \"ssl\": false, \"ssl-verify\": true }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("connect:google:google.fr6667:false:true", last->ident());
}

TEST_F(TransportTest, disconnect)
{
	m_client.send("{ \"command\": \"disconnect\", \"server\": \"localhost\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("disconnect:localhost", last->ident());
}

TEST_F(TransportTest, invite)
{
	m_client.send("{ \"command\": \"invite\", \"server\": \"localhost\", \"target\": \"francis\", \"channel\": \"staff\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("invite:localhost:francis:staff", last->ident());
}

TEST_F(TransportTest, join1)
{
	m_client.send("{ \"command\": \"join\", \"server\": \"localhost\", \"channel\": \"#staff\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("join:localhost:#staff:", last->ident());
}

TEST_F(TransportTest, join2)
{
	m_client.send("{ \"command\": \"join\", \"server\": \"localhost\", \"channel\": \"#secure\", \"password\": \"abcdef\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("join:localhost:#secure:abcdef", last->ident());
}

TEST_F(TransportTest, kick1)
{
	m_client.send("{ \"command\": \"kick\", \"server\": \"localhost\", \"target\": \"jean\", \"channel\": \"#staff\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("kick:localhost:jean:#staff:", last->ident());
}

TEST_F(TransportTest, kick2)
{
	m_client.send("{ \"command\": \"kick\", \"server\": \"localhost\", \"target\": \"jean\", \"channel\": \"#staff\", \"reason\": \"bad OS\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("kick:localhost:jean:#staff:bad OS", last->ident());
}

TEST_F(TransportTest, load1)
{
	m_client.send("{ \"command\": \"load\", \"path\": \"/opt/breakmyplugin.js\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("load:/opt/breakmyplugin.js:false", last->ident());
}

TEST_F(TransportTest, load2)
{
	m_client.send("{ \"command\": \"load\", \"name\": \"breakmyplugin.js\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("load:breakmyplugin.js:true", last->ident());
}

TEST_F(TransportTest, me1)
{
	m_client.send("{ \"command\": \"me\", \"server\": \"localhost\", \"channel\": \"#staff\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("me:localhost:#staff:", last->ident());
}

TEST_F(TransportTest, me2)
{
	m_client.send("{ \"command\": \"me\", \"server\": \"localhost\", \"channel\": \"#food\", \"message\": \"is hungry\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("me:localhost:#food:is hungry", last->ident());
}

TEST_F(TransportTest, mode)
{
	m_client.send("{ \"command\": \"mode\", \"server\": \"localhost\", \"channel\": \"#staff\", \"mode\": \"+b francis\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("mode:localhost:#staff:+b francis", last->ident());
}

TEST_F(TransportTest, notice)
{
	m_client.send("{ \"command\": \"notice\", \"server\": \"localhost\", \"target\": \"francis\", \"message\": \"stop flooding\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("notice:localhost:francis:stop flooding", last->ident());
}

TEST_F(TransportTest, part1)
{
	m_client.send("{ \"command\": \"part\", \"server\": \"localhost\", \"channel\": \"#visualstudio\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("part:localhost:#visualstudio:", last->ident());
}

TEST_F(TransportTest, part2)
{
	m_client.send("{ \"command\": \"part\", \"server\": \"localhost\", \"channel\": \"#visualstudio\", \"reason\": \"too few features\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("part:localhost:#visualstudio:too few features", last->ident());
}

TEST_F(TransportTest, reconnect1)
{
	m_client.send("{ \"command\": \"reconnect\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("reconnect:", last->ident());
}

TEST_F(TransportTest, reconnect2)
{
	m_client.send("{ \"command\": \"reconnect\", \"server\": \"localhost\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("reconnect:localhost", last->ident());
}

TEST_F(TransportTest, reload)
{
	m_client.send("{ \"command\": \"reload\", \"plugin\": \"crazy\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("reload:crazy", last->ident());
}

TEST_F(TransportTest, say1)
{
	m_client.send("{ \"command\": \"say\", \"server\": \"localhost\", \"target\": \"francis\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("say:localhost:francis:", last->ident());
}

TEST_F(TransportTest, say2)
{
	m_client.send("{ \"command\": \"say\", \"server\": \"localhost\", \"target\": \"francis\", \"message\": \"lol\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("say:localhost:francis:lol", last->ident());
}

TEST_F(TransportTest, topic)
{
	m_client.send("{ \"command\": \"topic\", \"server\": \"localhost\", \"channel\": \"#staff\", \"topic\": \"new release\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("topic:localhost:#staff:new release", last->ident());
}

TEST_F(TransportTest, umode)
{
	m_client.send("{ \"command\": \"umode\", \"server\": \"localhost\", \"mode\": \"+i\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("umode:localhost:+i", last->ident());
}

TEST_F(TransportTest, unload)
{
	m_client.send("{ \"command\": \"unload\", \"plugin\": \"crazy\" }\r\n\r\n");

	std::this_thread::sleep_for(DELAY);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("unload:crazy", last->ident());
}

int main(int argc, char **argv)
{
	// Disable logging
	Logger::setStandard<LoggerSilent>();
	Logger::setError<LoggerSilent>();
	testing::InitGoogleTest(&argc, argv);

	manager = std::make_unique<TransportService>();
	manager->add<TransportInet>(TransportAbstract::IPv4, 25000);
	manager->setOnEvent([&] (std::unique_ptr<TransportCommand> command) {
		last = std::move(command);
	});
	manager->start();

	int ret = RUN_ALL_TESTS();

	manager->stop();

	return ret;
}

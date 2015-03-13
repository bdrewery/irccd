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

#include "TransportManager.h"
#include "TransportCommand.h"

using namespace std::literals::chrono_literals;

using namespace irccd;
using namespace address;

std::unique_ptr<TransportManager> manager;
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

	std::this_thread::sleep_for(1500ms);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("cnotice:localhost:#staff:hello world", last->ident());
}

TEST_F(TransportTest, connect)
{
	m_client.send("{ \"command\": \"connect\", \"name\": \"google\", \"host\": \"google.fr\", \"port\": 6667, \"ssl\": false, \"ssl-verify\": true }\r\n\r\n");

	std::this_thread::sleep_for(1500ms);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("connect:google:google.fr6667:false:true", last->ident());
}

TEST_F(TransportTest, disconnect)
{
	m_client.send("{ \"command\": \"disconnect\", \"server\": \"localhost\" }\r\n\r\n");

	std::this_thread::sleep_for(1500ms);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("disconnect:localhost", last->ident());
}

TEST_F(TransportTest, invite)
{
	m_client.send("{ \"command\": \"invite\", \"server\": \"localhost\", \"target\": \"francis\", \"channel\": \"staff\" }\r\n\r\n");

	std::this_thread::sleep_for(1500ms);

	ASSERT_TRUE(last != nullptr);
	ASSERT_EQ("invite:localhost:francis:staff", last->ident());
}

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);

	manager = std::make_unique<TransportManager>();
	manager->add<TransportInet>(TransportAbstract::IPv4, 25000);
	manager->onEvent([&] (std::unique_ptr<TransportCommand> command) {
		last = std::move(command);
	});
	manager->start();

	return RUN_ALL_TESTS();
}

/*
 * SocketServerUnix.cpp -- unix domain sockets
 *
 * Copyright (c) 2011, 2012, 2013 David Demelier <markand@malikania.fr>
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

#include <iostream>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "SocketServerUnix.h"

using namespace irccd;
using namespace std;

SocketServerUnix::SocketServerUnix(void)
{
}

SocketServerUnix::~SocketServerUnix(void)
{
}

SocketServerUnix::SocketServerUnix(const std::string &path)
	:m_path(path)
{
	m_family = AF_UNIX;
}

bool SocketServerUnix::bind(void)
{
	sockaddr_un sun;	

	if (!create())
		return false;

	if (access(m_path.c_str(), F_OK) == 0 && ::remove(m_path.c_str()) < 0)
		return false;

	memset(&sun, 0, sizeof (sockaddr_un));
	strlcpy(sun.sun_path, m_path.c_str(), sizeof (sun.sun_path));
	sun.sun_family = AF_UNIX;

	return ::bind(m_sock, (sockaddr *)&sun, SUN_LEN(&sun)) != -1;
}

SocketClient * SocketServerUnix::accept(void)
{
	SocketClient *client;
	int sock;

	sock = ::accept(m_sock, nullptr, nullptr);
	if (sock < 0)
		return nullptr;

	client = new SocketClient();
	client->setSock(sock);

	return client;
}

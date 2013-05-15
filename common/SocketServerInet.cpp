/*
 * SocketServerInet.cpp -- internet domain socket
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

#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "SocketServerInet.h"

using namespace irccd;
using namespace std;

SocketServerInet::SocketServerInet(void)
{
}

SocketServerInet::~SocketServerInet(void)
{
}

SocketServerInet::SocketServerInet(const string &host, uint16_t port, int family)
	:m_host(host), m_port(port), m_inet(family)
{
	if (m_inet & SocketServerInet::Inet6)
		m_family = AF_INET6;
	else
		m_family = AF_INET;
}

bool SocketServerInet::bind6(void)
{
	sockaddr_in6 sin;
	in6_addr addr;
	bool ret = false;
	bool any = m_host == "*";

	if (inet_pton(AF_INET6, m_host.c_str(), &addr) || any) {
		memset(&sin, 0, sizeof (sockaddr_in6));
		sin.sin6_family = AF_INET6;
		sin.sin6_port = htons(m_port);
		sin.sin6_addr = (any) ? in6addr_any : addr;

		ret = ::bind(m_sock, (sockaddr *)&sin, sizeof (sin)) != -1;

		// On success, disable ipv6 only if v4 is wanted too
		if (ret) {
			int mode = (m_family & Socket::Inet4) ? 0 : 1;
			setsockopt(m_sock, IPPROTO_IPV6, IPV6_V6ONLY, &mode, sizeof (mode));
		}
	}

	return ret;
}

bool SocketServerInet::bind4(void)
{
	sockaddr_in sin;
	in_addr addr;
	bool any = m_host == "*";

	if (inet_pton(AF_INET, m_host.c_str(), &addr) == 1 || any) {
		memset(&sin, 0, sizeof (sockaddr_in));
		sin.sin_family = AF_INET;
		sin.sin_port = htons(m_port);
		if (any)
			sin.sin_addr.s_addr = INADDR_ANY;
		else
			sin.sin_addr = addr;

		return ::bind(m_sock, (sockaddr *)&sin, sizeof (sin)) != -1;
	}

	return false;
}

bool SocketServerInet::bind(void)
{
	ostringstream oss;
	int val = 1;
	bool ret;

	if (!create()) {
		oss << "socket: " << strerror(errno);
		return setErrorMessage(oss.str());
	}

	// avoid bind already used
	(void)setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof (val));

	if (m_inet & SocketServerInet::Inet6) {
		ret = bind6();
	} else
		ret = bind4();

	if (!ret) {
		oss << "bind: " << strerror(errno);
		setErrorMessage(oss.str());
	}

	return ret;
}

SocketClient * SocketServerInet::accept(void)
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

/*
 * SocketUtil.cpp -- socket utilities
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

#include <cerrno>
#include <sstream>

#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <netdb.h>

#include <iostream>

#include "SocketUtil.h"

using namespace irccd;
using namespace std;

SocketClient *SocketUtil::connectInet(const std::string &host, uint16_t port, int family)
{
	SocketClient *client = new SocketClient();
	addrinfo hints, *res;
	char servname[32];
	int error;

	(void)snprintf(servname, sizeof (servname) - 1, "%u", port);

	memset(&hints, 0, sizeof (hints));
	hints.ai_family = (family & Socket::Inet6) ? AF_INET6 : AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((error = getaddrinfo(host.c_str(), servname, &hints, &res))) {
		delete client;
		throw ErrorException(gai_strerror(error));
	}

	client->setFamily(res->ai_family);
	if (!client->create() ||
	    ::connect(client->getSock(), res->ai_addr, res->ai_addrlen) < 0) {
		delete client;
		freeaddrinfo(res);
		throw ErrorException(strerror(errno));
	}

	freeaddrinfo(res);

	return client;
}

SocketClient *SocketUtil::connectUnix(const string &path)
{
	sockaddr_un sun;
	string errorMessage;
	SocketClient *client = new SocketClient();

	client->setFamily(AF_UNIX);
	if (!client->create()) {
		errorMessage = client->getErrorMessage();
		delete client;
		throw ErrorException(errorMessage);
	}

	memset(&sun, 0, sizeof (sun));
	strlcpy(sun.sun_path, path.c_str(), sizeof (sun.sun_path));
	sun.sun_family = AF_UNIX;

	if (::connect(client->getSock(), (sockaddr *)&sun, SUN_LEN(&sun)) < 0) {
		ostringstream oss;

		oss << strerror(errno);
		errorMessage = oss.str();

		delete client;
		throw ErrorException(errorMessage);
	}

	return client;
}

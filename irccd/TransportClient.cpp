/*
 * TransportClient.cpp -- client connected to irccd
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

#include <Logger.h>

#include "SocketListener.h"
#include "TransportClient.h"

namespace irccd {

void TransportClientAbstract::receive()
{
	std::string incoming = socket().recv(512);
	std::string::size_type pos;

	if (incoming.size() == 0) {
		throw std::invalid_argument("client disconnected");
	}

	m_input += incoming;

	while ((pos = m_input.find("\r\n\r\n")) != std::string::npos) {
		/*
		 * Make a copy and erase it in case that onComplete function
		 * throws.
		 */
		std::string message = m_input.substr(0, pos);

		m_input.erase(m_input.begin(), m_input.begin() + pos + 4);

		try {
			m_onComplete(message);
		} catch (const std::exception &ex) {
			Logger::warning() << "transport: " << ex.what() << std::endl;
		}
	}
}

void TransportClientAbstract::send()
{
	m_output.erase(0, socket().send(m_output));
}

void TransportClientAbstract::setOnComplete(std::function<void (const std::string &)> func)
{
	m_onComplete = std::move(func);
}

void TransportClientAbstract::setOnWrite(std::function<void ()> func)
{
	m_onWrite = std::move(func);
}

void TransportClientAbstract::setOnDie(std::function<void ()> func)
{
	m_onDie = std::move(func);
}

void TransportClientAbstract::process(int direction)
{
	try {
		if (direction & SocketListener::Read) {
			receive();
		}
		if (direction & SocketListener::Write) {
			send();
		}
	} catch (const std::exception &ex) {
		m_onDie();
	}
}

void TransportClientAbstract::send(std::string message, bool notify)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_output += message;
	m_output += "\r\n\r\n";

	if (notify) {
		m_onWrite();
	}
}

bool TransportClientAbstract::hasOutput() const noexcept
{
	std::lock_guard<std::mutex> lock(m_mutex);

	return m_output.size() > 0;
}

} // !irccd

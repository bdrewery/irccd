#include "SocketListener.h"
#include "TransportClient.h"

// TMP
#include <iostream>

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
			// TODO ERROR
		}
	}
}

void TransportClientAbstract::send()
{
	m_output.erase(0, socket().send(m_output));
}

void TransportClientAbstract::onComplete(std::function<void (const std::string &)> func)
{
	m_onComplete = std::move(func);
}

void TransportClientAbstract::onWrite(std::function<void ()> func)
{
	m_onWrite = std::move(func);
}

void TransportClientAbstract::onDie(std::function<void ()> func)
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

void TransportClientAbstract::send(std::string message)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_output += message;
	m_onWrite();
}

bool TransportClientAbstract::hasOutput() const noexcept
{
	std::lock_guard<std::mutex> lock(m_mutex);

	return m_output.size() > 0;
}

} // !irccd

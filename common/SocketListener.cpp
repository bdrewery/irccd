#include <algorithm>

#include "SocketListener.h"

using namespace irccd;

const char * SocketListener::TimeoutException::what(void) const throw()
{
	return "Timeout occured";
}

void SocketListener::add(Socket &s)
{
	m_clients.push_back(s);
}

void SocketListener::remove(Socket &s)
{
	m_clients.erase(std::remove(m_clients.begin(), m_clients.end(), s), m_clients.end());
}

void SocketListener::clear(void)
{
	m_clients.clear();
}

Socket & SocketListener::select(int timeout)
{
	fd_set fds;
	timeval maxwait, *towait;
	int error;
	int fdmax;

	if (m_clients.size() == 0)
		throw Socket::ErrorException("No socket to listen to");
	
	fdmax = m_clients.front().getSocket();

	FD_ZERO(&fds);
	for (Socket &c : m_clients) {
		FD_SET(c.getSocket(), &fds);
		if ((int)c.getSocket() > fdmax)
			fdmax = c.getSocket();
	}

	maxwait.tv_sec = timeout;
	maxwait.tv_usec = 0;

        // Set to NULL for infinite timeout.
	towait = (timeout == 0) ? nullptr : &maxwait;

	error = ::select(fdmax + 1, &fds, NULL, NULL, towait);
	if (error == SOCKET_ERROR) 
		throw ErrorException(getLastSysError());
	if (error == 0)
		throw SocketListener::TimeoutException();

	for (Socket &c : m_clients)
		if (FD_ISSET(c.getSocket(), &fds))
			return c;

	throw ErrorException("No socket found");
}

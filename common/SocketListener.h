#ifndef _SOCKET_LISTENER_H_
#define _SOCKET_LISTENER_H_

#include <vector>

#include "Socket.h"

namespace irccd {

class SocketListener : public Socket {
public:
	class TimeoutException : public std::exception {
	public:
		virtual const char * what(void) const throw();
	};

private:
	std::vector<Socket> m_clients;

public:
	/**
	 * Add a socket to listen to.
	 *
	 * @param s the socket
	 */
	void add(Socket &s);

	/**
	 * Remove a socket from the list.
	 *
	 * @param s the socket
	 */
	void remove(Socket &s);

	/**
	 * Remove every sockets in the listener.
	 */
	void clear(void);

	/**
	 * Wait for an event in the socket liste.
	 *
	 * @param timeout an optional timeout, 0 means forever
	 * @return the socket ready
	 * @throw Socket::ErrorException on error
	 * @throw SocketListener::TimeoutException on timeout
	 */
	Socket & select(int timeout = 0);
};

} // !irccd

#endif // !_SOCKET_LISTENER_H_

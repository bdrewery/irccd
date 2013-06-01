#ifndef _SOCKET_TCP_H_
#define _SOCKET_TCP_H_

#include "Socket.h"

namespace irccd {

class SocketTCP : public Socket {
public:
	/**
	 * Default constructor
	 */
	SocketTCP(void);

	/**
	 * Constructor used for accept().
	 *
	 * @param sock the sock
	 * @param endPoint the client endPoint
	 */
	SocketTCP(socket_t sock, EndPoint endPoint);

	/**
	 * Default destructor.
	 */
	virtual ~SocketTCP(void);

	/**
	 * Create a socket with TCP type.
	 *
	 * @param family the family
	 * @throw Socket::ErrorException on error
	 */
	void create(int family);

	/**
	 * Try to connect to the specified end point.
	 *
	 * @param endPoint the endPoint
	 * @throw Socket::ErrorException on error
	 */
	void connect(EndPoint endPoint);

	/**
	 * Listen to a specific number of pending connections.
	 *
	 * @param max the max number of clients
	 * @throw Socket::ErrorException on error
	 */
	void listen(int max);

	/**
	 * Accept a client.
	 *
	 * @return a client ready to use
	 * @throw Socket::ErrorException on error
	 */
	SocketTCP accept(void);

	/**
	 * Receive some data.
	 *
	 * @param data the destination pointer
	 * @param dataLen max length to receive
	 * @return the number of bytes received
	 * @throw ErrorException on error
	 */
	unsigned recv(void *data, unsigned dataLen);

	/**
	 * Send some data.
	 *
	 * @param data the data to send
	 * @param dataLen the data length
	 * @return the number of bytes sent
	 * @throw ErrorException on error
	 */
	unsigned send(const void *data, unsigned dataLen);
};

bool operator<(const SocketTCP &s1, const SocketTCP &s2);
bool operator==(const SocketTCP &s1, const SocketTCP &s2);

} // !irccd

#endif // !_SOCKET_TCP_H_

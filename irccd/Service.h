/*
 * Service.h -- provide interruptible select(2) based services
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

#ifndef _IRCCD_SERVICE_H_
#define _IRCCD_SERVICE_H_

/**
 * @file Service.h
 * @brief Provide interruptible select(2) based call
 *
 * This class provide convenient way of defining a threadable class that
 * uses a SocketListener to monitor network activity.
 *
 * It provides stop(), start() and reload() function which can stop,
 * start and reload the thread respectively.
 *
 * It use a local UDP socket to interrupt the select(2) call immediately
 * if needed, thus making a very responsive application as we do not need
 * to wait that the select(2) timeouts before refreshing the sets.
 */

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>

#include <IrccdConfig.h>

#include <Signals.h>
#include <Socket.h>
#include <SocketAddress.h>
#include <SocketListener.h>

namespace irccd {

/**
 * @class ServiceSocketAbstract
 * @brief Base interface for service interruption implementation
 *
 * This class is used to store the underlying socket and to notify it when
 * appliable.
 *
 * For better performance, the socket must be placed non-blocking and silently
 * discard errors.
 */
class ServiceSocketAbstract {
public:
	/**
	 * Virtual destructor defaulted.
	 */
	virtual ~ServiceSocketAbstract() = default;

	/**
	 * Get the underlying socket as base type.
	 *
	 * @return the socket
	 */
	virtual SocketAbstract &socket() noexcept = 0;

	/**
	 * Send a small notification packet to notify the listener.
	 */
	virtual void notify() = 0;

	/**
	 * Flush the pending byte.
	 */
	virtual void flush() = 0;
};

/**
 * @class ServiceSocket
 * @brief Template wrapper that automatically define functions
 * @see ServiceSocketUnix
 * @see ServiceSocketIp
 */
template <typename Address>
class ServiceSocket : public ServiceSocketAbstract {
protected:
	SocketUdp<Address> m_socket;
	Address m_address;

	/**
	 * Constructor,
	 *
	 * 1. Create the socket
	 * 2. Bind it
	 * 3. Make it non blocking
	 *
	 * @param domain the domain (AF_INET, ...)
	 * @param address the address
	 */
	inline ServiceSocket(int domain, Address address)
		: m_socket{domain, 0}
		, m_address{std::move(address)}
	{
		m_socket.set(SOL_SOCKET, SO_REUSEADDR, 1);
		m_socket.setBlockMode(false);
		m_socket.bind(address);
	}

	/**
	 * @copydoc ServiceSocketAbstract::socket
	 */
	SocketAbstract &socket() noexcept override
	{
		return m_socket;
	}

	/**
	 * @copydoc ServiceSocketAbstract::notify
	 */
	void notify() override
	{
		static char dummyByte{1};

		m_socket.sendto(&dummyByte, sizeof (char), m_address);
	}

	/**
	 * @copydoc ServiceSocketAbstract::flush
	 */
	void flush() override
	{
		static char dummyByte;
		static Address dummyAddress;

		m_socket.recvfrom(&dummyByte, sizeof (char), dummyAddress);
	}
};

#if !defined(IRCCD_SYSTEM_WINDOWS)

/**
 * @class ServiceSocketUnix
 * @brief Interruption implemented as unix socket
 */
class ServiceSocketUnix : public ServiceSocket<address::Unix> {
private:
	std::string m_path;

public:
	inline ServiceSocketUnix(std::string path)
		: ServiceSocket{AF_LOCAL, address::Unix{path, true}}
		, m_path{std::move(path)}
	{
	}

	~ServiceSocketUnix() noexcept
	{
		::remove(m_path.c_str());
	}
};

#else

/**
 * @class ServiceSocketIp
 * @brief Interruption implemented as IP sockets
 */
class ServiceSocketIp : public ServiceSocket<address::Ipv4> {
public:
	inline ServiceSocketIp()
		: ServiceSocket{AF_INET, address::Ipv4{"127.0.0.1", 0}}
	{
		// Get the address back from the generated port
		auto address = m_socket.address();
		auto port = ntohs(reinterpret_cast<const sockaddr_in &>(address.address()).sin_port);

		m_address = address::Ipv4{"127.0.0.1", port};
	}
};

#endif

/**
 * @enum ServiceState
 * @brief Service thread state
 */
enum class ServiceState {
	Paused,			//!< The thread is actually paused
	Running,		//!< The thread is active and running
	Stopped			//!< The thread is completely stopped
};

/**
 * @class Service
 * @brief Provide an asynchronous I/O event loop
 *
 * This class register sockets into a SocketListener and wait for events on it for further
 * operations. It runs in a thread that is interruptible with a small UDP socket that is bound
 * to a file on Unix systems and in a random port on Windows.
 *
 * Just like the SocketListener, this class does not take ownership of the sockets, therefore they
 * must be kept somewhere.
 *
 * Most of the functions are thread-safe but whose who are not are meant to be called only by
 * the owner of the Service (e.g addAcceptor).
 */
class Service {
public:
	/*
	 * onAcceptor
	 * ------------------------------------------------
	 *
	 * This signal is emitted when an acceptor socket is ready for accepting
	 * a new connection.
	 *
	 * Arguments:
	 * - The socket ready for accept **not the new client socket**
	 */
	Signal<SocketAbstract &> onAcceptor;

	/**
	 * onIncoming
	 * ------------------------------------------------
	 *
	 * This signal is emitted when a client has incoming data available.
	 *
	 * Arguments:
	 * - The socket ready for read operation
	 */
	Signal<SocketAbstract &> onIncoming;

	/**
	 * onOutgoing
	 * ------------------------------------------------
	 *
	 * This signal is emitted when a client has outgoing data available.
	 *
	 * Arguments:
	 * - The socket ready for write.
	 */
	Signal<SocketAbstract &> onOutgoing;

	/**
	 * onError
	 * ------------------------------------------------
	 *
	 * This signal is emitted when an error occurs.
	 *
	 * Arguments:
	 * - The socket error
	 */
	Signal<const SocketError &> onError;

	/**
	 * onTimeout
	 * ------------------------------------------------
	 *
	 * This signal is emitted when the listener did timeout.
	 */
	Signal<> onTimeout;

private:
	enum class Owner {
		Service,
		Acceptor,
		Client
	};

	/* State, its lock and its condition variable */
	ServiceState m_state{ServiceState::Stopped};
	std::mutex m_mutexState;
	std::condition_variable m_condition;

	/* Socket listener and its mutex */
	SocketListener m_listener;
	int m_timeout;
	std::mutex m_mutexListener;

	/* Select interrupt interface and its mutex */
	std::unique_ptr<ServiceSocketAbstract> m_interface;
	std::mutex m_mutexInterface;

	/* Thread and mutex */
	std::atomic<bool> m_running{false};
	std::thread m_thread;
	std::string m_servname;

	/* Lookup table for acceptors and clients */
	std::set<SocketAbstract::Handle> m_acceptors;
	std::set<SocketAbstract::Handle> m_clients;

	void run();
	void flush();
	void notify();

	/*
	 * Determine the socket owner.
	 */
	inline Owner owner(SocketAbstract &sc) const noexcept
	{
		if (sc == m_interface->socket()) {
			return Owner::Service;
		}
		if (m_acceptors.count(sc.handle()) != 0) {
			return Owner::Acceptor;
		}
		if (m_clients.count(sc.handle()) != 0) {
			return Owner::Client;
		}

		throw std::runtime_error{"Unknown socket selected"};
	}

	/*
	 * Update the SocketListener safely by pausing the thread and locking
	 * the listener.
	 *
	 * The function is free to use m_listener safely.
	 *
	 * See set and unset.
	 */
	template <typename Operation>
	void updateListener(Operation &&func)
	{
		if (m_state != ServiceState::Paused) {
			pause();
		}

		{
			std::lock_guard<std::mutex> lock(m_mutexListener);

			// This will call SocketListener::set or SocketListener::unset
			func();
		}

		resume();
	}

public:
	/**
	 * Construct the service.
	 *
	 * This create the socket.
	 *
	 * @param timeout the time to wait in milliseconds
	 * @param name the service name (for debugging purposes)
	 * @param path the path to the Unix file (not needed on Windows)
	 */
	Service(int timeout, std::string name, std::string path);

	/**
	 * Virtual destructor defaulted.
	 *
	 * This function close the socket.
	 *
	 * @pre stop() must have been called
	 */
	virtual ~Service();

	/**
	 * Get the current service state.
	 *
	 * The service will not update its state by itself so it's perfectly safe to call this
	 * function to get the state from the service owner but it's not if multiple threads
	 * have access to the service.
	 *
	 * @return the state
	 * @warning Thread-safe with some considerations
	 */
	inline ServiceState state() const noexcept
	{
		return m_state;
	}

	/**
	 * Add a socket for accepting clients.
	 *
	 * This function should be called before starting the thread.
	 *
	 * @param sc the socket
	 * @pre state() must not be running
	 * @warning Not thread-safe
	 */
	inline void addAcceptor(SocketAbstract &sc)
	{
		assert(m_state != ServiceState::Running);

		m_acceptors.insert(sc.handle());
		m_listener.set(sc, SocketListener::Read);
	}

	/**
	 * Start the thread.
	 *
	 * @pre state() must return Stopped
	 * @warning Not thread-safe
	 */
	void start();

	/**
	 * Pause the thread.
	 *
	 * @pre state() must return Running
	 * @note Thread-safe
	 */
	void pause();

	/**
	 * Resume the thread.
	 *
	 * @pre state() must return Paused
	 * @note Thread-safe
	 */
	void resume();

	/**
	 * Stop the thread.
	 *
	 * @pre state() must return Running or Paused
	 * @warning Not thread-safe
	 */
	void stop();

	/**
	 * Set a socket for listening on input, write or both.
	 *
	 * @param sc the socket
	 * @param flags the flags (SocketListener::Read or SocketListener::Write)
	 * @note Thread-safe
	 */
	inline void set(SocketAbstract &sc, int flags)
	{
		updateListener([&] () {
			m_clients.insert(sc.handle());
			m_listener.set(sc, flags);
		});
	}

	/**
	 * Unregister a listening operation.
	 *
	 * @param sc the socket
	 * @param flags the flags (SocketListener::Read or SocketListener::Write)
	 * @note Thread-safe
	 */
	inline void unset(SocketAbstract &sc, int flags)
	{
		updateListener([&] () {
			m_listener.unset(sc, flags);
		});
	}

	/**
	 * Completely remove a client.
	 *
	 * @param sc the socket to remove
	 * @note Thread-safe
	 */
	inline void remove(SocketAbstract &sc)
	{
		updateListener([&] () {
			m_listener.remove(sc);
			m_clients.erase(sc.handle());
		});
	}
};

} // !irccd

#endif // !_IRCCD_SERVICE_H_

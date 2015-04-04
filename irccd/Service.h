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
#include <functional>
#include <mutex>
#include <thread>

#include <IrccdConfig.h>

#include <SocketAddress.h>
#include <SocketUdp.h>

/**
 * @enum ServiceAction
 * @brief Which action do execute
 */
enum class ServiceAction {
	Reload,			//!< Reload the listener
	Stop			//!< Stop the thread
};

namespace irccd {

/**
 * @class Service
 * @brief Provide a UDP socket for select interrupt
 *
 * The TransportService and ServerService use a select(2) based loop
 * to monitor sockets.
 *
 * Because these classes use threads and have a blocking select(2) call we
 * provide a UDP socket to interrupt the selection immediately.
 *
 * This can be used both to notify we have modified a socket I/O or because
 * we are shutting down irccd.
 *
 * A typical service run function should look like this:
 *
 * @code
 * void MyService::run()
 * {
 *   SocketListener listener;
 *
 *   while (isRunning()) {
 *     listener.set(socket(), SocketListener::Read);
 *
 *     // Fill with the service sockets here
 *
 *     SocketStatus st = listener.select();
 *
 *     if (isService(st.socket)) {
 *       if (action() == ServiceAction::Reload) {
 *         // Reload the sets
 *       } else {
 *         // isRunning() will return false, so just break the loop
 *         continue;
 *       }
 *     } else {
 *       // Do you service stuff here
 *   }
 * }
 * @endcode
 */
class Service {
private:
	/* Select interrupt */
	SocketUdp m_signal;
	SocketAddress m_address;

	/* Thread and mutex */
	std::atomic<bool> m_running{false};
	std::thread m_thread;
	std::string m_servname;

	/*
	 * Windows does not support Unix sockets and we require socket so we
	 * use a AF_INET address with a random port stored here.
	 *
	 * Otherwise, we use a Unix socket at a path specified by the derivated class.
	 */
#if !defined(IRCCD_SYSTEM_WINDOWS)
	std::string m_path;
#endif

protected:
	/**
	 * The mutex to use.
	 */
	mutable std::mutex m_mutex;

protected:
	/**
	 * The function to run when start() is called.
	 */
	virtual void run() = 0;

	/**
	 * Tell if the user selection has selected the service.
	 *
	 * @param s the selected socket
	 * @return true if the socket is the service
	 * @note Thread-safe
	 */
	bool isService(Socket &s) const noexcept;

	/**
	 * Get the socket to be put in the SocketListener.
	 *
	 * @return the sockets
	 * @note Thread-safe
	 */
	Socket &socket() noexcept;

	/**
	 * Tell which action must be taken.
	 *
	 * @return the action
	 * @note Thread-safe
	 */
	ServiceAction action();

public:
	/**
	 * Construct the service.
	 *
	 * This create the socket.
	 *
	 * @param name the service name (for debugging purposes)
	 * @param path the path to the Unix file (not needed on Windows)
	 */
	Service(std::string name, std::string path);

	/**
	 * Virtual destructor defaulted.
	 *
	 * This function close the socket.
	 *
	 * @pre stop() must have been called
	 */
	virtual ~Service();

	/**
	 * Check if the thread is running.
	 *
	 * @return true if running
	 * @note Thread-safe
	 */
	bool isRunning() const noexcept;

	/**
	 * Ask immediat reload.
	 *
	 * This function should be called from a different thread than the
	 * service.
	 *
	 * @pre isRunning() must return true
	 * @note Thread-safe
	 */
	void reload();

	/**
	 * Start the thread.
	 *
	 * @pre isRunning() must returm false
	 */
	virtual void start();

	/**
	 * Request to stop.
	 *
	 * This function should be called from a different thread than the
	 * service.
	 *
	 * This function does not close the socket so it can be reused.
	 *
	 * @note Thread-safe
	 * @pre isRunning() must return true
	 */
	virtual void stop();
};

} // !irccd

#endif // !_IRCCD_SERVICE_H_

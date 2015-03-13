#ifndef _IRCCD_TRANSPORT_MANAGER_H_
#define _IRCCD_TRANSPORT_MANAGER_H_

/**
 * @class TransportManager
 * @brief Manage all transports and the clients
 */

#include <atomic>
#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <thread>
#include <unordered_map>

#include "SocketUdp.h"
#include "Transport.h"
#include "TransportClient.h"

class JsonObject;

namespace irccd {

/**
 * @class TransportManager
 * @brief Manage transports and clients
 *
 * This class contains a transport for each one defined in the user
 * configuration, a thread waits for clients and receive their
 * messages for further usage.
 *
 * This class has also a socket for very basic IPC between Irccd and this
 * manager. This allows large timeout but quick reload of the listener
 * set in case of changes.
 */
class TransportManager {
public:
	using CommandHandler = void (TransportManager::*)(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);
	using CommandMap = std::unordered_map<std::string, CommandHandler>;

	/**
	 * @class Code
	 * @brief Command to control TransportManager
	 */
	enum Code {
		Reload,		//!< Reload the SocketListener set immediately
		Stop		//!< Stop the thread
	};

private:
	SocketUdp m_signal;
	SocketAddress m_signalAddress;

	/*
	 * Windows does not support Unix sockets and we require socket so we
	 * use a AF_INET address with a random port stored here.
	 *
	 * Otherwise, we use a Unix socket at a random path in the temporary
	 * directory.
	 */
#if !defined(_WIN32)
	std::string m_path;
#endif

	CommandMap m_commandMap;
	std::atomic<bool> m_running{false};
	std::map<Socket, std::unique_ptr<TransportAbstract>> m_transports;
	std::map<Socket, std::shared_ptr<TransportClientAbstract>> m_clients;
	std::thread m_thread;
	std::mutex m_mutex;

	// commands
	void cnotice(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);
	void connect(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);
	void disconnect(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);
	void invite(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);
	void load(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);
	void reload(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);
	void say(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);
	void unload(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);

	// transport slot
	void onMessage(const std::shared_ptr<TransportClientAbstract> &, const std::string &);
	void onWrite();
	void onDie(const std::shared_ptr<TransportClientAbstract> &);

	// private helpers
	void accept(const Socket &s);
	void process(const Socket &s, int direction);
	bool isTransport(const Socket &s) const noexcept;
	void run() noexcept;

public:
	/**
	 * Create the transport manager, this create the Udp IPC socket.
	 *
	 * @throw SocketError on errors
	 */
	TransportManager();

	/**
	 * Destructor, closes the thread and all sockets.
	 */
	~TransportManager();

	/**
	 * Create a new transport in-place.
	 *
	 * @pre isRunning() must return false
	 * @param args the arguments to the transport constructor
	 */
	template <typename T, typename... Args>
	void add(Args&&... args) noexcept
	{
		assert(!m_running);

		try {
			auto ptr = std::make_unique<T>(std::forward<Args>(args)...);

			ptr->bind();

			m_transports.emplace(ptr->socket(), std::move(ptr));
		} catch (const std::exception &ex) {
			printf("FAIL: %s\n", ex.what());
		}
	}

	/**
	 * Start the thread.
	 *
	 * @pre isRunning() must return false
	 */
	inline void start()
	{
		assert(!m_running);

		m_running = true;
		m_thread = std::thread(std::bind(&TransportManager::run, this));
	}

	/**
	 * Stop the thread and clean everything. This is called automatically
	 * from the destructor.
	 *
	 * @note Thread-safe
	 */
	void stop();

	/**
	 * Tell if the transport manager is currently running.
	 *
	 * @return 
	 */
	inline bool isRunning() const noexcept
	{
		return m_running;
	}

	/**
	 * Send a message to all connected clients.
	 */
	inline void broadcast(const std::string &msg)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		for (auto &tc : m_clients) {
			tc.second->send(msg);
		}
	}
};

} // !irccd

#endif // !_IRCCD_TRANSPORT_MANAGER_H_

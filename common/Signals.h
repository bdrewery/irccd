/*
 * Signals.h -- synchronous observer mechanism
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

#ifndef _IRCCD_SIGNALS_H_
#define _IRCCD_SIGNALS_H_

#include <functional>
#include <stack>
#include <unordered_map>
#include <vector>

/**
 * @file Signals.h
 * @brief Similar Qt signal subsystem for irccd
 */

namespace irccd {

/**
 * @class SignalConnection
 * @brief Stores the reference to the callable
 *
 * This class can be stored to remove a registered function from a Signal, be
 * careful to not mix connections between different signals as they are just
 * referenced by ids.
 */
class SignalConnection {
private:
	unsigned m_id;

public:
	/**
	 * Create a signal connection.
	 *
	 * @param id the id
	 */
	inline SignalConnection(unsigned id) noexcept
		: m_id{id}
	{
	}

	/**
	 * Get the reference object.
	 *
	 * @return the id
	 */
	inline unsigned id() const noexcept
	{
		return m_id;
	}
};

/**
 * @class Signal
 * @brief Stores and call registered functions
 *
 * This class is intended to be use as a public field in the desired object.
 *
 * The user just have to call one of connect(), disconnect() or the call
 * operator to use this class.
 *
 * It stores the callable as std::function so type-erasure is complete.
 *
 * The user is responsible of taking care that the object is still alive
 * in case that the function takes a reference to the object.
 */
template <typename... Args>
class Signal {
private:
	using Function = std::function<void (Args...)>;
	using FunctionMap = std::unordered_map<unsigned, Function>;
	using Stack = std::stack<unsigned>;

	FunctionMap m_functions;
	Stack m_stack;
	unsigned m_max{0};

public:
	/**
	 * Register a new function to the signal.
	 *
	 * @param function the function
	 * @return the connection in case you want to remove it
	 */
	inline SignalConnection connect(Function function) noexcept
	{
		unsigned id;

		if (!m_stack.empty()) {
			id = m_stack.top();
			m_stack.pop();
		} else {
			id = m_max ++;
		}

		m_functions.emplace(id, std::move(function));

		return SignalConnection{id};
	}

	/**
	 * Disconnect a connection.
	 *
	 * @param connection the connection
	 * @warning Be sure that the connection belongs to that signal
	 */
	inline void disconnect(const SignalConnection &connection) noexcept
	{
		auto value = m_functions.find(connection.id());

		if (value != m_functions.end()) {
			m_functions.erase(connection.id());
			m_stack.push(connection.id());
		}
	}

	/**
	 * Remove all registered functions.
	 */
	inline void clear()
	{
		m_functions.clear();
		m_max = 0;

		while (!m_stack.empty()) {
			m_stack.pop();
		}
	}

	/**
	 * Call every functions.
	 *
	 * @param args the arguments to pass to the signal
	 */
	void operator()(Args... args) const
	{
		/*
		 * Make a copy of the ids before iterating because the callbacks may eventually remove or modify
		 * the list.
		 */
		std::vector<unsigned> ids;

		for (auto &pair : m_functions) {
			ids.push_back(pair.first);
		}

		/*
		 * Now iterate while checking if the next id is still available, however if any new signals were
		 * added while iterating, they will not be called immediately.
		 */
		for (unsigned i : ids) {
			auto it = m_functions.find(i);

			if (it != m_functions.end()) {
				it->second(args...);
			}
		}
	}
};

} // !irccd

#endif // !_IRCCD_SIGNALS_H_

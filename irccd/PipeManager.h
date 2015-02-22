/*
 * PipeManager.h -- manage pipes
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

#ifndef _IRCCD_PIPE_MANAGER_H_
#define _IRCCD_PIPE_MANAGER_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

/**
 * @file PipeManager.h
 * @brief Manage pipes
 */

#include <Singleton.h>

namespace irccd {

class Pipe;

/**
 * @class PipeManager
 * @brief Get or delete pipes
 */
class PipeManager final : public Singleton<PipeManager> {
private:
	SINGLETON(PipeManager);

	using Mutex	= std::mutex;
	using Lock	= std::lock_guard<Mutex>;
	using Pipes	= std::unordered_map<std::string, std::shared_ptr<Pipe>>;

	Mutex		m_lock;
	Pipes		m_pipes;

public:
	/**
	 * Get (or create) a pipe.
	 *
	 * @param name the name
	 * @return the pipe ready to be used
	 */
	std::shared_ptr<Pipe> get(const std::string &name);

	/**
	 * Remove the pipe specified by name. Should only be called from the pipe destructor.
	 *
	 * @param name the pipe name
	 */
	void remove(const std::string &name);
};

} // !irccd

#endif // !_IRCCD_PIPE_MANAGER_H_

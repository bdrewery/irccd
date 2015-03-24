/*
 * IO.h -- general outgoint / incoming message
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

#ifndef _IRCCD_IO_H_
#define _IRCCD_IO_H_

/**
 * @file IO.h
 * @brief Incoming and outgoing messages
 *
 * This file is used with IRC events and server commands. It contains an optional
 * server and channel name to apply an encoding rule if needed.
 *
 * @warning Do not rename the include guard, it conflicts with a Windows header
 */

#include <string>

namespace irccd {

/**
 * @class IO
 * @brief The base IO class
 */
class IO {
private:
	std::string	m_serverName;
	std::string	m_targetName;

protected:
	bool		m_mustEncode { false };	//!< should encode ?
	std::string	m_encoding;		//!< the destination encoding

	/**
	 * Try to reencode the input to the specified encoding, returns
	 * the input string on failures.
	 *
	 * @param from the source encoding
	 * @param to the destination encoding
	 * @param input the input
	 * @return the converted string or input on failures
	 */
	std::string tryEncodeFull(const std::string &from, const std::string &to, const std::string &input) const;

public:
	/**
	 * Copy constructor disabled.
	 */
	IO(const IO &) = delete;

	/**
	 * Move constructor defaulted.
	 */
	IO(IO &&) = default;

	/**
	 * Default destructor.
	 */
	virtual ~IO() = default;

	/**
	 * Constructor with server name and target name.
	 *
	 * @param serverName the server name
	 * @param targetName the target name
	 */
	IO(std::string serverName, std::string targetName);

	/**
	 * Get the server name.
	 *
	 * @return the server name
	 */
	const std::string &server() const;

	/**
	 * Get the target name.
	 *
	 * @return the target name
	 */
	const std::string &target() const;

	/**
	 * Set the required encoding, this set m_mustEncode to true
	 * and the new m_encoding to encoding.
	 *
	 * @param encoding the desired encoding
	 */
	void encode(const std::string &encoding);

	/**
	 * Tells if the IO is empty, such as not suitable for
	 * rule matching.
	 *
	 * It returns true for private notices, queries and such.
	 *
	 * @return true if rule should not be applied
	 */
	virtual bool empty() const;

	/**
	 * The copy assignment is disabled.
	 */
	IO &operator=(const IO &) = delete;

	/**
	 * The move assignment is defaulted.
	 */
	IO &operator=(IO &&) = default;
};

} // !irccd

#endif // !_IRCCD_IO_H_

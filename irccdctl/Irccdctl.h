/*
 * Irccdctl.h -- irccd controller class
 *
 * Copyright (c) 2011, 2012, 2013 David Demelier <markand@malikania.fr>
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

#ifndef _IRCCDCTL_H_
#define _IRCCDCTL_H_

#include <string>

#include <config.h>

#include <Parser.h>
#include <SocketTCP.h>

namespace irccd {

class Irccdctl {
private:
	SocketTCP m_socket;
	std::string m_configPath;

#if !defined(_WIN32)
	void connectUnix(const Section &section);
#endif

	void connectInet(const Section &section);

	void readConfig(Parser &parser);
	void openConfig();

	void usage();
public:
	Irccdctl();
	~Irccdctl();

	/**
	 * Set the config path to open.
	 *
	 * @param path the config file path
	 */
	void setConfigPath(const std::string &path);

	/**
	 * Set the verbosity.
	 *
	 * @param verbose true means more verbose
	 */
	void setVerbosity(bool verbose);

	/**
	 * Send a raw message to irccd.
	 *
	 * @param message the message
	 */
	void sendRaw(const std::string &message);

	/**
	 * Get the server response.
	 *
	 * @return the code to return to main
	 */
	int getResponse();

	/**
	 * Run the application with the arguments.
	 *
	 * @param argc main argc
	 * @param argv main argv
	 * @return the return code to exit with
	 */
	int run(int argc, char **argv);
};

} // !irccd

#endif // !_IRCCDCTL_H_

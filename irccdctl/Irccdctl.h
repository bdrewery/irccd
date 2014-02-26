/*
 * Irccdctl.h -- irccd controller class
 *
 * Copyright (c) 2013, 2014 David Demelier <markand@malikania.fr>
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
#include <unordered_map>

#include <config.h>

#include <Parser.h>
#include <SocketAddress.h>

namespace irccd {

class Irccdctl {
private:
	using Args = std::unordered_map<char, std::string>;

	Socket m_socket;
	SocketAddress m_addr;
	std::string m_configPath;
	Args m_args;
	bool m_needResponse;

	// For specifying socket to connect at command line
	bool m_readConfig;
	int m_domain;
	int m_type;

	// If defined at command line (internet)
	std::string m_host;
	int m_port;

#if !defined(_WIN32)
	bool m_removeFiles;
	std::string m_tmpDir;
	std::string m_tmpPath;

	// If defined at command line (unix)
	std::string m_unixPath;

	void loadUnix(const Section &section);
	void connectUnix();

	void removeUnixFiles();
#endif

	void loadInet(const Section &section);
	void connectInet();

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
	 * Specify a socket by command line for AF_INET[6].
	 *
	 * @param host the hostname
	 * @param port the port number
	 * @param domain the domain AF_INET or AF_INET6
	 * @param type the type SOCK_STREAM or SOCK_DGRAM
	 */
	void useInternet(const std::string &host,
			 int port,
			 int domain,
			 int type);

#if !defined(_WIN32)
	/**
	 * Specify a socket by command line for Unix.
	 *
	 * @param path the path
	 * @param type the type SOCK_STREAM or SOCK_DGRAM
	 */
	void useUnix(const std::string &path, int type);
#endif

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
	 * Add an optional argument 
	 *
	 * @param c the character used
	 * @param arg the value
	 */
	void addArg(char c, const std::string &arg);

	bool hasArg(char c);

	const std::string &getArg(char c);

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

/*
 * Irccdctl.h -- irccd controller class
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

#ifndef _IRCCDCTL_H_
#define _IRCCDCTL_H_

/**
 * @file Irccdctl.h
 * @brief Main irccdctl class
 */

#include <string>
#include <unordered_map>

#include <SocketTcp.h>
#include <Util.h>

namespace irccd {

class IniSection;

class Irccdctl {
private:
	using Helper = void (Irccdctl::*)() const;
	using Handler = void (Irccdctl::*)(int argc, char **argv);

	/* Socket for connecting */
	std::unique_ptr<SocketTcp> m_socket;

	/* Commands and help */
	std::unordered_map<std::string, Helper> m_helpers;
	std::unordered_map<std::string, Handler> m_handlers;

	/* Help messages */
	void helpChannelNotice() const;
	void helpConnect() const;
	void helpDisconnect() const;
	void helpInvite() const;
	void helpJoin() const;
	void helpKick() const;
	void helpLoad() const;
	void helpMe() const;
	void helpMessage() const;
	void helpMode() const;
	void helpNick() const;
	void helpNotice() const;
	void helpPart() const;
	void helpReconnect() const;
	void helpReload() const;
	void helpTopic() const;
	void helpUnload() const;
	void helpUserMode() const;

	/* Commands */
	void handleHelp(int, char **);
	void handleChannelNotice(int, char **);
	void handleConnect(int, char **);
	void handleDisconnect(int, char **);
	void handleInvite(int, char **);
	void handleJoin(int, char **);
	void handleKick(int, char **);
	void handleLoad(int, char **);
	void handleMe(int, char **);
	void handleMessage(int, char **);
	void handleMode(int, char **);
	void handleNick(int, char **);
	void handleNotice(int, char **);
	void handlePart(int, char **);
	void handleReconnect(int, char **);
	void handleReload(int, char **);
	void handleTopic(int, char **);
	void handleUnload(int, char **);
	void handleUserMode(int, char **);

	/* Private functions */
	void send(std::string message);
	void response();

	void usage();

	void loadGeneral(const IniSection &sc);
	void loadSocket(const IniSection &sc);
	void loadConfig();

public:
	Irccdctl();

	/**
	 * Define an option from command line.
	 *
	 * @param name the option name (short or long)
	 * @param value the value
	 */
	void define(std::string name, std::string value);

	/**
	 * Execute the client command.
	 *
	 * The caller must remove the initial argv[0] which contains the
	 * program name and reduce the argc from one.
	 *
	 * @param argc the number of arguments
	 * @param argv the arguments
	 * @return 0 or 1
	 */
	int exec(int argc, char **argv) noexcept;

};

} // !irccd

#if 0

#include <string>
#include <unordered_map>

#include <IrccdConfig.h>

#include <Parser.h>
#include <SocketAddress.h>

namespace irccd {

/**
 * @class Irccdctl
 * @brief Main irccdctl class
 */
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

	/**
	 * Check if argument is set.
	 *
	 * @param c the option
	 * @return true if set
	 */
	bool hasArg(char c);

	/**
	 * Get the argument value.
	 *
	 * @param c the option
	 * @return the value
	 */
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

#endif

#endif // !_IRCCDCTL_H_

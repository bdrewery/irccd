/*
 * main.cpp -- irccd controller main
 *
 * Copyright (c) 2013 David Demelier <markand@malikania.fr>
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

#include <iostream>
#include <unordered_map>
#include <stdexcept>
#include <string>

#include <Logger.h>

#include "Irccdctl.h"

using namespace irccd;
using namespace std;

void verifyParams(const unordered_map<char, string> &params)
{
	try {
		string type;
		string protocol;

		if (params.find('t') == params.end())
			throw runtime_error("irccdctl: no type given (-t internet or unix)");
		if (params.find('T') == params.end())
			throw runtime_error("irccdctl: no protocol given (-T tcp or udp)");

		type = params.at('t');
		protocol = params.at('T');

		if (protocol != "tcp" && protocol != "udp")
			throw runtime_error("irccdctl: invalid protocol `" + protocol + "'");

		if (type == "internet") {
			if (params.find('4') == params.end() && params.find('6') == params.end())
				throw runtime_error("irccdctl: no family given (-4 or -6)");
			if (params.find('h') == params.end())
				throw runtime_error("irccdctl: no hostname given (-h)");
			if (params.find('p') == params.end())
				throw runtime_error("irccdctl: no port given (-p)");
		} else if (type == "unix") {
#if defined(_WIN32)
			Logger::fatal(1, "irccdctl: unix sockets are not supported on Windows");
#else
			if (params.find('P') == params.end())
				throw runtime_error("irccdctl: no path given (-P)");
#endif
		} else
			Logger::fatal(1, "irccdctl: invalid type `%s'", type.c_str());
	} catch (runtime_error error) {
		Logger::fatal(1, error.what());
	}
}

void useParams(Irccdctl &ctl, const unordered_map<char, string> &params)
{
	string domstr	= params.at('t');
	string proto	= params.at('T');
	int domain, type;

	type = (proto == "tcp") ? SOCK_STREAM : SOCK_DGRAM;

	if (domstr == "internet") {
		if (params.find('4') != params.end())
			domain = AF_INET;
		else
			domain = AF_INET6;

		try {
			ctl.useInternet(
			    params.at('h'),
			    stol(params.at('p')),
			    domain,
			    type
			);
		} catch (...) {
			Logger::fatal(1, "socket: invalid port number `%s'",
			    params.at('p').c_str());
		}
	} else {
#if !defined(_WIN32)
		ctl.useUnix(params.at('P'), type);
#endif
	}
}

int main(int argc, char **argv)
{
	Irccdctl ctl;
	unordered_map<char, string> params;
	int ch;

	setprogname("irccdctl");

	opterr = false;
	while ((ch = getopt(argc, argv, "46c:h:i:k:P:p:T:t:v")) != -1) {
		switch (ch)
		{
		case '4':
		case '6':
			params[ch] = "1";			// IPv4 or IPv6?
			break;
		case 'c':
			ctl.setConfigPath(optarg);
			break;
		case 'i':					// identity
		case 'k':					// key (password)
			ctl.addArg(ch, optarg);
			break;
		case 'h':					// host
		case 'P':					// unix path
		case 'p':					// port
		case 'T':					// protocol
		case 't':					// internet or unix
			params[ch] = optarg;
			break;
		case 'v':
			ctl.setVerbosity(true);
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (params.size() > 0) {
		verifyParams(params);
		useParams(ctl, params);
	}

	return ctl.run(argc, argv);
}

/*
 * SocketClient.cpp -- client socket stream
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

#include "SocketClient.h"

using namespace irccd;
using namespace std;

void SocketClient::addData(const char *data)
{
	m_buffer << data;
}

bool SocketClient::isFinished(void) const
{
	string cmd = m_buffer.str();

	return cmd.length() > 0 && cmd[cmd.length() - 1] == '\n';
}

string SocketClient::getCommand(void)
{
	string cmd = m_buffer.str();
	string result;

	if (cmd.length() > 0 && cmd[cmd.length() - 1] == '\n') {
		result = cmd.substr(0, cmd.length() - 1);

		// remove possible '\r'
		if (result.length() > 0 && result[result.length() - 1] == '\r')
			result[result.length() - 1] = '\0';

		m_buffer.str("");
	}

	return result;
}

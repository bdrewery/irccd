/*
 * Util.cpp -- some utilities
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

#include <sstream>

#include <basedir.h>

#include "Util.h"

using namespace irccd;
using namespace std;

vector<string> Util::split(const string &list, const string &delimiter, int max)
{
	vector<string> result;
	size_t next = -1, current;
	int count = 1;
	bool finished = false;

	do {
		string val;

		current = next + 1;
		next = list.find_first_of(delimiter, current);

		// split max, get until the end
		if (max >= 0 && count++ >= max) {
			val = list.substr(current, string::npos);
			finished = true;
		} else{
			val = list.substr(current, next - current);
			finished = next == string::npos;
		}

		result.push_back(val);
	} while (!finished);

	return result;
}

string Util::configDirectory(void)
{
	xdgHandle handle;
	ostringstream oss;

	if ((xdgInitHandle(&handle)) == nullptr) {
		const char *home = getenv("HOME");

		if (home != nullptr)
			oss << home;

		// append default path.
		oss << "/.config/irccd/";
	} else {
		oss << xdgConfigHome(&handle);
		oss << "/irccd/";
	}

	xdgWipeHandle(&handle);

	return oss.str();
}

string Util::configFilePath(const std::string filename)
{
	ostringstream oss;

	oss << Util::configDirectory();
	oss << filename;

	return oss.str();
}

/*
 * main.cpp -- linkify executable
 *
 * Copyright (c) 2014 David Demelier <markand@malikania.fr>
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

/*
 * This executable allows the documentation process to update links relative to
 * the directory where the files are installed.
 *
 * For example:
 *
 * doc
 *   | -- index.html
 *   | -- level-1
 *   |        | -- index.html
 *
 * If the level-1/index.html files wants to refer to the top level index.html,
 * then a relative link of "../" is prepended.
 *
 * The file which have @baseurl@ strings are replaced to the appropriate root
 * directory.
 *
 * For example:
 *
 * When invoking linkify with /usr/share/doc /usr/share/doc/irccd/foo, the link
 * will be resolved to "../../".
 *
 * This make documentation more easier to write and to redistribute.
 */

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <string>

using namespace std;
using namespace std::placeholders;

namespace {

void usage()
{
	cerr << "usage: linkify input output base directory" << endl;
	exit(1);
}

constexpr bool ispath(char a, char b)
{
	return a == '/' && b == '/';
}

string clean(string path)
{
	replace(path.begin(), path.end(), '\\', '/');
	path.erase(unique(path.begin(), path.end(), ispath), path.end());

	while (path.size() > 1 && path[path.size() - 1] == '/') {
		path.pop_back();
	}

	return path;
}

string relative(string from, string to)
{
	string result{"."};

	from = clean(from);
	to = clean(to);

	while (from != to) {
		string::size_type pos = from.rfind("/");

		if (pos == string::npos) {
			throw std::invalid_argument("no common parent directory found");
		}

		from.erase(pos);
		result += "/..";
	}

	return result;
}

string replace(string content, const string &replacement)
{
	bool done{false};

	do {
		auto position = content.find("@baseurl@");

		if (position == string::npos) {
			done = true;
		} else {
			content.erase(position, /* @baseurl@ */ 9);
			content.insert(position, replacement);
		}
	} while (!done);

	return content;
}

} // !namespace

int main(int argc, char **argv)
{
	-- argc;
	++ argv;

	if (argc < 4)
		usage();
		// NOTREACHED

	string replacement = relative(argv[3], argv[2]);
	string content;
	ifstream input(argv[0]);

	copy(istreambuf_iterator<char>(input), istreambuf_iterator<char>(), back_inserter(content));

	try {
		ofstream out(argv[1], ofstream::out);

		out << replace(move(content), move(replacement)) << endl;
	} catch (const exception &) {
		return 1;
	}

	return 0;
}

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
	cerr << "usage: templator base directory" << endl;
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

	while (path.size() > 1 && path[path.size() - 1] == '/')
		path.pop_back();

	return path;
}

string baseName(const string &path)
{
	string result;

	auto first = path.rbegin();
	auto last = path.rend();

	while (first != last && *first != '/')
		result.insert(result.begin(), *(first++));

	return result;
}

string relative(string base, string path)
{
	string result;
	bool done{false};

	base = clean(base);

	do {
		path = clean(path);

		auto b1 = baseName(base);
		auto b2 = baseName(path);

		if (b1 == b2) {
			done = true;
		} else if (b2.size() == 0) {
			throw invalid_argument("no common parent directory found");
		} else {
			path.erase(path.end() - b2.size(), path.end());
			result += "../";
		}
	} while (!done);

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

	string replacement = relative(argv[2], argv[3]);
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

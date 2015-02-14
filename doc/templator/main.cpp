/*
 * main.cpp -- templator executable
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
#include <iostream>
#include <iterator>
#include <regex>
#include <string>
#include <unordered_map>

using namespace std;
using namespace std::literals::string_literals;

using Variables = unordered_map<string, string>;

namespace {

void usage()
{
	cerr << "usage: templator source destination [var1=file1 [var2=file2 [...]]]" << endl;
	exit(1);
}

void add(Variables &vars, const string &varname, const string &filename)
{
	ifstream file;
	string content;

	file.open(filename, ifstream::in);

	if (!file.is_open()) {
		cerr << "failed to open: " << filename << endl;
		exit(1);
	}

	copy(istreambuf_iterator<char>(file), istreambuf_iterator<char>(), back_inserter(content));
	vars.insert({varname, content});	
}

int process(const Variables &vars, const string &inputpath, const string &outputpath)
{
	ifstream input;
	ofstream output;
	string content;

	input.open(inputpath, ifstream::in);
	output.open(outputpath, ofstream::out);

	if (!input.is_open()) {
		cerr << "failed to open: " << inputpath << endl;
		exit(1);
	}
	if (!output.is_open()) {
		cerr << "failed to open: " << outputpath << endl;
		exit(1);
	}

	// Read all
	copy(istreambuf_iterator<char>(input), istreambuf_iterator<char>(), back_inserter(content));

	// Extract forever
	regex regex{"%(\\w+)%"};
	smatch match;

	for (;;) {
		if (!regex_search(content, match, regex))
			break;

		auto name = match[1].str();

		if (vars.count(name) == 0) {
			cerr << "error: " << name << " found in file but not defined" << endl;
			exit(1);
		}

		auto position = content.find("%"s + name + "%"s);
		auto value = vars.at(name);

		// Erase
		content.erase(begin(content) + position, begin(content) + position + (name.length() + 2));

		// Insert
		content.insert(begin(content) + position, begin(value), end(value));
	}

	// Write all
	copy(begin(content), end(content), ostreambuf_iterator<char>(output));

	return 0;
}

} // !namespace

int main(int argc, char **argv)
{
	Variables vars;

	if (argc < 3)
		usage();
		// NOTREACHED

	for (int i = 3; i < argc; ++i) {
		regex regex{"^([^=]*)=(.*)$"};
		string arg{argv[i]};
		smatch match;

		if (!regex_match(arg, match, regex)) {
			cerr << "invalid format: " << argv[i] << endl;
			usage();
			// NOTREACHED
		}

		add(vars, match[1].str(), match[2].str());
	}

	return process(vars, argv[1], argv[2]);
}

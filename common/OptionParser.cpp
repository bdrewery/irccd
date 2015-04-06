/*
 * OptionParser.cpp -- command line option parser
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

#include <algorithm>

#include "OptionParser.h"

namespace irccd {

bool OptionParser::isShort(const std::string &arg) const
{
	return arg.size() >= 2 && arg[0] == '-' && arg[1] != '-';
}

bool OptionParser::isLong(const std::string &arg) const
{
	return arg.size() >= 3 && arg[0] == '-' && arg[1] == '-' && arg[2] != '-';
}

bool OptionParser::isOption(const std::string &arg) const
{
	return isShort(arg) || isLong(arg);
}

std::string OptionParser::key(const std::string &arg) const
{
	if (isShort(arg))
		return arg.substr(1, 1);

	return arg.substr(2);
}

bool OptionParser::isShortCompacted(const std::string &arg) const
{
	return arg.size() >= 3;
}

bool OptionParser::isDefined(const std::string &arg) const
{
	auto n = key(arg);
	auto it = std::find_if(m_options.begin(), m_options.end(), [&] (const Option &o) -> bool {
		return o.key() == n || o.full() == n;
	});

	return it != m_options.end();
}

const Option &OptionParser::get(const std::string &arg) const
{
	std::string n = key(arg);

	return *std::find_if(m_options.begin(), m_options.end(), [&] (const Option &o) -> bool {
		return o.key() == n || o.full() == n;
	});
}

bool OptionParser::isToggle(const std::string &arg) const
{
	return (get(arg).flags() & Option::NoArg);
}

void OptionParser::readShort(OptionPack &pack, Args::const_iterator &it, Args::const_iterator end) const
{
	/*
	 * There are many options when passing a short option:
	 *
	 * 1. -cmyconfig	Takes on argument but parsed as unique,
	 * 2. -c myconfig	Takes on argument but parsed as two strings
	 * 3. -abc		If a is not a toggle option, its argument is `bc'
	 * 4. -abc		If a is a toggle option and b, c are toggle, they are added
	 */

	std::string v = it->substr(2);
	std::string k = key(*it);
	const Option &option = get(std::string("-") + k);

	if (isToggle(*it)) {
		// 3. and optionally 4.
		pack.push_back(OptionValue(option, ""));
		pack.m_argsParsed += 1;

		if (isShortCompacted(*it)) {
			for (char c : v) {
				if (!isDefined("-" + std::string(1, c))) {
					pack.m_error = "-" + std::string(1, c) + " is not a valid option";
					break;
				}

				const Option &sub = get("-" + std::string(1, c));

				pack.push_back(OptionValue(sub, ""));
			}
		}

		++ it;
	} else {
		// 1.
		if (isShortCompacted(*it++)) {
			pack.push_back(OptionValue(option, v));
			pack.m_argsParsed += 1;
		} else {
			// 2.
			if (it == end) {
				pack.m_error = option.key() + " requires an option";
			} else {
				pack.push_back(OptionValue(option, *it++));
				pack.m_argsParsed += 2;
			}
		}
	}
}

void OptionParser::readFull(OptionPack &pack, Args::const_iterator &it, Args::const_iterator end) const
{
	/*
	 * Long options can't be compacted, there are only two possibilities:
	 *
	 * 1. --fullscreen	No argument
	 * 2. --config foo	One argument
	 */
	const Option &option = get(*it);

	if (!isToggle(*it)) {
		// 2.
		if (++it == end) {
			pack.m_error = "--" + option.full() + " requires an option";
		} else {
			pack.push_back(OptionValue(option, *it++));
			pack.m_argsParsed += 2;
		}
	} else {
		pack.push_back(OptionValue(option, ""));
		pack.m_argsParsed ++;

		++ it;
	}
}

OptionParser::OptionParser(std::initializer_list<Option> options)
	: m_options(options.begin(), options.end())
{
}

OptionParser::OptionParser(std::vector<Option> options)
	: m_options(std::move(options))
{
}

OptionPack OptionParser::parse(Args::const_iterator it, Args::const_iterator end, int flags) const
{
	OptionPack pack;

	while (it != end) {
		if (!isOption(*it)) {
			if (flags & Unstrict) {
				pack.m_argsParsed ++;
				it ++;
				continue;
			} else {
				pack.m_error = *it + " is not an option";
				return pack;
			}
		}

		if (!isDefined(*it)) {
			pack.m_error = *it + ": invalid option";
			return pack;
		}

		if (isShort(*it)) {
			readShort(pack, it, end);
		} else {
			readFull(pack, it, end);
		}

		// Read failure
		if (pack.m_error != "No error") {
			return pack;
		}
	}

	return pack;
}

OptionPack OptionParser::parse(int argc, char **argv, int flags) const
{
	std::vector<std::string> args;

	for (int i = 0; i < argc; ++i)
		args.push_back(argv[i]);

	return parse(args, flags);
}

OptionPack OptionParser::parse(const std::vector<std::string> &args, int flags) const
{
	return parse(args.begin(), args.end(), flags);
}

} // !irccd

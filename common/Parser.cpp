/*
 * Parser.h -- config file parser
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

#include <cstring>
#include <cerrno>
#include <iostream>
#include <fstream>

#include "Parser.h"

namespace irccd {

/* --------------------------------------------------------
 * Section public members
 * -------------------------------------------------------- */

Section::Section()
	: m_allowed(true)
{
}

Section::Section(const std::string &name)
	: m_name(name)
	, m_allowed(true)
{

}

const std::string &Section::getName() const
{
	return m_name;
}

bool Section::hasOption(const std::string &name) const
{
	return m_options.count(name) >= 1;
}

Section::Map::iterator Section::begin()
{
	return m_options.begin();
}

Section::Map::const_iterator Section::cbegin() const
{
	return m_options.cbegin();
}

Section::Map::iterator Section::end()
{
	return m_options.end();
}

Section::Map::const_iterator Section::cend() const
{
	return m_options.end();
}

bool operator==(const Section &s1, const Section &s2)
{
	return s1.m_name == s2.m_name && s1.m_options == s2.m_options;
}

/* --------------------------------------------------------
 * Parser private members
 * -------------------------------------------------------- */

void Parser::addOption(const std::string &key, const std::string &value)
{
	m_sections.back().m_options.insert(std::make_pair(key, value));
}

void Parser::readSection(int lineno, const std::string &line)
{
	size_t end;

	if ((end = line.find_first_of(']')) != std::string::npos) {
		if (end > 1) {
			auto name = line.substr(1, end - 1);

			/*
			 * Check if we can add a section, if redefinition is
			 * disabled, we must disable the previous section so the
			 * further read options should not be enabled until
			 * a correct section is found again.
			 */
			if (hasSection(name) && (m_tuning & DisableRedefinition)) {
				if (!(m_tuning & DisableVerbosity))
					log(lineno, name, "redefinition not allowed");
				m_sections.back().m_allowed = false;
			} else {
				m_sections.push_back(Section(name));
			}
		} else if (!(m_tuning & DisableVerbosity)) {
			/*
			 * Do not add options at this step because it will
			 * corrupt the previous one.
			 */
			m_sections.back().m_allowed = false;
			log(lineno, "", "empty section name");
		}
	}
}

void Parser::readOption(int lineno, const std::string &line)
{
	auto &current = m_sections.back();
	size_t epos;
	std::string key, value;

	// Error on last section?
	if (!current.m_allowed) {
		/*
		 * If it is the root section, this has been probably set by
		 * DisableRootSection flag, otherwise an error has occured
		 * so no need to log.
		 */
		if (current.m_name == "" && !(m_tuning == DisableVerbosity))
			log(lineno, "", "option not allowed in that scope");

		return;
	}

	if ((epos = line.find_first_of('=')) == std::string::npos) {
		if (!(m_tuning & DisableVerbosity))
			log(lineno, current.m_name, "missing `=' keyword");
		return;
	}

	if (epos > 0) {
		size_t i, begin, last;
		char c;

		key = line.substr(0, epos - 1);
		value = line.substr(epos + 1);

		// clean option key
		for (i = 0; !isspace(key[i]) && i < key.length(); ++i)
			continue;
		key = key.substr(0, i);

		// clean option value
		for (begin = 0; isspace(value[begin]) && begin < value.length(); ++begin)
			continue;
		value = value.substr(begin);
	
		c = value[0];
		begin = 0;
		if (c == '\'' || c == '"') {
			for (last = begin = 1; value[last] != c && last < value.length(); ++last)
				continue;
			if (value[last] != c && !(m_tuning & DisableVerbosity))
				if (!(m_tuning & DisableVerbosity))
					log(lineno, current.m_name, "undisclosed std::string");
		} else {
			for (last = begin; !isspace(value[last]) && last < value.length(); ++last)
				continue;
		}

		if (last - begin > 0)
			value = value.substr(begin, last - begin);
		else
			value.clear();

		// Add the option if the key is not empty
		if (key.length() > 0)
			addOption(key, value);
	}
}

void Parser::readLine(int lineno, const std::string &line)
{
	size_t i;
	std::string buffer;

	// Skip default spaces
	for (i = 0; isspace(line[i]) && i < line.length(); ++i)
		continue;

	buffer = line.substr(i);
	if (buffer.length() > 0) {
		if (buffer[0] != m_commentChar) {
			if (buffer[0] == '[')
				readSection(lineno, buffer);
			else
				readOption(lineno, buffer);
		}
	}
}

/* --------------------------------------------------------
 * Parser public methods
 * -------------------------------------------------------- */

const char Parser::DEFAULT_COMMENT_CHAR = '#';

void Parser::open()
{
	std::ifstream file;
	std::string line;
	int lineno = 1;

	file.open(m_path.c_str());
	if (!file.is_open())
		throw std::runtime_error(m_path + std::string(std::strerror(errno)));

	while (std::getline(file, line))
		readLine(lineno++, line);

	file.close();
}

Parser::Parser()
{
}

Parser::Parser(const std::string &path, int tuning, char commentToken)
	: m_path(path)
	, m_tuning(tuning)
	, m_commentChar(commentToken)
{
	Section s("");

	s.m_allowed = (tuning & DisableRootSection) ? false : true;

	m_sections.push_back(s);
	open();
}

Parser::~Parser()
{
}

Parser::List::iterator Parser::begin()
{
	return m_sections.begin();
}

Parser::List::const_iterator Parser::cbegin() const
{
	return m_sections.cbegin();
}

Parser::List::iterator Parser::end()
{
	return m_sections.end();
}

Parser::List::const_iterator Parser::cend() const
{
	return m_sections.end();
}

void Parser::findSections(const std::string &name, FindFunc func) const
{
	for (const auto &s : m_sections)
		if (s.m_name == name)
			func(s);
}

bool Parser::hasSection(const std::string &name) const
{
	for (const auto &s : m_sections)
		if (s.m_name == name)
			return true;

	return false;
}

const Section &Parser::getSection(const std::string &name) const
{
	for (const auto &s : m_sections)
		if (s.m_name == name)
			return s;

	throw std::out_of_range(name + " not found");
}

void Parser::log(int number, const std::string &, const std::string &message)
{
	std::cout << "line " << number << ": " << message << std::endl;
}

bool operator==(const Parser &p1, const Parser &p2)
{
	return p1.m_sections == p2.m_sections &&
	    p1.m_path == p2.m_path &&
	    p1.m_tuning == p2.m_tuning &&
	    p1.m_commentChar == p2.m_commentChar;
}

} // !irccd

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

#include <iostream>
#include <fstream>

#include "Parser.h"

namespace irccd
{

/* --------------------------------------------------------
 * Option public members
 * -------------------------------------------------------- */

bool operator==(const Option &o1, const Option &o2)
{
	return o1.m_key == o2.m_key &&
	    o1.m_value == o2.m_value;
}

/* --------------------------------------------------------
 * Section public members
 * -------------------------------------------------------- */

Section::Section()
	:m_allowed(true)
{
}

Section::~Section()
{
}

Section::Section(const Section &s)
{
	m_name = s.m_name;
	m_options = s.m_options;
	m_allowed = s.m_allowed;
}

const std::string Section::findOption(const std::string &name) const
{
	std::string ret;

	for (const Option &o : m_options)
		if (o.m_key == name) {
			ret = o.m_value;
			break;
		}

	return ret;
}

template <> bool Section::getOption(const std::string &name) const
{
	bool result = false;

	if (hasOption(name)) {
		std::string value = findOption(name);

		if (value == "yes" || value == "true"|| value == "1")
			result = true;
		else if (value == "no" || value == "false" || value == "0")
			result = false;
	}

	return result;
}

template <> int Section::getOption(const std::string &name) const
{
	int result = -1;

	if (hasOption(name)) {
		result = atoi(findOption(name).c_str());
	}

	return result;
}

template <> std::string Section::getOption(const std::string &name) const
{
	std::string result;

	if (hasOption(name))
		result = findOption(name);

	return result;
}

const std::string & Section::getName() const
{
	return m_name;
}

const std::vector<Option> & Section::getOptions() const
{
	return m_options;
}

bool Section::hasOption(const std::string &name) const
{
	for (const Option &o : m_options)
		if (o.m_key == name)
			return true;

	return false;
}

bool operator==(const Section &s1, const Section &s2)
{
	if (s1.m_name != s2.m_name)	
		return false;

	return s1.m_options == s2.m_options;
}

/* --------------------------------------------------------
 * Parser private members
 * -------------------------------------------------------- */

void Parser::addSection(const std::string &name)
{
	Section section;

	section.m_name = name;
	section.m_allowed = true;

	m_sections.push_back(section);
}

void Parser::addOption(const std::string &key, const std::string &value)
{
	Option option;
	Section &current = m_sections.back();

	option.m_key = key;
	option.m_value = value;

	current.m_options.push_back(option);
}

void Parser::readSection(int lineno, const std::string &line)
{
	size_t end;

	if ((end = line.find_first_of(']')) != std::string::npos) {
		if (end > 1) {
			std::string name = line.substr(1, end - 1);

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
				addSection(name);
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
	size_t epos;
	std::string key, value;
	Section &current = m_sections.back();

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

Parser::Parser(const std::string &path, int tuning, char commentToken)
	:m_path(path), m_tuning(tuning), m_commentChar(commentToken)
{
	Section root;

	// Add a default root section
	root.m_name = "";
	root.m_allowed = (tuning & DisableRootSection) ? false : true;

	m_sections.push_back(root);
}

Parser::Parser()
{
}

Parser::~Parser()
{
}

bool Parser::open()
{
	std::ifstream file;
	std::string line;
	int lineno = 1;

	file.open(m_path.c_str());
	if (!file.is_open()) {
		m_error = "could not open file " + m_path;	// XXX: add a real error
		return false;
	}

	// Avoid use of C getline
	while (std::getline(file, line)) {
		readLine(lineno++, line);
	}

	file.close();

	return true;
}

const std::string & Parser::getError() const
{
	return m_error;
}

const std::vector<Section> & Parser::getSections() const
{
	return m_sections;
}

std::vector<Section> Parser::findSections(const std::string &name) const
{
	std::vector<Section> list;

	for (const Section &s : m_sections) {
		if (s.m_name == name) {
			Section copy = s;
			list.push_back(copy);
		}
	}

	return list;
}

bool Parser::hasSection(const std::string &name) const
{
	for (const Section &s : m_sections)
		if (s.m_name == name)
			return true;

	return false;
}

Section Parser::getSection(const std::string &name) const
{
	Section ret;	

	for (const Section &s : m_sections)
		if (s.m_name == name)
			ret = s;

	return ret;
}

Section Parser::requireSection(const std::string &name) const
{
	if (!hasSection(name))
		throw NotFoundException(name);

	return getSection(name);
}

void Parser::log(int number, const std::string &, const std::string &message)
{
	std::cout << "line " << number << ": " << message << std::endl;
}

void Parser::dump()
{
	for (auto s : m_sections) {
		dumpSection(s);
		for (auto o : s.m_options)
			dumpOption(o);
	}
}

void Parser::dumpSection(const Section &section)
{
	std::cout << "Section " << section.m_name << std::endl;
}

void Parser::dumpOption(const Option &option)
{
	std::cout << "    Option " << option.m_key << " = " << option.m_value << std::endl;
}

} // !irccd

/*
 * Ini.cpp -- .ini file parsing
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

#include <cctype>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <ostream>
#include <sstream>
#include <vector>

#if defined(_WIN32)
#  include <Shlwapi.h>	// for PathIsRelative
#endif

#include "Ini.h"

namespace irccd {

namespace {

/* --------------------------------------------------------
 * Tokens
 * -------------------------------------------------------- */

enum class TokenType {
	Comment = '#',
	SectionBegin = '[',
	SectionEnd = ']',
	Escape = '\\',
	QuoteSimple = '\'',
	QuoteDouble = '"',
	NewLine = '\n',
	Assign = '=',
	Include = '@',
	Word,
	Space
};

class Token {
private:
	TokenType m_type;
	int m_line;
	int m_position;
	std::string m_value;

public:
	inline Token(TokenType type, int line, int position, std::string value = "")
		: m_type(type)
		, m_line(line)
		, m_position(position)
		, m_value(std::move(value))
	{
	}

	inline TokenType type() const noexcept
	{
		return m_type;
	}

	inline int line() const noexcept
	{
		return m_line;
	}

	inline int position() const noexcept
	{
		return m_position;
	}

	inline std::string value() const
	{
		switch (m_type) {
		case TokenType::Comment:
			return "#";
		case TokenType::SectionBegin:
			return "[";
		case TokenType::SectionEnd:
			return "]";
		case TokenType::QuoteSimple:
			return "'";
		case TokenType::QuoteDouble:
			return "\"";
		case TokenType::NewLine:
			return "\n";
		case TokenType::Assign:
			return "=";
		case TokenType::Include:
			return "@";
		case TokenType::Space:
			return m_value;
		case TokenType::Word:
			return m_value;
		default:
			break;
		}

		return "";
	}

	inline std::string toString() const
	{
		switch (m_type) {
		case TokenType::Comment:
			return "'#'";
		case TokenType::SectionBegin:
			return "'['";
		case TokenType::SectionEnd:
			return "']'";
		case TokenType::QuoteSimple:
			return "'";
		case TokenType::QuoteDouble:
			return "\"";
		case TokenType::NewLine:
			return "<newline>";
		case TokenType::Assign:
			return "=";
		case TokenType::Include:
			return "@";
		case TokenType::Space:
			return "<blank>";
		case TokenType::Word:
			return "`" + m_value + "'";
		default:
			break;
		}

		return "";
	}
};

using TokenStack = std::vector<Token>;

/* --------------------------------------------------------
 * IniBuilder
 * -------------------------------------------------------- */

class IniBuilder {
private:
	std::string m_path;
	std::string m_base;
	Ini &m_ini;

private:
	inline bool isReserved(char c) const noexcept
	{
		return c == '\n' || c == '#' || c == '"' || c == '\'' || c == '=' || c == '[' || c == ']' || c == '@';
	}

	std::string base(std::string path)
	{
		auto pos = path.find_last_of("/\\");

		if (pos != std::string::npos) {
			path.erase(pos);
		} else {
			path = ".";
		}

		return path;
	}

#if defined(_WIN32)
	bool isAbsolute(const std::string &path)
	{
		return !PathIsRelative(path.c_str());
	}
#else
	bool isAbsolute(const std::string &path)
	{
		return path.size() > 0 && path[0] == '/';
	}
#endif

	std::vector<Token> analyze(std::istream &stream) const
	{
		std::istreambuf_iterator<char> it(stream);
		std::istreambuf_iterator<char> end;
		std::vector<Token> tokens;

		int lineno{1};
		int position{0};

		while (it != end) {
			std::string value;

			if (isReserved(*it)) {
				while (it != end && isReserved(*it)) {
					// Single character tokens
					switch (*it) {
					case '\n':
						++lineno;
						position = 0;
					case '#':
					case '[':
					case ']':
					case '\'':
					case '"':
					case '=':
					case '@':
						tokens.push_back({ static_cast<TokenType>(*it), lineno, position });
						++it;
						++position;
					default:
						break;
					}
				}
			} else if (std::isspace(*it)) {
				while (it != end && std::isspace(*it) && *it != '\n') {
					value.push_back(*it++);
				}

				tokens.push_back({ TokenType::Space, lineno, position, std::move(value) });
			} else {
				while (it != end && !std::isspace(*it) && !isReserved(*it)) {
					value.push_back(*it++);
				}

				tokens.push_back({ TokenType::Word, lineno, position, std::move(value) });
			}
		}

		return tokens;
	}

	void readComment(TokenStack::iterator &it, TokenStack::iterator end)
	{
		while (it != end && it->type() != TokenType::NewLine) {
			++ it;
		}

		// remove new line
		++ it;
	}

	void readSpace(TokenStack::iterator &it, TokenStack::iterator end)
	{
		while (it != end && it->type() == TokenType::Space) {
			++ it;
		}
	}

	void readNewLine(TokenStack::iterator &it, TokenStack::iterator end)
	{
		while (it != end && it->type() == TokenType::NewLine) {
			++ it;
		}
	}

	IniSection readSection(TokenStack::iterator &it, TokenStack::iterator end)
	{
		if (++it == end || it->type() != TokenType::Word) {
			throw IniError(it->line(), it->position(), "word expected after [, got " + it->toString());
		}

		IniSection section(it->value());

		if (++it == end || it->type() != TokenType::SectionEnd) {
			throw IniError(it->line(), it->position(), "] expected, got " + it->toString());
		}

		// Remove ]
		++ it;

		if (it == end) {
			return section;
		}

		while (it != end && it->type() != TokenType::SectionBegin) {
			if (it->type() == TokenType::Space) {
				readSpace(it, end);
			} else if (it->type() == TokenType::NewLine) {
				readNewLine(it, end);
			} else if (it->type() == TokenType::Comment) {
				readComment(it, end);
			} else if (it->type() == TokenType::Word) {
				section.push_back(readOption(it, end));
			} else {
				throw IniError(it->line(), it->position(), "unexpected token " + it->toString());
			}
		}

		return section;
	}

	IniOption readOption(TokenStack::iterator &it, TokenStack::iterator end)
	{
		std::string key = it->value();

		if (++it == end) {
			throw IniError(it->line(), it->position(), "expected '=' after option declaration, got <EOF>");
		}

		readSpace(it, end);

		if (it == end || it->type() != TokenType::Assign) {
			throw IniError(it->line(), it->position(), "expected '=' after option declaration, got " + it++->toString());
		}

		readSpace(++it, end);

		std::ostringstream oss;

		if (it->type() == TokenType::QuoteSimple || it->type() == TokenType::QuoteDouble) {
			TokenStack::iterator save = it++;

			while (it != end && it->type() != save->type()) {
				oss << it++->value();
			}

			if (it == end)
				throw IniError(save->line(), save->position(), "undisclosed quote: " + save->toString() + " expected");

			++ it;
		} else if (it->type() == TokenType::Word) {
			oss << it++->value();
		} else if (it->type() != TokenType::NewLine && it->type() != TokenType::Comment) {
			// No value requested, must be NewLine or comment
			throw IniError(it->line(), it->position(), "expected option value after '=', got " + it->toString());
		}


		return IniOption(std::move(key), oss.str());
	}

	void readInclude(TokenStack::iterator &it, TokenStack::iterator end)
	{
		if (++it == end || (it->type() != TokenType::Word || it->value() != "include")) {
			throw IniError(it->line(), it->position(), "expected `include' after '@' token, got " + it->toString());
		}

		readSpace(++it, end);

		// Quotes mandatory
		TokenStack::iterator save = it;

		if (it == end || (it->type() != TokenType::QuoteSimple && it->type() != TokenType::QuoteDouble)) {
			throw IniError(it->line(), it->position(), "expected filename after @include statement");
		}

		// Filename
		if (++it == end || it->type() != TokenType::Word) {
			throw IniError(it->line(), it->position(), "expected filename after @include statement");
		}

		std::string value = it->value();
		std::string fullpath;

		if (isAbsolute(value)) {
			fullpath = value;
		} else {
			fullpath = m_base + "/" + it->value();
		}

		// Must be closed with the same quote
		if (++it == end || it->type() != save->type()) {
			throw IniError(save->line(), save->position(), "undiclosed quote: " + save->toString() + " expected");
		}

		// Remove quote
		++ it;

		IniBuilder(m_ini, fullpath);
	}

public:
	IniBuilder(Ini &ini, std::string path)
		: m_path(path)
		, m_base(base(std::move(path)))
		, m_ini(ini)
	{
		std::ifstream file(m_path);

		if (!file.is_open())
			throw std::runtime_error(std::strerror(errno));

		std::vector<Token> ts = analyze(file);

		auto it = ts.begin();
		auto end = ts.end();

		while (it != end) {
			if (it->type() == TokenType::Space) {
				readSpace(it, end);
			} else if (it->type() == TokenType::NewLine) {
				readNewLine(it, end);
			} else if (it->type() == TokenType::Comment) {
				readComment(it, end);
			} else if (it->type() == TokenType::Include) {
				readInclude(it, end);
			} else if (it->type() == TokenType::SectionBegin) {
				m_ini.push_back(readSection(it, end));
			} else {
				throw IniError(it->line(), it->position(), "unexpected " + it->toString() + " on root document");
			}
		}
	}
};

} // !namespace

/* --------------------------------------------------------
 * Ini
 * -------------------------------------------------------- */

Ini::Ini(const std::string &path)
{
	IniBuilder(*this, path);
}

} // !irccd

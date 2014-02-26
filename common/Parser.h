/*
 * Parser.h -- config file parser
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

#ifndef _PARSER_H_
#define _PARSER_H_

#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace irccd {

/**
 * @class Section
 * @brief The option container
 *
 * A list of section found in the file. If root
 * options are allowed (default behavior), the root
 * section is "".
 */
class Section {
public:
	friend class Parser;

	using Map = std::unordered_map<std::string, std::string>;

	// C++ Container compatibility
	using value_type	= Map::value_type;
	using iterator		= Map::iterator;
	using const_iterator	= Map::const_iterator;

private:
	std::string	m_name;		/*! name of section */
	Map		m_options;	/*! list of options inside */
	bool		m_allowed;	/*! is authorized to push */

public:
	template <typename T>
	struct Converter {
		static const bool supported = false;
	};

	/**
	 * Default constructor.
	 */
	Section();

	/**
	 * Named constructor.
	 *
	 * @param name the section name
	 */
	Section(const std::string &name);

	/**
	 * Tells if that section has the specified option name.
	 *
	 * @param name the option name
	 * @return true if has
	 */
	bool hasOption(const std::string &name) const;

	/**
	 * Get the section name
	 *
	 * @return the section name
	 */
	const std::string &getName() const;

	/**
	 * Return an iterator to the beginning.
	 *
	 * @return the iterator.
	 */
	Map::iterator begin();

	/**
	 * Return a const iterator to the beginning.
	 *
	 * @return the iterator.
	 */
	Map::const_iterator cbegin() const;

	/**
	 * Return an iterator to the end.
	 *
	 * @return the iterator.
	 */
	Map::iterator end();
	
	/**
	 * Return a const iterator to the end.
	 *
	 * @return the iterator.
	 */
	Map::const_iterator cend() const;

	/**
	 * Template all functions for retrieving options value.
	 *
	 * @param name the option name
	 * @return the value if found
	 */
	template <typename T>
	T getOption(const std::string &name) const
	{
		try {
			return requireOption<T>(name);
		} catch (...) {
			// Catch any conversion error.
		}

		return T();
	}

	/**
	 * Requires an option, this works like getOption except
	 * that if an option is not found, an exception is
	 * thrown.
	 *
	 * @param name the name
	 * @return the value
	 * @throw std::out_of_range if not found
	 * @throw std::invalid_argument on conversion failures
	 */
	template <typename T>
	T requireOption(const std::string &name) const
	{
		static_assert(Converter<T>::supported, "invalid type requested");

		if (!hasOption(name))
			throw std::out_of_range(name + " not found");

		return Converter<T>::convert(m_options.at(name));
	}

	friend bool operator==(const Section &s1, const Section &s2);
};

template <>
struct Section::Converter<bool> {
	static const bool supported = true;

	static bool convert(const std::string &value)
	{
		bool result(false);

		if (value == "yes" || value == "true"|| value == "1")
			result = true;
		else if (value == "no" || value == "false" || value == "0")
			result = false;

		return result;
	}
};

template <>
struct Section::Converter<int> {
	static const bool supported = true;

	static int convert(const std::string &value)
	{
		return std::stoi(value);
	}
};

template <>
struct Section::Converter<float> {
	static const bool supported = true;

	static float convert(const std::string &value)
	{
		return std::stof(value);
	}
};

template <>
struct Section::Converter<double> {
	static const bool supported = true;

	static double convert(const std::string &value)
	{
		return std::stod(value);
	}
};

template <>
struct Section::Converter<std::string> {
	static const bool supported = true;

	static std::string convert(const std::string &value)
	{
		return value;
	}
};

/**
 * @class Parser
 * @brief Config file parser
 *
 * Open and read .ini files.
 */
class Parser {
public:
	/**
	 * Options available for the parser.
	 */
	enum Tuning {
		DisableRootSection	= 1,	/*! disable options on root */
		DisableRedefinition	= 2,	/*! disable multiple redefinition */
		DisableVerbosity	= 4	/*! be verbose by method */
	};

	using FindFunc	= std::function<void (const Section &)>;
	using List	= std::vector<Section>;

	// C++ Container compatibility
	using value_type	= List::value_type;
	using iterator		= List::iterator;
	using const_iterator	= List::const_iterator;

private:
	List		m_sections;		/*! list of sections found */
	std::string	m_path;			/*! path file */
	int		m_tuning;		/*! options for parsing */
	char		m_commentChar;		/*! the comment token default (#) */

	void addSection(const std::string &name);
	void addOption(const std::string &key, const std::string &value);

	void readSection(int lineno, const std::string &line);
	void readOption(int lineno, const std::string &line);

	void readLine(int lineno, const std::string &line);

	void open();

public:
	static const char DEFAULT_COMMENT_CHAR;

	/**
	 * Create a parser at the specified file path. Optional
	 * options may be added.
	 *
	 * @param path the file path
	 * @param tuning optional tuning flags
	 * @param commentToken an optional comment delimiter
	 * @throw std::runtime_error on errors
	 * @see Tuning
	 */
	Parser(const std::string &path, int tuning = 0, char commentToken = Parser::DEFAULT_COMMENT_CHAR);

	/**
	 * Default constructor.
	 */
	Parser();

	/**
	 * Default destructor.
	 */
	virtual ~Parser();

	/**
	 * Return an iterator to the beginning.
	 *
	 * @return the iterator.
	 */
	List::iterator begin();

	/**
	 * Return a const iterator to the beginning.
	 *
	 * @return the iterator.
	 */
	List::const_iterator cbegin() const;

	/**
	 * Return an iterator to the end.
	 *
	 * @return the iterator.
	 */
	List::iterator end();
	
	/**
	 * Return a const iterator to the end.
	 *
	 * @return the iterator.
	 */
	List::const_iterator cend() const;

	/**
	 * Find all sections matching the name.
	 *
	 * @param name the sections name
	 * @param func the function 
	 * @return a list of section with the options
	 */
	void findSections(const std::string &name, FindFunc func) const;

	/**
	 * Tell if a section is existing.
	 *
	 * @return true if exists
	 */
	bool hasSection(const std::string &name) const;

	/**
	 * Get a specified section.
	 *
	 * @param name the section name
	 * @return a section
	 * @throw std::out_of_range if not found
	 */
	const Section &getSection(const std::string &name) const;

	/**
	 * Logging function, used only if DisableVerbosity is not set. The
	 * default behavior is to print to stdout something like:
	 * line 10: syntax error
	 * line 8: missing =
	 *
	 * @param number the line number
	 * @param section the current section worked on
	 * @param message the message
	 */
	virtual void log(int number, const std::string &section, const std::string &message);

	friend bool operator==(const Parser &p1, const Parser &p2);
};

} // !irccd

#endif // !_PARSER_H_

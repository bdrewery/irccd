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

#ifndef _PARSER_H_
#define _PARSER_H_

#include <exception>
#include <iostream>
#include <string>
#include <vector>

namespace irccd {
namespace parser {

/**
 * Thrown when a section or an option is not found.
 */
class NotFoundException : public std::exception {
private:
	std::string m_key;

public:
	NotFoundException(const std::string &key)
		:m_key(key)
	{
	}

	const std::string & which(void) const {
		return m_key;
	}

	virtual const char *what(void) const throw() {
		return "Property not found";
	}
};

/**
 * An option referenced by a key and a value.
 */
struct Option {
	std::string m_key;		/*! option name */
	std::string m_value;		/*! option value */
};

/**
 * A list of section found in the file. If root
 * options are allowed (default behavior), the root
 * section is "".
 */
struct Section {
	std::string m_name;		/*! name of section */
	std::vector<Option> m_options;	/*! list of options inside */
	bool m_allowed;			/*! is authorized to push */

	Section(void);
	~Section(void);

	/**
	 * Copy constructor
	 */
	Section(const Section &s);

	/**
	 * Get the section name
	 *
	 * @return the section name
	 */
	const std::string & getName(void) const;

	/**
	 * Get all options from that section.
	 *
	 * @return the list of options
	 */
	const std::vector<Option> & getOptions(void) const;

	/**
	 * Tells if that section has the specified option name.
	 *
	 * @param name the option name
	 * @return true if has
	 */
	bool hasOption(const std::string &name) const;

	/**
	 * Get an option specified by name.
	 *
	 * @param name the option name
	 * @throw NotFoundException
	 * @return the option
	 */
	const Option & findOption(const std::string &name) const;

	/**
	 * Template all functions for retrieving options value.
	 */
	template <typename T>
	bool getOption(const std::string &, T &, bool required = false) const;

	friend std::ostream & operator<<(std::ostream & stream, const Section &section)
	{
		stream << "[" << section.getName() << "]" << std::endl;

		for (auto p : section.getOptions())
			stream << p.m_key << "=" << p.m_value << std::endl;

		return stream;
	}
};

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

private:
	std::vector<Section> m_sections;	/*! list of sections found */
	std::string m_error;			/*! if an error occured */
	std::string m_path;			/*! path file */
	int m_tuning;				/*! options for parsing */
	char m_commentChar;			/*! the comment token default (#) */

	void addSection(const std::string &name);
	void addOption(const std::string &key, const std::string &value);

	void readSection(int lineno, const std::string &line);
	void readOption(int lineno, const std::string &line);

	void readLine(int lineno, const std::string &line);

public:
	static const char DEFAULT_COMMENT_CHAR;

	/**
	 * Create a parser at the specified file path. Optional
	 * options may be added.
	 *
	 * @param path the file path
	 * @param tuning optional tuning flags
	 * @param commentToken an optional comment delimiter
	 * @see Tuning
	 */
	Parser(const std::string &path, int tuning = 0, char commentToken = Parser::DEFAULT_COMMENT_CHAR);

	/**
	 * Default destructor.
	 */
	virtual ~Parser(void);

	/**
	 * Open the config file.
	 *
	 * @return true on success
	 */
	bool open(void);

	/**
	 * Get the error message if any
	 *
	 * @return the error message
	 */
	const std::string & getError(void) const;

	/**
	 * Tell if a section is existing.
	 *
	 * @return true if exists
	 */
	bool hasSection(const std::string &name);

	/**
	 * Get a specified section.
	 *
	 * @param name the section name
	 * @throw NotFoundException if not found
	 * @return a section
	 */
	const Section & getSection(const std::string &name) const;

	/**
	 * Get all sections found
	 *
	 * @return all sections
	 */
	const std::vector<Section> & getSections(void) const;

	/**
	 * Get a list of sections for config which multiple
	 * definitions are allowed. This does a full copy of sections
	 * and options.
	 *
	 * @param name the sections name
	 * @return a list of section with the options
	 */
	std::vector<Section> findSections(const std::string &name) const;

	/**
	 * Tells if a section has an option registered.
	 *
	 * @param section the section name
	 * @param name the option name
	 * @return true on success
	 */
	bool hasOption(const std::string &section, const std::string &name) const;

	/**
	 * Get an option value
	 *
	 * @param section the section name
	 * @param name the option name
	 * @throw NotFoundException if not found
	 * @return the value
	 */
	const Option & getOption(const std::string &section, const std::string &key) const;

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

	/**
	 * Dump all sections and options.
	 */
	void dump(void);

	/**
	 * Dump function used in the dump() method. This default method
	 * only print the section name like:
	 * Section foo
	 *
	 * @param section the current section
	 * @see dump
	 */
	virtual void dumpSection(const Section &section);

	/**
	 * Dump the option. The default method only print the option name
	 * and value.
	 *
	 * @param option the current option
	 * @see dump
	 */
	virtual void dumpOption(const Option &option);

	friend std::ostream & operator<<(std::ostream & stream, const Parser &parser)
	{
		for (auto s : parser.m_sections)
			stream << s;;

		return stream;
	}
};

} // !parser
} // !irccd

#endif // !_PARSER_H_

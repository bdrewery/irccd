/*
 * OptionParser.h -- command line option parser
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

#ifndef _OPTION_PARSER_H_
#define _OPTION_PARSER_H_

/**
 * @file OptionParser.h
 * @brief Command line option parser
 */

#include <initializer_list>
#include <string>
#include <vector>

namespace irccd {

/**
 * @class Option
 * @brief Option definition
 */
class Option {
public:
	enum Flags {
		NoArg	= (1 << 0),
	};

private:
	std::string m_key;
	std::string m_full;
	int m_flags;

public:
	/**
	 * Construct an option. By default, an option requires an argument
	 * unless flags is set to NoArg.
	 *
	 * You <strong>must</strong> not prepend dashes to the option names.
	 *
	 * You don't need to set both short and long names, but you need at
	 * least one.
	 *
	 * @param key the short name (e.g v)
	 * @param full the long name (e.g verbose)
	 * @param flags the optional flags
	 * @see Flags
	 */
	inline Option(std::string key, std::string full, int flags = 0)
		: m_key(std::move(key))
		, m_full(std::move(full))
		, m_flags(flags)
	{
	}

	/**
	 * Get the short name (e.g v)
	 *
	 * @return the short name
	 */
	inline const std::string &key() const noexcept
	{
		return m_key;
	}

	/**
	 * Get the long name (e.g verbose)
	 *
	 * @return the long name
	 */
	inline const std::string &full() const noexcept
	{
		return m_full;
	}

	/**
	 * Get the flags.
	 *
	 * @return the flags
	 * @see Flags
	 */
	inline int flags() const noexcept
	{
		return m_flags;
	}
};

/**
 * @class OptionValue
 * @brief Result of an option parse
 */
class OptionValue {
private:
	std::string m_key;
	std::string m_full;
	std::string m_value;

public:
	/**
	 * Construct an option value
	 *
	 * @param option the option
	 * @param value the value
	 */
	inline OptionValue(const Option &option, std::string value)
		: m_key(option.key())
		, m_full(option.full())
		, m_value(std::move(value))
	{
	}

	/**
	 * Get the value (if the option requires an argument).
	 *
	 * @return the value
	 */
	inline const std::string &value() const noexcept
	{
		return m_value;
	}

	friend bool operator==(const OptionValue &o1, const std::string &name);
};

/**
 * Test the option value with the specified option name.
 *
 * You can use both the short option or the long option name depending
 * on what you have registered to the OptionParser class.
 *
 * @param o the option
 * @param name the short or the full name
 * @return true if matches
 */
inline bool operator==(const OptionValue &o, const std::string &name)
{
	return o.m_key == name || o.m_full == name;
}

/**
 * @class OptionPack
 * @brief Object containing results of a parse
 *
 * Because parsing bad options does not throw exceptions, this class is
 * convertible to bool and has the error contained.
 *
 * It also have the number of arguments parsed so you can cut your options
 * depending on the full command line.
 *
 * Example:
 *	-y install -d foo
 *	-y remove -f
 *
 * In that case, you can do two parsing, it will stops (unless Unstrict is set)
 * until install or remove.
 */
class OptionPack : public std::vector<OptionValue> {
private:
	friend class OptionParser;

	std::string m_error{"No error"};
	int m_argsParsed{0};

public:
	/**
	 * Get the error.
	 *
	 * @return the error
	 */
	inline const std::string &error() const noexcept
	{
		return m_error;
	}

	/**
	 * Get the number of arguments parsed <strong>not the number of
	 * options</strong>.
	 *
	 * @return the number of arguments parsed
	 */
	inline int parsed() const noexcept
	{
		return m_argsParsed;
	}

	/**
	 * Convert to true on success.
	 *
	 * @return true on success
	 */
	inline operator bool() const noexcept
	{
		return m_error == "No error";
	}
};

/**
 * @class OptionParser
 * @brief Base class for parsing command line options
 *
 * The option parser is a replacement for getopt(3) which is reentrant
 * and does not use globals.
 */
class OptionParser {
public:
	using Map = std::vector<Option>;
	using Args = std::vector<std::string>;

	enum Flags {
		Unstrict =	(1 << 0)
	};

private:
	Map m_options;

	const Option &get(const std::string &arg) const;
	std::string key(const std::string &arg) const;
	bool isDefined(const std::string &arg) const;
	bool isToggle(const std::string &arg) const;
	bool isShortCompacted(const std::string &arg) const;
	bool isShort(const std::string &arg) const;
	bool isLong(const std::string &arg) const;
	bool isOption(const std::string &arg) const;
	void readShort(OptionPack &pack, Args::const_iterator &it, Args::const_iterator end) const;
	void readFull(OptionPack &pack, Args::const_iterator &it, Args::const_iterator end) const;
	OptionPack parse(Args::const_iterator it, Args::const_iterator end, int flags) const;

public:
	/**
	 * Construct an option parser from an initializer_list of options.
	 *
	 * @param options the list of options
	 */
	OptionParser(std::initializer_list<Option> options);

	/**
	 * Construct an option parser from a vector of options.
	 *
	 * @param options the options
	 */
	OptionParser(std::vector<Option> options);

	/**
	 * Parse the arguments from main arguments.
	 *
	 * @param argc the number of arguments
	 * @param argv the arguments
	 * @param flags the optional flags
	 * @return the packed result
	 */
	OptionPack parse(int argc, char **argv, int flags = 0) const;

	/**
	 * Parse the arguments from a vector.
	 *
	 * @param args the arguments
	 * @param flags the optional flags
	 * @return the packed result
	 */
	OptionPack parse(const std::vector<std::string> &args, int flags = 0) const;
};

} // !irccd

#endif // !_OPTION_PARSER_H_

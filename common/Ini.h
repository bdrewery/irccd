/*
 * Ini.h -- .ini file parsing
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

#ifndef _INI_H_
#define _INI_H_

/**
 * @file Ini.h
 * @brief Configuration file parser
 */

#include <algorithm>
#include <deque>
#include <stdexcept>
#include <string>

namespace irccd {

/**
 * @class IniError
 * @brief Error in a file
 */
class IniError : public std::exception {
private:
	int m_line;
	int m_position;
	std::string m_error;

public:
	/**
	 * Construct an error.
	 *
	 * @param line the line
	 * @param position the position
	 * @param error the error
	 */
	inline IniError(int line, int position, std::string error)
		: m_line(line)
		, m_position(position)
		, m_error(std::move(error))
	{
	}

	/**
	 * Return the line number.
	 *
	 * @return the line
	 */
	inline int line() const noexcept
	{
		return m_line;
	}

	/**
	 * Return the position in the current line.
	 *
	 * @return the position
	 */
	inline int position() const noexcept
	{
		return m_position;
	}

	/**
	 * Get the error string.
	 *
	 * @return the string
	 */
	inline const char *what() const noexcept
	{
		return m_error.c_str();
	}
};

/**
 * @class IniOption
 * @brief Option definition
 */
class IniOption {
private:
	std::string m_key;
	std::string m_value;

public:
	/**
	 * Construct an option.
	 *
	 * @param key the key
	 * @param value the value
	 */
	inline IniOption(std::string key, std::string value)
		: m_key(std::move(key))
		, m_value(std::move(value))
	{
	}

	/**
	 * Get the option key.
	 *
	 * @return the key
	 */
	inline const std::string &key() const noexcept
	{
		return m_key;
	}

	/**
	 * Get the option value.
	 *
	 * @return the value
	 */
	inline const std::string &value() const noexcept
	{
		return m_value;
	}
};

/**
 * @class IniSection
 * @brief Section that contains one or more options
 */
class IniSection {
private:
	std::string m_key;
	std::deque<IniOption> m_options;

	template <typename T>
	T find(const std::string &key) const
	{
		auto it = std::find_if(m_options.begin(), m_options.end(), [&] (const IniOption &o) {
			return o.key() == key;
		});

		if (it == m_options.end()) {
			throw std::out_of_range("option " + key + " not found");
		}

		return const_cast<T>(*it);
	}

public:
	/**
	 * Default constructor has no sections and no values.
	 */
	IniSection() = default;

	/**
	 * Construct a section with a set of options.
	 *
	 * @param key the section name
	 * @param options the list of options
	 */
	inline IniSection(std::string key, std::deque<IniOption> options = {}) noexcept
		: m_key(std::move(key))
		, m_options(std::move(options))
	{
	}

	/**
	 * Get the section key.
	 *
	 * @return the key
	 */
	inline const std::string &key() const noexcept
	{
		return m_key;
	}

	/**
	 * Get an iterator to the beginning.
	 *
	 * @return the iterator
	 */
	inline auto begin() noexcept
	{
		return m_options.begin();
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline auto begin() const noexcept
	{
		return m_options.begin();
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline auto cbegin() const noexcept
	{
		return m_options.cbegin();
	}

	/**
	 * Get an iterator to the end.
	 *
	 * @return the iterator
	 */
	inline auto end() noexcept
	{
		return m_options.end();
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline auto end() const noexcept
	{
		return m_options.end();
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline auto cend() const noexcept
	{
		return m_options.cend();
	}

	/**
	 * Append an option.
	 *
	 * @param option the option to add
	 */
	inline void push_back(IniOption option)
	{
		m_options.push_back(std::move(option));
	}

	/**
	 * Push an option to the beginning.
	 *
	 * @param option the option to add
	 */
	inline void push_front(IniOption option)
	{
		m_options.push_front(std::move(option));
	}

	/**
	 * Get the number of options in that section.
	 *
	 * @return the size
	 */
	inline unsigned size() const noexcept
	{
		return m_options.size();
	}

	/**
	 * Tells if an option exists in the section.
	 *
	 * @param name option name
	 * @return true if exists
	 */
	inline bool contains(const std::string &name) const noexcept
	{
		try {
			(void)find<IniOption &>(name);
		} catch (...) {
			return false;
		}

		return true;
	}

	/**
	 * Access an option at the specified index.
	 *
	 * @param index the index
	 * @return the option
	 * @warning No bounds checking is performed
	 */
	inline IniOption &operator[](int index) noexcept
	{
		return m_options[index];
	}

	/**
	 * Access an option at the specified index.
	 *
	 * @param index the index
	 * @return the option
	 * @warning No bounds checking is performed
	 */
	inline const IniOption &operator[](int index) const noexcept
	{
		return m_options[index];
	}

	/**
	 * Access an option at the specified key.
	 *
	 * @param key the key
	 * @return the option
	 * @warning No bounds checking is performed
	 */
	inline IniOption &operator[](const std::string &key)
	{
		return find<IniOption &>(key);
	}

	/**
	 * Access an option at the specified key.
	 *
	 * @param key the key
	 * @return the option
	 * @warning No bounds checking is performed
	 */
	inline const IniOption &operator[](const std::string &key) const
	{
		return find<const IniOption &>(key);
	}
};

/**
 * @class Ini
 * @brief Ini config file loader
 */
class Ini {
private:
	std::deque<IniSection> m_sections;

	template <typename T>
	T find(const std::string &key) const
	{
		auto it = std::find_if(m_sections.begin(), m_sections.end(), [&] (const IniSection &s) {
			return s.key() == key;
		});

		if (it == m_sections.end())
			throw std::out_of_range("section " + key + " not found");

		return const_cast<T>(*it);
	}

public:
	/**
	 * Default constructor with an empty configuration.
	 */
	Ini() = default;

	/**
	 * Open the path as the configuration file.
	 *
	 * @param path the path
	 * @throw IniError on any error
	 */
	Ini(const std::string &path);

	/**
	 * Get an iterator to the beginning.
	 *
	 * @return the iterator
	 */
	inline auto begin() noexcept
	{
		return m_sections.begin();
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline auto begin() const noexcept
	{
		return m_sections.begin();
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline auto cbegin() const noexcept
	{
		return m_sections.cbegin();
	}

	/**
	 * Get an iterator to the end.
	 *
	 * @return the iterator
	 */
	inline auto end() noexcept
	{
		return m_sections.end();
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline auto end() const noexcept
	{
		return m_sections.end();
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline auto cend() const noexcept
	{
		return m_sections.cend();
	}

	/**
	 * Get the number of sections in the configuration.
	 *
	 * @return the size
	 */
	inline unsigned size() const noexcept
	{
		return m_sections.size();
	}

	/**
	 * Append a section to the end.
	 *
	 * @param section the section to add
	 */
	inline void push_back(IniSection section)
	{
		m_sections.push_back(std::move(section));
	}

	/**
	 * Add a section to the beginning.
	 *
	 * @param section the section to add
	 */
	inline void push_front(IniSection section)
	{
		m_sections.push_front(std::move(section));
	}

	/**
	 * Access a section at the specified index.
	 *
	 * @param index the index
	 * @return the section
	 * @warning No bounds checking is performed
	 */
	inline IniSection &operator[](int index) noexcept
	{
		return m_sections[index];
	}

	/**
	 * Access a section at the specified index.
	 *
	 * @param index the index
	 * @return the section
	 * @warning No bounds checking is performed
	 */
	inline const IniSection &operator[](int index) const noexcept
	{
		return m_sections[index];
	}

	/**
	 * Access a section at the specified key.
	 *
	 * @param key the key
	 * @return the section
	 * @warning No bounds checking is performed
	 */
	inline IniSection &operator[](const std::string &key)
	{
		return find<IniSection &>(key);
	}

	/**
	 * Access a section at the specified key.
	 *
	 * @param key the key
	 * @return the section
	 * @warning No bounds checking is performed
	 */
	inline const IniSection &operator[](const std::string &key) const
	{
		return find<IniSection &>(key);
	}
};

} // !irccd

#endif // !_INI_H_

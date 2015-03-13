/*
 * Json.h -- jansson C++11 wrapper
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

#ifndef _JSON_H_
#define _JSON_H_

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <utility>

#include <jansson.h>

/**
 * @file Json.h
 * @brief A jansson C++ modern wrapper
 *
 * Because of the Jansson implementation, all these classes are implicitly
 * shared.
 *
 * This means that you can't set any value to an existing value as it would
 * change a value which may be used somewhere else, instead you must set
 * or replace elements in JsonObject and JsonArray respectively.
 *
 * However, copy constructors are implemented as deep copy so take care of
 * not copying values mistakenly.
 */

/**
 * @class JsonType
 * @brief Json value type
 */
enum class JsonType {
	Object = JSON_OBJECT,		//!< Object
	Array = JSON_ARRAY,		//!< Array
	String = JSON_STRING,		//!< String
	Integer = JSON_INTEGER,		//!< Integer
	Real = JSON_REAL,		//!< Floating point
	True = JSON_TRUE,		//!< Boolean true
	False = JSON_FALSE,		//!< Boolean false
	Null = JSON_NULL		//!< Empty or null
};

/**
 * @class JsonError
 * @brief Error thrown for any error
 */
class JsonError final : public std::exception {
private:
	std::string	m_text;
	std::string	m_source;
	int		m_line{};
	int		m_column{};
	int		m_position{};

public:
	/**
	 * Custom error with no line, no column and no position.
	 *
	 * @param error the error message
	 */
	inline JsonError(std::string error)
		: m_text(std::move(error))
	{
	}

	/**
	 * Error from a json_error_t.
	 *
	 * @param error the error
	 */
	inline JsonError(const json_error_t &error)
		: m_text(error.text)
		, m_source(error.source)
		, m_line(error.line)
		, m_column(error.column)
		, m_position(error.position)
	{
	}

	/**
	 * Get the error message.
	 *
	 * @return the message
	 */
	const char *what() const noexcept override
	{
		return m_text.c_str();
	}

	/**
	 * Get the text message.
	 *
	 * @return the text
	 */
	inline const std::string &text() const noexcept
	{
		return m_text;
	}

	/**
	 * Get the source.
	 *
	 * @return the source
	 */
	inline const std::string &source() const noexcept
	{
		return m_source;
	}

	/**
	 * Get the line.
	 *
	 * @return the line
	 */
	inline int line() const noexcept
	{
		return m_line;
	}

	/**
	 * Get the column.
	 *
	 * @return the column
	 */
	inline int column() const noexcept
	{
		return m_column;
	}

	/**
	 * Get the position.
	 *
	 * @return the position
	 */
	inline int position() const noexcept
	{
		return m_position;
	}
};

class JsonObject;
class JsonArray;

/**
 * @class JsonValue
 * @brief Encapsulate any JSON value
 */
class JsonValue {
public:
	using Handle = std::unique_ptr<json_t, void (*)(json_t *)>;

	friend class JsonObject;
	friend class JsonArray;

protected:
	/**
	 * The unique_ptr handle of json_t, will automatically decrease
	 * the reference count in its deleter.
	 */
	Handle m_handle;

	inline void check() const
	{
		if (m_handle == nullptr)
			throw JsonError(std::strerror(errno));
	}

public:
	/**
	 * Deep copy of that element.
	 *
	 * @param value the other value
	 * @throw JsonError on allocation error
	 */
	inline JsonValue(const JsonValue &value)
		: m_handle(json_deep_copy(value.m_handle.get()), json_decref)
	{
		check();
	}

	/**
	 * Assign a deep copy of the other element.
	 *
	 * @return *this
	 * @throw JsonError on allocation error
	 */
	inline JsonValue &operator=(const JsonValue &value)
	{
		m_handle = Handle(json_deep_copy(value.m_handle.get()), json_decref);

		check();

		return *this;
	}

	/**
	 * Move constructor, the other value is left empty (JsonType::Null).
	 *
	 * @param other the other value
	 */
	inline JsonValue(JsonValue &&other) noexcept
		: m_handle(std::move(other.m_handle))
	{
		other.m_handle = Handle(json_null(), json_decref);
	}

	/**
	 * Move assignment, the other value is left empty (JsonType::Null).
	 *
	 * @param other the other value
	 */
	inline JsonValue &operator=(JsonValue &&other) noexcept
	{
		m_handle = std::move(other.m_handle);
		other.m_handle = Handle(json_null(), json_decref);

		return *this;
	}

	/**
	 * Create a JsonValue from a native Jansson type. This function
	 * will increment the json_t reference count.
	 *
	 * @param json the value
	 */
	inline JsonValue(json_t *json) noexcept
		: m_handle(json, json_decref)
	{
	}

	/**
	 * Construct a null value from a nullptr argument.
	 */
	inline JsonValue(std::nullptr_t) noexcept
		: m_handle(json_null(), json_decref)
	{
	}

	/**
	 * Create an empty value (JsonType::Null).
	 */
	inline JsonValue() noexcept
		: m_handle(json_null(), json_decref)
	{
	}

	/**
	 * Create a boolean value.
	 *
	 * @param value the value
	 */
	inline JsonValue(bool value) noexcept
		: m_handle(json_boolean(value), json_decref)
	{
	}

	/**
	 * Create a integer value (JsonType::Integer).
	 *
	 * @param value the value
	 * @throw JsonError on allocation error
	 */
	inline JsonValue(int value)
		: m_handle(json_integer(value), json_decref)
	{
		check();
	}

	/**
	 * Create a real value (JsonType::Real).
	 *
	 * @param value the value
	 * @throw JsonError on allocation error
	 */
	inline JsonValue(double value)
		: m_handle(json_real(value), json_decref)
	{
		check();
	}

	/**
	 * Create a string value (JsonType::String).
	 *
	 * @param value the value
	 * @throw JsonError on allocation error
	 */
	inline JsonValue(std::string value)
		: m_handle(json_string(value.c_str()), json_decref)
	{
		check();
	}

	/**
	 * Create from a C string (JsonType::String).
	 *
	 * @param value the string
	 * @throw JsonError on allocation error
	 */
	inline JsonValue(const char *value)
		: m_handle(json_string(value), json_decref)
	{
		check();
	}

	/**
	 * Create from a string literal (JsonType::String).
	 *
	 * @param value the value
	 * @throw JsonError on allocation error
	 */
	template <size_t Size>
	inline JsonValue(char (&value)[Size])
		: m_handle(json_string(value), json_decref)
	{
		check();
	}

	/**
	 * Default destructor.
	 */
	virtual ~JsonValue() = default;

	/**
	 * Get the type of value.
	 *
	 * @return the type
	 */
	inline JsonType typeOf() const noexcept
	{
		return static_cast<JsonType>(json_typeof(m_handle.get()));
	}

	/**
	 * Tells if the json value is an JSON_OBJECT.
	 *
	 * @return true or false
	 */
	inline bool isObject() const noexcept
	{
		return json_is_object(m_handle.get());
	}

	/**
	 * Tells if the json value is an JSON_ARRAY.
	 *
	 * @return true or false
	 */
	inline bool isArray() const noexcept
	{
		return json_is_array(m_handle.get());
	}

	/**
	 * Tells if the json value is an JSON_STRING.
	 *
	 * @return true or false
	 */
	inline bool isString() const noexcept
	{
		return json_is_string(m_handle.get());
	}

	/**
	 * Tells if the json value is an JSON_REAL.
	 *
	 * @return true or false
	 */
	inline bool isReal() const noexcept
	{
		return json_is_real(m_handle.get());
	}

	/**
	 * Tells if the json value is an JSON_TRUE.
	 *
	 * @return true or false
	 */
	inline bool isTrue() const noexcept
	{
		return json_is_true(m_handle.get());
	}

	/**
	 * Tells if the json value is an JSON_FALSE.
	 *
	 * @return true or false
	 */
	inline bool isFalse() const noexcept
	{
		return json_is_false(m_handle.get());
	}

	/**
	 * Tells if the json value is an JSON_NULL.
	 *
	 * @return true or false
	 */
	inline bool isNull() const noexcept
	{
		return json_is_null(m_handle.get());
	}

	/**
	 * Tells if the json value is an JSON_INTEGER or JSON_REAL.
	 *
	 * @return true or false
	 */
	inline bool isNumber() const noexcept
	{
		return json_is_number(m_handle.get());
	}

	/**
	 * Tells if the json value is an JSON_INTEGER.
	 *
	 * @return true or false
	 */
	inline bool isInteger() const noexcept
	{
		return json_is_integer(m_handle.get());
	}

	/**
	 * Tells if the json value is an JSON_TRUE or JSON_FALSE.
	 *
	 * @return true or false
	 */
	inline bool isBoolean() const noexcept
	{
		return json_is_boolean(m_handle.get());
	}

	/**
	 * Get the string value.
	 *
	 * @return the string
	 */
	inline std::string toString() const noexcept
	{
		auto value = json_string_value(m_handle.get());

		return (value == nullptr) ? "" : value;
	}

	/**
	 * Get the integer value.
	 *
	 * @return the value or 0
	 */
	inline int toInteger() const noexcept
	{
		return json_integer_value(m_handle.get());
	}

	/**
	 * Get the real value.
	 *
	 * @return the value or 0
	 */
	inline double toReal() const noexcept
	{
		return json_real_value(m_handle.get());
	}

	/**
	 * Convert to object.
	 *
	 * @return an object
	 */
	JsonObject toObject() const noexcept;

	/**
	 * Convert to array.
	 *
	 * @return an array
	 */
	JsonArray toArray() const noexcept;

	/**
	 * Write to a stream.
	 *
	 * @param out the out
	 * @param flags the optional Jansson flags
	 */
	inline void write(std::ofstream &out, int flags = 0) const
	{
		auto content = dump(flags);

		std::copy(std::begin(content), std::end(content), std::ostreambuf_iterator<char>(out));
	}

	/**
	 * Overloaded function.
	 *
	 * @param out the out
	 * @param flags the optional Jansson flags
	 */
	inline void write(std::ofstream &&out, int flags = 0) const
	{
		write(out, flags);
	}

	/**
	 * Convert the Json value as a string.
	 *
	 * @return the string
	 * @param flags the optional Jansson flags
	 */
	inline std::string dump(int flags = 0) const
	{
		auto str = json_dumps(m_handle.get(), flags);

		if (str == nullptr)
			return "";

		std::string ret(str);
		std::free(str);

		return ret;
	}

	/**
	 * Convert to native Jansson type.
	 *
	 * You should not call json_incref or json_decref on it as it is
	 * automatically done.
	 *
	 * @return the json_t handle
	 * @warning use this function with care
	 */
	inline operator json_t *() noexcept
	{
		return m_handle.get();
	}

	/**
	 * Overloaded function.
	 *
	 * @return the json_t handle
	 */
	inline operator const json_t *() const noexcept
	{
		return m_handle.get();
	}

	/**
	 * Equality operator.
	 */
	inline bool operator==(const JsonValue &other) const noexcept
	{
		return json_equal(m_handle.get(), other.m_handle.get());
	}
};

/**
 * @class JsonArray
 * @brief Manipulate JSON arrays
 */
class JsonArray final : public JsonValue {
public:
	/**
	 * @class Ref
	 * @brief Reference wrapper to be assignable
	 */
	class Ref final : public JsonValue {
	private:
		JsonArray &m_array;
		int m_index;

	public:
		explicit inline Ref(JsonValue value, JsonArray &array, int index)
			: JsonValue(std::move(value))
			, m_array(array)
			, m_index(index)
		{
		}

		inline operator JsonValue() const noexcept
		{
			return *this;
		}

		inline JsonValue &operator*() noexcept
		{
			return *this;
		}

		inline JsonValue *operator->() noexcept
		{
			return this;
		}

		inline Ref &operator=(const JsonValue &value)
		{
			m_array.replace(value, m_index);

			return *this;
		}

		inline Ref &operator=(JsonValue &&value)
		{
			m_array.replace(std::move(value), m_index);

			return *this;
		}
	};

	/**
	 * @class Ptr
	 * @brief Pointer wrapper for JsonValue iterators
	 */
	class Ptr final : public JsonValue {
	public:
		explicit Ptr(JsonValue value) noexcept
			: JsonValue(std::move(value))
		{
		}

		inline JsonValue &operator*() noexcept
		{
			return *this;
		}

		inline JsonValue *operator->() noexcept
		{
			return this;
		}
	};

	class iterator final {
	public:
		using iterator_category = std::random_access_iterator_tag;
		using difference_type = int;
		using value_type = JsonValue;
		using reference = Ref;
		using pointer = Ptr;

		friend class JsonArray;

	private:
		JsonArray &m_array;
		int m_index;

	public:
		explicit inline iterator(JsonArray &array, int index = 0) noexcept
			: m_array(array)
			, m_index(index)
		{
		}

		inline Ref operator*() const
		{
			return Ref(m_array.at(m_index), m_array, m_index);
		}

		inline Ptr operator->() const
		{
			return Ptr(m_array.at(m_index));
		}

		inline Ref operator[](int nindex) const noexcept
		{
			return Ref(m_array.at(m_index + nindex), m_array, m_index + nindex);
		}

		inline bool operator==(const iterator &other) const noexcept
		{
			return m_index == other.m_index;
		}

		inline bool operator!=(const iterator &other) const noexcept
		{
			return m_index != other.m_index;
		}

		inline bool operator<(const iterator &other) const noexcept
		{
			return m_index < other.m_index;
		}

		inline bool operator<=(const iterator &other) const noexcept
		{
			return m_index <= other.m_index;
		}

		inline bool operator>(const iterator &other) const noexcept
		{
			return m_index > other.m_index;
		}

		inline bool operator>=(const iterator &other) const noexcept
		{
			return m_index >= other.m_index;
		}

		inline iterator &operator++() noexcept
		{
			++m_index;

			return *this;
		}

		inline iterator operator++(int) noexcept
		{
			iterator save = *this;

			++m_index;

			return save;
		}

		inline iterator &operator--() noexcept
		{
			m_index--;

			return *this;
		}

		inline iterator operator--(int) noexcept
		{
			iterator save = *this;

			m_index--;

			return save;
		}

		inline iterator &operator+=(int nindex) noexcept
		{
			m_index += nindex;

			return *this;
		}

		inline iterator &operator-=(int nindex) noexcept
		{
			m_index -= nindex;

			return *this;
		}

		inline iterator operator+(int nindex) const noexcept
		{
			return iterator(m_array, m_index + nindex);
		}

		inline iterator operator-(int nindex) const noexcept
		{
			return iterator(m_array, m_index - nindex);
		}

		inline int operator-(iterator other) const noexcept
		{
			return m_index - other.m_index;
		}
	};

	class const_iterator final {
	public:
		using iterator_category = std::random_access_iterator_tag;
		using difference_type = int;
		using value_type = JsonValue;
		using reference = JsonValue;
		using pointer = Ptr;

		friend class JsonArray;

	private:
		const JsonArray &m_array;
		int m_index;

	public:
		explicit inline const_iterator(const JsonArray &array, int index = 0) noexcept
			: m_array(array)
			, m_index(index)
		{
		}

		inline JsonValue operator*() const
		{
			return m_array.at(m_index);
		}

		inline Ptr operator->() const
		{
			return Ptr(m_array.at(m_index));
		}

		inline JsonValue operator[](int nindex) const noexcept
		{
			return m_array.at(m_index + nindex);
		}

		inline bool operator==(const const_iterator &other) const noexcept
		{
			return m_index == other.m_index;
		}

		inline bool operator!=(const const_iterator &other) const noexcept
		{
			return m_index != other.m_index;
		}

		inline bool operator<(const const_iterator &other) const noexcept
		{
			return m_index < other.m_index;
		}

		inline bool operator<=(const const_iterator &other) const noexcept
		{
			return m_index <= other.m_index;
		}

		inline bool operator>(const const_iterator &other) const noexcept
		{
			return m_index > other.m_index;
		}

		inline bool operator>=(const const_iterator &other) const noexcept
		{
			return m_index >= other.m_index;
		}

		inline const_iterator &operator++() noexcept
		{
			++m_index;

			return *this;
		}

		inline const_iterator operator++(int) noexcept
		{
			const_iterator save = *this;

			++m_index;

			return save;
		}

		inline const_iterator &operator--() noexcept
		{
			m_index--;

			return *this;
		}

		inline const_iterator operator--(int) noexcept
		{
			const_iterator save = *this;

			m_index--;

			return save;
		}

		inline const_iterator &operator+=(int nindex) noexcept
		{
			m_index += nindex;

			return *this;
		}

		inline const_iterator &operator-=(int nindex) noexcept
		{
			m_index -= nindex;

			return *this;
		}

		inline const_iterator operator+(int nindex) const noexcept
		{
			return const_iterator(m_array, m_index + nindex);
		}

		inline const_iterator operator-(int nindex) const noexcept
		{
			return const_iterator(m_array, m_index - nindex);
		}

		inline int operator-(const_iterator other) const noexcept
		{
			return m_index - other.m_index;
		}
	};

	using size_type = int;
	using value_type = JsonValue;
	using const_pointer = const value_type *;
	using reference = JsonValue &;
	using const_reference = const JsonValue &;
	using difference_type = int;

protected:
	using JsonValue::JsonValue;

public:
	/**
	 * Create an empty array.
	 *
	 * @throw JsonError on allocation error
	 */
	inline JsonArray()
		: JsonValue(json_array())
	{
		check();
	}

	/**
	 * Create an array from a list of values.
	 *
	 * @param list the list
	 * @throw JsonError on allocation error
	 */
	inline JsonArray(std::initializer_list<value_type> list)
		: JsonArray()
	{
		for (auto &v : list)
			append(std::move(v));
	}

	/**
	 * Returns an iterator to the beginning.
	 *
	 * @return the iterator
	 */
	inline iterator begin() noexcept
	{
		return iterator(*this, 0);
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline const_iterator begin() const noexcept
	{
		return const_iterator(*this, 0);
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline const_iterator cbegin() const noexcept
	{
		return const_iterator(*this, 0);
	}

	/**
	 * Returns an iterator to the end.
	 *
	 * @return the iterator
	 */
	inline iterator end() noexcept
	{
		return iterator(*this, size());
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline const_iterator end() const noexcept
	{
		return const_iterator(*this, size());
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline const_iterator cend() const noexcept
	{
		return const_iterator(*this, size());
	}

	/**
	 * Get a value.
	 *
	 * @param index the index
	 * @throw JsonError on error
	 */
	JsonValue at(int index) const;

	/**
	 * Erase the array content.
	 */
	inline void clear() noexcept
	{
		json_array_clear(m_handle.get());
	}

	/**
	 * Remove the element at the specified index.
	 *
	 * @param index the index
	 */
	inline void erase(int index) noexcept
	{
		json_array_remove(m_handle.get(), index);
	}

	/**
	 * Overloaded function.
	 *
	 * @param it the iterator
	 */
	inline void erase(iterator it) noexcept
	{
		erase(it.m_index);
	}

	/**
	 * Overloaded function.
	 *
	 * @param it the iterator
	 */
	inline void erase(const_iterator it) noexcept
	{
		erase(it.m_index);
	}

	/**
	 * Get the number of values in the array
	 *
	 * @return the number or 0
	 */
	inline unsigned size() const noexcept
	{
		return json_array_size(m_handle.get());
	}

	/**
	 * Insert the value at the beginning.
	 *
	 * @param value the value
	 */
	inline void push(const JsonValue &value) noexcept
	{
		json_array_insert(m_handle.get(), 0, value.m_handle.get());
	}

	/**
	 * Insert a copy of the value at the end.
	 *
	 * @param value the value to insert
	 */
	inline void append(const JsonValue &value)
	{
		json_array_append(m_handle.get(), value.m_handle.get());
	}

	/**
	 * Insert a copy of the value at the specified index.
	 *
	 * @param value the value to insert
	 * @param index the position
	 */
	inline void insert(const JsonValue &value, int index)
	{
		json_array_insert(m_handle.get(), index, value.m_handle.get());
	}

	/**
	 * Replace the value at the given index.
	 *
	 * @param value the value
	 * @param index the index
	 */
	inline void replace(const JsonValue &value, int index)
	{
		json_array_set(m_handle.get(), index, value.m_handle.get());
	}

	/**
	 * Get the value at the specified index.
	 *
	 * @param index the position
	 * @return the value
	 */
	JsonValue operator[](int index) const noexcept;

	/**
	 * Access a value as a reference wrapper.
	 *
	 * @param index the position
	 * @return the reference wrapper over the value
	 */
	Ref operator[](int index) noexcept;
};

/**
 * @class JsonObject
 * @brief Object wrapper
 */
class JsonObject final : public JsonValue {
public:
	using key_type = std::string;
	using mapped_type = JsonValue;
	using size_type = int;
	using value_type = std::pair<key_type, mapped_type>;
	using const_pointer = const value_type *;
	using reference = JsonValue &;
	using const_reference = const JsonValue &;
	using difference_type = int;

	/**
	 * @class Ref
	 * @brief Wrapper for updating JsonObject
	 *
	 * This class is only used for the following functions:
	 *
	 *	JsonObject::operator[]
	 */
	class Ref final : public JsonValue {
	private:
		JsonObject &m_object;
		std::string m_key;

	public:
		explicit inline Ref(JsonValue value, JsonObject &object, std::string key)
			: JsonValue(std::move(value))
			, m_object(object)
			, m_key(std::move(key))
		{
		}

		inline operator JsonValue() const noexcept
		{
			return *this;
		}

		inline JsonValue &operator*() noexcept
		{
			return *this;
		}

		inline JsonValue *operator->() noexcept
		{
			return this;
		}

		inline Ref &operator=(const JsonValue &value)
		{
			m_object.set(m_key, value);

			return *this;
		}

		inline Ref &operator=(JsonValue &&value)
		{
			m_object.set(m_key, std::move(value));

			return *this;
		}
	};

	/**
	 * @class Ptr
	 * @brief Pointer wrapper for JsonValue iterators
	 *
	 * For const iterators, the real type is a JsonValue, for non const
	 * iterators it's a ref so that user can edit it.
	 */
	template <typename Type>
	class Ptr final {
	private:
		std::pair<std::string, Type> m_pair;

	public:
		inline Ptr(std::pair<std::string, Type> value) noexcept
			: m_pair(std::move(value))
		{
		}

		inline std::pair<std::string, Type> &operator*() noexcept
		{
			return m_pair;
		}

		inline std::pair<std::string, Type> *operator->() noexcept
		{
			return &m_pair;
		}
	};

	/**
	 * @class iterator
	 * @brief Forward iterator
	 */
	class iterator {
	public:
		using value_type = std::pair<std::string, Ref>;

		friend class JsonObject;

	private:
		JsonObject &m_object;
		void *m_keyIt;

		inline std::string key() const noexcept
		{
			return json_object_iter_key(m_keyIt);
		}

	public:
		explicit inline iterator(JsonObject &object, void *keyIt) noexcept
			: m_object(object)
			, m_keyIt(keyIt)
		{
		}

		inline value_type operator*() const
		{
			auto k = key();

			return value_type(k, Ref(m_object[k], m_object, k));
		}

		inline Ptr<Ref> operator->() const
		{
			auto k = key();

			return Ptr<Ref>({k, Ref(m_object[k], m_object, k)});
		}

		inline iterator &operator++() noexcept
		{
			m_keyIt = json_object_iter_next(m_object.m_handle.get(), m_keyIt);

			return *this;
		}

		inline iterator operator++(int) noexcept
		{
			iterator save = *this;

			m_keyIt = json_object_iter_next(m_object.m_handle.get(), m_keyIt);

			return save;
		}

		inline bool operator==(iterator other) const noexcept
		{
			return m_keyIt == other.m_keyIt;
		}

		inline bool operator!=(iterator other) const noexcept
		{
			return m_keyIt != other.m_keyIt;
		}
	};

	/**
	 * @class const_iterator
	 * @brief Forward iterator
	 */
	class const_iterator {
	public:
		using value_type = std::pair<std::string, JsonValue>;

		friend class JsonObject;

	private:
		const JsonObject &m_object;
		void *m_keyIt;

		inline std::string key() const noexcept
		{
			return json_object_iter_key(m_keyIt);
		}

	public:
		explicit inline const_iterator(const JsonObject &object, void *keyIt) noexcept
			: m_object(object)
			, m_keyIt(keyIt)
		{
		}

		inline value_type operator*() const
		{
			auto k = key();

			return value_type(k, m_object[k]);
		}

		inline Ptr<JsonValue> operator->() const
		{
			auto k = key();

			return Ptr<JsonValue>({k, m_object[k]});
		}

		inline const_iterator &operator++() noexcept
		{
			m_keyIt = json_object_iter_next(m_object.m_handle.get(), m_keyIt);

			return *this;
		}

		inline const_iterator operator++(int) noexcept
		{
			const_iterator save = *this;

			m_keyIt = json_object_iter_next(m_object.m_handle.get(), m_keyIt);

			return save;
		}

		inline bool operator==(const_iterator other) const noexcept
		{
			return m_keyIt == other.m_keyIt;
		}

		inline bool operator!=(const_iterator other) const noexcept
		{
			return m_keyIt != other.m_keyIt;
		}
	};

protected:
	using JsonValue::JsonValue;

public:
	/**
	 * Create empty object.
	 *
	 * @throw JsonError on allocation error
	 */
	inline JsonObject()
		: JsonValue(json_object())
	{
		check();
	}

	/**
	 * Create a JsonObject from an initializer_list.
	 *
	 * @param list the list of key-value pairs
	 * @throw JsonError on allocation error
	 */
	inline JsonObject(std::initializer_list<value_type> list)
		: JsonObject()
	{
		for (auto &v : list)
			set(v.first, std::move(v.second));
	}

	/**
	 * Returns an iterator to the beginning.
	 *
	 * @return the iterator
	 */
	inline iterator begin() noexcept
	{
		return iterator(*this, json_object_iter(m_handle.get()));
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline const_iterator begin() const noexcept
	{
		return const_iterator(*this, json_object_iter(m_handle.get()));
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline const_iterator cbegin() const noexcept
	{
		return const_iterator(*this, json_object_iter(m_handle.get()));
	}

	/**
	 * Returns an iterator to the end.
	 *
	 * @return the iterator
	 */
	inline iterator end() noexcept
	{
		return iterator(*this, nullptr);
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline const_iterator end() const noexcept
	{
		return const_iterator(*this, nullptr);
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline const_iterator cend() const noexcept
	{
		return const_iterator(*this, nullptr);
	}

	/**
	 * Check if the object contains a specific property.
	 *
	 * @param key the key
	 * @return true if the object contains the key
	 */
	inline bool contains(const std::string &key) const noexcept
	{
		return json_object_get(m_handle.get(), key.c_str()) != nullptr;
	}

	/**
	 * Get the number of items in the object.
	 *
	 * @return the number of items
	 */
	inline unsigned size() const noexcept
	{
		return json_object_size(m_handle.get());
	}

	/**
	 * Remove all elements from the object.
	 */
	inline void clear() noexcept
	{
		json_object_clear(m_handle.get());
	}

	/**
	 * Remove element `key' if exists.
	 *
	 * @param key the key
	 */
	inline void erase(const std::string &key) noexcept
	{
		json_object_del(m_handle.get(), key.c_str());
	}

	/**
	 * Overloaded function.
	 *
	 * @param it the iterator
	 */
	inline void erase(iterator it) noexcept
	{
		erase(it.key());
	}

	/**
	 * Overloaded function.
	 *
	 * @param it the iterator
	 */
	inline void erase(const_iterator it) noexcept
	{
		erase(it.key());
	}

	/**
	 * Set the value as key in the object.
	 *
	 * @param key the key
	 * @param value the value
	 */
	inline void set(const std::string &key, const JsonValue &value) noexcept
	{
		json_object_set(m_handle.get(), key.c_str(), value.m_handle.get());
	}

	/**
	 * Access an object as a wrapper so that you can update its content
	 * with convenience.
	 *
	 * @param key the key
	 */
	Ref operator[](const std::string &key);

	/**
	 * Get the value at the specified key. If the value is not found, an
	 * empty value is returned
	 *
	 * @param key the key
	 * @return the value
	 */
	JsonValue operator[](const std::string &key) const;
};

/**
 * @class JsonDocument
 * @brief Read files and strings to create Json values
 */
class JsonDocument final {
private:
	JsonValue m_value;

	JsonValue read(std::string, int flags) const;
	JsonValue read(std::ifstream &stream, int flags) const;

public:
	/**
	 * Construct a document from a file.
	 *
	 * @param stream the stream
	 * @param flags the optional Jansson flags
	 * @throw JsonError on errors
	 */
	JsonDocument(std::ifstream &fstream, int flags = 0);

	/**
	 * Construct a document from a file.
	 *
	 * @param stream the stream
	 * @param flags the optional Jansson flags
	 * @throw JsonError on errors
	 */
	JsonDocument(std::ifstream &&stream, int flags = 0);

	/**
	 * Construct a document from a file.
	 *
	 * @param stream the stream
	 * @param flags the optional Jansson flags
	 * @throw JsonError on errors
	 */
	JsonDocument(std::string content, int flags = 0);

	/**
	 * Check if the document contains an object.
	 *
	 * @return true if object
	 */
	inline bool isObject() const noexcept
	{
		return m_value.isObject();
	}

	/**
	 * Check if the document contains an array.
	 *
	 * @return true if array
	 */
	inline bool isArray() const noexcept
	{
		return m_value.isArray();
	}

	/**
	 * Convert the document value to object
	 *
	 * @return the value as object
	 */
	inline JsonObject toObject() const noexcept
	{
		return m_value.toObject();
	}

	/**
	 * Convert the document value to array
	 *
	 * @return the value as array
	 */
	inline JsonArray toArray() const noexcept
	{
		return m_value.toArray();
	}
};

#endif // !_JSON_H_

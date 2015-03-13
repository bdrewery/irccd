/*
 * Json.cpp -- jansson C++11 wrapper
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

#include <stdexcept>

#include "Json.h"

/* --------------------------------------------------------
 * JsonObject
 * -------------------------------------------------------- */

JsonObject JsonValue::toObject() const noexcept
{
	json_incref(m_handle.get());

	return JsonObject(m_handle.get());
}

JsonArray JsonValue::toArray() const noexcept
{
	json_incref(m_handle.get());

	return JsonArray(m_handle.get());
}

/* --------------------------------------------------------
 * JsonArray
 * -------------------------------------------------------- */

JsonValue JsonArray::at(int index) const
{
	auto value = json_array_get(m_handle.get(), index);

	if (value == nullptr)
		throw JsonError("index out of bounds");

	json_incref(value);

	return JsonValue{value};
}

JsonValue JsonArray::operator[](int index) const noexcept
{
	auto value = json_array_get(m_handle.get(), index);

	if (value == nullptr)
		return JsonValue();

	json_incref(value);

	return JsonValue(value);
}

JsonArray::Ref JsonArray::operator[](int index) noexcept
{
	auto value = json_array_get(m_handle.get(), index);

	if (value == nullptr)
		value = json_null();
	else
		json_incref(value);

	return Ref(value, *this, index);
}

/* --------------------------------------------------------
 * JsonObject
 * -------------------------------------------------------- */

JsonObject::Ref JsonObject::operator[](const std::string &name)
{
	if (typeOf() != JsonType::Object)
		return Ref(JsonValue(), *this, name);

	auto value = json_object_get(m_handle.get(), name.c_str());

	json_incref(value);

	return Ref(value, *this, name);
}

JsonValue JsonObject::operator[](const std::string &name) const
{
	if (typeOf() != JsonType::Object)
		return JsonValue();

	auto value = json_object_get(m_handle.get(), name.c_str());

	if (value == nullptr)
		return JsonValue();

	json_incref(value);

	return JsonValue(value);
}

/* --------------------------------------------------------
 * JsonDocument
 * -------------------------------------------------------- */

JsonValue JsonDocument::read(std::string content, int flags) const
{
	json_error_t error;
	json_t *json = json_loads(content.c_str(), flags, &error);

	if (json == nullptr)
		throw JsonError(error);

	return JsonValue(json);
}

JsonValue JsonDocument::read(std::ifstream &stream, int flags) const
{
	if (!stream.is_open())
		throw JsonError("File not opened");

	stream.seekg(0, stream.end);
	auto length = stream.tellg();
	stream.seekg(0, stream.beg);

	std::string buffer;
	buffer.resize(length, ' ');

	stream.read(&buffer[0], length);
	stream.close();

	return read(std::move(buffer), flags);
}

JsonDocument::JsonDocument(std::ifstream &stream, int flags)
{
	m_value = read(stream, flags);
}

JsonDocument::JsonDocument(std::ifstream &&stream, int flags)
{
	m_value = read(stream, flags);
}

JsonDocument::JsonDocument(std::string content, int flags)
{
	m_value = read(std::move(content), flags);
}
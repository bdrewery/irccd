/*
 * Singleton.h -- singleton template
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

#ifndef _SINGLETON_H_
#define _SINGLETON_H_

/**
 * @file Singleton.h
 * @brief The singleton template
 */

#include <memory>

namespace irccd {

/**
 * @class Singleton
 * @brief The singleton
 */
template <typename T>
class Singleton {
private:
	static std::unique_ptr<T>	s_instance;

public:
	/**
	 * Default destructor.
	 */
	virtual ~Singleton() = default;

	/**
	 * Get the singleton instance.
	 *
	 * @return the reference instance
	 */
	static T &instance()
	{
		if (!s_instance)
			s_instance = std::make_unique<T>();

		return *s_instance;
	}
};

template <typename T>
std::unique_ptr<T> Singleton<T>::s_instance;

} // !irccd

/**
 * Needed if you make your class constructor private.
 *
 * @param cls the class name
 * @example SINGLETON(MyObject)
 */
#define SINGLETON(cls)							\
	friend std::unique_ptr<cls> std::make_unique<cls>()

#endif // !_SINGLETON_H_

#
# CMakeLists.txt -- CMake build system for irccd
#
# Copyright (c) 2013, 2014, 2015 David Demelier <markand@malikania.fr>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

project(util)

set(
	UTIL_SOURCES
	${util_SOURCE_DIR}/index.txt
	${util_SOURCE_DIR}/enum/attribute.txt
	${util_SOURCE_DIR}/enum/color.txt
	${util_SOURCE_DIR}/function/date.txt
	${util_SOURCE_DIR}/function/format.txt
	${util_SOURCE_DIR}/function/splithost.txt
	${util_SOURCE_DIR}/function/splituser.txt
	${util_SOURCE_DIR}/type/Date/index.txt
	${util_SOURCE_DIR}/type/Date/method/calendar.txt
	${util_SOURCE_DIR}/type/Date/method/format.txt
	${util_SOURCE_DIR}/type/Date/method/timestamp.txt
	${util_SOURCE_DIR}/type/Date/metamethod/__eq.txt
	${util_SOURCE_DIR}/type/Date/metamethod/__le.txt
	${util_SOURCE_DIR}/type/Date/metamethod/__tostring.txt
	PARENT_SCOPE
)



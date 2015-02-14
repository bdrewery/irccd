#
# CMakeLists.txt -- CMake build system for irccd
#
# Copyright (c) 2013, 2014 David Demelier <markand@malikania.fr>
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

project(api-util)

set(
	FUNCTIONS
	${api-util_SOURCE_DIR}/Functions/date.txt
	${api-util_SOURCE_DIR}/Functions/format.txt
	${api-util_SOURCE_DIR}/Functions/splithost.txt
	${api-util_SOURCE_DIR}/Functions/splituser.txt
)

set(
	ENUMS
	${api-util_SOURCE_DIR}/Enums/color.txt
	${api-util_SOURCE_DIR}/Enums/attribute.txt
)

irccd_generate_api(
	TARGET api-util
	DIRECTORY util
	CATEGORY "irccd.util"
	SOURCES ${FUNCTIONS}
	TYPE function
	INDEX ${api-util_SOURCE_DIR}/index.txt
)

irccd_generate_api(
	TARGET api-util-Enums
	DIRECTORY util/Enums
	CATEGORY "irccd.util/Enums"
	SOURCES ${ENUMS}
	TYPE enum
)

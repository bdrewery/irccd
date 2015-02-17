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

project(parser)

set(
	PARSER_SOURCES
	${parser_SOURCE_DIR}/index.txt
	${parser_SOURCE_DIR}/function/new.txt
	${parser_SOURCE_DIR}/type/Parser/index.txt
	${parser_SOURCE_DIR}/type/Parser/method/findSections.txt
	${parser_SOURCE_DIR}/type/Parser/method/getSection.txt
	${parser_SOURCE_DIR}/type/Parser/method/hasSection.txt
	${parser_SOURCE_DIR}/type/Parser/method/onLog.txt
	${parser_SOURCE_DIR}/type/Parser/method/open.txt
	${parser_SOURCE_DIR}/type/Parser/method/requireSection.txt
	${parser_SOURCE_DIR}/type/Section/index.txt
	${parser_SOURCE_DIR}/type/Section/method/getOption.txt
	${parser_SOURCE_DIR}/type/Section/method/getOptions.txt
	${parser_SOURCE_DIR}/type/Section/method/hasOption.txt
	${parser_SOURCE_DIR}/type/Section/method/requireOption.txt
	PARENT_SCOPE
)

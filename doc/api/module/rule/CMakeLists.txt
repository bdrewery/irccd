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

project(rule)

set(
	RULE_SOURCES
	${rule_SOURCE_DIR}/index.txt
	${rule_SOURCE_DIR}/function/add.txt
	${rule_SOURCE_DIR}/function/clear.txt
	${rule_SOURCE_DIR}/function/count.txt
	${rule_SOURCE_DIR}/function/get.txt
	${rule_SOURCE_DIR}/function/list.txt
	${rule_SOURCE_DIR}/function/remove.txt
	${rule_SOURCE_DIR}/type/Rule/index.txt
	PARENT_SCOPE
)

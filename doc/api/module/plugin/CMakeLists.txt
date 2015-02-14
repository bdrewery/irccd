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

project(api-plugin)

set(
	FUNCTIONS
	${api-plugin_SOURCE_DIR}/Functions/addPath.txt
	${api-plugin_SOURCE_DIR}/Functions/info.txt
	${api-plugin_SOURCE_DIR}/Functions/list.txt
	${api-plugin_SOURCE_DIR}/Functions/load.txt
	${api-plugin_SOURCE_DIR}/Functions/reload.txt
	${api-plugin_SOURCE_DIR}/Functions/unload.txt
)

irccd_generate_api(
	TARGET plugin
	DIRECTORY plugin
	CATEGORY "irccd.plugin"
	SOURCES ${FUNCTIONS}
	INDEX ${api-plugin_SOURCE_DIR}/index.txt
	TYPE function
)

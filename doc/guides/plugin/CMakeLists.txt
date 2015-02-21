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

project(guide-plugin)

set(
	PLUGIN_SOURCES
	${guide-plugin_SOURCE_DIR}/01-Introduction/Intro.txt
	${guide-plugin_SOURCE_DIR}/02-Syntax/Intro.txt
	${guide-plugin_SOURCE_DIR}/03-Paths/Intro.txt
	${guide-plugin_SOURCE_DIR}/04-Common-Patterns/Intro.txt
	${guide-plugin_SOURCE_DIR}/05-First-Plugin/Intro.txt
)

irccd_generate_guide(plugin Plugin "${PLUGIN_SOURCES}")

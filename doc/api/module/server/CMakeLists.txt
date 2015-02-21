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

project(server)

set(
	SERVER_SOURCES
	${server_SOURCE_DIR}/index.txt
	${server_SOURCE_DIR}/function/find.txt
	${server_SOURCE_DIR}/function/connect.txt
	${server_SOURCE_DIR}/type/Server/index.txt
	${server_SOURCE_DIR}/type/Server/method/cnotice.txt
	${server_SOURCE_DIR}/type/Server/method/join.txt
	${server_SOURCE_DIR}/type/Server/method/kick.txt
	${server_SOURCE_DIR}/type/Server/method/me.txt
	${server_SOURCE_DIR}/type/Server/method/mode.txt
	${server_SOURCE_DIR}/type/Server/method/names.txt
	${server_SOURCE_DIR}/type/Server/method/nick.txt
	${server_SOURCE_DIR}/type/Server/method/notice.txt
	${server_SOURCE_DIR}/type/Server/method/part.txt
	${server_SOURCE_DIR}/type/Server/method/query.txt
	${server_SOURCE_DIR}/type/Server/method/say.txt
	${server_SOURCE_DIR}/type/Server/method/topic.txt
	${server_SOURCE_DIR}/type/Server/method/umode.txt
	${server_SOURCE_DIR}/type/Server/method/whois.txt
	${server_SOURCE_DIR}/type/Server/metamethod/__eq.txt
	${server_SOURCE_DIR}/type/Server/metamethod/__tostring.txt
	PARENT_SCOPE
)

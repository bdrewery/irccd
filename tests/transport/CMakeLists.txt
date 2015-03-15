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

include(${irccd_SOURCE_DIR}/transportcommand/TransportCommand.cmake)

set(
	TRANSPORT_SOURCES
	${TRANSPORT_COMMAND_SOURCES}
	${irccd_SOURCE_DIR}/Transport.h
	${irccd_SOURCE_DIR}/TransportManager.cpp
	${irccd_SOURCE_DIR}/TransportManager.h
	${irccd_SOURCE_DIR}/TransportClient.cpp
	${irccd_SOURCE_DIR}/TransportClient.h
	${irccd_SOURCE_DIR}/TransportCommand.h
	TestTransport.cpp
)

create_test(test-transport "${TRANSPORT_SOURCES}")

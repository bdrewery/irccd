#
# CMakeLists.txt -- CMake build system for irccd
#
# Copyright (c) 2013 David Demelier <markand@malikania.fr>
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

project(contrib)

option(WITH_SYSTEMD "Install systemd service" Off)

#
# File for systemd
#
if (WITH_SYSTEMD)
	# Directory for systemd
	set(SYSTEMDDIR "/usr/lib/systemd/system"
	    CACHE STRING "Absolute path where to install systemd files")

	install(FILES irccd.service DESTINATION ${SYSTEMDDIR})
endif ()

# vim: set syntax=cmake:

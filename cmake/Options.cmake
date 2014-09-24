#
# Options.cmake -- CMake build system for irccd
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

# Options that controls the build:
#
# WITH_IPV6		Enable IPv6 support (default: on)
# WITH_SSL		Enable OpenSSL (default: on)
# WITH_LIBICONV		Enable libiconv reencoding (default: on)
# WITH_LUA		Enable Lua (default: on)
# WITH_TESTS		Enable unit testing (default: on)
# WITH_DOXYGEN		Enable internal irccd documentation (default: on)
#
option(WITH_IPV6 "Enable IPv6" On)
option(WITH_SSL "Enable SSL" On)
option(WITH_LIBICONV "Enable libiconv for reencoding" On)
option(WITH_TESTS "Enable unit testing" On)

# Manual pages on Windows are pretty useless
if (WIN32)
	set(USE_MAN "No")
else ()
	set(USE_MAN "Yes")
endif ()

# Options that controls the installation:
#
# WITH_MAN		Install manpages (default: on, off for Windows)
# WITH_SYSTEMD		Install systemd service (default: off)
#
option(WITH_MAN "Install man pages" ${USE_MAN})
option(WITH_SYSTEMD "Install systemd service" Off)

# Lua version:
#
# WITH_LUAVER		Selects the Lua version, allowed: 5.1, 5.2 and JIT (default: 5.2)
#
set(WITH_LUAVER "5.2" CACHE STRING "Lua version")

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

option(WITH_IPV6 "Enable IPv6" On)
option(WITH_SSL "Enable SSL" On)
option(WITH_LIBICONV "Enable libiconv for reencoding" On)

# Use bundled xdg-basedir or not
option(WITH_BUNDLE_XDGBASEDIR "Build with bundled libxdg-basedir" Off)

# Optional Lua support
option(WITH_LUA52 "Build with Lua plugin support" On)
option(WITH_LUAJIT "Build with LuaJIT plugin support" Off)

if(WITH_LUA52 AND WITH_LUAJIT)
	message(FATAL_ERROR "Please select WITH_LUA52 or WITH_LUAJIT")
endif()

# Manual pages on Windows are pretty useless
if(WIN32)
	set(USE_MAN "No")
else()
	set(USE_MAN "Yes")
endif()

option(WITH_MAN "Install man pages" ${USE_MAN})

#
# Additional contributions, not always tested!
#
option(WITH_SYSTEMD "Install systemd service" Off)

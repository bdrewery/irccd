#
# CMakeLists.txt -- CMake build system for irccd
#
# Copyright (c) 2011, 2012, 2013 David Demelier <markand@malikania.fr>
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

option(WITH_MAN "Install man pages" On)

function(DEFINE_MAN file man)
	if (WITH_MAN)
		# Remove .in end
		string(REGEX REPLACE "(.*)\\.in$" "\\1" output ${file})
		configure_file(${file} "${CMAKE_BINARY_DIR}/${output}")

		install(
			FILES "${CMAKE_BINARY_DIR}/${output}"
			DESTINATION "${MANDIR}/${man}"
		)
	endif()
endfunction()

DEFINE_MAN(irccd.1.in man1)
DEFINE_MAN(irccd.conf.5.in man5)
DEFINE_MAN(irccdctl.conf.5.in man5)

# vim: set syntax=cmake:

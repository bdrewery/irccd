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

# Shortcut for installing modules
function(define_plugin name file description default)
	option(${name} ${description} ${default})

	if (${name})
		install(FILES ${file} DESTINATION ${MODDIR})
	endif()
endfunction()

define_plugin(MOD_ANTIFLOOD antiflood.lua "Avoid flood on channel" On)
define_plugin(MOD_ASK ask.lua "Ask sage irccd about something" On)
define_plugin(MOD_AUTH auth.lua "Generic authentication plugin" On)
define_plugin(MOD_BADWORDS badwords.lua "Anti bad words modules" On)
define_plugin(MOD_HISTORY history.lua "Check for nickname history" On)
define_plugin(MOD_LOGGER logger.lua "A simple server logger" On)
define_plugin(MOD_ROULETTE roulette.lua "A harmless russian roulette" On)

# vim: set syntax=cmake:

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

project(plugins)

set(outputs)
set(sources)

#
# Create a macro that automatically do:
#
# 1. Add the appropriate WITH_PLUGIN_<name> option for each plugin and install
#    it to the fakeroot/plugins directory plus the installation.
#
macro(irccd_define_plugin name description)
	option(WITH_PLUGIN_${name} ${description} On)
	string(TOUPPER ${name} optname)

	if (WITH_PLUGIN_${optname})
		set(output ${CMAKE_BINARY_DIR}/fakeroot/${MODDIR}/${name}.lua)
		set(input ${plugins_SOURCE_DIR}/${name}.lua)

		add_custom_command(
			OUTPUT ${output}
			DEPENDS ${input}
			COMMENT "Copying plugin ${name} to binary directory"
			COMMAND ${CMAKE_COMMAND} -E copy ${input} ${output}
		)

		install(FILES ${input} DESTINATION ${MODDIR})

		list(APPEND outputs ${output})
		list(APPEND sources ${input})
	endif ()
endmacro()

irccd_define_plugin(antiflood "Avoid flood on channel")
irccd_define_plugin(ask "Ask sage irccd about something")
irccd_define_plugin(auth "Generic authentication plugin")
irccd_define_plugin(badwords "Anti bad words modules")
irccd_define_plugin(date "Date plugin")
irccd_define_plugin(history "Check for nickname history")
irccd_define_plugin(logger "A simple server logger")
irccd_define_plugin(plugin "Plugin inspection")
irccd_define_plugin(reminder "A reminder for IRC")
irccd_define_plugin(roulette "A harmless russian roulette")

add_custom_target(
	plugins
	ALL
	COMMENT "Copying plugins to binary directory"
	DEPENDS ${outputs}
	SOURCES ${sources}
)
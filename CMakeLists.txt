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

# Where to start
# ---------------------------------------------------------
#
# If you plan to modify the build systems there are several places to look
# to do what you want.
#
# cmake/CMake.cmake		- Sources files for cmake/ directory
# cmake/IrccdSystem.cmake	- Contains some platforms checks and compile flags.
# cmake/IrccdVersion.cmake	- Defines the Irccd version.
# cmake/IrccdOptions.cmake	- User definable options.
# cmake/IrccdFunctions.cmake	- Functions available for the build system.
#
# Build system is then processed in different directories:
#
# irccd			- The main irccd executable.
# irccdctl		- The irccdctl utility.
# doc			- The documentation process.
# tests			- The unit tests.
# contrib		- User contributions not maintained by irccd authors.
# extern		- External libraries.

cmake_minimum_required(VERSION 3.0)
project(irccd)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${irccd_SOURCE_DIR}/cmake/packages)

include(cmake/CMake.cmake)
include(cmake/IrccdFunctions.cmake)
include(cmake/IrccdVersion.cmake)
include(cmake/IrccdOptions.cmake)
include(cmake/IrccdSystem.cmake)

add_subdirectory(extern/libircclient)

add_subdirectory(common)
add_subdirectory(doc)
add_subdirectory(irccd)
add_subdirectory(irccdctl)
add_subdirectory(contrib)

# Tests
include(CTest)
add_subdirectory(tests)

message("Installation directories:")
message("    MANDIR            ${CMAKE_INSTALL_PREFIX}/${MANDIR}")
message("    DOCDIR            ${CMAKE_INSTALL_PREFIX}/${DOCDIR}")
message("    MODDIR            ${CMAKE_INSTALL_PREFIX}/${MODDIR}")
message("")

message("Compiling with the following flags:")
message("    General flags:    ${CMAKE_CXX_FLAGS}")
message("    Debug flags:      ${CMAKE_CXX_FLAGS_DEBUG}")
message("    Release flags:    ${CMAKE_CXX_FLAGS_RELEASE}")
message("")

message("Compiling irccd with following options:")
message("    OpenSSL:          ${WITH_SSL_MSG}")
message("    Lua:              ${WITH_LUA_MSG}")
message("    Tests:            ${WITH_TESTS_MSG}")
message("    User docs (HTML): ${WITH_DOCS_GUIDES_HTML_MSG}")
message("    User docs (PDF):  ${WITH_DOCS_GUIDES_PDF_MSG}")
message("    Lua docs:         ${WITH_DOCS_LUA_MSG}")
message("    Doxygen:          ${WITH_DOCS_DOXYGEN_MSG}")
message("    irccd config:     ${CMAKE_INSTALL_PREFIX}/${ETCDIR}/irccd.conf")
message("    irccdctl config:  ${CMAKE_INSTALL_PREFIX}/${ETCDIR}/irccdctl.conf")
message("")

message("Installing plugins:")
message("    antiflood:        ${WITH_PLUGIN_ANTIFLOOD_MSG}")
message("    ask:              ${WITH_PLUGIN_ASK_MSG}")
message("    auth:             ${WITH_PLUGIN_AUTH_MSG}")
message("    badwords:         ${WITH_PLUGIN_BADWORDS_MSG}")
message("    date:             ${WITH_PLUGIN_DATE_MSG}")
message("    history:          ${WITH_PLUGIN_HISTORY_MSG}")
message("    logger:           ${WITH_PLUGIN_LOGGER_MSG}")
message("    plugin:           ${WITH_PLUGIN_PLUGIN_MSG}")
message("    reminder:         ${WITH_PLUGIN_REMINDER_MSG}")
message("    roulette:         ${WITH_PLUGIN_ROULETTE_MSG}")

# CPack
include(cmake/IrccdCPackConfig.cmake)
include(CPack)

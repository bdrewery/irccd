#
# Options.cmake -- CMake build system for irccd
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

#
# Options that controls the build:
#
# WITH_IPV6		Enable IPv6 support (default: on)
# WITH_SSL		Enable OpenSSL (default: on)
# WITH_JS		Enable JavaScript (default: on)
# WITH_TESTS		Enable unit testing (default: off)
# WITH_SYSTEMD		Install systemd service (default: off)
# WITH_DOCS		Enable building of documentation (default: on)
# WITH_DOCS_DOXYGEN	Enable internal irccd documentation (default: on)
# WITH_DOCS_JS		Enable building of Lua documentation (default: on)
# WITH_DOCS_GUIDES_PDF	Enable user guides in PDF
# WITH_DOCS_GUIDES_HTML	Enable user guides in HTML
# WITH_DOCS_MAN		Install manpages (default: on, off for Windows)
# WITH_PLUGIN_ANTIFLOOD	Enable antiflood plugin
# WITH_PLUGIN_ASK	Enable ask plugin
# WITH_PLUGIN_AUTH	Enable auth plugin
# WITH_PLUGIN_BADWORDS	Enable badwords plugin
# WITH_PLUGIN_DATE	Enable date plugin
# WITH_PLUGIN_HISTORY	Enable history plugin
# WITH_PLUGIN_LOGGER	Enable logger plugin
# WITH_PLUGIN_PLUGIN	Enable plugin plugin
# WITH_PLUGIN_REMINDER	Enable reminder plugin
# WITH_PLUGIN_ROULETTE	Enable roulette plugin
#
# Note: the option() commands for WITH_PLUGIN_<name> variables are defined in plugins/CMakeLists.txt
#

#
# Options that controls both installations and the irccd runtime:
#
# Note: it is allowed to use absolute path but it's *strongly* recommended to
#       use relative paths if you want to keep irccd relocatable.
#
# WITH_BINDIR		Binary directory for irccd, irccdctl
# WITH_PLUGINDIR	Path where plugins must be installed
# WITH_DOCSDIR		Path where to install documentation
# WITH_MANDIR		Path where to install manuals
# WITH_CONFDIR		Path where to search configuration files
#

#
# Options for unit tests only:
#
# WITH_TEST_IRCHOST	Which IRC server to use for tests (default: 127.0.0.1)
# WITH_TEST_IRCPORT	Which IRC server port to use for tests (default: 6667)
#

# Manual pages on Windows are pretty useless
if (WIN32)
	set(DEFAULT_MAN "No")
else ()
	set(DEFAULT_MAN "Yes")
endif ()

option(WITH_IPV6 "Enable IPv6" On)
option(WITH_SSL "Enable SSL" On)
option(WITH_JS "Enable embedded Duktape" On)
option(WITH_TESTS "Enable unit testing" Off)
option(WITH_SYSTEMD "Install systemd service" Off)
option(WITH_DOCS "Enable building of all documentation" On)
option(WITH_DOCS_GUIDES_PDF "Enable building of PDF guides" On)
option(WITH_DOCS_GUIDES_HTML "Enable building of HTML guides" On)
option(WITH_DOCS_DOXYGEN "Enable doxygen" On)
option(WITH_DOCS_MAN "Install man pages" ${DEFAULT_MAN})
option(WITH_DOCS_JS "Enable building of JavaScript documentation" On)

set(WITH_TEST_IRCHOST "127.0.0.1"
	CACHE STRING "IRC host for tests")
set(WITH_TEST_IRCPORT 6667
	CACHE STRING "IRC port for test")

# ---------------------------------------------------------
# Installation paths
# ---------------------------------------------------------

set(WITH_BINDIR "bin"
	CACHE STRING "Binary directory")
set(WITH_DATADIR "share/irccd"
	CACHE STRING "Data directory")
set(WITH_PLUGINDIR "share/irccd/plugins"
	CACHE STRING "Module prefix where to install")
set(WITH_DOCDIR "share/doc/irccd"
	CACHE STRING "Documentation directory")
set(WITH_MANDIR "share/man"
	CACHE STRING "Man directory")
set(WITH_CONFDIR "etc"
	CACHE STRING "Configuration directory")

#
# Check if any of these path is absolute and mark irccd relocatable
# only if all paths are relative
#
set(IRCCD_RELOCATABLE TRUE)

foreach (d WITH_BINDIR WITH_DATADIR WITH_CONFDIR WITH_PLUGINDIR)
	if (IS_ABSOLUTE ${${d}})
		list(APPEND IRCCD_ABSOLUTE_PATHS ${d})
		set(IRCCD_RELOCATABLE FALSE)
	endif ()
endforeach ()

# ---------------------------------------------------------
# Internal dependencies
# ---------------------------------------------------------

if (WITH_JS)
	add_subdirectory(extern/duktape)
	set(WITH_JS_MSG "Yes")
else ()
	set(WITH_JS_MSG "No")
endif ()

if (WITH_TESTS)
	add_subdirectory(extern/gtest)
	set(WITH_TESTS_MSG "Yes")
else ()
	set(WITH_TESTS_MSG "No")
endif ()

foreach (p antiflood ask auth badwords date history logger plugin reminder roulette)
	string(TOUPPER ${p} var)

	if (WITH_PLUGIN_${var})
		set(WITH_PLUGIN_${var}_MSG "Yes")
	else ()
		set(WITH_PLUGIN_${var}_MSG "No")
	endif ()
endforeach ()

# ---------------------------------------------------------
# External dependencies
# ---------------------------------------------------------

find_package(Doxygen)
find_package(Pandoc)
find_package(LATEX)
find_package(OpenSSL)

if (NOT WITH_DOCS)
	set(WITH_DOCS_GUIDES_PDF FALSE)
	set(WITH_DOCS_GUIDES_HTML FALSE)
	set(WITH_DOCS_DOXYGEN FALSE)
	set(WITH_DOCS_MAN FALSE)
	set(WITH_DOCS_JS FALSE)
endif ()

if (WITH_SSL)
	if (OPENSSL_FOUND)
		set(WITH_SSL_MSG "Yes")
	else ()
		set(WITH_SSL_MSG "No (OpenSSL not found)")
		set(WITH_SSL FALSE)
	endif ()
else()
	set(WITH_SSL_MSG "No (disabled by user)")
endif ()

if (WITH_DOCS_DOXYGEN)
	if (DOXYGEN_FOUND)
		set(WITH_DOCS_DOXYGEN_MSG "Yes")
	else ()
		set(WITH_DOCS_DOXYGEN_MSG "No (doxygen not found)")
		set(WITH_DOCS_DOXYGEN FALSE)
	endif ()
else ()
	set(WITH_DOCS_DOXYGEN_MSG "No (disabled by user)")
endif ()

if (WITH_DOCS_GUIDES_PDF)
	if (Pandoc_FOUND)
		if (LATEX_COMPILER OR PDFLATEX_COMPILER OR BIBTEX_COMPILER OR MAKEINDEX_COMPILER)
			set(WITH_DOCS_GUIDES_PDF_MSG "Yes")
		else ()
			set(WITH_DOCS_GUIDES_PDF_MSG "No (LaTeX not found)")
			set(WITH_DOCS_GUIDES_PDF FALSE)
		endif ()
	else ()
		set(WITH_DOCS_GUIDES_PDF_MSG "No (pandoc not found)")
		set(WITH_DOCS_GUIDES_PDF FALSE)
	endif ()
else ()
	set(WITH_DOCS_GUIDES_PDF_MSG "No (disabled by user)")
	set(WITH_DOCS_GUIDES_PDF FALSE)
endif ()

if (WITH_DOCS_GUIDES_HTML)
	if (Pandoc_FOUND)
		set(WITH_DOCS_GUIDES_HTML_MSG "Yes")
	else ()
		set(WITH_DOCS_GUIDES_HTML_MSG "No (pandoc not found)")
		set(WITH_DOCS_GUIDES_HTML FALSE)
	endif ()
else ()
	set(WITH_DOCS_GUIDES_HTML_MSG "No (disabled by user)")
	set(WITH_DOCS_GUIDES_HTML FALSE)
endif ()

if (WITH_DOCS_JS)
	if (Pandoc_FOUND)
		set(WITH_DOCS_JS_MSG "Yes")
	else ()
		set(WITH_DOCS_JS FALSE)
		set(WITH_DOCS_JS_MSG "No (pandoc not found)")
	endif ()
else ()
	set(WITH_DOCS_JS_MSG "No (disabled by user)")
	set(WITH_DOCS_JS FALSE)
endif ()

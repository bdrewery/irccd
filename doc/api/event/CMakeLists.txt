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

project(event)

set(
	EVENT_SOURCES
	${event_SOURCE_DIR}/onCommand.txt
	${event_SOURCE_DIR}/onConnect.txt
	${event_SOURCE_DIR}/onChannelNotice.txt
	${event_SOURCE_DIR}/onInvite.txt
	${event_SOURCE_DIR}/onJoin.txt
	${event_SOURCE_DIR}/onKick.txt
	${event_SOURCE_DIR}/onLoad.txt
	${event_SOURCE_DIR}/onMessage.txt
	${event_SOURCE_DIR}/onMe.txt
	${event_SOURCE_DIR}/onMode.txt
	${event_SOURCE_DIR}/onNames.txt
	${event_SOURCE_DIR}/onNick.txt
	${event_SOURCE_DIR}/onNotice.txt
	${event_SOURCE_DIR}/onPart.txt
	${event_SOURCE_DIR}/onQuery.txt
	${event_SOURCE_DIR}/onReload.txt
	${event_SOURCE_DIR}/onTopic.txt
	${event_SOURCE_DIR}/onUnload.txt
	${event_SOURCE_DIR}/onUserMode.txt
	${event_SOURCE_DIR}/onWhois.txt
	PARENT_SCOPE
)

--
-- fake-test.lua -- fake testing using irccd test command
--
-- Copyright (c) 2013, 2014 David Demelier <markand@malikania.fr>
--
-- Permission to use, copy, modify, and/or distribute this software for any
-- purpose with or without fee is hereby granted, provided that the above
-- copyright notice and this permission notice appear in all copies.
--
-- THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
-- WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
-- MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
-- ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
-- WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
-- ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
-- OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
--

function onConnect(server)
	print("Successfully connected!")
end

function onChannelNotice(server, nick, channel, notice)
	print("Notice from " .. nick .. " on " .. channel .. " : " .. notice)
end

function onInvite(server, channel, who)
	print(who .. " has been invited to " .. channel)
end

function onJoin(server, channel, who)
	print(who .. " joined " .. channel)
end

function onKick(server, channel, who, kicked, reason)
	print(kicked .. " kicked from " .. channel .. " by " .. who .. " [" .. reason .. "]")
end

function onMessage(server, channel, who, message)
	print(who .. "@" .. channel .. ": " .. message)
end

function onMode(server, channel, who, mode, modeArg)
	print(who .. "@" .. channel .. " changed mode to " .. mode .. " " .. modeArg)
end

function onNick(server, oldnick, newnick)
	print(oldnick .. " -> " .. newnick)
end

function onNotice(server, who, target, notice)
	print("Notice from " .. who .. " to " .. target .. " : " .. notice)
end

function onPart(server, channel, nick, reason)
	print(nick .. " left " .. channel .. " [" .. reason .. "]")
end

function onQuery(server, who, message)
	print("query message from " .. who .. ": " .. message)
end

function onTopic(server, channel, who, topic)
	print(who .. " changed the " .. channel .. " topic to " .. topic)
end

function onUserMode(server, who, mode)
	print(who .. " changed my mode to " .. mode)
end

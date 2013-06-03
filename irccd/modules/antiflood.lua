--
-- antiflood.lua -- crazy module for asking a medium
--
-- Copyright (c) 2011, 2012, 2013 David Demelier <markand@malikania.fr>
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

local util = require "irccd.util"

local nicknames = { }
local conf = {
	maxm	= 5,
	delay	= 2000
}

function onMessage(server, channel, who, message)
	if nicknames[who] == nil then
		nicknames[who] = {
			current		= util.getTicks(),
			last		= util.getTicks(),
			count		= 0
		}
	else
		-- Spammed
		nicknames[who].current = util.getTicks()
		if nicknames[who].current - nicknames[who].last < conf.delay then
			nicknames[who].last = nicknames[who].current
			nicknames[who].count = nicknames[who].count + 1

			if nicknames[who].count >= conf.maxm then
				server:say(channel, "stop flooding")
				nicknames[who] = nil
			end
		else
			nicknames[who] = nil
		end
	end
end

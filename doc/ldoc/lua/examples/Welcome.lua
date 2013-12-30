-- Welcome.lua
-- This little script wait for someone to come on a channel and
-- welcome it

local util = require "irccd.util"

-- Welcome someone on a channel
function onJoin(server, channel, nickname)
	server:say(channel, "Welcome " .. util.splituser(nickname) .. "!")
end

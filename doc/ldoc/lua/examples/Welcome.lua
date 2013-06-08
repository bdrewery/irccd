-- Welcome.lua
-- This little script wait for someone to come on a channel and
-- welcome it

-- Welcome someone on a channel
function onJoin(server, channel, nickname)
	server:say(channel, "Welcome " .. nickname .. "!")
end

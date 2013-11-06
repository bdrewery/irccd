-- Formatting.lua
-- Colors and attributes usage.

local util = require "irccd.util"

--
-- Say hello in random colors to people that joins.
--
function onJoin(server, channel, who)
	local user = util.splitUser(who)

	-- Be sure to say hello to other people
	if user ~= server:getIdentity().nickname then
		local color = math.random(1, 15)

		-- Format the user nickname in colour
		local text = util.format(user, { fg = color })

		server:say(channel, "Hello " .. text)
	end
end

--
-- Send a message with the reversed attribute
--
function onCommand(server, channel, who, message)
	--
	-- It's also possible to use a sequence table for
	-- attrs to add more attributes like:
	-- attrs = { util.attribute.Reverse, util.attribute.Bold }
	--
	local text = util.format("I'm reversed", {
		attrs = util.attribute.Reverse
	})

	server:say(channel, text)
end

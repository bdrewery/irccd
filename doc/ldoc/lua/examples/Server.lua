-- Server.lua
-- This shows how we can use server as table key

local mylist = { }

function onMessage(server, channel, nick, message)
	-- Create a reference for that server.
	if mylist[server] == nil then
		mylist[server] = { }
	end

	-- Store all message from servers.
	table.insert(mylist[server], nick .. ":" .. message)
end

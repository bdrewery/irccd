-- Deferred.lua
-- This script shows how to get deferred informations.

local util = require "irccd.util"

-- In that example, I would like to get information from "markand" when
-- I successfully connect to a server
function onConnect(server)
	server:whois(
		"markand",
		function (info)
			print("nickname:" .. info.nickname)
			print("user:" .. info.user)
			print("host:" .. info.host)
			print("realname:" .. info.realname)

			print("markand is on the following channels:")
			for _, v in ipairs(info.channels) do
				print(v)
			end
		end
	)
end

-- On this, I would like to get all nicks from the channel I've just joined.
function onJoin(server, channel, who)
	local myNick = util.splitUser(who)

	-- Call names only if I've joined
	if server:getIdentity().nickname == myNick then
		server:names(
			channel,
			function (list)
				print("List of users on " .. channel .. ":")
				for _, v in ipairs(list) do
					print(v)
				end
			end
		)
	end
end

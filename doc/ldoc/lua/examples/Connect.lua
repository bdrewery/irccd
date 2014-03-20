-- Connect.lua
-- Connect to a server while running.

local server	= require "irccd.server"

--
-- Connect to a server by hand when the plugin is loaded
--
function onLoad()
	server.connect{
		name		= "test",
		host		= "the.hostname",
		port		= 6667,

		channels	= {
			"#foo",				-- no password
			{ "#staff" },			-- no password neither (alternative)
			{ "#secret", "pass" }		-- channel #secret with "pass" as password
		},

		-- Use a different identity
		identity	= {
			name		= "mario",
			nickname	= "mario",
			username	= "mario",
			realname	= "Super Mario"
		}
	}
end


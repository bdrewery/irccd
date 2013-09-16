local logger	= require "irccd.logger"
local parser	= require "irccd.parser"
local plugin	= require "irccd.plugin"

-- Authentication methods by servers
local auth	= {}

--
-- Load the nickserv backend settings.
--
local function loadNickServ(section, backend, server)
	local password	= section:requireOption("password")	

	--
	-- Username is optional, so as we don't know it yet
	-- we set it to nil and it will be taken from the
	-- onConnect callback
	--
	auth[server] = {
		backend		= backend,
		password	= password,
		username	= section:getOption("username")
	}
end

--
-- Load the quakenet backend settings.
--
local function loadQuakenet(section, backend, server)
	local username	= section:requireOption("username")
	local password	= section:requireOption("password")

	auth[server] = {
		backend		= backend,
		password	= password,
		username	= username
	}
end

local function loadConfig()
	local path = plugin.getHome() .. "/auth.conf"
	local conf = parser.new(path, { parser.DisableRootSection })

	local ret, err = conf:open()
	if not ret then
		logger.warn("unable to load file " .. path)
		logger.warn("authentication disabled")
	end

	-- Reset
	auth = { }

	-- Iterate over all authentication modules
	for s in conf:findSections("auth") do
		local backend	= s:requireOption("backend")
		local server	= s:requireOption("server")

		if backend == "nickserv" then
			loadNickServ(s, backend, server)
		elseif backend == "quakenet" then
			loadQuakenet(s, backend, server)
		else
			logger.warn("invalid backend " .. backend)
		end
	end
end

local function onConnectNickServer(server)
	logger.log("authenticating to NickServ")

	--
	-- If the username is nil, we need to get it from server
	--
	local a = auth[server:getName()]

	local nickname	= a.username or server:getIdentity().nickname
	local cmd = "identify " .. nickname .. " " .. a.password

	server:say("NickServ", cmd)

end

local function onConnectQuakenet(server)
	logger.log("authenticating to Q")

	local a = auth[server:getName()]

	local cmd = string.format("Q@CServe.quakenet.org AUTH %s %s\r\n", a.username, a.password)
	server:send(cmd)
end

function onConnect(server)
	if auth[server:getName()] then
		if auth[server:getName()].backend == "nickserv" then
			onConnectNickServer(server)
		elseif auth[server:getName()].backend == "quakenet" then
			onConnectQuakenet(server)
		end
	end
end

function onNotice(server, who, target, notice)
	logger.log("NOTICE: " .. notice)
end

function onReload()
	loadConfig()
end

loadConfig()

--
-- auth.lua -- generic plugin to authenticate to services
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

-- Plugin information
AUTHOR		= "David Demelier <markand@malikania.fr>"
VERSION		= "1.1"
COMMENT		= "Generic plugin to authenticate to services"
LICENSE		= "ISC"

-- Modules
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
	local path = plugin.info().home .. "/auth.conf"
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

	local a = auth[server:getName()]
	local cmd = "identify "

	-- The username is optional
	if a.username then
		cmd = cmd .. a.username .. " "
	end

	cmd = cmd .. a.password

	server:say("NickServ", cmd)
end

local function onConnectQuakenet(server)
	logger.log("authenticating to Q")

	local a = auth[server:getName()]
	local cmd = string.format("AUTH %s %s", a.username, a.password)

	server:say("Q@CServe.quakenet.org", cmd)
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

function onReload()
	loadConfig()
end

function onLoad()
	loadConfig()
end

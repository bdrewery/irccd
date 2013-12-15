--
-- antiflood.lua -- prevent excess flood on channels
--
-- Copyright (c) 2013 David Demelier <markand@malikania.fr>
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
COMMENT		= "Prevent excess flood on channels"
LICENSE		= "ISC"

-- Modules
local logger	= require "irccd.logger"
local parser	= require "irccd.parser"
local plugin	= require "irccd.plugin"
local util	= require "irccd.util"

-- Table of nicknames to check
local nicknames = { }

-- List of super user to ignore
local ignored = { }

-- Default configuration
local conf = {
	[ "max-messages" ]	= 5,
	[ "max-delay" ]		= 5000,
	[ "action" ]		= "kick",		-- kick, ban or message
	[ "reason" ]		= "please not flood"
}

local function loadIgnore(ignore)
	if ignore:hasOption("list") then
		local value = ignore:getOption("list")

		for v in value:gmatch("(%w+)") do
			table.insert(ignored, v)
			logger.log("Ignoring " .. v)
		end
	end
end

local function loadGeneral(general)
	for k in pairs(conf) do
		if general:hasOption(k) then
			if (type(conf[k] == "number")) then
				conf[k] = tonumber(general:getOption(k)) or conf[k]
			else
				conf[k] = general:getOption(k)
			end	
		end
	end
end

local function loadConfig()
	local path = plugin.info().home .. "/antiflood.conf"

	local parser = parser.new(path, { parser.DisableRedefinition })
	local ret, err = parser:open()

	if not ret then
		logger.log(err .. ", using defaults")
		return
	end

	if parser:hasSection("general") then
		loadGeneral(parser:getSection("general"))
	end

	-- Empty the list
	ignored = { }

	if parser:hasSection("ignore") then
		loadIgnore(parser:getSection("ignore"))
	end
end

local function manage(server, channel, nickname)
	if conf.action == "kick" then
		local realNick = util.splitUser(nickname)

		logger.log("kicking " .. nickname .. " from " .. channel)
		server:kick(realNick, channel, "stop flooding")
	elseif conf.action == "ban" then
		logger.log("banning " .. nickname .. " from " .. channel)
		server:mode(channel, "+b " .. nickname)
	end
end

local function isIgnored(server, channel, nickname)
	for _, v in ipairs(ignored) do
		if v == nickname then
			return true
		end
	end

	return false
end

function onConnect(server)
	local ident = server:getIdentity()

	table.insert(ignored, ident.nickname)
end

function onMessage(server, channel, who, message)
	who = util.splitUser(who)

	if isIgnored(server, channel, who) then
		return
	end

	if nicknames[who] == nil then
		nicknames[who] = {
			current		= util.getTicks(),
			last		= util.getTicks(),
			count		= 0
		}
	else
		-- Spammed
		nicknames[who].current = util.getTicks()
		if nicknames[who].current - nicknames[who].last < conf["max-delay"] then
			nicknames[who].last = nicknames[who].current
			nicknames[who].count = nicknames[who].count + 1

			if nicknames[who].count >= conf["max-messages"] then
				manage(server, channel, who)
				nicknames[who] = nil
			end
		else
			nicknames[who] = nil
		end
	end
end

function onReload()
	loadConfig()
end

function onLoad()
	loadConfig()
end

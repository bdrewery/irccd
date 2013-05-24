--
-- logger.lua -- a logger module to log everything
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

-- Modules
local parser = require("parser")
local plugin = require("plugin")
local util = require("util")

-- Logger configuration
local configuration = { }

local format = {
	cnotice	= "[#c] #m",
	join	= ">> #u joined #c",
	me	= "* #u #m",
	message	= "%H:%M #u: #m",
	mode	= ":: #u changed the mode to: #m #M",
	notice	= "[notice] (#u) #t: #m",
	part	= "<< #u left #c [#m]",
	topic	= ":: #u changed the topic to: #m",
	umode	= "#u set mode #m to #t"
}

-- Load the config.
local function loadConfig()
	local config = parser.new(
	    plugin.getHome() .. "/logger.conf",
	    { parser.DisableRedefinition }
	)

	local ret, err = config:open()
	if not ret then
		error(err)
	end

	local general, err = config:getSection("general")
	if general == nil then
		error(err)
	end

	-- Extract parameters
	configuration.home = general:requireOption("directory")

	-- TODO: Beautify this.
	if general:hasOption("format-cnotice") then
		format.cnotice	= general:getOption("format-cnotice")
	end
	if general:hasOption("format-join") then
		format.join	= general:getOption("format-join")
	end
	if general:hasOption("format-me") then
		format.me	= general:getOption("format-me")
	end
	if general:hasOption("format-message") then
		format.message	= general:getOption("format-message")
	end
	if general:hasOption("format-mode") then
		format.message	= general:getOption("format-mode")
	end
	if general:hasOption("format-notice") then
		format.notice	= general:getOption("format-notice")
	end
	if general:hasOption("format-part") then
		format.part	= general:getOption("format-part")
	end
	if general:hasOption("format-topic") then
		format.topic	= general:getOption("format-topic")
	end
	if general:hasOption("format-umode") then
		format.umode	= general:getOption("format-umode")
	end

	-- Set to nil means no log, so in config is ""
	for k, v in pairs(format) do
		if #format[k] <= 0 then
			format[k] = nil
		end
	end
end

loadConfig()

-- Formatter, used to convert % and # variables
local function convert(what, keywords)
	-- First convert dates from % with date:format.
	local date = util.dateNow()
	local line = date:format(what)

	-- Remove ~ if found.
	line = line:gsub("~", util.getHome())

	-- Now convert the # with gsub.
	return line:gsub("#(.)", keywords)
end

-- Open the file
local function open(keywords)
	-- Convert variable substitutions
	local path = convert(configuration.home, keywords)

	-- Test if parent directory exists
	local parent = util.dirname(path)

	if not util.exist(parent) then
		local ret, err = util.mkdir(parent)
		if not ret then
			error(err)
		end
	end

	-- Try to open the file
	local file, ret = io.open(path, "a+")
	if file == nil then
		error(ret)
	end

	return file
end

local function write(what, table)
	if what ~= nil then
		local file = open(table)
		local line = convert(what, table)

		file:write(line .. "\n")
		file:close()
	end
end

function onChannelNotice(server, who, channel, notice)
	local keywords = {
		c = channel,
		m = notice,
		s = server:getName(),
		u = who
	}

	write(format.cnotice, keywords)
end

-- Log joins
function onJoin(server, channel, nickname)
	local keywords = {
		c = channel,
		s = server:getName(),
		u = nickname
	}

	write(format.join, keywords)
end

function onMe(server, channel, who, message)
	local keywords = {
		c = channel,
		s = server:getName(),
		m = message,
		u = who
	}

	write(format.me, keywords)
end

-- Log message
function onMessage(server, channel, who, message)
	local keywords = {
		c = channel,
		s = server:getName(),
		m = message,
		u = who
	}

	write(format.message, keywords)
end

function onMode(server, channel, who, mode, modeArg)
	local keywords = {
		c = channel,
		s = server:getName(),
		m = mode,
		M = modeArg,
		u = who
	}

	write(format.mode, keywords)
end

function onNotice(server, who, target, notice)
	local keywords = {
		c = who,			-- for file loggin like channel
		m = notice,
		s = server:getName(),
		t = target,
		u = who
	}

	write(format.notice, keywords)
end

-- Log parts
function onPart(server, channel, nickname, reason)
	local keywords = {
		c = channel,
		s = server:getName(),
		m = reason,
		u = nickname
	}

	write(format.part, keywords)
end

-- Log topic
function onTopic(server, channel, who, topic)
	local keywords = {
		c = channel,
		s = server:getName(),
		m = topic,
		u = who
	}

	write(format.topic, keywords)
end

function onUserMode(server, who, mode)
	local identity = server:getIdentity()

	local keywords = {
		m = mode,
		s = server:getName(),
		t = identity.nickname,
	}

	write(format.umode, keywords)
end

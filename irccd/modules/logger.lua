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
	join	= ">> #u joined #c",
	me	= "* #u #m",
	message	= "%H:%M #u: #m",
	part	= "<< #u left #c [#m]"
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

	if (general:hasOption("format-join")) then
		format.join	= general:getOption("format-join")
	end
	if general:hasOption("format-me") then
		format.me	= general:getOption("format-me")
	end
	if general:hasOption("format-message") then
		format.message	= general:getOption("format-message")
	end
	if (general:hasOption("format-part")) then
		format.part	= general:getOption("format-part")
	end
end

loadConfig()

-- Formatter, used to convert % and # variables
local function convert(what, server, channel, who, message)
	-- First convert dates from % with date:format.
	local date = util.dateNow()
	local line = date:format(what)

	-- Remove ~ if found
	line = line:gsub("~", util.getHome())

	-- Now convert the # with gsub.
	local t = {
		[ "c" ] = channel,
		[ "m" ] = message,
		[ "u" ] = who,
		[ "s" ] = server:getName()
	}

	return line:gsub("#(.)", t)
end

-- Open the file
local function open(server, channel)
	-- Convert variable substitutions
	local path = convert(configuration.home, server, channel)

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

-- Log joins
function onJoin(server, channel, nickname)
	local file = open(server, channel)

	local line = convert(format.join, server, channel, nickname)

	file:write(line .. "\n")
	file:close()
end

function onMe(server, channel, who, message)
	local file = open(server, channel)

	-- Convert the message line
	local line = convert(format.me, server, channel, who, message)

	file:write(line .. "\n")
	file:close()
end

-- Log message
function onMessage(server, channel, who, message)
	local file = open(server, channel)

	-- Convert the message line
	local line = convert(format.message, server, channel, who, message)

	file:write(line .. "\n")
	file:close()
end

-- Log parts
function onPart(server, channel, nickname, reason)
	local file = open(server, channel)

	local line = convert(format.part, server, channel, nickname, reason)

	file:write(line .. "\n")
	file:close()
end

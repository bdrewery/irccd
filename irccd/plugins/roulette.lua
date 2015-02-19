--
-- roulette.lua -- shot yourself with a russian roulette
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
COMMENT		= "Shot yourself with a russian roulette"
LICENSE		= "ISC"

-- Modules
local logger	= require "irccd.logger"
local util	= require "irccd.util"
local parser	= require "irccd.parser"
local plugin	= require "irccd.plugin"

-- Revolvers
local instances = { }

-- Formats
local format = {
	lucky	= "#U, you're lucky this time [#i/6]",
	shot	= "HEADSHOT"
}

local function loadFormats()
	local path = plugin.info().home .. "/roulette.conf"
	local config = parser.new(path, { parser.DisableRedefinition })

	if not config:open() then
		logger.warn(path .. "  not found, using default answers")
		return
	end

	local fmt = config:getSection("formats")

	if fmt == nil then
		return
	end

	for k in pairs(format) do
		if fmt:hasOption(k) then
			local val = fmt:getOption(k)

			-- I only want non null string
			if #val > 0 then
				format[k] = val
			end
		end
	end
end

--- Reload the revolver.
-- This function assumes that init() already been called. It reinit the
-- revolver for the current server and channel.
-- @param s the server
-- @param c the channel
local function reload(s, c)
	instances[s:getName()][c] = {
		cylinder	= { },
		count		= 1
	}
	
	-- Fill with false value
	for i = 1, 6 do
		instances[s:getName()][c].cylinder[i] = false
	end
	
	instances[s:getName()][c].cylinder[math.random(1, 6)] = true
	instances[s:getName()][c].count = 1
end

--- Init if needed.
-- We used this function as the plugin may be loaded at any time, so
-- create server / channels instances if needed.
-- @param s the server
-- @param c the channel
local function init(s, c)
	if instances[s:getName()] == nil then
		instances[s:getName()] = { }
	end

	if instances[s:getName()][c] == nil then
		instances[s:getName()][c] = { }
		reload(s, c)
	end
end

--- Do a shot.
-- Test if the user should be shot or not.
-- @param s the server
-- @param c the channel
-- @param who the user
local function shot(s, c, who)
	init(s, c)

	local keywords = {
		U = util.splituser(who),
		u = who,
		i = instances[s:getName()][c].count
	}

	-- Get the instances
	local i = instances[s:getName()][c]

	if not i.cylinder[i.count] then
		local line = format.lucky:gsub("#(.)", keywords)

		s:say(c, line)
		i.count = i.count + 1
	else
		local line = format.shot:gsub("#(.)", keywords)

		s:kick(keywords.U, c, line)
		reload(s, c)
	end
end

function onCommand(server, channel, who, message)
	shot(server, channel, who)
end

function onReload()
	loadFormats()
end

function onLoad()
	loadFormats()
end

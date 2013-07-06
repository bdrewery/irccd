--
-- roulette.lua -- shot yourself with a russian roulette
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
local logger	= require "irccd.logger"
local util	= require "irccd.util"
local parser	= require "irccd.parser"
local plugin	= require "irccd.plugin"

-- Revolver
local channels	= { }

-- Formats
local format = {
	lucky	= "#U, you're lucky this time [#i/6]",
	shot	= "HEADSHOT"
}

local function loadFormats()
	local path = plugin.getHome() .. "/roulette.conf"
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

local function reload(c)
	channels[c] = {
		cylinder	= { },
		count		= 1
	}
	
	-- Fill with false value
	for i = 1, 6 do
		channels[c].cylinder[i] = false
	end
	
	channels[c].cylinder[math.random(1, 6)] = true
	channels[c].count = 1
end

local function shot(server, c, who)
	-- First initialization on that channel
	if channels[c] == nil then
		reload(c)
	end

	local keywords = {
		U = util.splitUser(who),
		u = who,
		i = channels[c].count
	}

	if not channels[c].cylinder[channels[c].count] then
		local line = format.lucky:gsub("#(.)", keywords)

		server:say(c, line)
		channels[c].count = channels[c].count + 1
	else
		local line = format.shot:gsub("#(.)", keywords)

		server:kick(keywords.U, c, line)
		reload(c)
	end
end

function onCommand(server, channel, who, message)
	shot(server, channel, who)
end

function onReload()
	loadFormats()
end

loadFormats()

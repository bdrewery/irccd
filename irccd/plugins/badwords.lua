--
-- badwords.lua -- module for anti badwords
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

local plugin = require "irccd.plugin"
local parser = require "irccd.parser"
local util = require "irccd.util"

local words = { }
local answer = "Please check your spelling"

-- Reload every words
function loadWords()
	local f, err = io.open(plugin.info().home .. "/words.txt")

	if (f ~= nil) then
		for w in f:lines() do
			if #w > 0 then
				table.insert(words, w)
			end
		end
	end
end

function loadConfig()
	local path = plugin.info().home .. "/badwords.conf"
	local parser = parser.new(path, { parser.DisableRedefinition })

	local ret, err = parser:open()
	if ret then
		local general = parser:getSection("general")
		if general then
			if general:hasOption("response") then
				answer = general:getOption("response")	
			end
		end
	end
end

function onLoad()
	loadWords()
	loadConfig()
end

function onMessage(server, channel, nick, message)
	local message = message:lower()

	for _, w in pairs(words) do
		if message:find("%f[%a]" .. w:lower() .. "%f[%A]") then
			server:say(channel, util.splitUser(nick) .. ": " .. answer)

			-- do not multiple
			break;
		end
	end
end

function onReload()
	loadWords()
	loadConfig()
end

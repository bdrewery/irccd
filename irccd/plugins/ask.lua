--
-- ask.lua -- crazy module for asking a medium
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
COMMENT		= "Crazy module for asking a medium"
LICENSE		= "ISC"

-- Modules
local logger	= require "irccd.logger"
local plugin	= require "irccd.plugin"
local util	= require "irccd.util"

local default = {
	"Yes",
	"No"
}

local answers = { }

-- Load every words
local function loadWords()
	local path = plugin.info().home .. "/answers.txt"
	local file, err = io.open(path)

	if file == nil then
		logger.warn("Could not open file: " .. err)
		logger.warn("Using default answers")
		answers = default
	else
		answers = { }
		for l in file:lines() do
			table.insert(answers, l)
		end
	end
end

function onLoad()
	loadWords()
end

function onCommand(server, channel, nick, message)
	local pick = math.random(1, #answers)
	local nick = util.splituser(nick)
	server:say(channel, nick .. ": " .. answers[pick])
end

function onReload()
	logger.log("Reloading words")
	loadWords()
end

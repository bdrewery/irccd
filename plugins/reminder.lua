--
-- reminder.lua -- a reminder for IRC
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
COMMENT		= "A reminder for IRC"
LICENSE		= "ISC"

-- Modules
local fs	= require "irccd.fs"
local logger	= require "irccd.logger"
local plugin	= require "irccd.plugin"
local parser	= require "irccd.parser"
local util	= require "irccd.util"

-- Manual validating
local manual	= true

local formats	= {
	header	= "#U, this is an automatic message from #o",
	message	= "#m",
	note	= "#U, type #m seen to remove this message"
}

local helps	= {
	invalid	= "#U, invalid command",
	usage	= "#U, usage: #m tell | seen | remove. Type #m help <name> for more information",
	tell	= "#U, set a message for a user. Usage: #m tell <nickname> <message>",
	seen	= "#U, mark a message as read. Usage: #m seen",
	remove	= "#U, forget all messages to a user. Usage: #m remove <target>"
}

--
-- List of message to paste, it is stored like this:
--
-- server:channel:sender:receiver:message
--
local list = { }

local truths = {
	["True"]	= true,
	["true"]	= true,
	["TRUE"]	= true,
	["Yes"]		= true,
	["yes"]		= true,
	["YES"]		= true,
	["1"]		= true,
}

---
-- Convert a string to a boolean.
--
-- @param str the string
-- @return the value
--
local function tobool(str)
	return truths[str] or false
end

---
-- Load the configuration.
--
local function loadconfig()
	local path = plugin.info().home .. "/reminder.conf"
	local c = parser.new(path, { parser.DisableRedefinition })

	local r, err = c:open()
	if not r then
		logger.warn(err)
		return
	end

	-- [general]
	local g = c:getSection "general"
	if g then
		if g:hasOption "manual-read" then
			manual = tobool(g:getOption "manual-read")
		end
	end

	-- [formats]
	local s = c:getSection "formats"	
	if s then
		for k in pairs(formats) do
			if s:hasOption(k) then
				formats[k] = s:getOption(k)
			end
		end
	end

	-- [help]
	local h = c:getSection "help"		
	if h then
		for k in pairs(helps) do
			if h:hasOption(k) then
				helps[k] = s:getOption(k)
			end
		end
	end
end

---
-- Get the full plugin name with its command character.
--
-- @return the full name
--
local function fullname(server)
	return server:info().commandChar .. plugin.info().name
end

---
-- Split a line to arguments.
--
-- @return the server name
-- @return the channel
-- @return the sender
-- @return the target
-- @return the message
--
local function toargs(line)
	return line:match("([^:]+):([^:]+):([^:]+):([^:]+):(.*)")
end

---
-- Return a formatted string with arguments.
--
-- @param s the server
-- @param c the channel
-- @param o the orinigator
-- @param t the target
-- @param m the message
--
local function toline(s, c, o, t, m)
	return string.format("%s:%s:%s:%s:%s", s, c, o, t, m)
end

---
-- Load all lines.
--
local function loadlines()
	local path = plugin.info().home .. "/db.txt"
	local file = io.open(path)

	-- Erase lines before
	list = { }

	if file then
		for l in file:lines() do
			table.insert(list, l)
		end

		file:close()
	end
end

---
-- Store all lines to the database file.
--
-- @param line an optional line to add
--
local function storelines(line)
	local home = plugin.info().home
	local path = string.format("%s/db.txt", home)

	-- Try to create the parent directory
	if not fs.exists(home) then
		if not fs.mkdir(home) then
			logger.warn("could not create " .. home)
			return
		else
			logger.log("created " .. home)
		end
	end

	local f, err = io.open(path, "w")

	if not f then
		logger.warn("%s: %s", path, err)
		return false
	end

	-- Insert an optional new line
	if line then
		table.insert(list, line)
	end

	for _, l in ipairs(list) do
		f:write(l .. "\n")
	end

	f:close()

	return true
end

---
-- Add a line to the database.
--
-- @param server the server
-- @param channel the channel
-- @param who the sender
-- @param args the arguments
--
local function tell(server, channel, who, args)
	local nick, message = args:match("%s*(%g+)%s*(.*)")
	local kw = {
		s = server:info().name,
		c = channel,
		u = who,
		U = util.splituser(who),
		m = fullname(server)
	}

	if not nick or #nick <= 0 or #message <= 0 then
		server:say(channel, util.convert(helps.tell, kw))
	else
		local line = toline(server:info().name, channel, who, nick, message)

		if not storelines(line) then
			server:say(channel, who .. ", sorry an error happened")
		end
	end

end

---
-- Mark a message as read.
--
-- @param server the server
-- @param channel the channel
-- @param who the sender
--
local function seen(server, channel, who)	
	local i = 1
	while i <= #list do
		local s, c, o, t = toargs(list[i])

		if server:info().name == s and c == channel and t == who then
			table.remove(list, i)
		else
			i = i + 1
		end
	end

	storelines()
end

---
-- Remove one or more message from the database.
--
-- @param server the server
-- @param channel the channel
-- @param who the sender
-- @param args the arguments (target)
--
local function remove(server, channel, who, args)
	local target = args:match("(%g+)")
	local kw = {
		s = server:info().name,
		c = channel,
		u = who,
		U = util.splituser(who),
		m = fullname(server)
	}

	if not target or #target <= 0 then
		server:say(channel, util.convert(helps.remove, kw))
	else
		local i = 1
		while i <= #list do
			local s, c, o, t = toargs(list[i])

			if server:info().name == s and channel == c and who == o and target == t then
				table.remove(list, i)
			else
				i = i + 1
			end
		end

		storelines()
	end
end

---
-- Get help for a command.
--
-- @param server the server
-- @param channel the channel
-- @param who the sender
-- @param args the arguments (target)
--
local function help(server, channel, who, args)
	local which = args:match("^(%w+)")	
	local line
	local kw = {
		s = server:info().name,
		c = channel,
		u = who,
		U = util.splituser(who),
		m = fullname(server)
	}

	if not which or #which <= 0 then
		line = helps.usage
	elseif not helps[which] then
		line = helps.invalid
	else
		line = helps[which]
	end
		
	server:say(channel, util.convert(line, kw))
end

local commands = {
	tell	= tell,
	seen	= seen,
	remove	= remove,
	help	= help
}

function onCommand(server, channel, nick, message)
	local cmd = message:match("^(%w+)")
	local who = util.splituser(nick)

	local kw = {
		s = server:info().name,
		c = channel,
		u = nick,
		U = who,
		m = fullname(server)
	}

	if commands[cmd] == nil then
		server:say(channel, util.convert(helps.usage, kw))
	else
		commands[cmd](server, channel, who, message:sub(#cmd + 2))
	end
end

function onJoin(server, channel, nickname)
	local who = util.splituser(nickname)

	local i = 1
	while i <= #list do
		local s, c, o, t, m = toargs(list[i])
		local kw = {
			c = channel,
			s = server:info().name,
			u = nickname,
			U = who,
			o = o
		}

		if server:info().name == s and c == channel and t == who then
			server:say(c, util.convert(formats.header, kw))

			-- Add m to the keywords now to avoid usage in header
			kw.m = m
			kw.o = nil
			server:say(c, util.convert(formats.message, kw))

			if manual then
				-- Now m is the plugin name and its command character
				kw.m = fullname(server)
				server:say(c, util.convert(formats.note, kw))

				-- As it requires manual removing, don't remove it yet
				i = i + 1
			else
				-- Remove the line from the table
				table.remove(list, i)
				storelines()
			end
		else
			i = i + 1
		end
	end
end

function onLoad()
	loadconfig()
	loadlines()	
end

function onReload()
	loadconfig()
	loadlines()
end

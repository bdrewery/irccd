--
-- history.lua -- track nickname's history
--
-- Copyright (c) 2013, 2014, 2015 David Demelier <markand@malikania.fr>
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
COMMENT		= "Track nickname's history"
LICENSE		= "ISC"

-- Modules
local fs	= require "irccd.fs"
local logger	= require "irccd.logger"
local parser	= require "irccd.parser"
local plugin	= require "irccd.plugin"
local system	= require "irccd.system"
local util	= require "irccd.util"

local format = {
	error	= "#U, I could not open my database file",
	seen	= "#U, I've seen #U for the last time on %m/%d/%y %H:%M",
	said	= "#U, the last message that #U said is: #m",
	unknown	= "#U, i've never known #U",
	usage	= "#U, usage: #P seen target | #P said target. Type #P help <command> for more information"
}

local help = {
	seen	= "#U, report the most recent time the user was seen. Usage: #P seen <target>",
	said	= "#U, get the last message said from the target. Usage: #P said <target>"
}

local function loadFormats()
	local path = plugin.info().home .. "/history.conf"
	local config = parser.new(path, { parser.DisableRedefinition })

	if not config:open() then
		logger.warn(path .. " not found, using default answers")
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

local function openFile(server, channel, mode)
	local base = plugin.info().home
	local srvdir = base .. "/" .. server:info().name

	-- Test if the directory exists
	if not fs.exists(srvdir) then
		logger.log(srvdir .. " does not exists, creating it")

		local r, err = fs.mkdir(srvdir)

		if not r then
			return nil, err
		end
	end

	-- Open the file
	return io.open(srvdir .. "/" .. channel, mode)
end

local function loadFile(f)
	-- Create a table
	local result = { }

	for l in f:lines() do
		table.insert(result, { l:match("^([^:]+):(%d+):(.*)$") })
	end

	return result
end

local function findEntry(list, nickname)
	if list == nil then
		return nil
	end

	for i, t in pairs(list) do
		if t[1] == nickname then
			return t, i
		end
	end
end

local function updateDatabase(server, channel, nickname, message)
	local f = openFile(server, channel, "r")
	local list, t, i

	--
	-- Load the database file, if not exists, create the first table
	-- list.
	--
	if f then
		list = loadFile(f)
		f:close()

		t, i = findEntry(list, nickname)
	else
		-- First call
		list = { }
	end

	local date = util.date()

	-- Not found or empty database yet, append it
	if t == nil then
		local entry = { nickname, date:timestamp(), message or "" }
		table.insert(list, entry)
	else
		-- Found, update
		t[2] = date:timestamp()
		t[3] = message or t[3]
	end

	-- Reopen the file as writing
	f, err = openFile(server, channel, "w")

	if f == nil then
		logger.warn(err)
	else
		for _, t in pairs(list) do
			f:write(string.format("%s:%d:%s\n", t[1], t[2], t[3]))
		end

		f:close()
	end
end

function onCommand(server, channel, who, message)
	local command, arg = message:match "^%s*(%w+)%s+(%S+)%s*$"
	local orig = util.splituser(who)
	local kw = {
		c = channel,
		U = orig,
		p = plugin.info().name,
		P = server:info().commandChar .. plugin.info().name,
		s = server:info().name
	}

	if not command then
		server:say(channel, util.convert(format.usage, kw))
	elseif command == "help" then
		if not arg then
			server:say(channel, util.convert(format.usage, kw))
		elseif arg == "seen" then
			server:say(channel, util.convert(help.seen, kw))
		elseif arg == "said" then
			server:say(channel, util.convert(help.said, kw))
		end
	else
		local f = openFile(server, channel, "r")

		if not f then
			server:say(channel, util.convert(format.error))
		else
			local list = loadFile(f)
			local entry = findEntry(list, arg)

			if not entry then
				server:say(channel, util.convert(format.unknown, kw))
			else
				local flags = {
					util.flags.ConvertDate
				}

				kw.date = util.date(entry[2])

				if command == "seen" then
					server:say(channel, util.convert(format.seen, kw, flags))
				elseif command == "said" then
					-- Add the message in case of said
					kw.m = entry[3]

					server:say(channel, util.convert(format.said, kw, flags))
				end
			end
		end
	end
end

function onJoin(server, channel, nickname)
	nickname = util.splituser(nickname)
	updateDatabase(server, channel, nickname)
end

function onNames(server, channel, names)
	for _, n in ipairs(names) do
		updateDatabase(server, channel, n)
	end
end

function onMessage(server, channel, nickname, message)
	updateDatabase(server, channel, util.splituser(nickname), message)
end

function onReload()
	loadFormats()
end

function onLoad()
	loadFormats()
end

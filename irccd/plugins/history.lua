--
-- history.lua -- track nickname's history
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
local parser	= require "irccd.parser"
local plugin	= require "irccd.plugin"
local util	= require "irccd.util"

local format = {
	error	= "I could not open my database file",
	seen	= "I've seen #U for the last time on %m/%d/%y %H:%M",
	said	= "The last message that #U said is: #m",
	unknown	= "I've never known #U"
}

local function loadFormats()
	local path = plugin.getHome() .. "/history.conf"
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
	local base = plugin.getHome()
	local srvdir = base .. "/" .. server:getName()

	-- Test if the directory exists
	if not util.exist(srvdir) then
		logger.log(srvdir .. " does not exists, creating it")

		local r, err = util.mkdir(srvdir)

		if not r then
			return nil, err
		end
	end

	-- Open the file
	local f, err = io.open(srvdir .. "/" .. channel, mode)

	return f, err
end

local function loadFile(f)
	-- Create a table
	local result = { }

	for l in f:lines() do
		local n, t, m = l:match("(%w+):(%w+):(.*)")
		table.insert(result, { n, t, m })
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

	return nil
end

-- Formatter, used to convert % and # variables
local function convert(what, keywords, date)
	local line = what

	-- First convert dates from % with date:format.
	if date ~= nil then
		line = date:format(line)
	end

	-- Remove ~ if found.
	line = line:gsub("~", util.getHome())

	-- Add environment variable.
	line = line:gsub("%${(%w+)}", util.getEnv)

	-- Now convert the # with gsub.
	return line:gsub("#(.)", keywords)
end

local function updateDatabase(server, channel, nickname, message)
	local f = openFile(server, channel, "r")
	local list, t, i

	--
	-- Load the database file, if not exists, create the first table
	-- list.
	--
	if f ~= nil then
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
		t[3] = message or ""
	end

	-- Reopen the file as writing
	f, err = openFile(server, channel, "w")

	if f == nil then
		logger.warn(err)
	else
		for _, t in pairs(list) do
			f:write(string.format("%s:%s:%s\n", t[1], t[2], t[3]))
		end

		f:close()
	end
end

function onCommand(server, channel, who, message)
	local f = openFile(server, channel, "r")
	who = util.splitUser(who)

	if f == nil then
		server:say(channel, who .. ", " .. format.error)
	else
		local c, U = message:match("%s*(%w+)%s+(%w+)%s*")
		local list = loadFile(f)
		local entry = findEntry(list, U)

		-- Convert format
		local kw = {
			U = U,
			c = c
		}

		if entry == nil then
			server:say(channel, who .. ", " .. convert(format.unknown, kw))
		else
			local d = util.date(entry[2])

			if c == "seen" then
				server:say(channel, who .. ", " .. convert(format.seen, kw, d))
			elseif c == "said" then
				-- Add the message in case of said
				kw.m = entry[3]

				server:say(channel, who .. ", " .. convert(format.said, kw, d))
			end
		end
	end
end

function onJoin(server, channel, nickname)
	nickname = util.splitUser(nickname)
	updateDatabase(server, channel, nickname)

	-- Update all nickname when I join a channel
	local ident = server:getIdentity()
	if ident.nickname == nickname then
		server:names(
			channel,
			function (names)
				for _, n in ipairs(names) do
					updateDatabase(server, channel, n)
				end
			end
		)
	end
end

function onMessage(server, channel, nickname, message)
	updateDatabase(server, channel, util.splitUser(nickname), message)
end

function onReload()
	loadFormats()
end

loadFormats()

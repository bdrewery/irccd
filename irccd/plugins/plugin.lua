--
-- plugin.lua -- plugin information
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
VERSION		= "1.2"
COMMENT		= "Plugin manager"
LICENSE		= "ISC"

-- Modules
local logger	= require "irccd.logger"
local parser	= require "irccd.parser"
local plugin	= require "irccd.plugin"
local util	= require "irccd.util"

local config	= {
	order	= { "summary", "comment", "author", "version" }
}

local formats	= {
	summary	= "#M: ",
	comment	= "#M. ",
	author	= "Written by #M. ",
	version	= "Version #M. ",
	unknown	= "there is no plugin #p loaded",
	list	= "#M"
}

local helps	= {
	usage	= "#U, usage: #P list | info <name>"
}

local function loadconfig()
	local path = plugin.info().home .. "/plugin.conf"
	local cfg = parser.new(path, { parser.DisableRedefinition })

	local ret, err = cfg:open()
	if not ret then
		logger.warn(err)
		return
	end

	-- [general]
	local g = cfg:getSection "general"
	if g then
		if g:hasOption "order" then
			config.order = { }

			for _, v in ipairs(util.split(g:getOption("order"), " \t")) do
				table.insert(config.order, v)
			end
		end
	end

	-- [formats]
	local s = cfg:getSection "formats"
	if s then
		for k in pairs(formats) do
			if s:hasOption(k) then
				formats[k] = s:getOption(k)
			end
		end
	end

	-- [helps]
	local s = cfg:getSection "helps"
	if s then
		for k in pairs(helps) do
			if s:hasOption(k) then
				helps[k] = s:getOption(k)
			end
		end
	end
end

local function list(server, channel, who)
	local str = ""

	for p in plugin.list() do
		str = str .. p .. " "
	end

	local kw = {
		p = plugin.info().name,
		P = server:info().commandChar .. plugin.info().name,
		u = who,
		U = util.splituser(who),
		M = str
	}

	server:say(channel, util.convert(formats.list, kw))
end

local function info(server, channel, who, args)
	local name = args:match("(%w+)")
	local kw = {
		p = plugin.info().name,
		P = server:info().commandChar .. plugin.info().name,
		u = who,
		U = util.splituser(who)
	}

	if not name or #name <= 0 then
		server:say(channel, who .. ", please specify a plugin")
	else
		local info = plugin.info(name)

		if not info then
			server:say(channel, util.convert(formats.unknown, kw))
		else
			local str = ""

			for _, item in ipairs(config.order) do
				if not formats[item] then
					logger.warn("warning: unknown item description `" .. item .. "'")
				else
					--
					-- Use the same key from formats table and the plugin
					-- information table, but there is no 'summary' field
					-- so check for it or use the key instead.
					--
					if item == "summary" then
						kw.M = info.name
					else
						kw.M = info[item]
					end
	
					str = str .. util.convert(formats[item], kw)
				end
			end

			server:say(channel, str)
		end
	end
end

local commands = {
	list = list,
	info = info,
}

function onCommand(server, channel, who, message)
	local c = message:match("^(%w+)")
	local kw = {
		u = who,
		U = util.splituser(who),
		p = plugin.info().name,
		P = server:info().commandChar .. plugin.info().name
	}

	if not c or not commands[c] then
		server:say(channel, util.convert(helps.usage, kw))
	else
		commands[c](server, channel, who, message:sub(#c + 2))
	end
end

onLoad = loadconfig
onReload = loadconfig

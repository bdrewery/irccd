--
-- logger.lua -- a logger plugin to log everything
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
COMMENT		= "A logger plugin to log everything"
LICENSE		= "ISC"

-- Modules
local fs	= require "irccd.fs"
local parser	= require "irccd.parser"
local plugin	= require "irccd.plugin"
local system	= require "irccd.system"
local util	= require "irccd.util"

-- Logger configuration
local configuration = { }

local format = {
	cnotice	= "[#c] #m",
	join	= ">> #U joined #c",
	kick	= "#t has been kicked by #U [reason: #m]",
	me	= "* #U #m",
	message	= "%H:%M #U: #m",
	mode	= ":: #U changed the mode to: #m #M",
	notice	= "[notice] (#U) #T: #m",
	part	= "<< #U left #c [#m]",
	topic	= ":: #U changed the topic to: #t",
	umode	= "#U set mode #m to #T"
}

-- Load the config.
local function loadConfig()
	local config = parser.new(
	    plugin.info().home .. "/logger.conf",
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

	local formats = config:getSection("formats")
	if formats ~= nil then
		for k, _ in pairs(format) do
			if formats:hasOption(k) then
				format[k] = formats:getOption(k)

				-- Set to nil means no log, so in config is ""
				if #format[k] <= 0 then
					format[k] = nil
				end
			end
		end
	end
end

function onLoad()
	loadConfig()
end

-- Formatter, used to convert % and # variables
local function convert(what, keywords)
	-- First convert dates from % with date:format.
	local date = util.date()
	local line = date:format(what)

	-- Remove ~ if found.
	line = line:gsub("~", system.home)

	-- Add environment variable.
	line = line:gsub("%${(%w+)}", system.env)

	-- Now convert the # with gsub.
	return line:gsub("#(.)", keywords)
end

-- Open the file
local function open(keywords)
	-- Convert variable substitutions
	local path = convert(configuration.home, keywords)

	-- Test if parent directory exists
	local parent = fs.dirname(path)

	if not fs.exists(parent) then
		local ret, err = fs.mkdir(parent)
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
		s = server:info().name,
		u = who,
		U = util.splituser(who)
	}

	write(format.cnotice, keywords)
end

function onJoin(server, channel, nickname)
	local keywords = {
		c = channel,
		s = server:getName(),
		u = nickname,
		U = util.splituser(nickname)
	}

	write(format.join, keywords)
end

function onKick(server, channel, who, target, reason)
	local keywords = {
		c = channel,
		s = server:getName(),
		t = target,
		u = who,
		U = util.splituser(who),
		m = reason
	}

	write(format.kick, keywords)
end

function onMe(server, channel, who, message)
	local keywords = {
		c = channel,
		s = server:getName(),
		m = message,
		u = who,
		U = util.splituser(who)
	}

	write(format.me, keywords)
end

function onMessage(server, channel, who, message)
	local keywords = {
		c = channel,
		s = server:getName(),
		m = message,
		u = who,
		U = util.splituser(who)
	}

	write(format.message, keywords)
end

function onMode(server, channel, who, mode, modeArg)
	local keywords = {
		c = channel,
		s = server:getName(),
		m = mode,
		M = modeArg,
		u = who,
		U = util.splituser(who)
	}

	write(format.mode, keywords)
end

function onNotice(server, who, target, notice)
	local keywords = {
		c = util.splituser(who),	-- for file logging like channel
		m = notice,
		s = server:getName(),
		T = target,
		u = who,
		U = util.splituser(who)
	}

	write(format.notice, keywords)
end

function onPart(server, channel, nickname, reason)
	local keywords = {
		c = channel,
		s = server:getName(),
		m = reason,
		u = who,
		U = util.splituser(nickname)
	}

	write(format.part, keywords)
end

function onReload()
	loadConfig()
end

function onTopic(server, channel, who, topic)
	local keywords = {
		c = channel,
		s = server:getName(),
		t = topic,
		u = who,
		U = util.splituser(who)
	}

	write(format.topic, keywords)
end

function onUserMode(server, who, mode)
	local identity = server:getIdentity()

	local keywords = {
		m = mode,
		s = server:getName(),
		T = identity.nickname,
	}

	write(format.umode, keywords)
end

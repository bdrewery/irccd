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

local plugin = require("plugin")

local words = { }
local authorized = { }

function readFile(table, filename)
	local f, err = io.open(plugin.getHome() .. "/" .. filename)
	table = { }

	if (f ~= nil) then
		for w in f:lines() do
			if #w > 0 then
				table.insert(table, w)
			end
		end
	end
end

-- Reload every words
function reloadWords()
	readFile(words, "words.txt")
end

-- Reload every users allowed to add, remove or reload
function reloadUsers()
	readFile(authorized, "users.txt")
end

reloadWords()
reloadUsers()

function onMessage(server, channel, nick, message)
	for _, w in pairs(words) do
		if string.find(message, " " .. w .. " ") then
			server:say(channel, nick .. ": ne soyez pas vulgaire!")

			-- do not multiple
			break;
		end
	end
end

function onCommand(server, channel, nick, message)

end

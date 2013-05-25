--
-- api-parser-test.lua -- test the parser API
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

local parser = require("parser")

-- Open and log errors
local config = parser.new("Parser-log.conf", { parser.DisableRedefinition })

config:onLog(function (lineno, section, message)
	print("Warning: line " .. tostring(lineno) .. ": [" .. section .. "] => " .. message)
end)

config:open()
config = nil

-- Open a correct config file
local config = parser.new("Parser-correct.conf")

config:open()
local general = config:getSection("general")

print("general.verbose = " .. general:getOption("verbose"))
print("general.nickname = " .. general:getOption("nickname"))

-- Iterate over the [object] sections
for s in config:findSections("object") do
	print("object " .. s:getOption("id") .. " is named " .. s:getOption("name"))
end

-- Open and read sections

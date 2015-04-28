local util	= require "irccd.util"

function split()
	local s = util.split("a:b:c:d", ":")

	assert(s[1] == "a", string.format("s[1] error, %s expected, got %s", "a", tostring(s[1])))
	assert(s[2] == "b", string.format("s[2] error, %s expected, got %s", "b", tostring(s[2])))
	assert(s[3] == "c", string.format("s[3] error, %s expected, got %s", "c", tostring(s[3])))
	assert(s[4] == "d", string.format("s[4] error, %s expected, got %s", "d", tostring(s[4])))

	local s = util.split("a:b:c:d", ":", 2)

	assert(s[1] == "a", string.format("s[1] error, %s expected, got %s", "a", tostring(s[1])))
	assert(s[2] == "b:c:d", string.format("s[2] error, %s expected, got %s", "b:c:d", tostring(s[2])))

	local s = util.split("a:b,c;d", ":,;")

	assert(s[1] == "a", string.format("s[1] error, %s expected, got %s", "a", tostring(s[1])))
	assert(s[2] == "b", string.format("s[2] error, %s expected, got %s", "b", tostring(s[2])))
	assert(s[3] == "c", string.format("s[3] error, %s expected, got %s", "c", tostring(s[3])))
	assert(s[4] == "d", string.format("s[4] error, %s expected, got %s", "d", tostring(s[4])))
end

function strip()
	local s1 = "simple string"
	local s2 = "simple string    "
	local s3 = "    simple string"

	assert(util.strip(s1) == "simple string", "\"simple string\" expected")
	assert(util.strip(s2) == "simple string", "\"simple string\" expected")
	assert(util.strip(s3) == "simple string", "\"simple string\" expected")
end

function convert()
	local kw = {
		p = "foo",
		k  = "123"
	}

	local input = "abc #p #k cba"
	local expected = "abc foo 123 cba"
	local output = util.convert(input, kw)

	assert(output == expected, string.format("%s expected, got %s", expected, output))
end

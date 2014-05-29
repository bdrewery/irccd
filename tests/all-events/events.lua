--
-- events.lua -- test all events
--

local splituser	= require("irccd.util").splituser

local function printf(fmt, ...)
	print(string.format(fmt, ...))
end

function onMe(s, c, n, m)
	printf("onMe: (%s) * %s@%s %s", s:info().name, splituser(n), c, m)
end

function onCommand(s, c, n, m)
	printf("onCommand: (%s) %s@%s: %s", s:info().name, splituser(n), c, m)
end

function onConnect(s)
	printf("onConnect: (%s)", s:info().name)

	-- Generate onWhois event for myself
	s:whois(s:info().identity.nickname)
end

function onChannelNotice(s, w, c, n)
	printf("onChannelNotice: (%s) [%s] %s: %s", s:info().name, c, splituser(w), n)
end

function onInvite(s, c, w)
	printf("onInvite: (%s) %s invited you on %s", s:info().name, splituser(w), c)
end

function onJoin(s, c, w)
	printf("onJoin: (%s) %s joined %s", s:info().name, splituser(w), c)
end

function onKick(s, c, w, k, r)
	printf("onKick: (%s) %s has been kicked by %s from %s [%s]", s:info().name, splituser(k), splituser(w), c, r)
end

function onLoad()
	printf("onLoad")
end

function onMessage(s, c, n, m)
	printf("onMessage: (%s) %s@%s: %s", s:info().name, splituser(n), c, m)
end

function onMode(s, c, w, m, arg)
	printf("onMode: (%s) %s@%s %s %s", s:info().name, splituser(w), c, m, arg)
end

function onNames(s, c, list)
	printf("onNames: (%s) users on %s:", s:info().name, c)

	for _, v in ipairs(list) do
		printf("    %s", v)
	end
end

function onNick(s, old, new)
	printf("onNick: (%s) %s -> %s", s:info().name, splituser(old), splituser(new))
end

function onNotice(s, w, t, n)
	printf("onNotice: (%s) %s -> %s: %s", s:info().name, splituser(w), splituser(t), n)
end

function onPart(s, c, w, r)
	printf("onPart: (%s) %s left %s [%s]", s:info().name, splituser(w), c, r)
end

function onQuery(s, w, m)
	printf("onQuery: (%s) %s: %s", s:info().name, splituser(w), m)
end

function onQueryCommand(s, w, m)
	printf("onQueryCommand: (%s) %s: %s", s:info().name, splituser(w), m)
end

function onReload()
	printf("onReload")
end

function onTopic(s, c, w, t)
	printf("onTopic: (%s) %s changed the %s topic to: %s", s:info().name, splituser(w), c, t)
end

function onUnload()
	printf("onUnload")
end

function onUserMode(s, n, m)
	printf("onUserMode: (%s) %s set the mode to %s", s:info().name, splituser(n), m)
end

function onWhois(s, w)
	printf("onWhois: (%s) information on %s", s:info().name, w.nickname)	
	printf("    username: %s", w.user)
	printf("    host: %s", w.host)
	printf("    realname: %s", w.realname)

	local channels = ""
	for _, v in ipairs(w.channels) do
		channels = channels .. v .. " "
	end

	printf("    channels: %s", channels)
end

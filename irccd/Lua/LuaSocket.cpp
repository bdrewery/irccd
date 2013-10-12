/*
 * LuaSocket.cpp -- Lua bindings for Sockets
 * 
 * Copyright 2013 David Demelier <markand@malikania.fr>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <unordered_map>

#include <Socket.h>
#include <SocketAddress.h>
#include <SocketListener.h>

#include "Luae.h"
#include "LuaSocket.h"

#define SOCKET_TYPE		"Socket"
#define ADDRESS_TYPE		"SocketAddress"
#define LISTENER_TYPE		"SocketListener"

namespace irccd
{

namespace
{

/* ---------------------------------------------------------
 * Enumerations
 * --------------------------------------------------------- */

typedef std::unordered_map<std::string, int> EnumMap;

EnumMap createSockFamilies()
{
	EnumMap map;

	map["Inet"]	= AF_INET;
	map["Inet6"]	= AF_INET6;

#if !defined(_WIN32)
	map["Unix"]	= AF_UNIX;
#endif

	return map;
}

EnumMap createSockTypes()
{
	EnumMap map;

	map["Stream"]	= SOCK_STREAM;
	map["Datagram"]	= SOCK_DGRAM;

	return map;
}

EnumMap createSockProtocols()
{
	EnumMap map;

	map["Tcp"]	= IPPROTO_TCP;
	map["Udp"]	= IPPROTO_UDP;
	map["IPv4"]	= IPPROTO_IP;
	map["IPv6"]	= IPPROTO_IPV6;

	return map;
}

EnumMap sockFamilies	= createSockFamilies();
EnumMap sockTypes	= createSockTypes();
EnumMap sockProtocols	= createSockProtocols();

/* ---------------------------------------------------------
 * Socket options
 * --------------------------------------------------------- */

/*
 * Windows setsockopt() says that options which takes a bool may use
 * the bool type. On some Unix systems (including FreeBSD), passing
 * a bool to SO_REUSEADDR for example, will result in a EINVAL.
 */
#if defined(_WIN32)

typedef bool 		OptionBool;
typedef int		OptionInteger;

#else

typedef int 		OptionBool;
typedef int		OptionInteger;

#endif

enum class ArgType
{
	Invalid,
	Boolean,
	Integer
};

struct Option
{
	int		m_level;
	int		m_optname;
	ArgType		m_argtype;

	Option()
		: m_level(0)
		, m_optname(0)
		, m_argtype(ArgType::Invalid)
	{
	}

	Option(int level, int optname, ArgType type)
		: m_level(level)
		, m_optname(optname)
		, m_argtype(type)
	{
	}
};

typedef std::unordered_map<std::string,
	std::unordered_map<std::string, Option>
> OptionMap;

/*
 * Map here the socket options from the C side to Lua. It's very
 * close to the standard API.
 *
 * This map is used to retrieve which argument we must pass to the
 * Socket::set function. It also setup the enumerations to be bound
 * as tables.
 */
OptionMap mapOfOptions()
{
	OptionMap map;

	/*
	 * Standard sockets options SOL_SOCKET
	 */

#if defined(SO_REUSEADDR)
	map["socket"]["reuse-address"]	= Option(SOL_SOCKET,
						 SO_REUSEADDR,
						 ArgType::Boolean);
#endif
#if defined(SO_BROADCAST)
	map["socket"]["broadcast"]	= Option(SOL_SOCKET,
						 SO_BROADCAST,
						 ArgType::Boolean);
#endif
#if defined(SO_DEBUG)
	map["socket"]["debug"]		= Option(SOL_SOCKET,
						 SO_DEBUG,
						 ArgType::Boolean);
#endif
#if defined(SO_KEEPALIVE)
	map["socket"]["keep-alive"]	= Option(SOL_SOCKET,
						 SO_KEEPALIVE,
						 ArgType::Boolean);
#endif
#if defined(SO_RCVBUF)
	map["socket"]["receive-buffer"]	= Option(SOL_SOCKET,
						 SO_RCVBUF,
						 ArgType::Integer);
#endif

	/*
	 * TCP socket options
	 */

#if defined(TCP_NODELAY)
	map["tcp"]["no-delay"]		= Option(IPPROTO_TCP,
						 TCP_NODELAY,
						 ArgType::Boolean);
#endif

	/*
	 * IPv6 options
	 */

#if defined(IPV6_V6ONLY)
	map["ipv6"]["v6only"]		= Option(IPPROTO_IPV6,
						 IPV6_V6ONLY,
						 ArgType::Boolean);
#endif

	return map;
}

OptionMap options = mapOfOptions();

/* ---------------------------------------------------------
 * Socket functions
 * --------------------------------------------------------- */

void mapToTable(lua_State *L,
		const EnumMap &map,
		int index,
		const std::string &name)
{
	lua_createtable(L, 0, 0);

	for (auto p : map)
	{
		lua_pushinteger(L, p.second);
		lua_setfield(L, -2, p.first.c_str());
	}

	if (index < 0)
		-- index;

	lua_setfield(L, index, name.c_str());
}

int genericReceive(lua_State *L, bool udp)
{
	Socket *s	= Luae::toType<Socket *>(L, 1, SOCKET_TYPE);
	int requested	= luaL_checkinteger(L, 2);
	SocketAddress *sa;
	int nret;
	char *data;
	size_t nbread;

	/*
	 * This parameter only needed for UDP sockets
	 */
	if (udp)
		sa = Luae::toType<SocketAddress *>(L, 2, ADDRESS_TYPE);

	/*
	 * Allocate a temporarly buffer for receiveing the data.
	 */
	data = static_cast<char *>(std::malloc(requested));
	if (data == nullptr)
	{
		lua_pushnil(L);
		lua_pushstring(L, std::strerror(errno));

		return 2;
	}

	try
	{
		if (!udp)
			nbread = s->recv(data, requested);
		else
			nbread = s->recvfrom(data, requested, *sa);

		lua_pushlstring(L, data, nbread);

		nret = 1;
	}
	catch (SocketError error)
	{
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		nret = 2;
	}

	std::free(data);

	return nret;
}

int genericSend(lua_State *L, bool udp)
{
	Socket *s	= Luae::toType<Socket *>(L, 1, SOCKET_TYPE);
	const char *msg	= luaL_checkstring(L, 2);
	SocketAddress *sa;
	long nbsent;

	/*
	 * This parameter only needed for UDP sockets
	 */
	if (udp)
		sa = Luae::toType<SocketAddress *>(L, 3, ADDRESS_TYPE);

	try
	{
		if (!udp)
			nbsent = s->send(msg, strlen(msg));
		else
			nbsent = s->sendto(msg, strlen(msg), *sa);
	}
	catch (SocketError error)
	{
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 2;
	}

	lua_pushnumber(L, nbsent);

	return 1;
}

int socketNew(lua_State *L)
{
	int domain;
	int type = SOCK_STREAM;
	int protocol = 0;

	// Domain is the only one mandatory
	domain = luaL_checkinteger(L, 1);

	if (lua_gettop(L) >= 2)
		type = luaL_checkinteger(L, 2);
	if (lua_gettop(L) >= 3)
		protocol = luaL_checkinteger(L, 3);

	try
	{
		new (L, SOCKET_TYPE) Socket(domain, type, protocol);
	}
	catch (SocketError error)
	{
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 2;
	}

	return 1;
}

int socketBlockMode(lua_State *L)
{
	Socket *s	= Luae::toType<Socket *>(L, 1, SOCKET_TYPE);
	bool mode	= lua_toboolean(L, 2);

	try
	{
		s->blockMode(mode);
	}
	catch (SocketError error)
	{
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 2;
	}

	lua_pushboolean(L, true);

	return 1;
}

int socketBind(lua_State *L)
{
	Socket *s = Luae::toType<Socket *>(L, 1, SOCKET_TYPE);
	SocketAddress *a = Luae::toType<SocketAddress *>(L, 2, ADDRESS_TYPE);

	/*
	 * Get nil + error message for chained expression like:
	 * s:bind(address.bindInet { port = 80, family = 1 })
	 */
	if (lua_type(L, 1) == LUA_TNIL && lua_type(L, 2) == LUA_TSTRING)
	{
		lua_pushnil(L);
		lua_pushvalue(L, 2);

		return 2;
	}

	try
	{
		s->bind(*a);
	}
	catch (SocketError error)
	{
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 2;
	}

	lua_pushboolean(L, true);

	return 1;
}

int socketClose(lua_State *L)
{
	Socket *s = Luae::toType<Socket *>(L, 1, SOCKET_TYPE);

	s->close();

	return 0;
}

int socketConnect(lua_State *L)
{
	Socket *s = Luae::toType<Socket *>(L, 1, SOCKET_TYPE);
	SocketAddress *a = Luae::toType<SocketAddress *>(L, 2, ADDRESS_TYPE);

	/*
	 * Get nil + error message for chained expression like:
	 * s:bind(address.bindInet { port = 80, family = 1 })
	 */
	if (lua_type(L, 1) == LUA_TNIL && lua_type(L, 2) == LUA_TSTRING)
	{
		lua_pushnil(L);
		lua_pushvalue(L, 2);

		return 2;
	}

	try
	{
		s->connect(*a);
	}
	catch (SocketError error)
	{
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 2;
	}

	lua_pushboolean(L, true);

	return 1;
}

int socketAccept(lua_State *L)
{
	Socket *s = Luae::toType<Socket *>(L, 1, SOCKET_TYPE);
	Socket client;
	SocketAddress info;

	try
	{
		client = s->accept(info);
		new (L, SOCKET_TYPE) Socket(client);
		new (L, ADDRESS_TYPE) SocketAddress(info);
	}
	catch (SocketError error)
	{
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 3;
	}

	return 2;
}

int socketListen(lua_State *L)
{
	Socket *s = Luae::toType<Socket *>(L, 1, SOCKET_TYPE);
	int max = 64;

	if (lua_gettop(L) >= 2)
		max = luaL_checkinteger(L, 2);

	try
	{
		s->listen(max);
	}
	catch (SocketError error)
	{
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 2;
	}

	lua_pushboolean(L, true);

	return 1;
}

int socketReceive(lua_State *L)
{
	return genericReceive(L, false);
}

int socketReceiveFrom(lua_State *L)
{
	return genericReceive(L, true);
}

int socketSend(lua_State *L)
{
	return genericSend(L, false);
}

int socketSendTo(lua_State *L)
{
	return genericSend(L, true);
}

int socketSet(lua_State *L)
{
	Socket *s	= Luae::toType<Socket *>(L, 1, SOCKET_TYPE);
	const char *lvl	= luaL_checkstring(L, 2);
	const char *nm	= luaL_checkstring(L, 3);
	int nret;

	try
	{
		OptionBool bvalue;
		OptionInteger ivalue;
		void *ptr = nullptr;
		size_t size;

		auto o = options.at(lvl).at(nm);

		switch (o.m_argtype)
		{
		case ArgType::Boolean:
			bvalue = lua_toboolean(L, 4);
			ptr = static_cast<void *>(&bvalue);
			size = sizeof (OptionBool);
			break;
		case ArgType::Integer:
			ivalue = luaL_checkinteger(L, 4);
			ptr = static_cast<void *>(&ivalue);
			size = sizeof (OptionInteger);
			break;
		default:
			break;
		}

		if (ptr != nullptr)
			s->set(o.m_level, o.m_optname, ptr, size);
		lua_pushboolean(L, true);

		nret = 1;
	}
	catch (std::out_of_range)
	{
		lua_pushnil(L);
		lua_pushstring(L, "invalid level or option name");

		nret = 2;
	}
	catch (SocketError error)
	{
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		nret = 2;
	}


	return nret;
}

int sockEq(lua_State *L)
{
	Socket *s1 = Luae::toType<Socket *>(L, 1, SOCKET_TYPE);
	Socket *s2 = Luae::toType<Socket *>(L, 2, SOCKET_TYPE);

	lua_pushboolean(L, *s1 == *s2);

	return 1;
}

int sockToString(lua_State *L)
{
	Socket *s = Luae::toType<Socket *>(L, 1, SOCKET_TYPE);

	lua_pushfstring(L, "socket %d", s->getType());

	return 1;
}

int sockGc(lua_State *L)
{
	Luae::toType<Socket *>(L, 1, SOCKET_TYPE)->~Socket();

	return 0;
}

const luaL_Reg sockFunctions[] = {
	{ "new",		socketNew		},
	{ nullptr,		nullptr			}
};

const luaL_Reg sockMethods[] = {
	{ "blockMode",		socketBlockMode		},
	{ "bind",		socketBind		},
	{ "close",		socketClose		},
	{ "connect",		socketConnect		},
	{ "accept",		socketAccept		},
	{ "listen",		socketListen		},
	{ "receive",		socketReceive		},
	{ "receiveFrom",	socketReceiveFrom	},
	{ "send",		socketSend		},
	{ "sendTo",		socketSendTo		},
	{ "set",		socketSet		},
	{ nullptr,		nullptr			}
};

const luaL_Reg sockMeta[] = {
	{ "__eq",		sockEq			},
	{ "__tostring",		sockToString		},
	{ "__gc",		sockGc			},
	{ nullptr,		nullptr			}
};

/* ---------------------------------------------------------
 * Socket address functions
 * --------------------------------------------------------- */

int addrConnectInet(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TTABLE);

	std::string host	= Luae::requireField<std::string>(L, 1, "host");
	int port		= Luae::requireField<int>(L, 1, "port");
	int family		= Luae::requireField<int>(L, 1, "family");

	try
	{
		new (L, ADDRESS_TYPE) ConnectAddressIP(host, port, family);
	}
	catch (SocketError error)
	{
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 2;
	}

	return 1;
}

int addrBindInet(lua_State *L)
{
	std::string address = "*";
	int port, family;

	luaL_checktype(L, 1, LUA_TTABLE);

	// Mandatory fields
	port = Luae::requireField<int>(L, 1, "port");
	family = Luae::requireField<int>(L, 1, "family");

	// Optional fields
	if (Luae::typeField(L, 1, "address") == LUA_TSTRING)
		address = Luae::requireField<std::string>(L, 1, "address");

	try
	{
		new (L, ADDRESS_TYPE) BindAddressIP(address, port, family);
	}
	catch (SocketError error)
	{
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 2;
	}

	return 1;
}

int addrUnix(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	bool rem = false;

	if (lua_gettop(L) >= 2)
		rem = lua_toboolean(L, 2);

	new (L, ADDRESS_TYPE) AddressUnix(path, rem);

	return 1;
}

int addrToString(lua_State *L)
{
	SocketAddress *sa = Luae::toType<SocketAddress *>(L, 1, ADDRESS_TYPE);

	lua_pushfstring(L, "address of length %d", sa->length());

	return 1;
}

int addrGc(lua_State *L)
{
	Luae::toType<SocketAddress *>(L, 1, ADDRESS_TYPE)->~SocketAddress();

	return 0;
}

const luaL_Reg addrFunctions[] = {
	{ "connectInet",	addrConnectInet		},
	{ "bindInet",		addrBindInet		},
	{ "unix",		addrUnix		},
	{ nullptr,		nullptr			}
};

const luaL_Reg addrMeta[] = {
	{ "__tostring",		addrToString		},
	{ "__gc",		addrGc			},
	{ nullptr,		nullptr			}
};

/* ---------------------------------------------------------
 * Socket listener functions
 * --------------------------------------------------------- */

int listenerNew(lua_State *L)
{
	new (L, LISTENER_TYPE) SocketListener();

	return 1;
}

int listenerAdd(lua_State *L)
{
	SocketListener *l = Luae::toType<SocketListener *>(L, 1, LISTENER_TYPE);
	Socket *s = Luae::toType<Socket *>(L, 2, SOCKET_TYPE);

	l->add(*s);

	return 0;
}

int listenerRemove(lua_State *L)
{
	SocketListener *l = Luae::toType<SocketListener *>(L, 1, LISTENER_TYPE);
	Socket *s = Luae::toType<Socket *>(L, 2, SOCKET_TYPE);

	l->remove(*s);

	return 0;
}

int listenerClear(lua_State *L)
{
	Luae::toType<SocketListener *>(L, 1, LISTENER_TYPE)->clear();

	return 0;
}

int listenerSelect(lua_State *L)
{
	SocketListener *l = Luae::toType<SocketListener *>(L, 1, LISTENER_TYPE);
	int seconds = 0, ms = 0, nret;

	if (lua_gettop(L) >= 2)
		seconds = luaL_checkinteger(L, 2);
	if (lua_gettop(L) >= 3)
		ms = luaL_checkinteger(L, 3);

	try
	{
		Socket selected = l->select(seconds, ms);
		new (L, SOCKET_TYPE) Socket(selected);

		nret = 1;
	}
	catch (SocketError error)
	{
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		nret = 2;
	}
	catch (SocketTimeout timeout)
	{
		lua_pushnil(L);
		lua_pushstring(L, timeout.what());

		nret = 2;
	}

	return nret;
}

int listenerToStr(lua_State *L)
{
	SocketListener *l = Luae::toType<SocketListener *>(L, 1, LISTENER_TYPE);

	lua_pushfstring(L, "listener of %d clients", l->size());

	return 1;
}

int listenerGc(lua_State *L)
{
	Luae::toType<SocketListener *>(L, 1, LISTENER_TYPE)->~SocketListener();

	return 0;
}

const luaL_Reg listenerFunctions[] = {
	{ "new",			listenerNew	},
	{ nullptr,			nullptr		}
};

const luaL_Reg listenerMethods[] = {
	{ "add",			listenerAdd	},
	{ "remove",			listenerRemove	},
	{ "clear",			listenerClear	},
	{ "select",			listenerSelect	},
	{ nullptr,			nullptr		}
};

const luaL_Reg listenerMeta[] = {
	{ "__tostring",			listenerToStr	},
	{ "__gc",			listenerGc	},
	{ nullptr,			nullptr		}
};

}

int luaopen_socket(lua_State *L)
{
	// Socket functions
	luaL_newlib(L, sockFunctions);

	// Map families, types
	mapToTable(L, sockFamilies, -1, "family");
	mapToTable(L, sockTypes, -1, "type");
	mapToTable(L, sockProtocols, -1, "protocol");

	// Create Socket type
	luaL_newmetatable(L, SOCKET_TYPE);
	luaL_setfuncs(L, sockMeta, 0);
	luaL_newlib(L, sockMethods);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	return 1;
}

int luaopen_socket_address(lua_State *L)
{
	luaL_newlib(L, addrFunctions);

	// Create SocketAddress type
	luaL_newmetatable(L, ADDRESS_TYPE);
	luaL_setfuncs(L, addrMeta, 0);
	lua_pop(L, 1);

	return 1;
}

int luaopen_socket_listener(lua_State *L)
{
	luaL_newlib(L, listenerFunctions);

	// Create the SocketListener type
	luaL_newmetatable(L, LISTENER_TYPE);
	luaL_setfuncs(L, listenerMeta, 0);
	luaL_newlib(L, listenerMethods);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	return 1;
}

} // !irccd

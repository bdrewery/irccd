/*
 * LuaSocket.cpp -- Lua bindings for Sockets
 *
 * Copyright (c) 2013, 2014, 2015 David Demelier <markand@malikania.fr>
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

#if !defined(_WIN32)
#  include <sys/un.h>
#endif

#include <IrccdConfig.h>

#include <common/Socket.h>
#include <common/SocketAddress.h>
#include <common/SocketListener.h>

#include <irccd/Luae.h>

#include "LuaSocket.h"

namespace irccd {

namespace {

/*
 * This field is the one that is put into the address table for further
 * usage. It should not be used by the end user.
 */
const char *AddrField		= "__address";

/*
 * This field is used to store the length of the binary sockaddr_storage
 * data.
 */
const char *LengthField		= "__addrlen";

/*
 * This field is used to store addresses from recvfrom in the registry so we
 * can return the same if the user does not discard the return value.
 */
const char *RegField		= "__addresses";

const char *SocketType		= "Socket";
const char *ListenerType	= "Listener";

/* ---------------------------------------------------------
 * Enumerations
 * --------------------------------------------------------- */

LuaeEnum::Def sockFamilies {
	{ "Inet",	AF_INET		},
	{ "Inet6",	AF_INET6	},

#if !defined(_WIN32)
	{ "Unix",	AF_UNIX		}
#endif
};

LuaeEnum::Def sockTypes {
	{ "Stream",	SOCK_STREAM	},
	{ "Datagram",	SOCK_DGRAM	}
};

LuaeEnum::Def sockProtocols {
	{ "Tcp",	IPPROTO_TCP	},
	{ "Udp",	IPPROTO_UDP	},
	{ "IPv4",	IPPROTO_IP	},
	{ "IPv6",	IPPROTO_IPV6	}
};

/* ---------------------------------------------------------
 * Socket options
 * --------------------------------------------------------- */

/*
 * Windows setsockopt() says that options which takes a bool may use
 * the bool type. On some Unix systems (including FreeBSD), passing
 * a bool to SO_REUSEADDR for example, will result in a EINVAL.
 */
#if defined(_WIN32)

using OptionBool	= bool;
using OptionInteger	= int;

#else

using OptionBool	= int;
using OptionInteger	= int;

#endif

enum class ArgType {
	Invalid,
	Boolean,
	Integer
};

struct Option {
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

using OptionMap	= std::unordered_map<std::string,
			std::unordered_map<std::string, Option>
		  >;

/*
 * Map here the socket options from the C side to Lua. It's very
 * close to the standard API.
 *
 * This map is used to retrieve which argument we must pass to the
 * Socket::set function. It also setup the enumerations to be bound
 * as tables.
 */
OptionMap prepareOptions()
{
	OptionMap options;

	// "socket" -> SOL_SOCKET
#if defined(SO_REUSEADDR)
	options["socket"]["reuse-address"] = Option(SOL_SOCKET, SO_REUSEADDR, ArgType::Boolean);
#endif
#if defined(SO_BROADCAST)
	options["socket"]["broadcast"] = Option(SOL_SOCKET, SO_BROADCAST, ArgType::Boolean);
#endif
#if defined(SO_DEBUG)
	options["socket"]["debug"] = Option(SOL_SOCKET, SO_DEBUG, ArgType::Boolean);
#endif
#if defined(SO_KEEPALIVE)
	options["socket"]["keep-alive"] = Option(SOL_SOCKET, SO_KEEPALIVE, ArgType::Boolean);
#endif
#if defined(SO_RCVBUF)
	options["socket"]["receive-buffer"] = Option(SOL_SOCKET, SO_RCVBUF, ArgType::Integer);
#endif

	// "tcp" -> IPPROTO_TCP
#if defined(TCP_NODELAY)
	options["tcp"]["no-delay"] = Option(IPPROTO_TCP, TCP_NODELAY, ArgType::Boolean);
#endif
	// "ipv6" -> IPPROTO_IPV6
#if defined(IPV6_V6ONLY)
	options["ipv6"]["v6only"] = Option(IPPROTO_IPV6, IPV6_V6ONLY, ArgType::Boolean);
#endif

	return options;
}

OptionMap options = prepareOptions();

/* ---------------------------------------------------------
 * Private helpers
 * --------------------------------------------------------- */

using SocketAddressPtr = std::unique_ptr<SocketAddress>;

/**
 * Store the SocketAddress into the AddrField to the table at the
 * given index.
 *
 * @param L the Lua state
 * @param index the table index
 * @param sa the socket address
 */
void storeAddressData(lua_State *L, int index, const SocketAddress &sa)
{
	if (LuaeTable::type(L, index, AddrField) == LUA_TNIL) {
		if (index < 0)
			-- index;

		lua_pushlstring(L, reinterpret_cast<const char *>(&sa.address()), sa.length());
		lua_setfield(L, -2, AddrField);

		lua_pushinteger(L, sa.length());
		lua_setfield(L, -2, LengthField);
	}
}

#if !defined(WIN32)

/**
 * Check that the table at the given index can be used as a socket address
 * for AF_UNIX family.
 *
 * The table must have the following fields:
 *
 *	path	- (string) Required, the path to the socket
 *	remove	- (bool) Optional, tell to remove the file before usage
 *
 * @param L the Lua state
 * @param index the table index
 * @return a ready to use address
 * @throw SocketError on errors
 */
SocketAddressPtr checkUnix(lua_State *L, int index, const Socket &)
{
	LUAE_STACK_CHECKBEGIN(L);

	auto path = LuaeTable::require<std::string>(L, index, "path");
	auto rm = false;

	if (LuaeTable::type(L, index, "remove") != LUA_TNIL)
		rm = lua_toboolean(L, index);

	LUAE_STACK_CHECKEQUALS(L);

	return SocketAddressPtr(new AddressUnix(path, rm));
}

#endif

/**
 * Check that a table can be used as both binding or connecting to a socket
 * with AF_INET or AF_INET6 families.
 *
 * The table must have the following fields:
 *
 *	port	- (number) Required, the port number
 *	family	- (enum) Optional, the family to use. Default: the same as socket
 *	type	- (enum) Optiona, type of socket. Default: the same as socket
 *
 * Additional fields for connecting:
 *	host	- (string) Required, the host to connect to
 *
 * Additional fields for binding:
 *	address	- (string) Required, the address to bind to or "*" for any.
 *
 * @param L the Lua state
 * @param index the table index
 * @param sc the socket
 * @return a ready to use address
 * @throw SocketError on errors
 */
SocketAddressPtr checkInet(lua_State *L, int index, const Socket &sc)
{
	LUAE_STACK_CHECKBEGIN(L);

	SocketAddressPtr ptr;

	auto port = LuaeTable::require<int>(L, index, "port");
	auto family = sc.getDomain();

	// If no family, we use the same as the socket
	if (LuaeTable::type(L, index, "family") == LUA_TNUMBER)
		family = LuaeTable::require<int>(L, index, "family");

	// If we have address field, we want to bind
	if (LuaeTable::type(L, index, "address") == LUA_TSTRING) {
		auto address = LuaeTable::require<std::string>(L, index, "address");

		ptr = SocketAddressPtr(new BindAddressIP(address, port, family));
	} else {
		auto host = LuaeTable::require<std::string>(L, index, "host");
		auto type = sc.getType();

		// If no type, we use the same as the socket
		if (LuaeTable::type(L, index, "type") == LUA_TNUMBER)
			type = LuaeTable::require<int>(L, index, "type");

		ptr = SocketAddressPtr(new ConnectAddressIP(host, port, family, type));
	}

	LUAE_STACK_CHECKEQUALS(L);

	return ptr;
}

/**
 * Get an address from a table parameter.
 *
 * @param L the Lua state
 * @param index the table index
 * @param sc the socket
 * @see checkInet
 * @see checkUnit
 * @return ready to use address
 * @throw SocketError on errors
 */
SocketAddressPtr checkAddress(lua_State *L, int index, const Socket &sc)
{
	LUAE_STACK_CHECKBEGIN(L);

	SocketAddressPtr address;

	Luae::checktype(L, index, LUA_TTABLE);

	/*
	 * If the field AddrField is present, we create the address with that
	 * data, otherwise, we return a new one
	 */
	if (LuaeTable::type(L, index, AddrField) == LUA_TSTRING) {
		sockaddr_storage st;

		auto data = LuaeTable::require<std::string>(L, index, AddrField);
		auto length = LuaeTable::require<int>(L, index, LengthField);

		std::memset(&st, 0, sizeof (sockaddr_storage));
		std::memcpy(&st, data.c_str(), length);

		address = SocketAddressPtr(new SocketAddress(st, length));
	} else {
#if !defined(_WIN32)
		if (LuaeTable::type(L, index, "path") != LUA_TNIL) {
			address = checkUnix(L, index, sc);
		} else {
			address = checkInet(L, index, sc);
		}
#else
		address = checkInet(L, index, sc);
#endif
		// Store the binary data
		storeAddressData(L, index, *address);
	}

	LUAE_STACK_CHECKEQUALS(L);

	return address;
}

#if !defined(WIN32)

/**
 * Set the Unix fields for an address.
 *
 * The table will have the following fields:
 *
 *	path	- (string) the path to the socket
 *
 * @param L the Lua state
 * @param address the socket address
 */
void pushUnix(lua_State *L, const SocketAddress &address)
{
	auto sun = reinterpret_cast<const sockaddr_un *>(&address.address());

	Luae::push(L, sun->sun_path);
	Luae::setfield(L, -2, "path");
}

#endif

/**
 * Set the fields for an internet socket.
 *
 * The table will have the following fields:
 *
 *	host	- (string) the hostname
 *	service	- (string) the service (port) as a string if available
 *	ip	- (string) the IP address
 *	port	- (number) the port number
 *
 * @param L the Lua state
 * @param address the socket address
 */
void pushInet(lua_State *L, const SocketAddress &address)
{
	char host[NI_MAXHOST], service[NI_MAXSERV];
	auto addr = address.address();
	auto len = address.length();

	// 1. First get all info by host and service names
	auto e = getnameinfo((sockaddr *)&addr, len, host, sizeof (host),
	    service, sizeof (service), 0);

	if (e == 0) {
		LuaeTable::set(L, -2, "host", host);
		LuaeTable::set(L, -2, "service", service);
	}

	// 2. Now get these info with numeric variants
	auto flags = 0 | NI_NUMERICHOST | NI_NUMERICSERV;
	e = getnameinfo((sockaddr *)&addr, len, host, sizeof (host),
	    service, sizeof (service), flags);

	if (e == 0) {
		LuaeTable::set(L, -2, "ip", host);
		LuaeTable::set(L, -2, "port", std::atoi(service));
	}
}

/**
 * Push an address or an existing one if any. The address are weak-valued
 * stored in the registry so if the user wants to keep them we use existing
 * one for performance reasons.
 *
 * Only addresses from Socket:accept() and Socket:recvfrom() are stored.
 *
 * @see pushUnix
 * @see pushInet
 */
void pushAddress(lua_State *L, const SocketAddress &address)
{
	LUAE_STACK_CHECKBEGIN(L);

	const auto &st = address.address();
	const auto &len = address.length();

	// 1. Check in the registry if we already have it.
	lua_getfield(L, LUA_REGISTRYINDEX, RegField);
	lua_pushlstring(L, reinterpret_cast<const char *>(&st), len);
	lua_rawget(L, -2);

	if (lua_type(L, -1) == LUA_TNIL) {
		lua_pop(L, 1);
		lua_createtable(L, 0, 0);
	
		// Store the binary data
		storeAddressData(L, -1, address);

#if !defined(WIN32)
		// Unix
		if (st.ss_family == AF_UNIX) {
			pushUnix(L, address);
		} else {
			pushInet(L, address);
		}
#else
		// Windows only
		pushInet(L, address);
#endif

		lua_pushlstring(L, reinterpret_cast<const char *>(&st), len);
		lua_pushvalue(L, -2);
		lua_rawset(L, -4);
	}

	lua_replace(L, -2);
	LUAE_STACK_CHECKEND(L, - 1);
}

int genericReceive(lua_State *L, bool udp)
{
	auto s = Luae::toType<Socket *>(L, 1, SocketType);
	int amount = luaL_checkinteger(L, 2);
	int ret;
	char *data;
	long nbread;

	/*
	 * Allocate a temporarly buffer for receiveing the data.
	 */
	data = static_cast<char *>(std::malloc(amount));
	if (data == nullptr) {
		lua_pushnil(L);
		lua_pushstring(L, std::strerror(errno));

		return 2;
	}

	try {
		SocketAddress info;

		if (!udp)
			nbread = s->recv(data, amount);
		else
			nbread = s->recvfrom(data, amount, info);

		lua_pushlstring(L, data, nbread);

		// Push the address for UDP
		if (udp) {
			pushAddress(L, info);
			ret = 2;
		} else
			ret = 1;
	} catch (SocketError error) {
		lua_pushnil(L);

		// If UDP we push a second nil for the address
		if (udp) {
			lua_pushnil(L);
			ret = 3;
		} else
			ret = 2;

		lua_pushstring(L, error.what());
	}

	std::free(data);

	return ret;
}

int genericSend(lua_State *L, bool udp)
{
	auto s = Luae::toType<Socket *>(L, 1, SocketType);
	auto msg = luaL_checkstring(L, 2);
	long nbsent;

	try {
		if (!udp)
			nbsent = s->send(msg, strlen(msg));
		else {
			auto addr = checkAddress(L, 3, *s);
			nbsent = s->sendto(msg, strlen(msg), *addr);
		}
	} catch (SocketError error) {
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 2;
	}

	lua_pushnumber(L, nbsent);

	return 1;
}

/* ---------------------------------------------------------
 * Socket functions
 * --------------------------------------------------------- */

int l_new(lua_State *L)
{
	int family = luaL_checkinteger(L, 1);
	int type = SOCK_STREAM;
	int protocol = 0;

	if (lua_gettop(L) >= 2)
		type = luaL_checkinteger(L, 2);
	if (lua_gettop(L) >= 3)
		protocol = luaL_checkinteger(L, 3);

	try {
		new (L, SocketType) Socket(family, type, protocol);
	} catch (SocketError error) {
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 2;
	}

	return 1;
}

int l_blockMode(lua_State *L)
{
	auto s = Luae::toType<Socket *>(L, 1, SocketType);
	auto mode = lua_toboolean(L, 2) ? true : false;

	try {
		s->blockMode(mode);
	} catch (SocketError error) {
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 2;
	}

	lua_pushboolean(L, true);

	return 1;
}

int l_bind(lua_State *L)
{
#if defined(COMPAT_1_1)
	/*
	 * Get nil + error message for chained expression like:
	 * s:bind(address.bindInet { port = 80, family = 1 })
	 */
	if (lua_type(L, 1) == LUA_TNIL && lua_type(L, 2) == LUA_TSTRING) {
		lua_pushnil(L);
		lua_pushvalue(L, 2);

		return 2;
	}
#endif

	try {
		auto s = Luae::toType<Socket *>(L, 1, SocketType);
		auto addr = checkAddress(L, 2, *s);

		s->bind(*addr);
	} catch (SocketError error) {
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 2;
	}

	lua_pushboolean(L, true);

	return 1;
}

int l_close(lua_State *L)
{
	auto s = Luae::toType<Socket *>(L, 1, SocketType);

	s->close();

	return 0;
}

int l_connect(lua_State *L)
{
	auto s = Luae::toType<Socket *>(L, 1, SocketType);
	auto addr = checkAddress(L, 2, *s);

#if defined(COMPAT_1_1)
	/*
	 * Get nil + error message for chained expression like:
	 * s:bind(address.bindInet { port = 80, family = 1 })
	 */
	if (lua_type(L, 1) == LUA_TNIL && lua_type(L, 2) == LUA_TSTRING) {
		lua_pushnil(L);
		lua_pushvalue(L, 2);

		return 2;
	}
#endif

	try {
		s->connect(*addr);
	} catch (SocketError error) {
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 2;
	}

	lua_pushboolean(L, true);

	return 1;
}

int l_accept(lua_State *L)
{
	auto s = Luae::toType<Socket *>(L, 1, SocketType);

	try {
		Socket client;
		SocketAddress info;

		client = s->accept(info);
		new (L, SocketType) Socket(client);
		pushAddress(L, info);
	} catch (SocketError error) {
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 3;
	}

	return 2;
}

int l_listen(lua_State *L)
{
	auto s = Luae::toType<Socket *>(L, 1, SocketType);
	auto max = 64;

	if (lua_gettop(L) >= 2)
		max = luaL_checkinteger(L, 2);

	try {
		s->listen(max);
	} catch (SocketError error) {
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 2;
	}

	lua_pushboolean(L, true);

	return 1;
}

int l_set(lua_State *L)
{
	auto s = Luae::toType<Socket *>(L, 1, SocketType);
	auto lvl = luaL_checkstring(L, 2);
	auto nm = luaL_checkstring(L, 3);
	int nret;

	try {
		OptionBool bvalue;
		OptionInteger ivalue;
		void *ptr = nullptr;
		size_t size;

		auto o = options.at(lvl).at(nm);

		switch (o.m_argtype) {
		case ArgType::Boolean:
			bvalue = lua_toboolean(L, 4) ? true : false;
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
	} catch (std::out_of_range) {
		lua_pushnil(L);
		lua_pushstring(L, "invalid level or option name");

		nret = 2;
	} catch (SocketError error) {
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		nret = 2;
	}

	return nret;
}

int l_send(lua_State *L)
{
#if defined(COMPAT_1_1)
	if (lua_gettop(L) >= 2 && lua_type(L, 2) == LUA_TUSERDATA) {
		Luae::deprecate(L, "send(data, address)", "sendto(data, address)");
		return genericSend(L, true);
	}
#endif

	return genericSend(L, false);
}

int l_sendto(lua_State *L)
{
	return genericSend(L, true);
}

int l_recv(lua_State *L)
{
	return genericReceive(L, false);
}

int l_recvfrom(lua_State *L)
{
	return genericReceive(L, true);
}

int l_eq(lua_State *L)
{
	auto s1 = Luae::toType<Socket *>(L, 1, SocketType);
	auto s2 = Luae::toType<Socket *>(L, 2, SocketType);

	lua_pushboolean(L, *s1 == *s2);

	return 1;
}

int l_tostring(lua_State *L)
{
	auto s = Luae::toType<Socket *>(L, 1, SocketType);

	Luae::pushfstring(L, "socket %d", s->getType());

	return 1;
}

int l_gc(lua_State *L)
{
	Luae::toType<Socket *>(L, 1, SocketType)->~Socket();

	return 0;
}

#if defined(COMPAT_1_1)

int l_receive(lua_State *L)
{
	Luae::deprecate(L, "receive", "recv");

	return genericReceive(L, false);
}

int l_receiveFrom(lua_State *L)
{
	Luae::deprecate(L, "receiveFrom", "recvfrom");

	return genericReceive(L, true);
}

#endif

const luaL_Reg sockFunctions[] = {
	{ "new",		l_new				},
	{ nullptr,		nullptr				}
};

const luaL_Reg sockMethods[] = {
/*
 * DEPRECATION:	1.2-002
 *
 * These functions have been renamed to match closer the C API.
 */
#if defined(COMPAT_1_1)
	{ "receive",		l_receive			},
	{ "receiveFrom",	l_receiveFrom			},
#endif
	{ "blockMode",		l_blockMode			},
	{ "bind",		l_bind				},
	{ "close",		l_close				},
	{ "connect",		l_connect			},
	{ "accept",		l_accept			},
	{ "listen",		l_listen			},
	{ "send",		l_send				},
	{ "sendto",		l_sendto			},
	{ "recv",		l_recv				},
	{ "recvfrom",		l_recvfrom			},
	{ "set",		l_set				},
	{ nullptr,		nullptr				}
};

const luaL_Reg sockMeta[] = {
	{ "__eq",		l_eq				},
	{ "__tostring",		l_tostring			},
	{ "__gc",		l_gc				},
	{ nullptr,		nullptr				}
};

/* ---------------------------------------------------------
 * Socket address deprecated functions
 * --------------------------------------------------------- */

#if defined(COMPAT_1_1)

int l_unix(lua_State *L)
{
#if defined(WIN32)
	lua_pushnil(L);
	lua_pushstring(L, "Unix address are not supported on Windows");

	return 2;
#else
	Luae::deprecate(L, "unix");

	auto path = luaL_checkstring(L, 1);
	auto rm = false;

	if (lua_gettop(L) >= 2)
		rm = lua_toboolean(L, 2);

	lua_createtable(L, 0, 2);
	lua_pushstring(L, path);
	lua_setfield(L, -2, "path");
	lua_pushboolean(L, rm);
	lua_setfield(L, -2, "remove");

	return 1;
#endif
}

int l_bindInet(lua_State *L)
{
	Luae::deprecate(L, "bindInet");

	luaL_checktype(L, 1, LUA_TTABLE);

	auto port = LuaeTable::require<int>(L, 1, "port");
	auto family = LuaeTable::require<int>(L, 1, "family");
	auto address = std::string("*");

	if (LuaeTable::type(L, 1, "address") == LUA_TSTRING)
		address = LuaeTable::require<std::string>(L, 1, "address");

	lua_createtable(L, 0, 0);

	try {
		auto sa = BindAddressIP(address, port, family);

		storeAddressData(L, -1, sa);
	} catch (SocketError error) {
		// Remove the table created just before
		lua_pop(L, 1);
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 2;
	}

	lua_createtable(L, 0, 3);
	lua_pushstring(L, address.c_str());
	lua_setfield(L, -2, "address");
	lua_pushinteger(L, port);
	lua_setfield(L, -2, "port");
	lua_pushinteger(L, family);
	lua_setfield(L, -2, "family");

	return 1;
}

int l_connectInet(lua_State *L)
{
	Luae::deprecate(L, "connectInet");

	luaL_checktype(L, 1, LUA_TTABLE);

	auto port = LuaeTable::require<int>(L, 1, "port");
	auto family = LuaeTable::require<int>(L, 1, "family");
	auto host = LuaeTable::require<std::string>(L, 1, "host");
	auto type = SOCK_STREAM;

	lua_createtable(L, 0, 0);

	try {
		auto sa = ConnectAddressIP(host, port, family, type);

		storeAddressData(L, -1, sa);
	} catch (SocketError error) {
		// Remove the table created just before
		lua_pop(L, 1);
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 2;
	}

	lua_createtable(L, 0, 3);
	lua_pushstring(L, host.c_str());
	lua_setfield(L, -2, "host");
	lua_pushinteger(L, port);
	lua_setfield(L, -2, "port");
	lua_pushinteger(L, family);
	lua_setfield(L, -2, "family");
	lua_pushinteger(L, type);
	lua_setfield(L, -2, "type");

	return 1;
}

const luaL_Reg addressFunctions[] = {
	{ "unix",		l_unix				},
	{ "bindInet",		l_bindInet			},
	{ "connectInet",	l_connectInet			},
	{ nullptr,		nullptr				}
};

#endif

/* ---------------------------------------------------------
 * Socket listener functions
 * --------------------------------------------------------- */

int l_listenerNew(lua_State *L)
{
	new (L, ListenerType) SocketListener();

	return 1;
}

int l_listenerAdd(lua_State *L)
{
	auto l = Luae::toType<SocketListener *>(L, 1, ListenerType);
	auto s = Luae::toType<Socket *>(L, 2, SocketType);

	l->add(*s);

	return 0;
}

int l_listenerRemove(lua_State *L)
{
	auto l = Luae::toType<SocketListener *>(L, 1, ListenerType);
	auto s = Luae::toType<Socket *>(L, 2, SocketType);

	l->remove(*s);

	return 0;
}

int l_listenerClear(lua_State *L)
{
	Luae::toType<SocketListener *>(L, 1, ListenerType)->clear();

	return 0;
}

int l_listenerSelect(lua_State *L)
{
	auto l = Luae::toType<SocketListener *>(L, 1, ListenerType);
	int seconds = 0, ms = 0, nret;

	if (lua_gettop(L) >= 2)
		seconds = luaL_checkinteger(L, 2);
	if (lua_gettop(L) >= 3)
		ms = luaL_checkinteger(L, 3);

	try {
		auto selected = l->select(seconds, ms);
		new (L, SocketType) Socket(selected);

		nret = 1;
	} catch (SocketError error) {
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		nret = 2;
	} catch (SocketTimeout timeout) {
		lua_pushnil(L);
		lua_pushstring(L, timeout.what());

		nret = 2;
	}

	return nret;
}

int l_listenerToStr(lua_State *L)
{
	auto l = Luae::toType<SocketListener *>(L, 1, ListenerType);

	Luae::pushfstring(L, "listener of %d clients", l->size());

	return 1;
}

int l_listenerGc(lua_State *L)
{
	Luae::toType<SocketListener *>(L, 1, ListenerType)->~SocketListener();

	return 0;
}

const luaL_Reg listenerFunctions[] = {
	{ "new",			l_listenerNew		},
	{ nullptr,			nullptr			}
};

const luaL_Reg listenerMethods[] = {
	{ "add",			l_listenerAdd		},
	{ "remove",			l_listenerRemove	},
	{ "clear",			l_listenerClear		},
	{ "select",			l_listenerSelect	},
	{ nullptr,			nullptr			}
};

const luaL_Reg listenerMeta[] = {
	{ "__tostring",			l_listenerToStr		},
	{ "__gc",			l_listenerGc		},
	{ nullptr,			nullptr			}
};

}

int luaopen_socket(lua_State *L)
{
	// Socket functions
	Luae::newlib(L, sockFunctions);

	// Map families, types
	LuaeEnum::create(L, sockFamilies, -1, "family");
	LuaeEnum::create(L, sockTypes, -1, "type");
	LuaeEnum::create(L, sockProtocols, -1, "protocol");

	// Create a special table for keeping addresses
	LuaeTable::create(L);
	LuaeTable::create(L);
	LuaeTable::set(L, -1, "__mode", "v");
	Luae::setmetatable(L, -2);
	Luae::setfield(L, LUA_REGISTRYINDEX, RegField);

	// Create Socket type
	Luae::newmetatable(L, SocketType);
	Luae::setfuncs(L, sockMeta);
	Luae::newlib(L, sockMethods);
	Luae::setfield(L, -2, "__index");
	Luae::pop(L, 1);

	return 1;
}

#if defined(COMPAT_1_1)

int luaopen_socket_address(lua_State *L)
{
	Luae::newlib(L, addressFunctions);

	return 1;
}

#endif

int luaopen_socket_listener(lua_State *L)
{
	Luae::newlib(L, listenerFunctions);

	// Create the SocketListener type
	Luae::newmetatable(L, ListenerType);
	Luae::setfuncs(L, listenerMeta);
	Luae::newlib(L, listenerMethods);
	Luae::setfield(L, -2, "__index");
	Luae::pop(L, 1);

	return 1;
}

} // !irccd

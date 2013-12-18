/*
 * LuaPipe.cpp -- Lua bindings for class Pipe
 * 
 * Copyright (c) 2013 David Demelier <markand@malikania.fr>
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

#include <string>

#include "LuaPipe.h"
#include "Pipe.h"

namespace irccd {

namespace {

const char *PIPE_TYPE	= "Pipe";

int l_pipeGet(lua_State *L)
{
	std::string name = luaL_checkstring(L, 1);
	Pipe::Ptr pipe = Pipe::get(name);

	new (L, PIPE_TYPE) Pipe::Ptr(pipe);

	return 1;
}

int l_pipePush(lua_State *L)
{
	Pipe::Ptr p = *Luae::toType<Pipe::Ptr *>(L, 1, PIPE_TYPE);
	LuaValue v;

	if (lua_gettop(L) == 1)
		return luaL_error(L, "expected one argument");

	v = LuaValue::copy(L, 2);
	p->push(v);

	return 0;
}

int l_pipeFirst(lua_State *L)
{
	Pipe::Ptr p = *Luae::toType<Pipe::Ptr *>(L, 1, PIPE_TYPE);

	LuaValue::push(L, p->first());

	return 1;
}

int l_pipeLast(lua_State *L)
{
	Pipe::Ptr p = *Luae::toType<Pipe::Ptr *>(L, 1, PIPE_TYPE);

	LuaValue::push(L, p->last());

	return 1;
}

int l_pipeWait(lua_State *L)
{
	Pipe::Ptr p = *Luae::toType<Pipe::Ptr *>(L, 1, PIPE_TYPE);
	int ms = 0;

	if (lua_gettop(L) >= 2)
		ms = luaL_checkinteger(L, 2);

	lua_pushboolean(L, p->wait(ms));

	return 1;
}

int l_pipeList(lua_State *L)
{
	Pipe::Ptr p = *Luae::toType<Pipe::Ptr *>(L, 1, PIPE_TYPE);
	Pipe::Queue q;

	/*
	 * Currently we only provide iterator method but this may
	 * change in the future.
	 */
	p->list([&] (const LuaValue &v) {
		q.push(v);
	});

	// Push as the upvalue
	new (L) Pipe::Queue(q);
	lua_pushcclosure(L, [] (lua_State *L) -> int {
		Pipe::Queue *q = reinterpret_cast<Pipe::Queue *>(lua_touserdata(L, lua_upvalueindex(1)));

		if (q->empty())
		{
			q->~queue<LuaValue>();
			return 0;
		}

		LuaValue::push(L, q->front());
		q->pop();

		return 1;
	}, 1);

	return 1;
}

int l_pipeClear(lua_State *L)
{
	Pipe::Ptr p = *Luae::toType<Pipe::Ptr *>(L, 1, PIPE_TYPE);

	p->clear();

	return 0;
}

int l_pipePop(lua_State *L)
{
	Pipe::Ptr p = *Luae::toType<Pipe::Ptr *>(L, 1, PIPE_TYPE);

	p->pop();

	return 0;
}

int l_pipeGc(lua_State *L)
{
	Pipe::Ptr *pipe = Luae::toType<Pipe::Ptr *>(L, 1, PIPE_TYPE);

	(*pipe).~shared_ptr<Pipe>();

	return 0;
}

const luaL_Reg functions[] = {
	{ "get",		l_pipeGet	},
	{ nullptr,		nullptr		}
};

const luaL_Reg pipeMethods[] = {
	{ "push",		l_pipePush	},
	{ "first",		l_pipeFirst	},
	{ "last",		l_pipeLast	},
	{ "wait",		l_pipeWait	},
	{ "list",		l_pipeList	},
	{ "clear",		l_pipeClear	},
	{ "pop",		l_pipePop	},
	{ nullptr,		nullptr		}
};

const luaL_Reg pipeMeta[] = {
	{ "__gc",		l_pipeGc	},
	{ nullptr,		nullptr		}
};

}

int luaopen_thread_pipe(lua_State *L)
{
	luaL_newlib(L, functions);

	// Create pipe object
	luaL_newmetatable(L, PIPE_TYPE);
	luaL_setfuncs(L, pipeMeta, 0);
	luaL_newlib(L, pipeMethods);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	return 1;
}

} // !irccd

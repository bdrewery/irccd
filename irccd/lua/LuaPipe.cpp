/*
 * LuaPipe.cpp -- Lua bindings for class Pipe
 *
 * Copyright (c) 2013, 2014 David Demelier <markand@malikania.fr>
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

#include "Luae.h"
#include "LuaPipe.h"
#include "Pipe.h"

namespace irccd {

namespace {

const char *PipeType	= "Pipe";

int l_pipeGet(lua_State *L)
{
	auto name = Luae::check<std::string>(L, 1);
	auto pipe = Pipe::get(name);

	new (L, PipeType) Pipe::Ptr(pipe);

	return 1;
}

int l_pipePush(lua_State *L)
{
	auto p = *Luae::toType<Pipe::Ptr *>(L, 1, PipeType);

	if (lua_gettop(L) == 1)
		return luaL_error(L, "expected one argument");

	auto v = LuaeValue::copy(L, 2);
	p->push(v);

	return 0;
}

int l_pipeFirst(lua_State *L)
{
	auto p = *Luae::toType<Pipe::Ptr *>(L, 1, PipeType);

	LuaeValue::push(L, p->first());

	return 1;
}

int l_pipeLast(lua_State *L)
{
	auto p = *Luae::toType<Pipe::Ptr *>(L, 1, PipeType);

	LuaeValue::push(L, p->last());

	return 1;
}

int l_pipeWait(lua_State *L)
{
	auto p = *Luae::toType<Pipe::Ptr *>(L, 1, PipeType);
	int ms = 0;

	if (Luae::gettop(L) >= 2)
		ms = Luae::check<int>(L, 2);

	Luae::push(L, p->wait(ms));

	return 1;
}

int l_pipeList(lua_State *L)
{
	Pipe::Ptr p = *Luae::toType<Pipe::Ptr *>(L, 1, PipeType);
	Pipe::Queue q;

	/*
	 * Currently we only provide iterator method but this may
	 * change in the future.
	 */
	p->list([&] (const LuaeValue &v) {
		q.push(v);
	});

	// Push as the upvalue
	new (L) Pipe::Queue(q);
	Luae::pushfunction(L, [] (lua_State *L) -> int {
		Pipe::Queue *q = reinterpret_cast<Pipe::Queue *>(lua_touserdata(L, lua_upvalueindex(1)));

		if (q->empty()) {
			q->~queue<LuaeValue>();
			return 0;
		}

		LuaeValue::push(L, q->front());
		q->pop();

		return 1;
	}, 1);

	return 1;
}

int l_pipeClear(lua_State *L)
{
	auto p = *Luae::toType<Pipe::Ptr *>(L, 1, PipeType);

	p->clear();

	return 0;
}

int l_pipePop(lua_State *L)
{
	auto p = *Luae::toType<Pipe::Ptr *>(L, 1, PipeType);

	p->pop();

	return 0;
}

int l_pipeGc(lua_State *L)
{
	auto pipe = Luae::toType<Pipe::Ptr *>(L, 1, PipeType);

	// Remove the pipe from the named ones.
	Pipe::destroy(*pipe);
	(*pipe).~shared_ptr<Pipe>();

	return 0;
}

const Luae::Reg functions {
	{ "get",		l_pipeGet	}
};

const Luae::Reg pipeMethods {
	{ "push",		l_pipePush	},
	{ "first",		l_pipeFirst	},
	{ "last",		l_pipeLast	},
	{ "wait",		l_pipeWait	},
	{ "list",		l_pipeList	},
	{ "clear",		l_pipeClear	},
	{ "pop",		l_pipePop	}
};

const luaL_Reg pipeMeta[] = {
	{ "__gc",		l_pipeGc	}
};

}

int luaopen_thread_pipe(lua_State *L)
{
	Luae::newlib(L, functions);

	// Create pipe object
	Luae::newmetatable(L, PipeType);
	Luae::setfuncs(L, pipeMeta);
	Luae::newlib(L, pipeMethods);
	Luae::setfield(L, -2, "__index");
	Luae::pop(L, 1);

	return 1;
}

} // !irccd

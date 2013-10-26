#include <string>
#include <thread>
#include <vector>

#include "Luae.h"
#include "LuaThread.h"

namespace irccd
{

typedef std::vector<char> Buffer;

class LuaThread
{
private:
	LuaState	m_state;
	int		m_ref;

	LuaThread()
	{
	}

public:
	LuaThread(const Buffer &buffer)
	{
		m_state = new LuaState(luaL_newstate());

		auto loader = [] (lua_State *L, void *data, size_t *size) -> const char *
		{
			Buffer *buffer = static_cast<Buffer *>(data);

			
		}
	}
};

namespace
{

int l_new(lua_State *L)
{
	std::thread thread;
	std::string name;
	Buffer buffer;

	// First argument is the name, then the function to call
	name = luaL_checkstring(L, 1);
	luaL_checktype(L, 2, LUA_TFUNCTION);

	/*
	 * Dump the function chunk as binary data and load it to the
	 * new state
	 */
	auto dumper = [] (lua_State *, const void *ptr, size_t sz, void *data) -> int {
		Buffer *array = reinterpret_cast<Buffer *>(data);

		array->reserve(array->capacity() + sz);
		for (size_t i = 0; i < sz; ++i)
		{
			char c = static_cast<const char *>(ptr)[i];
			array->push_back(c);
		}

		return 0;
	};

	lua_dump(L, dumper, &buffer);

	return 0;
}

const luaL_Reg functions[] = {
	{ "new",			l_new		},
	{ nullptr,			nullptr		}
};

}

int luaopen_thread(lua_State *L)
{
	luaL_newlib(L, functions);

	return 1;
}

} // !irccd

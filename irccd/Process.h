#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <unordered_map>

#include "Luae.h"

namespace irccd {

class Process {
public:
	using Ptr	= std::shared_ptr<Process>;
	using Libraries	= std::unordered_map<const char *, lua_CFunction>;

private:
	LuaState	m_state;
	std::string test;

	Process(LuaState &&);

public:
	/*
	 * The following fields are store in the lua_State * registry
	 * and may be retrieved at any time from any Lua API.
	 */
	static const char *	FieldName;
	static const char *	FieldHome;

	/*
	 * The following tables are libraries to load, luaLibs are
	 * required and irccdLibs are preloaded.
	 */
	static const Libraries luaLibs;
	static const Libraries irccdLibs;

	static std::string getName(lua_State *L) noexcept;
	static std::string getHome(lua_State *L) noexcept;
	static Ptr create(LuaState &&state);

	Process() = default;

	static void initialize(lua_State *L,
			       const std::string &name,
			       const std::string &home);

	operator lua_State *();
};

} // !irccd

#endif // !_PROCESS_H_

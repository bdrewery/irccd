// lua.hpp
// Lua header files for C++
// <<extern "C">> not supplied automatically because Lua also compiles as C++

/*
 * Changed for irccd:
 *
 * We compile Lua as C++ library which enable name mangling on Lua symbols so
 * we disable these extern "C".
 */

#if 0
extern "C" {
#endif

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#if 0
}
#endif

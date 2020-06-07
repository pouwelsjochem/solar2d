//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////


#ifndef _Rtt_LuaFrameworks_H__
#define _Rtt_LuaFrameworks_H__

// ----------------------------------------------------------------------------

struct lua_State;

namespace Rtt
{

// ----------------------------------------------------------------------------

// Frameworks that are implemented as pure Lua files are pre-compiled into bytecodes
// and then placed in a C byte array constant. The following functions load those
// bytecodes via luaL_loadbuffer. The corresponding .cpp file is dynamically generated.

int luaload_easing(lua_State* L);
int luaload_launchpad(lua_State* L);

int luaload_dkjson(lua_State* L);
int luaload_json(lua_State* L);

int luaload_re(lua_State* L);
extern "C" { int luaopen_lpeg (lua_State *L); }

#ifdef Rtt_DEBUGGER
int luaload_remdebug_engine(lua_State *L);
#endif

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_Frameworks_H__

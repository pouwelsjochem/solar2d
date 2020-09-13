//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_LuaLibNative_H__
#define _Rtt_LuaLibNative_H__

// ----------------------------------------------------------------------------

#include "Rtt_Lua.h"

namespace Rtt
{

class LuaResource;
class MPlatform;

// ----------------------------------------------------------------------------

class LuaLibNative
{
	public:
		typedef LuaLibNative Self;

	public:
		static void Initialize( lua_State *L );

	public:
		// buttonIndex is 0-based
		static void AlertComplete( LuaResource& resource, S32 buttonIndex, bool cancelled );
		static void PopupClosed( LuaResource& resource, const char *popupName, bool wasCanceled );

		enum
		{
			RecordStatusNone = 0,
			RecordStatusError = 1
		};
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_LuaLibNative_H__

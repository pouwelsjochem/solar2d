//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Solar2D game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@Solar2D.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Rtt_IPhoneText.h"

#include "Display/Rtt_LuaLibDisplay.h"
#include "Rtt_Lua.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

UIColor*
IPhoneText::GetTextColor( lua_State *L, int index )
{
	if ( L && ( LUA_TNUMBER == lua_type( L, index ) ) )
	{
		Color c = LuaLibDisplay::toColor( L, index );
		RGBA rgba = ( (ColorUnion*)(& c) )->rgba;
		return [UIColor colorWithRed:rgba.r green:rgba.g blue:rgba.b alpha:rgba.a];
	}

	return [UIColor blackColor];
}

UIReturnKeyType
IPhoneText::GetUIReturnKeyTypeFromIMEType( const char *imeType )
{
	if ( ! imeType )
	{
		return UIReturnKeyDefault;
	}

	if ( 0 == strcmp( imeType, "done" ) )
	{
		return UIReturnKeyDone;
	}
	else if ( 0 == strcmp( imeType, "go" ) )
	{
		return UIReturnKeyGo;
	}
	else if ( 0 == strcmp( imeType, "next" ) )
	{
		return UIReturnKeyNext;
	}
	else if ( 0 == strcmp( imeType, "search" ) )
	{
		return UIReturnKeySearch;
	}
	else if ( 0 == strcmp( imeType, "send" ) )
	{
		return UIReturnKeySend;
	}
	else if ( 0 == strcmp( imeType, "join" ) )
	{
		return UIReturnKeyJoin;
	}
	else if ( 0 == strcmp( imeType, "route" ) )
	{
		return UIReturnKeyRoute;
	}
	else if ( 0 == strcmp( imeType, "continue" ) )
	{
		return UIReturnKeyContinue;
	}
	else if ( 0 == strcmp( imeType, "emergencycall" ) )
	{
		return UIReturnKeyEmergencyCall;
	}
	else if ( 0 == strcmp( imeType, "google" ) )
	{
		return UIReturnKeyGoogle;
	}
	else if ( 0 == strcmp( imeType, "yahoo" ) )
	{
		return UIReturnKeyYahoo;
	}

	return UIReturnKeyDefault;
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_LuaLibNative.h"

#include "Display/Rtt_Display.h"
#include "Display/Rtt_DisplayDefaults.h"
#include "Display/Rtt_LuaLibDisplay.h"
#include "Rtt_Event.h"
#include "Rtt_LuaContext.h"
#include "Rtt_LuaProxyVTable.h"
#include "Rtt_LuaProxy.h"
#include "Rtt_LuaResource.h"
#include "Rtt_MCallback.h"
#include "Rtt_MPlatform.h"
#include "Rtt_PlatformDisplayObject.h"
#include "Rtt_Runtime.h"

//#include <string.h>

#include "CoronaLua.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

static const char kNativeAlert[] = "native.Alert";

// ----------------------------------------------------------------------------

// native.showAlert( title, message [, { buttonLabels } [, listener] ] )
// returns an alertId
static int
showAlert( lua_State *L )
{
	const char *title = lua_tostring( L, 1 );
	const char *msg = lua_tostring( L, 2 );
	if ( title && msg )
	{
		const size_t kMaxNumButtons = 5;
		const char *buttons[kMaxNumButtons];
		int numButtons = 0;

		int callbackIndex = 0;

		if ( lua_istable( L, 3 ) )
		{
			const int t = 3; // table index
			int i = 0;
			for ( bool isValidArg = true; isValidArg && i < (int)kMaxNumButtons; i++ )
			{
				lua_rawgeti( L, t, i+1 );
				isValidArg = lua_isstring( L, -1 );
				if ( isValidArg )
				{
					buttons[i] = lua_tostring( L, -1 );
					++numButtons;
				}
				lua_pop( L, 1 );
			}

//			Rtt_WARN_SIM(
//				( "WARNING: native.showAlert() is limited to %d or fewer buttons. The other buttons are ignored.\n", kMaxNumButtons ) );
			
			if ( Lua::IsListener( L, 4, CompletionEvent::kName ) )
			{
				callbackIndex = 4;
			}
            else if (lua_type(L, 4) != LUA_TNONE)
            {
                CoronaLuaError(L, "native.showAlert() expects a listener as argument #4 (got %s)", lua_typename( L, lua_type(L, 4)));
            }
		}

		const MPlatform& platform = LuaContext::GetPlatform( L );
		LuaResource *resource = NULL;
		if ( callbackIndex > 0 )
		{
			resource = Rtt_NEW( & platform.GetAllocator(), LuaResource( LuaContext::GetContext( L )->LuaState(), callbackIndex ) );
		}

		NativeAlertRef *p = (NativeAlertRef*)lua_newuserdata( L, sizeof( NativeAlertRef ) );
		*p = platform.ShowNativeAlert( title, msg, buttons, numButtons, resource );

		luaL_getmetatable( L, kNativeAlert );
		lua_setmetatable( L, -2 );
	}
	else
	{
        CoronaLuaError(L, "native.showAlert() called with unexpected parameters");

		lua_pushnil( L );
	}

	return 1;
}

// native.cancelAlert( alertId )
static int
cancelAlert( lua_State *L )
{
	NativeAlertRef *p = (NativeAlertRef*)luaL_checkudata( L, 1, kNativeAlert );
	if ( p )
	{
		NativeAlertRef alert = *p;
		if ( alert )
		{
			* p = NULL; // NULL-ify Lua reference to callback

			const MPlatform& platform = LuaContext::GetPlatform( L );

			int index = (int) (lua_isnumber( L, 2 ) ? lua_tointeger( L, 2 ) : 0);
			platform.CancelNativeAlert( alert, index );
		}
	}

	return 0;
}

// native.newWebView( left, top, width, height [,listener] )
static int
newWebView( lua_State *L )
{
	int luaArgumentCount = lua_gettop( L );
	int result = 0;

	Runtime& runtime = * LuaContext::GetRuntime( L );
	const MPlatform& platform = runtime.Platform();

	Real x = luaL_toreal( L, 1 );
	Real y = luaL_toreal( L, 2 );
	Real w = luaL_toreal( L, 3 );
	Real h = luaL_toreal( L, 4 );

	if ( w > 0 && h > 0 )
	{
		Rect bounds;
		Display& display = runtime.GetDisplay();
		bounds.Initialize( x, y, w, h );

		PlatformDisplayObject *t = platform.CreateNativeWebView( bounds );

		if ( t )
		{
			t->Preinitialize( display );
			t->SetHandle( & platform.GetAllocator(), runtime.VMContext().LuaState() );

			result = LuaLibDisplay::AssignParentAndPushResult( L, display, t, NULL );

			if ( ( luaArgumentCount >= 5 ) && Lua::IsListener( L, 5, UrlRequestEvent::kName ) )
			{
				CoronaLuaWarning( L, "The 'listener' argument to native.newWebView( left, top, width, height [, listener] ) is deprecated. Call the object method o:addEventListener( '%s', listener ) instead",
					UrlRequestEvent::kName );
				t->AddEventListener( L, 5, UrlRequestEvent::kName );
			}

			t->Initialize();
		}
	}

	return result;
}

// native.requestExit()
static int
requestExitApplication( lua_State *L )
{
	const MPlatform& platform = LuaContext::GetPlatform( L );
	platform.RequestSystem( L, "exitApplication", 0 );
	return 0;
}

static int
setProperty( lua_State *L )
{
	const MPlatform& platform = LuaContext::GetPlatform( L );

	const char *key = lua_tostring( L, 1 );
	if ( key )
	{
		luaL_argcheck( L, ! lua_isnone( L, 2 ), 2, "no value provided" );
		platform.SetNativeProperty( L, key, 2 );
	}
	else
	{
		luaL_argerror( L, 1, "no string key provided" );
	}

	return 0;
}

static int
getProperty( lua_State *L )
{
	int result = 0;

	const MPlatform& platform = LuaContext::GetPlatform( L );

	const char *key = lua_tostring( L, 1 );
	if ( key )
	{
		result = platform.PushNativeProperty( L, key );
	}
	else
	{
		luaL_argerror( L, 1, "no string key provided" );
	}

	return result;
}

// native.canShowPopup( name )
static int
canShowPopup( lua_State *L )
{
	bool result = false;

	const char *name = lua_tostring( L, 1 );
	if ( name )
	{
		const MPlatform& platform = LuaContext::GetPlatform( L );
		result = platform.CanShowPopup( name );
	}

	lua_pushboolean( L, result );

	return 1;
}

// native.showPopup( name [, options] )
// 
// iOS examples:
// -------------
// 	local mailOptions = {
//		to={ "a@z.com", "b@z.com" }, -- optional (string or, for multiple, array of strings)
//		cc={}, -- optional (string or, for multiple, array of strings)
//		bcc={}, -- optional (string or, for multiple, array of strings)
//		attachment = { baseDir=, filename= [, type=] }, -- optional (single element or, for multiple, array of them)
//		body="some text in the body", -- optional (string)
//		isBodyHtml= false, -- optional (boolean)
//		subject="", -- optional (string)
//		listener=function(event) end, -- optional (function) [TODO] We punt for now. And we may never offer this...
// 	}
// 	native.showPopup( "mail", mailOptions )
// 
static int
showPopup( lua_State *L )
{
	bool result = false;

	const MPlatform& platform = LuaContext::GetPlatform( L );

	const char *name = NULL;
	if ( LUA_TSTRING == lua_type( L, 1 ) )
	{
		name = lua_tostring( L, 1 );
	}

	int optionsIndex = 2;
	if ( lua_isnoneornil( L, optionsIndex ) )
	{
		optionsIndex = 0;
	}
	else if ( LUA_TTABLE != lua_type( L, optionsIndex ) )
	{
		optionsIndex = -1;
	}

	if ( ! name )
	{
		CoronaLuaError( L, "native.showPopup() bad argument #1 (expected string name of the popup but got %s)", lua_typename( L, lua_type( L, 1 ) ) );
	}
	else if ( optionsIndex < 0 )
	{
		CoronaLuaError( L, "native.showPopup() bad argument #2 (expected table but got %s)", lua_typename( L, lua_type( L, 2 ) ) );
	}
	else
	{
		result = platform.ShowPopup( L, name, optionsIndex );
		if ( ! result )
		{
			CoronaLuaError( L, "native.showPopup() does not support %s popups on this device", name );
		}
	}

	lua_pushboolean( L, result );
	return 1;
}

// native.hidePopup( name )
// 
// iOS examples:
// -------------
// native.hidePopup( "mail" )
static int
hidePopup( lua_State *L )
{
	const MPlatform& platform = LuaContext::GetPlatform( L );

	const char *name = NULL;
	if ( LUA_TSTRING == lua_type( L, 1 ) )
	{
		name = lua_tostring( L, 1 );
	}

	if ( ! name )
	{
		CoronaLuaError( L, "native.hidePopup() bad argument #1 (expected string name of the popup but got %s)", lua_typename( L, lua_type( L, 1 ) ) );
	}
	else if ( ! platform.HidePopup( name ) )
	{
		CoronaLuaError( L, "native.hidePopup() does not support %s popups on this device", name );
	}

	return 0;
}

static int
setSync( lua_State* L )
{
	const MPlatform& platform = LuaContext::GetPlatform( L );
	return platform.SetSync( L );
}

static int
getSync( lua_State* L )
{
	const MPlatform& platform = LuaContext::GetPlatform( L );
	return platform.GetSync( L );
}

void
LuaLibNative::Initialize( lua_State *L )
{
	const luaL_Reg kVTable[] =
	{
		{ "showAlert", showAlert },
		{ "cancelAlert", cancelAlert },
		{ "newWebView", newWebView },
		{ "requestExit", requestExitApplication },
		{ "setProperty", setProperty },
		{ "getProperty", getProperty },
		{ "canShowPopup", canShowPopup },
		{ "showPopup", showPopup },
		{ "hidePopup", hidePopup },
		{ "setSync", setSync },
		{ "getSync", getSync },

		{ NULL, NULL }
	};

	luaL_register( L, "native", kVTable );
	lua_pop( L, 1 ); // pop "native" table

	// Create kNativeAlert metatable
	luaL_newmetatable( L, kNativeAlert );
	lua_pop( L, 1 ); // remove mt from stack
}

void
LuaLibNative::AlertComplete( LuaResource& resource, S32 buttonIndex, bool cancelled )
{
	CompletionEvent e;
	int nargs = resource.PushListenerAndEvent( e );
	if ( nargs > 0 )
	{
		lua_State *L = resource.L(); Rtt_ASSERT( L );

		RuntimeGuard guard( * LuaContext::GetRuntime( L ) );

		lua_pushinteger( L, buttonIndex + 1 ); // Lua indices are 1-based
		lua_setfield( L, -2, "index" );
		lua_pushstring( L, cancelled ? "cancelled" : "clicked" );
		lua_setfield( L, -2, "action" );
		LuaContext::DoCall( L, nargs, 0 );
	}
/*
	int nargs = resource.PushListener( CompletionEvent::kName );
	if ( nargs > 0 )
	{
		// PushListener returns number of arguments it pushed on the stack
		// including the function itself. We should subtract one, but since 
		// we're pushing one more arg (below), the net effect is zero.
		lua_State *L = resource.LuaState().Dereference();
		if ( L )
		{
			CompletionEvent e;
			e.Push( L );
			lua_pushinteger( L, buttonIndex + 1 ); // Lua indices are 1-based
			lua_setfield( L, -2, "index" );
			lua_pushstring( L, cancelled ? "cancelled" : "clicked" );
			lua_setfield( L, -2, "action" );
			LuaContext::DoCall( L, nargs, 0 );
		}
	}
*/
/*
	resource.Push();
	lua_State *L = resource.L();

	if ( ! lua_isnil( L, -1 ) )
	{
		int nargs = 1;
		if ( lua_istable( L, -1 ) )
		{
			++nargs; // t is first arg to t.onComplete method
			lua_pushvalue( L, -1 );
			lua_getfield( L, -1, "onComplete" ); // t.onComplete
			lua_insert( L, -2 ); // move t.onComplete before t
		}

		lua_newtable( L );
		lua_pushinteger( L, buttonIndex + 1 ); // Lua indices are 1-based
		lua_setfield( L, -2, "index" );
		lua_pushstring( L, cancelled ? "cancelled" : "clicked" );
		lua_setfield( L, -2, "action" );
		LuaContext::DoCall( L, nargs, 0 );
	}
*/
}

void
LuaLibNative::PopupClosed( LuaResource& resource, const char *popupName, bool wasCanceled )
{
	PopupClosedEvent e(popupName, wasCanceled);
	int nargs = resource.PushListenerAndEvent( e );
	if ( nargs > 0 )
	{
		lua_State *L = resource.L(); Rtt_ASSERT( L );
		RuntimeGuard guard( * LuaContext::GetRuntime( L ) );
		LuaContext::DoCall( L, nargs, 0 );
	}
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------


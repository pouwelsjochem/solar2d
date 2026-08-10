//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_LuaLibSimulator.h"

#ifdef Rtt_AUTHORING_SIMULATOR

#include "Rtt_LuaContext.h"
#include "Rtt_MPlatform.h"
#include "Rtt_MSimulatorHost.h"

#include <float.h>
#include <math.h>
#include <string.h>
#include <string>
#include <vector>

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

static const MSimulatorHost*
GetSimulatorHost( lua_State *L )
{
	return LuaContext::GetPlatform( L ).GetSimulatorHost();
}

static int
AbsoluteLuaIndex( lua_State *L, int index )
{
	return index > 0 || index <= LUA_REGISTRYINDEX ? index : lua_gettop( L ) + index + 1;
}

static bool
IsAllowedOption( const char *key, const char * const allowedOptions[] )
{
	for ( int index = 0; allowedOptions[index]; index++ )
	{
		if ( 0 == strcmp( key, allowedOptions[index] ) )
		{
			return true;
		}
	}
	return false;
}

static void
ValidateOptionKeys( lua_State *L, int tableIndex, const char * const allowedOptions[], const char *context )
{
	if ( ! lua_istable( L, tableIndex ) )
	{
		luaL_error( L, "%s must be a table", context );
		return;
	}

	tableIndex = AbsoluteLuaIndex( L, tableIndex );
	lua_pushnil( L );
	while ( lua_next( L, tableIndex ) )
	{
		const char *key = lua_type( L, -2 ) == LUA_TSTRING ? lua_tostring( L, -2 ) : NULL;
		if ( ! key || ! IsAllowedOption( key, allowedOptions ) )
		{
			const char *keyDescription = key;
			if ( ! keyDescription )
			{
				if ( lua_type( L, -2 ) == LUA_TBOOLEAN )
				{
					keyDescription = lua_toboolean( L, -2 ) ? "true" : "false";
				}
				else
				{
					keyDescription = luaL_typename( L, -2 );
				}
			}
			luaL_error( L, "unknown %s option '%s'", context, keyDescription );
			return;
		}
		lua_pop( L, 1 );
	}
}

static bool
IsFiniteNumber( lua_Number value )
{
	return value == value && value <= DBL_MAX && value >= -DBL_MAX;
}

static bool
IsInteger( lua_Number value )
{
	return IsFiniteNumber( value ) && floor( value ) == value;
}

static double
ReadFiniteNumber(
	lua_State *L, int tableIndex, const char *key, const char *context, bool isRequired, double defaultValue )
{
	lua_getfield( L, tableIndex, key );
	if ( lua_isnil( L, -1 ) && ! isRequired )
	{
		lua_pop( L, 1 );
		return defaultValue;
	}

	if ( lua_type( L, -1 ) != LUA_TNUMBER || ! IsFiniteNumber( lua_tonumber( L, -1 ) ) )
	{
		luaL_error( L, "%s must be a finite number", context );
		return defaultValue;
	}

	double result = lua_tonumber( L, -1 );
	lua_pop( L, 1 );
	return result;
}

static bool
IsNilField( lua_State *L, int tableIndex, const char *key )
{
	lua_getfield( L, tableIndex, key );
	bool result = lua_isnil( L, -1 );
	lua_pop( L, 1 );
	return result;
}

static std::string
ReadRequiredString( lua_State *L, int tableIndex, const char *key, const char *errorMessage )
{
	lua_getfield( L, tableIndex, key );
	if ( lua_type( L, -1 ) != LUA_TSTRING )
	{
		luaL_error( L, "%s", errorMessage );
		return std::string();
	}

	size_t length = 0;
	const char *value = lua_tolstring( L, -1, &length );
	std::string result( value, length );
	lua_pop( L, 1 );
	return result;
}

static void
ReadInputBoolean( lua_State *L, int tableIndex, const char *key, bool& result )
{
	lua_getfield( L, tableIndex, key );
	if ( ! lua_isnil( L, -1 ) )
	{
		if ( lua_type( L, -1 ) != LUA_TBOOLEAN )
		{
			luaL_error( L, "simulator input %s must be a boolean", key );
			return;
		}
		result = 0 != lua_toboolean( L, -1 );
	}
	lua_pop( L, 1 );
}

static bool
ReadOptionalBoolean(
	lua_State *L, int tableIndex, const char *key, const char *context, bool defaultValue )
{
	lua_getfield( L, tableIndex, key );
	if ( lua_isnil( L, -1 ) )
	{
		lua_pop( L, 1 );
		return defaultValue;
	}
	if ( lua_type( L, -1 ) != LUA_TBOOLEAN )
	{
		luaL_error( L, "%s must be a boolean", context );
		return defaultValue;
	}
	bool result = 0 != lua_toboolean( L, -1 );
	lua_pop( L, 1 );
	return result;
}

static std::string
LuaValueDescription( lua_State *L, int index )
{
	switch ( lua_type( L, index ) )
	{
		case LUA_TNIL:
			return "nil";
		case LUA_TBOOLEAN:
			return lua_toboolean( L, index ) ? "true" : "false";
		case LUA_TSTRING:
		case LUA_TNUMBER:
		{
			size_t length = 0;
			const char *value = lua_tolstring( L, index, &length );
			return std::string( value, length );
		}
		default:
			return luaL_typename( L, index );
	}
}

static void
PushSimulatorSafeAreaInsets( lua_State *L, const MSimulatorHost::SafeAreaInsets& insets )
{
	lua_createtable( L, 0, 4 );
	lua_pushinteger( L, insets.top );
	lua_setfield( L, -2, "top" );
	lua_pushinteger( L, insets.left );
	lua_setfield( L, -2, "left" );
	lua_pushinteger( L, insets.bottom );
	lua_setfield( L, -2, "bottom" );
	lua_pushinteger( L, insets.right );
	lua_setfield( L, -2, "right" );
}

static void
PushSimulatorDevice( lua_State *L, const MSimulatorHost::Device& device, bool includeSelection )
{
	lua_createtable( L, 0, includeSelection ? 10 : 9 );
	lua_pushlstring( L, device.id.data(), device.id.length() );
	lua_setfield( L, -2, "id" );
	lua_pushlstring( L, device.name.data(), device.name.length() );
	lua_setfield( L, -2, "name" );
	lua_pushlstring( L, device.category.data(), device.category.length() );
	lua_setfield( L, -2, "category" );
	lua_pushinteger( L, device.width );
	lua_setfield( L, -2, "width" );
	lua_pushinteger( L, device.height );
	lua_setfield( L, -2, "height" );
	lua_pushboolean( L, device.isCustom );
	lua_setfield( L, -2, "isCustom" );
	lua_pushboolean( L, device.isProject );
	lua_setfield( L, -2, "isProject" );
	if ( device.hasRoundedCorners )
	{
		lua_pushboolean( L, device.roundedCorners );
		lua_setfield( L, -2, "roundedCorners" );
	}
	if ( includeSelection )
	{
		lua_pushboolean( L, device.isCurrent );
		lua_setfield( L, -2, "isCurrent" );
	}
	PushSimulatorSafeAreaInsets( L, device.safeAreaInsets );
	lua_setfield( L, -2, "safeAreaInsets" );
}

static int
GetCurrentSimulatorDevice( lua_State *L )
{
	const MSimulatorHost *host = GetSimulatorHost( L );
	MSimulatorHost::Device device;
	if ( ! host || ! host->GetCurrentDevice( device ) )
	{
		lua_pushnil( L );
		return 1;
	}

	PushSimulatorDevice( L, device, false );
	return 1;
}

static int
GetSimulatorState( lua_State *L )
{
	const MSimulatorHost *host = GetSimulatorHost( L );
	MSimulatorHost::State state;
	if ( ! host || ! host->GetState( state ) )
	{
		lua_pushnil( L );
		return 1;
	}

	lua_createtable( L, 0, 6 );
	PushSimulatorDevice( L, state.device, false );
	lua_setfield( L, -2, "device" );
	lua_pushboolean( L, state.isSuspended );
	lua_setfield( L, -2, "isSuspended" );
	lua_pushboolean( L, state.safeAreaGuidesVisible );
	lua_setfield( L, -2, "safeAreaGuidesVisible" );
	lua_pushboolean( L, state.isRelaunchPending );
	lua_setfield( L, -2, "isRelaunchPending" );
	lua_pushinteger( L, state.relaunchCount );
	lua_setfield( L, -2, "relaunchCount" );

	lua_createtable( L, 0, 6 );
	lua_pushnumber( L, state.window.x );
	lua_setfield( L, -2, "x" );
	lua_pushnumber( L, state.window.y );
	lua_setfield( L, -2, "y" );
	lua_pushnumber( L, state.window.width );
	lua_setfield( L, -2, "width" );
	lua_pushnumber( L, state.window.height );
	lua_setfield( L, -2, "height" );
	lua_pushnumber( L, state.window.backingScale );
	lua_setfield( L, -2, "backingScale" );
	lua_pushboolean( L, state.window.isFullscreen );
	lua_setfield( L, -2, "isFullscreen" );
	lua_setfield( L, -2, "window" );
	return 1;
}

static int
GetSimulatorDevices( lua_State *L )
{
	const MSimulatorHost *host = GetSimulatorHost( L );
	std::vector< MSimulatorHost::Device > devices;
	if ( ! host || ! host->GetDevices( devices ) )
	{
		lua_pushnil( L );
		return 1;
	}

	lua_createtable( L, (int)devices.size(), 0 );
	for ( size_t index = 0; index < devices.size(); index++ )
	{
		PushSimulatorDevice( L, devices[index], true );
		lua_rawseti( L, -2, (int)index + 1 );
	}
	return 1;
}

static void
ReadSafeAreaInsets( lua_State *L, int deviceIndex, int width, int height, MSimulatorHost::SafeAreaInsets& result )
{
	lua_getfield( L, deviceIndex, "safeAreaInsets" );
	if ( lua_isnil( L, -1 ) )
	{
		lua_pop( L, 1 );
		return;
	}

	const char * const allowedOptions[] = { "top", "left", "bottom", "right", NULL };
	ValidateOptionKeys( L, -1, allowedOptions, "custom simulator device safeAreaInsets" );

	const char * const keys[] = { "top", "left", "bottom", "right" };
	int *values[] = { &result.top, &result.left, &result.bottom, &result.right };
	for ( int index = 0; index < 4; index++ )
	{
		lua_getfield( L, -1, keys[index] );
		if ( lua_isnil( L, -1 ) )
		{
			*values[index] = 0;
		}
		else
		{
			lua_Number value = lua_type( L, -1 ) == LUA_TNUMBER ? lua_tonumber( L, -1 ) : -1.0;
			if ( ! IsInteger( value ) || value < 0 || value > 16384 )
			{
				luaL_error(
					L,
					"custom simulator device safeAreaInsets.%s "
					"must be an integer between 0 and 16384",
					keys[index] );
				return;
			}
			*values[index] = (int)value;
		}
		lua_pop( L, 1 );
	}

	if ( result.top + result.bottom > height || result.left + result.right > width )
	{
		luaL_error( L, "custom simulator device safeAreaInsets cannot exceed its dimensions" );
		return;
	}
	lua_pop( L, 1 );
}

static int
ConfigureSimulator( lua_State *L, bool onlyIfNeeded )
{
	const char * const allowedOptions[] = { "device", "roundedCorners", "temporary", NULL };
	ValidateOptionKeys( L, 1, allowedOptions, "simulator configuration" );

	MSimulatorHost::Configuration configuration;
	lua_getfield( L, 1, "device" );
	if ( lua_type( L, -1 ) == LUA_TSTRING )
	{
		size_t length = 0;
		const char *identifier = lua_tolstring( L, -1, &length );
		if ( 0 == length )
		{
			luaL_error( L, "simulator configuration expects a non-empty device identifier" );
			return 0;
		}
		configuration.deviceSelection = MSimulatorHost::Configuration::kNamedDevice;
		configuration.deviceId.assign( identifier, length );
	}
	else if ( lua_istable( L, -1 ) )
	{
		int deviceIndex = AbsoluteLuaIndex( L, -1 );
		const char * const deviceOptions[] = { "width", "height", "safeAreaInsets", NULL };
		ValidateOptionKeys( L, deviceIndex, deviceOptions, "custom simulator device" );

		lua_getfield( L, deviceIndex, "width" );
		lua_Number width = lua_type( L, -1 ) == LUA_TNUMBER ? lua_tonumber( L, -1 ) : 0.0;
		if ( ! IsInteger( width ) || width <= 0 || width > 16384 )
		{
			luaL_error( L, "custom simulator device expects an integer width between 1 and 16384" );
			return 0;
		}
		lua_pop( L, 1 );

		lua_getfield( L, deviceIndex, "height" );
		lua_Number height = lua_type( L, -1 ) == LUA_TNUMBER ? lua_tonumber( L, -1 ) : 0.0;
		if ( ! IsInteger( height ) || height <= 0 || height > 16384 )
		{
			luaL_error( L, "custom simulator device expects an integer height between 1 and 16384" );
			return 0;
		}
		lua_pop( L, 1 );

		configuration.deviceSelection = MSimulatorHost::Configuration::kCustomDevice;
		configuration.width = (int)width;
		configuration.height = (int)height;
		ReadSafeAreaInsets( L, deviceIndex, configuration.width, configuration.height, configuration.safeAreaInsets );
	}
	else if ( ! lua_isnil( L, -1 ) )
	{
		luaL_error( L, "simulator configuration device must be a string or table" );
		return 0;
	}
	lua_pop( L, 1 );

	lua_getfield( L, 1, "roundedCorners" );
	if ( ! lua_isnil( L, -1 ) )
	{
		if ( lua_type( L, -1 ) != LUA_TBOOLEAN )
		{
			luaL_error( L, "simulator configuration roundedCorners must be a boolean" );
			return 0;
		}
		configuration.hasRoundedCorners = true;
		configuration.roundedCorners = 0 != lua_toboolean( L, -1 );
	}
	lua_pop( L, 1 );

	lua_getfield( L, 1, "temporary" );
	if ( ! lua_isnil( L, -1 ) )
	{
		if ( lua_type( L, -1 ) != LUA_TBOOLEAN )
		{
			luaL_error( L, "simulator configuration temporary must be a boolean" );
			return 0;
		}
		configuration.temporary = 0 != lua_toboolean( L, -1 );
	}
	lua_pop( L, 1 );

	if ( MSimulatorHost::Configuration::kKeepCurrentDevice == configuration.deviceSelection &&
		! configuration.hasRoundedCorners )
	{
		luaL_error( L, "simulator configuration must specify device or roundedCorners" );
		return 0;
	}

	const MSimulatorHost *host = GetSimulatorHost( L );
	MSimulatorHost::ConfigureResult result =
		host ? host->ConfigureAndRelaunch( configuration, onlyIfNeeded ) : MSimulatorHost::kConfigureFailed;
	if ( MSimulatorHost::kConfigureFailed == result ||
		( ! onlyIfNeeded && MSimulatorHost::kConfigureAlreadyActive == result ) )
	{
		return luaL_error( L, "the Simulator could not apply the requested configuration" );
	}

	lua_pushboolean( L, MSimulatorHost::kConfigureApplied == result );
	return 1;
}

static int
ConfigureAndRelaunchSimulator( lua_State *L )
{
	return ConfigureSimulator( L, false );
}

static int
ConfigureAndRelaunchSimulatorIfNeeded( lua_State *L )
{
	return ConfigureSimulator( L, true );
}

static int
RelaunchSimulator( lua_State *L )
{
	const MSimulatorHost *host = GetSimulatorHost( L );
	if ( ! host || ! host->Relaunch() )
	{
		return luaL_error( L, "the Simulator could not relaunch the project" );
	}
	lua_pushboolean( L, 1 );
	return 1;
}

typedef bool (MSimulatorHost::*SimulatorBooleanSetter)( bool ) const;

static int
SetSimulatorBoolean( lua_State *L, const char *functionName, SimulatorBooleanSetter setter )
{
	if ( lua_type( L, 1 ) != LUA_TBOOLEAN )
	{
		return luaL_error( L, "simulator.%s() expects a boolean", functionName );
	}

	const MSimulatorHost *host = GetSimulatorHost( L );
	if ( ! host || ! ( host->*setter )( 0 != lua_toboolean( L, 1 ) ) )
	{
		return luaL_error( L, "the Simulator could not execute %s()", functionName );
	}
	lua_pushboolean( L, 1 );
	return 1;
}

static int
SetSimulatorSafeAreaGuidesVisible( lua_State *L )
{
	return SetSimulatorBoolean(
		L, "setSafeAreaGuidesVisible", &MSimulatorHost::SetSafeAreaGuidesVisible );
}

static int
SetSimulatorFullscreen( lua_State *L )
{
	return SetSimulatorBoolean( L, "setFullscreen", &MSimulatorHost::SetFullscreen );
}

static void
ReadSimulatorInputBooleans( lua_State *L, int tableIndex, MSimulatorHost::Input& input )
{
	ReadInputBoolean( L, tableIndex, "isShiftDown", input.isShiftDown );
	ReadInputBoolean( L, tableIndex, "isAltDown", input.isAltDown );
	ReadInputBoolean( L, tableIndex, "isCtrlDown", input.isCtrlDown );
	ReadInputBoolean( L, tableIndex, "isCommandDown", input.isCommandDown );
	ReadInputBoolean( L, tableIndex, "isPrimaryButtonDown", input.isPrimaryButtonDown );
	ReadInputBoolean( L, tableIndex, "isSecondaryButtonDown", input.isSecondaryButtonDown );
	ReadInputBoolean( L, tableIndex, "isMiddleButtonDown", input.isMiddleButtonDown );
}

static int
SendSimulatorInput( lua_State *L )
{
	const char * const allowedOptions[] =
	{
		"type", "phase", "keyName", "qwertyKeyName", "nativeKeyCode", "text",
		"x", "y", "xStart", "yStart", "scrollX", "scrollY", "clickCount",
		"isShiftDown", "isAltDown", "isCtrlDown", "isCommandDown",
		"isPrimaryButtonDown", "isSecondaryButtonDown", "isMiddleButtonDown",
		NULL
	};
	ValidateOptionKeys( L, 1, allowedOptions, "simulator input" );

	std::string inputType = ReadRequiredString(
		L, 1, "type", "simulator input type must be 'back', 'key', 'text', 'touch', 'tap', or 'mouse'" );
	MSimulatorHost::Input input;
	bool isTap = false;

	if ( "back" == inputType )
	{
		const char * const options[] = { "type", NULL };
		ValidateOptionKeys( L, 1, options, "back simulator input" );
		input.type = MSimulatorHost::Input::kBackInput;
	}
	else if ( "text" == inputType )
	{
		const char * const options[] = { "type", "text", NULL };
		ValidateOptionKeys( L, 1, options, "text simulator input" );
		input.type = MSimulatorHost::Input::kTextInput;
		input.text = ReadRequiredString( L, 1, "text", "text simulator input expects non-empty text" );
		if ( input.text.empty() )
		{
			return luaL_error( L, "text simulator input expects non-empty text" );
		}
	}
	else if ( "key" == inputType )
	{
		const char * const options[] =
		{
			"type", "phase", "keyName", "qwertyKeyName", "nativeKeyCode",
			"isShiftDown", "isAltDown", "isCtrlDown", "isCommandDown", NULL
		};
		ValidateOptionKeys( L, 1, options, "key simulator input" );
		input.type = MSimulatorHost::Input::kKeyInput;
		input.keyName = ReadRequiredString( L, 1, "keyName", "key simulator input expects a non-empty keyName" );
		if ( input.keyName.empty() )
		{
			return luaL_error( L, "key simulator input expects a non-empty keyName" );
		}

		lua_getfield( L, 1, "phase" );
		std::string phase = lua_isnil( L, -1 ) ? "pressed" : LuaValueDescription( L, -1 );
		lua_pop( L, 1 );
		if ( "down" == phase )
		{
			input.phase = MSimulatorHost::Input::kDownPhase;
		}
		else if ( "up" == phase )
		{
			input.phase = MSimulatorHost::Input::kUpPhase;
		}
		else if ( "pressed" == phase )
		{
			input.phase = MSimulatorHost::Input::kPressedPhase;
		}
		else
		{
			return luaL_error( L, "key simulator input phase must be 'down', 'up', or 'pressed'" );
		}

		lua_getfield( L, 1, "qwertyKeyName" );
		if ( ! lua_isnil( L, -1 ) )
		{
			if ( lua_type( L, -1 ) != LUA_TSTRING )
			{
				return luaL_error( L, "key simulator input qwertyKeyName must be a string" );
			}
			size_t length = 0;
			const char *value = lua_tolstring( L, -1, &length );
			input.hasQwertyKeyName = true;
			input.qwertyKeyName.assign( value, length );
		}
		lua_pop( L, 1 );

		lua_getfield( L, 1, "nativeKeyCode" );
		if ( ! lua_isnil( L, -1 ) )
		{
			lua_Number value = lua_type( L, -1 ) == LUA_TNUMBER ? lua_tonumber( L, -1 ) : 0.5;
			if ( ! IsInteger( value ) )
			{
				return luaL_error( L, "key simulator input nativeKeyCode must be an integer" );
			}
			input.hasNativeKeyCode = true;
			input.nativeKeyCode = (int)value;
		}
		lua_pop( L, 1 );
	}
	else if ( "touch" == inputType || "tap" == inputType )
	{
		const char * const options[] = { "type", "phase", "x", "y", "xStart", "yStart", NULL };
		ValidateOptionKeys( L, 1, options, ( inputType + " simulator input" ).c_str() );
		input.type = MSimulatorHost::Input::kTouchInput;
		input.x = ReadFiniteNumber( L, 1, "x", ( inputType + " simulator input x" ).c_str(), true, 0.0 );
		input.y = ReadFiniteNumber( L, 1, "y", ( inputType + " simulator input y" ).c_str(), true, 0.0 );
		isTap = "tap" == inputType;
		if ( isTap )
		{
			if ( ! IsNilField( L, 1, "phase" ) || ! IsNilField( L, 1, "xStart" ) ||
				! IsNilField( L, 1, "yStart" ) )
			{
				return luaL_error( L, "tap simulator input only accepts x and y" );
			}
			input.xStart = input.x;
			input.yStart = input.y;
			input.phase = MSimulatorHost::Input::kBeganPhase;
		}
		else
		{
			lua_getfield( L, 1, "phase" );
			std::string phase = LuaValueDescription( L, -1 );
			lua_pop( L, 1 );
			if ( "began" == phase )
			{
				input.phase = MSimulatorHost::Input::kBeganPhase;
			}
			else if ( "moved" == phase )
			{
				input.phase = MSimulatorHost::Input::kMovedPhase;
			}
			else if ( "ended" == phase )
			{
				input.phase = MSimulatorHost::Input::kEndedPhase;
			}
			else if ( "cancelled" == phase )
			{
				input.phase = MSimulatorHost::Input::kCancelledPhase;
			}
			else
			{
				return luaL_error(
					L, "touch simulator input phase must be 'began', 'moved', 'ended', or 'cancelled'" );
			}

			input.xStart = ReadFiniteNumber( L, 1, "xStart", "touch simulator input xStart", false, input.x );
			input.yStart = ReadFiniteNumber( L, 1, "yStart", "touch simulator input yStart", false, input.y );
		}
	}
	else if ( "mouse" == inputType )
	{
		const char * const options[] =
		{
			"type", "phase", "x", "y", "scrollX", "scrollY", "clickCount",
			"isShiftDown", "isAltDown", "isCtrlDown", "isCommandDown",
			"isPrimaryButtonDown", "isSecondaryButtonDown", "isMiddleButtonDown",
			NULL
		};
		ValidateOptionKeys( L, 1, options, "mouse simulator input" );
		input.type = MSimulatorHost::Input::kMouseInput;
		input.x = ReadFiniteNumber( L, 1, "x", "mouse simulator input x", true, 0.0 );
		input.y = ReadFiniteNumber( L, 1, "y", "mouse simulator input y", true, 0.0 );

		lua_getfield( L, 1, "phase" );
		std::string phase = LuaValueDescription( L, -1 );
		lua_pop( L, 1 );
		if ( "down" == phase )
		{
			input.phase = MSimulatorHost::Input::kDownPhase;
		}
		else if ( "up" == phase )
		{
			input.phase = MSimulatorHost::Input::kUpPhase;
		}
		else if ( "drag" == phase )
		{
			input.phase = MSimulatorHost::Input::kDragPhase;
		}
		else if ( "move" == phase )
		{
			input.phase = MSimulatorHost::Input::kMovePhase;
		}
		else if ( "exit" == phase )
		{
			input.phase = MSimulatorHost::Input::kExitPhase;
		}
		else if ( "scroll" == phase )
		{
			input.phase = MSimulatorHost::Input::kScrollPhase;
		}
		else
		{
			return luaL_error( L, "unknown mouse simulator input phase '%s'", phase.c_str() );
		}

		input.scrollX = ReadFiniteNumber( L, 1, "scrollX", "mouse simulator input scrollX", false, 0.0 );
		input.scrollY = ReadFiniteNumber( L, 1, "scrollY", "mouse simulator input scrollY", false, 0.0 );
		lua_getfield( L, 1, "clickCount" );
		if ( ! lua_isnil( L, -1 ) )
		{
			lua_Number value = lua_type( L, -1 ) == LUA_TNUMBER ? lua_tonumber( L, -1 ) : -1.0;
			if ( ! IsInteger( value ) || value < 0 )
			{
				return luaL_error( L, "mouse simulator input clickCount must be a non-negative integer" );
			}
			input.clickCount = (int)value;
		}
		lua_pop( L, 1 );
	}
	else
	{
		return luaL_error( L, "simulator input type must be 'back', 'key', 'text', 'touch', 'tap', or 'mouse'" );
	}

	ReadSimulatorInputBooleans( L, 1, input );
	const MSimulatorHost *host = GetSimulatorHost( L );
	if ( ! host )
	{
		return luaL_error( L, "the Simulator could not send the requested input" );
	}
	bool isKeyPress =
		MSimulatorHost::Input::kKeyInput == input.type &&
		MSimulatorHost::Input::kPressedPhase == input.phase;
	if ( isTap || isKeyPress )
	{
		if ( isKeyPress )
		{
			input.phase = MSimulatorHost::Input::kDownPhase;
		}
		if ( ! host->SendInput( input ) )
		{
			return luaL_error( L, "the Simulator could not send the requested input" );
		}
		input.phase = isTap ?
			MSimulatorHost::Input::kEndedPhase :
			MSimulatorHost::Input::kUpPhase;
	}
	if ( ! host->SendInput( input ) )
	{
		return luaL_error( L, "the Simulator could not send the requested input" );
	}

	lua_pushboolean( L, 1 );
	return 1;
}

static int
SimulateSimulatorEvent( lua_State *L )
{
	const char * const allowedOptions[] =
	{
		"type", "duration", "deltaTime", "isShake",
		"xGravity", "yGravity", "zGravity",
		"xInstant", "yInstant", "zInstant",
		"xRaw", "yRaw", "zRaw",
		"xRotation", "yRotation", "zRotation", NULL
	};
	ValidateOptionKeys( L, 1, allowedOptions, "simulated event" );

	std::string eventType = ReadRequiredString(
		L, 1, "type", "simulated event type must be 'memoryWarning', 'background', 'accelerometer', or 'gyroscope'" );
	MSimulatorHost::Event event;

	if ( "memoryWarning" == eventType )
	{
		const char * const options[] = { "type", NULL };
		ValidateOptionKeys( L, 1, options, "memoryWarning simulated event" );
		event.type = MSimulatorHost::Event::kMemoryWarningEvent;
	}
	else if ( "background" == eventType )
	{
		const char * const options[] = { "type", "duration", NULL };
		ValidateOptionKeys( L, 1, options, "background simulated event" );
		event.type = MSimulatorHost::Event::kBackgroundEvent;
		event.duration = ReadFiniteNumber( L, 1, "duration", "background simulated event duration", false, 1000.0 );
		if ( event.duration < 0 )
		{
			return luaL_error( L, "background simulated event duration cannot be negative" );
		}
	}
	else if ( "accelerometer" == eventType )
	{
		const char * const options[] =
		{
			"type", "deltaTime", "isShake",
			"xGravity", "yGravity", "zGravity",
			"xInstant", "yInstant", "zInstant",
			"xRaw", "yRaw", "zRaw", NULL
		};
		ValidateOptionKeys( L, 1, options, "accelerometer simulated event" );
		event.type = MSimulatorHost::Event::kAccelerometerEvent;

		const char * const keys[] =
		{
			"xGravity", "yGravity", "zGravity",
			"xInstant", "yInstant", "zInstant",
			"xRaw", "yRaw", "zRaw", "deltaTime"
		};
		double *values[] =
		{
			&event.xGravity, &event.yGravity, &event.zGravity,
			&event.xInstant, &event.yInstant, &event.zInstant,
			&event.xRaw, &event.yRaw, &event.zRaw, &event.deltaTime
		};
		for ( int index = 0; index < 10; index++ )
		{
			std::string context = std::string( "accelerometer simulated event " ) + keys[index];
			*values[index] = ReadFiniteNumber( L, 1, keys[index], context.c_str(), false, 0.0 );
		}

		lua_getfield( L, 1, "isShake" );
		if ( ! lua_isnil( L, -1 ) )
		{
			if ( lua_type( L, -1 ) != LUA_TBOOLEAN )
			{
				return luaL_error( L, "accelerometer simulated event isShake must be a boolean" );
			}
			event.isShake = 0 != lua_toboolean( L, -1 );
		}
		lua_pop( L, 1 );
	}
	else if ( "gyroscope" == eventType )
	{
		const char * const options[] = { "type", "deltaTime", "xRotation", "yRotation", "zRotation", NULL };
		ValidateOptionKeys( L, 1, options, "gyroscope simulated event" );
		event.type = MSimulatorHost::Event::kGyroscopeEvent;

		const char * const keys[] = { "xRotation", "yRotation", "zRotation", "deltaTime" };
		double *values[] = { &event.xRotation, &event.yRotation, &event.zRotation, &event.deltaTime };
		for ( int index = 0; index < 4; index++ )
		{
			std::string context = std::string( "gyroscope simulated event " ) + keys[index];
			*values[index] = ReadFiniteNumber( L, 1, keys[index], context.c_str(), false, 0.0 );
		}
	}
	else
	{
		return luaL_error(
			L, "simulated event type must be 'memoryWarning', 'background', 'accelerometer', or 'gyroscope'" );
	}

	const MSimulatorHost *host = GetSimulatorHost( L );
	if ( ! host || ! host->Simulate( event ) )
	{
		return luaL_error( L, "the Simulator could not dispatch the requested simulated event" );
	}
	lua_pushboolean( L, 1 );
	return 1;
}

static int
QuitSimulator( lua_State *L )
{
	lua_Number exitCode = lua_isnoneornil( L, 1 ) ? 0.0 : lua_tonumber( L, 1 );
	if ( ( ! lua_isnoneornil( L, 1 ) && lua_type( L, 1 ) != LUA_TNUMBER ) ||
		! IsInteger( exitCode ) || exitCode < 0 || exitCode > 255 )
	{
		return luaL_error( L, "simulator.quit() exitCode must be an integer between 0 and 255" );
	}

	const MSimulatorHost *host = GetSimulatorHost( L );
	if ( ! host || ! host->Quit( (int)exitCode ) )
	{
		return luaL_error( L, "the Simulator could not quit" );
	}
	lua_pushboolean( L, 1 );
	return 1;
}

static int
StartScreenRecording( lua_State *L )
{
	if ( ! lua_istable( L, 1 ) )
	{
		return luaL_error( L, "simulator.startScreenRecording() expects an options table" );
	}
	const char * const allowedOptions[] =
	{
		"path", "fps", "resolutionScale", "captureResolutionType", "outputWidth", "outputHeight",
		"includeAudio", "showCursor", "reuseCaptureStream", "overwrite", NULL
	};
	ValidateOptionKeys( L, 1, allowedOptions, "screen recording" );

	MSimulatorHost::ScreenRecordingOptions options;
	options.path = ReadRequiredString(
		L, 1, "path", "screen recording path must be a string ending in .mp4" );
	if ( options.path.empty() )
	{
		return luaL_error( L, "screen recording path cannot be empty" );
	}

	double framesPerSecond = ReadFiniteNumber(
		L, 1, "fps", "screen recording fps", false, 60.0 );
	if ( ! IsInteger( framesPerSecond ) || framesPerSecond < 1.0 || framesPerSecond > 240.0 )
	{
		return luaL_error( L, "screen recording fps must be an integer between 1 and 240" );
	}
	options.framesPerSecond = (int)framesPerSecond;
	double resolutionScale = ReadFiniteNumber(
		L, 1, "resolutionScale", "screen recording resolutionScale", false, 1.0 );
	if ( resolutionScale <= 0.0 || resolutionScale > 1.0 )
	{
		return luaL_error( L, "screen recording resolutionScale must be greater than 0 and no greater than 1" );
	}
	options.resolutionScale = resolutionScale;
	lua_getfield( L, 1, "captureResolutionType" );
	if ( lua_isnil( L, -1 ) )
	{
		options.captureResolutionType = "best";
	}
	else if ( lua_type( L, -1 ) != LUA_TSTRING )
	{
		return luaL_error( L, "screen recording captureResolutionType must be 'automatic', 'best', or 'nominal'" );
	}
	else
	{
		options.captureResolutionType = lua_tostring( L, -1 );
		if ( "automatic" != options.captureResolutionType &&
			"best" != options.captureResolutionType &&
			"nominal" != options.captureResolutionType )
		{
			return luaL_error( L, "screen recording captureResolutionType must be 'automatic', 'best', or 'nominal'" );
		}
	}
	lua_pop( L, 1 );
	double outputWidth = ReadFiniteNumber(
		L, 1, "outputWidth", "screen recording outputWidth", false, 0.0 );
	double outputHeight = ReadFiniteNumber(
		L, 1, "outputHeight", "screen recording outputHeight", false, 0.0 );
	if ( ( 0.0 == outputWidth ) != ( 0.0 == outputHeight ) ||
		( 0.0 != outputWidth &&
			( ! IsInteger( outputWidth ) || ! IsInteger( outputHeight ) ||
			outputWidth < 2.0 || outputWidth > 16384.0 ||
			outputHeight < 2.0 || outputHeight > 16384.0 ||
			0 != (int)outputWidth % 2 || 0 != (int)outputHeight % 2 ) ) )
	{
		return luaL_error( L,
			"screen recording outputWidth and outputHeight must both be even integers from 2 through 16384" );
	}
	options.outputWidth = (int)outputWidth;
	options.outputHeight = (int)outputHeight;
	options.includeAudio = ReadOptionalBoolean(
		L, 1, "includeAudio", "screen recording includeAudio", true );
	options.showsCursor = ReadOptionalBoolean(
		L, 1, "showCursor", "screen recording showCursor", false );
	options.reuseCaptureStream = ReadOptionalBoolean(
		L, 1, "reuseCaptureStream", "screen recording reuseCaptureStream", false );
	options.overwrite = ReadOptionalBoolean(
		L, 1, "overwrite", "screen recording overwrite", false );

	const MSimulatorHost *host = GetSimulatorHost( L );
	std::string error;
	bool accepted = host && host->StartScreenRecording( options, error );
	lua_pushboolean( L, accepted );
	if ( accepted )
	{
		lua_pushnil( L );
	}
	else
	{
		if ( error.empty() )
		{
			error = "the Simulator could not start screen recording";
		}
		lua_pushlstring( L, error.data(), error.length() );
	}
	return 2;
}

static int
StopScreenRecording( lua_State *L )
{
	const MSimulatorHost *host = GetSimulatorHost( L );
	std::string error;
	bool accepted = host && host->StopScreenRecording( error );
	lua_pushboolean( L, accepted );
	if ( accepted )
	{
		lua_pushnil( L );
	}
	else
	{
		if ( error.empty() )
		{
			error = "the Simulator could not stop screen recording";
		}
		lua_pushlstring( L, error.data(), error.length() );
	}
	return 2;
}

static int
GetScreenRecordingState( lua_State *L )
{
	const MSimulatorHost *host = GetSimulatorHost( L );
	MSimulatorHost::ScreenRecordingState state = host ?
		host->GetScreenRecordingState() : MSimulatorHost::kScreenRecordingUnavailable;
	const char *result = "unavailable";
	switch ( state )
	{
		case MSimulatorHost::kScreenRecordingIdle: result = "idle"; break;
		case MSimulatorHost::kScreenRecordingStarting: result = "starting"; break;
		case MSimulatorHost::kScreenRecordingRecording: result = "recording"; break;
		case MSimulatorHost::kScreenRecordingStopping: result = "stopping"; break;
		case MSimulatorHost::kScreenRecordingUnavailable: break;
	}
	lua_pushstring( L, result );
	return 1;
}

int
LuaLibSimulator::Open( lua_State *L )
{
	const luaL_Reg kVTable[] =
	{
		{ "getCurrentDevice", GetCurrentSimulatorDevice },
		{ "getState", GetSimulatorState },
		{ "getDevices", GetSimulatorDevices },
		{ "configureAndRelaunch", ConfigureAndRelaunchSimulator },
		{ "configureAndRelaunchIfNeeded", ConfigureAndRelaunchSimulatorIfNeeded },
		{ "relaunch", RelaunchSimulator },
		{ "setSafeAreaGuidesVisible", SetSimulatorSafeAreaGuidesVisible },
		{ "setFullscreen", SetSimulatorFullscreen },
		{ "sendInput", SendSimulatorInput },
		{ "simulate", SimulateSimulatorEvent },
		{ "quit", QuitSimulator },
		{ "startScreenRecording", StartScreenRecording },
		{ "stopScreenRecording", StopScreenRecording },
		{ "getScreenRecordingState", GetScreenRecordingState },

		{ NULL, NULL }
	};

	lua_newtable( L );
	luaL_register( L, NULL, kVTable );
	return 1;
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // Rtt_AUTHORING_SIMULATOR

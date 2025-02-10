//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

//  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//  NOTE:  this class is not used on Windows
//  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
#include "Core/Rtt_Build.h"

#include "Rtt_PlatformSimulator.h"

#include "Rtt_Archive.h"
#include "Rtt_Event.h"
#include "Rtt_Lua.h"
#include "Rtt_LuaFile.h"
#include "Rtt_MPlatform.h"
#include "Rtt_PlatformPlayer.h"
#include "Rtt_PlatformSurface.h"
#include "Rtt_String.h"
#include "Rtt_LuaContext.h"
#include "Rtt_FileSystem.h"

#include <string.h>

// ----------------------------------------------------------------------------

namespace Rtt
{

int luaload_ValidateSettings(lua_State* L);
int luaload_valid_build_settings(lua_State* L);
int luaload_valid_config_lua(lua_State* L);

const char PlatformSimulator::kPlatformKeyNameIPhone[] = "iphone";
const char PlatformSimulator::kPlatformKeyNameAndroid[] = "android";
const char PlatformSimulator::kPlatformKeyNameMac[] = "mac";
// ----------------------------------------------------------------------------

/*
SimulatorOptions::SimulatorOptions()
:	connectToDebugger( false ),
	isInteractive( false ),
	appPath( NULL )
{

// WARNING: Any non-NULL char *'s will be freed
SimulatorOptions::~SimulatorOptions()
{
	if ( appPath ) free( appPath );
}
*/

// ----------------------------------------------------------------------------

PlatformSimulator::Config::Config( Rtt_Allocator & allocator )
:	platform( TargetDevice::kUnknownPlatform ),
	deviceName( & allocator ),
	deviceWidth(0.0f),
	deviceHeight(0.0f)
{
}

PlatformSimulator::Config::~Config()
{
}

// ----------------------------------------------------------------------------

PlatformSimulator::PlatformSimulator( PlatformFinalizer finalizer )
:	fPlatform( NULL ),
	fPlayer( NULL ),
	fPlatformFinalizer( finalizer ),
	fProperties( 0 )
{
}

PlatformSimulator::~PlatformSimulator()
{
	WillExit();

	delete fPlayer;

	(*fPlatformFinalizer)( fPlatform );
}

void
PlatformSimulator::Initialize( MPlatform* platform, MCallback *viewCallback )
{
	Rtt_ASSERT( ! fPlatform && ! fPlayer );

	// OpenGL contexts must be available before we call Player c-tor
	PlatformPlayer* player = new PlatformPlayer( * platform, viewCallback );

	fPlatform = platform;
	fPlayer = player;
}

const char *
PlatformSimulator::GetPlatformName() const
{
	return NULL;
}

const char *
PlatformSimulator::GetPlatform() const
{
	return NULL;
}

static lua_Number
NumberForKey( lua_State *L, const char key[], lua_Number defaultValue )
{
    lua_Number result = defaultValue;
    
	lua_getfield( L, -1, key );
    if (lua_isnumber( L, -1 ) )
    {
        result = lua_tonumber( L, -1 );
    }
	lua_pop( L, 1 );
    
	return result;
}

static const char*
StringForKey( lua_State *L, const char key[], const char *defaultValue)
{
    const char *result = defaultValue;
    
	lua_getfield( L, -1, key );
    
    if ( lua_isstring( L, -1 ) )
    {
        result = lua_tostring( L, -1 );
    }
	lua_pop( L, 1 );

	return result;
}

#include "CoronaLua.h"

void
PlatformSimulator::LoadConfig( const char deviceConfigFile[], Config& rConfig )
{
	lua_State *L = luaL_newstate();
    String errorMesg;

	if ( 0 == Lua::DoFile( L, deviceConfigFile, 0, false, &errorMesg ))
	{
		lua_getglobal( L, "simulator" );

		rConfig.deviceName.Set( StringForKey( L, "deviceName", "Unknown" ) );
		rConfig.deviceWidth = (float) NumberForKey( L, "deviceWidth", 400 );
		rConfig.deviceHeight = (float) NumberForKey( L, "deviceHeight", 400 );

		lua_pop( L, 1 );
	}
	else
	{
		Rtt_TRACE(("WARNING: Could not load device config file '%s': %s\n", deviceConfigFile, errorMesg.GetString()));
	}

	lua_close( L );
}

void
PlatformSimulator::ValidateSettings(const MPlatform& platform)
{
	lua_State *L = luaL_newstate();

	luaL_openlibs(L);
	
	String resourcePath;
	platform.PathForFile( NULL, MPlatform::kResourceDir, MPlatform::kTestFileExists, resourcePath );

	Lua::RegisterModuleLoader( L, "valid_build_settings", Lua::Open< luaload_valid_build_settings > );
	Lua::RegisterModuleLoader( L, "valid_config_lua", Lua::Open< luaload_valid_config_lua > );

	Lua::DoBuffer(L, &luaload_ValidateSettings, NULL);

	lua_getglobal(L, "validateSettings" ); Rtt_ASSERT(lua_isfunction(L, -1 ));

	// First do build.settings

	String buildSettingsPath;
	buildSettingsPath.Set(resourcePath);
	buildSettingsPath.AppendPathComponent("build.settings");

	if (Rtt_FileExists(buildSettingsPath))
	{
		lua_pushstring( L, buildSettingsPath );
		lua_pushstring( L, "valid_build_settings" );

		// This will emit any diagnostics to the console
		Lua::DoCall(L, 2, 1);
	}

	// Now do config.lua

	String configLuaPath;
	configLuaPath.Set(resourcePath);
	configLuaPath.AppendPathComponent("config.lua");

	if (Rtt_FileExists(configLuaPath))
	{
		lua_getglobal(L, "validateSettings" ); Rtt_ASSERT(lua_isfunction(L, -1 ));
		lua_pushstring( L, configLuaPath );
		lua_pushstring( L, "valid_config_lua" );

		// This will emit any diagnostics to the console
		Lua::DoCall(L, 2, 1);
	}

	lua_close( L );
}

void
PlatformSimulator::LoadBuildSettings( const MPlatform& platform )
{
	lua_State *L = luaL_newstate();

	// Warn about logical errors in build.settings and config.lua
	ValidateSettings(platform);
	
	lua_close( L );
}

void
PlatformSimulator::Start( const SimulatorOptions& options )
{
	Rtt_ASSERT( fPlayer );

	Runtime& runtime = fPlayer->GetRuntime();

	// Currently, enterprise is required to have the Lua parser available
	bool isLuaParserAvailable = true;
	runtime.SetProperty( Runtime::kIsLuaParserAvailable, isLuaParserAvailable );

 	// Read plugins from build.settings
	runtime.FindDownloadablePlugins( GetPlatformName() );

	const char* appFilePath =
		( options.isInteractive
			? PlatformPlayer::InteractiveFilePath()
			: PlatformPlayer::DefaultAppFilePath() );

	fPlayer->Start( appFilePath, options.connectToDebugger );
	
	DidStart();
}

void
PlatformSimulator::SetProperty( U32 mask, bool value )
{
	if ( Rtt_VERIFY( mask > kUninitializedMask ) )
	{
		const U32 p = fProperties;
		fProperties = ( value ? p | mask : p & ~mask );
	}
}

static PlatformSimulator::PropertyMask
PropertyMaskForEventType( MPlatformDevice::EventType type )
{
	PlatformSimulator::PropertyMask mask = PlatformSimulator::kUninitializedMask;

	switch( type )
	{
		case MPlatformDevice::kAccelerometerEvent:
			mask = PlatformSimulator::kAccelerometerEventMask;
			break;
		case MPlatformDevice::kGyroscopeEvent:
			mask = PlatformSimulator::kGyroscopeEventMask;
			break;
		case MPlatformDevice::kMultitouchEvent:
			mask = PlatformSimulator::kMultitouchEventMask;
			break;
		case MPlatformDevice::kMouseEvent:
			mask = PlatformSimulator::kMouseEventMask;
			break;
		default:
			Rtt_ASSERT_NOT_REACHED();
			break;
	}

	return mask;
}

void
PlatformSimulator::BeginNotifications( MPlatformDevice::EventType type ) const
{
	PropertyMask mask = PropertyMaskForEventType( type );

	switch( mask )
	{
		case kAccelerometerEventMask:
			Rtt_TRACE_SIM( ( "WARNING: Simulator does not support accelerometer events\n" ) );
			break;
		case kGyroscopeEventMask:
			Rtt_TRACE_SIM( ( "WARNING: Simulator does not support gyroscope events\n" ) );
			break;
		case kMultitouchEventMask:
			Rtt_TRACE_SIM( ( "WARNING: Simulator does not support multitouch events\n" ) );
			break;
		default:
			break;
	}

	const_cast< Self* >( this )->SetProperty( mask, true );
}

void
PlatformSimulator::EndNotifications( MPlatformDevice::EventType type ) const
{
	const_cast< Self* >( this )->SetProperty( PropertyMaskForEventType( type ), false );
}

void
PlatformSimulator::WillSuspend()
{
	
}

void 
PlatformSimulator::DidSuspend()
{
	
}

void
PlatformSimulator::WillResume()
{
	
}
	
void
PlatformSimulator::DidResume()
{
	
}
	
void
PlatformSimulator::ToggleSuspendResume(bool sendApplicationEvents /* = true */)
{
//Rtt_TRACE( ( "DidResume\n" ) );
	
	// This assert interferes with suspending the app if a syntax error is thrown during
	// app load.  Since it's only active in debug builds removing it should be benign for
	// the shipping product.
	// Rtt_ASSERT( IsProperty( kIsAppStartedMask ) );

	PlatformPlayer* player = GetPlayer();

	if ( player )
	{
		Runtime& runtime = player->GetRuntime();
		bool isSuspended = runtime.IsSuspended();

		if ( isSuspended )
		{
			// System is suspending app, e.g. phone call
			WillResume();
			runtime.Resume(sendApplicationEvents);
			DidResume();
		}
		else
		{
			WillSuspend();
			runtime.Suspend(sendApplicationEvents);
			DidSuspend();
		}
	}
}

void
PlatformSimulator::DidStart()
{
//Rtt_TRACE( ( "DidStart\n" ) );
	SetProperty( kIsAppStartedMask, true );
}

void
PlatformSimulator::WillExit()
{
}


// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------


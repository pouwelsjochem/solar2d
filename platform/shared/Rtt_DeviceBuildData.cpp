//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_DeviceBuildData.h"

#include "Rtt_LuaFrameworks.h"
#include "Rtt_Lua.h"
#include "Rtt_FileSystem.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

static const char kPluginsMetadataKey[] = __FILE__ "-plugins";

static const char kBuildQueueKey[] = "buildQueue";
static const char kBuildBucketKey[] = "buildBucket";

static const char kBuildYearKey[] = "buildYear";
static const char kBuildRevisionKey[] = "buildRevision";

// ----------------------------------------------------------------------------

static lua_State *
NewLua()
{
	lua_State *L = Lua::New( true );

#ifdef Rtt_AUTHORING_SIMULATOR
	Lua::RegisterModuleLoader( L, "dkjson", Lua::Open< luaload_dkjson >, 0 );
#endif
	Lua::RegisterModuleLoader( L, "json", Lua::Open< luaload_json >, 0 );

	// TODO: Is this the best place for this?
	// By default, create an empty table for plugins
	lua_newtable( L );
	lua_setfield( L, LUA_REGISTRYINDEX, kPluginsMetadataKey );

	return L;
}
    
// ----------------------------------------------------------------------------

DeviceBuildData::DeviceBuildData(
	Rtt_Allocator *pAllocator,
	const char *appName,
	S32 targetDevice,
	TargetDevice::Platform targetPlatform,
	S32 targetPlatformVersion,
	const char *targetCertType, // bool isDistribution
	const char *clientDeviceId,
	const char *clientProductId,
	const char *clientPlatform )
:	fL( NewLua() ),
	fAppName( pAllocator, appName ),
	fTargetDevice( targetDevice ),
	fTargetPlatform( targetPlatform ),
	fTargetPlatformVersion( targetPlatformVersion ),
	fTargetCertType( pAllocator, targetCertType ),
	fClientDeviceId( pAllocator, clientDeviceId ),
	fClientProductId( pAllocator, clientProductId ),
	fClientPlatform( pAllocator, clientPlatform ),
	fBuildQueue( pAllocator ),
	fBuildBucket( pAllocator ),
	fBuildYear( Rtt_BUILD_YEAR ),
	fBuildRevision( Rtt_BUILD_REVISION )
{
}

DeviceBuildData::~DeviceBuildData()
{
	Lua::Delete( fL );
}

bool
DeviceBuildData::ReadAppSettings( lua_State *L, const char *appSettingsPath )
{
	bool result = true;

	Rtt_LUA_STACK_GUARD( L );

	// Read 'AppSettings' (if it exists)
	if ( appSettingsPath
		 && 0 == luaL_loadfile( L, appSettingsPath )
		 && 0 == lua_pcall( L, 0, 0, 0 ) )
	{
		lua_getglobal( L, kBuildBucketKey );
		if( lua_isstring( L, -1 ) )
		{
			fBuildBucket.Set( lua_tostring( L, -1 ) );
		}
		lua_pop( L, 1 );
	}

	return result;
}

//	settings =
//	{
//		plugins =
//		{
//			-- key is the name passed to Lua's 'require()'
//			moduleName1 =
//			{
//				-- required
//				publisherId = "string domain value",
//				
//				-- optional (if nil, assumes all platforms are supported)
//				platforms =
//				{
//					ios = true,
//					android = true,
//				}
//				
//				-- optional (for partners)
//				sandboxKey = "string hash value"
//			},
//
//			moduleName2 =
//			{
//			},
//		},
//		buildQueue = "",
//		buildBucket = "",
//      buildYear = 2014,        -- int. default is simulator's daily build year
//      buildRevision = 2511,    -- int. default is simulator's daily build revision
//	}
//
bool
DeviceBuildData::ReadBuildSettings( lua_State *L, const char *buildSettingsPath )
{
	bool result = true;

	Rtt_LUA_STACK_GUARD( L );

	// Read 'build.settings'. This is separate from the simulator's
	// b/c we want to support cmd-line build tools like CoronaBuilder.
	if ( buildSettingsPath
		 && Rtt_FileExists( buildSettingsPath )
		 && 0 == luaL_loadfile( L, buildSettingsPath )
		 && 0 == lua_pcall( L, 0, 0, 0 ) )
	{
		lua_getglobal( L, "settings" );
		if ( lua_istable( L, -1 ) )
		{
			lua_getfield( L, -1, kBuildYearKey );
			int buildYear = (int)lua_tointeger( L, -1 );
			lua_pop( L, 1 );

			lua_getfield( L, -1, kBuildRevisionKey );
			int buildRevision = (int)lua_tointeger( L, -1 );
			lua_pop( L, 1 );

			SetBuild( buildYear, buildRevision );

			// Find out which plugins are needed
			lua_getfield( L, -1, "plugins" ); // push plugins
			if ( lua_istable( L, -1 ) )
			{
				int index = Lua::Normalize( L, -1 );

				lua_pushnil( L );
				while ( lua_next( L, index ) != 0 )
				{
					// key is at -2. value is at -1
					if(lua_type(L, -2) == LUA_TSTRING) // prevent crash on digit ideces on next iteration due to tostring
					{
						const char *moduleName = lua_tostring( L, -2 );
						AddPlugin( L, moduleName, -1 );
					}
					else
					{
						Rtt_LogException("WARNING: plugins table in build.settings contains numeric indices or array");
					}
					lua_pop( L, 1 ); // pop value. keeps key for next iteration
				}
			}
			lua_pop( L, 1 ); // pop plugins

			// settings.buildQueue (test out changes to build queue code)
			lua_getfield( L, -1, kBuildQueueKey ); // push buildQueue
			if ( lua_isstring( L, -1 ) )
			{
				const char *buildQueue = lua_tostring( L, -1 );
				fBuildQueue.Set( buildQueue );
				Rtt_LogException("WARNING: Setting '%s = %s' in build.settings", kBuildQueueKey, buildQueue);
			}
			lua_pop( L, 1 ); // pop buildQueue

			// settings.buildBucket (test out sandboxed templates)
			lua_getfield( L, -1, kBuildBucketKey ); // push buildBucket
			if ( lua_isstring( L, -1 ) )
			{
				const char *buildBucket = lua_tostring( L, -1 );
				fBuildBucket.Set( buildBucket ); // Overrides any existing value
				Rtt_LogException("WARNING: Setting '%s = %s' in build.settings", kBuildBucketKey, buildBucket);
			}
			lua_pop( L, 1 ); // pop buildBucket
		}
		lua_pop( L, 1 ); // pop settings
	}

	return result;
}

bool
DeviceBuildData::Initialize(
	const char *appSettingsPath,
	const char *buildSettingsPath)
{
	bool result = true;

	lua_State *L = fL;

	Rtt_LUA_STACK_GUARD( L );

	ReadAppSettings( L, appSettingsPath );

	// NOTE: build.settings overrides AppSettings
	ReadBuildSettings( L, buildSettingsPath );

	return result;
}

void
DeviceBuildData::SetBuild( int buildYear, int buildRevision )
{
	if ( buildYear > 0 && buildRevision > 0 )
	{
		fBuildYear = buildYear;
		fBuildRevision = buildRevision;

		Rtt_TRACE( ( "NOTE: This project's build.settings overrides the default target build. Instead it targets a specific daily build (%d.%d)\n", fBuildYear, fBuildRevision ) );
	}
}

void
DeviceBuildData::AddPlugin( lua_State *L, const char *moduleName, int index )
{
	Rtt_LUA_STACK_GUARD( L );

	index = Lua::Normalize( L, index );

	if ( lua_istable( L, index ) )
	{
		lua_getfield( L, LUA_REGISTRYINDEX, kPluginsMetadataKey ); // push registry[kPluginsMetadataKey]
		{
			lua_pushvalue( L, index );
			lua_setfield( L, -2, moduleName );
		}
		lua_pop( L, 1 ); // pop registry[kPluginsMetadataKey]
	}
	else
	{
		Rtt_PRINT( ( "Plugin (%s) in build.settings not specified correctly. Got '%s'. Expected 'table'", moduleName, lua_typename( L, index ) ) );
	}
}

//	{
//		appName = ,
//		targetDevice = , -- device
//		targetPlatform = , -- platform
//		targetPlatformVersion = , -- version
//		targetCertType = , -- certType
//		clientDeviceId = , -- deviceId
//		clientProductId = , -- productId
//		clientPlatform = ,
//		dailyBuildYear = , -- [new]
//		dailyBuildRevision = , -- [new]
//
//		plugins = {
//			-- enforce uniqueness of name on a per-project basis
//			name1 = {
//				publisherId = ,
//				supportedPlatforms = { "ios", "android", } // optional
//				sandboxKey = "3473298abde99", // optional
//			},
//			name2 = {
//				publisherId = ,
//			},
//		}
//
//		-- (separate arg in POST) inputProject = , -- file
//
//
//		-- (?) appVersion = ,
//		-- (?) packageName = , -- appPackage
//		-- (X) customBuildId = ,
//		-- (X) simVersionTimestamp = , -- timestamp
//	}

void
DeviceBuildData::PushTable( lua_State *L ) const
{
	lua_newtable( L );

    // Don't guard the stack until we've pushed the table as that's the point of this
	Rtt_LUA_STACK_GUARD( L );
    
	lua_pushstring( L, fAppName.GetString() );
	lua_setfield( L, -2, "appName" );

	lua_pushinteger( L, fTargetDevice );
	lua_setfield( L, -2, "targetDevice" );

	lua_pushinteger( L, fTargetPlatform );
	lua_setfield( L, -2, "targetPlatform" );

	lua_pushinteger( L, fTargetPlatformVersion );
	lua_setfield( L, -2, "targetPlatformVersion" );

	lua_pushstring( L, fTargetCertType.GetString() );
	lua_setfield( L, -2, "targetCertType" );

	lua_pushstring( L, fClientDeviceId.GetString() );
	lua_setfield( L, -2, "clientDeviceId" );

	lua_pushstring( L, fClientProductId.GetString() );
	lua_setfield( L, -2, "clientProductId" );

	lua_pushstring( L, fClientPlatform.GetString() );
	lua_setfield( L, -2, "clientPlatform" );

	lua_pushinteger( L, fBuildYear );
	lua_setfield( L, -2, "dailyBuildYear" );

	lua_pushinteger( L, fBuildRevision );
	lua_setfield( L, -2, "dailyBuildRevision" );

	lua_pushstring( L, fBuildQueue.GetString() );
	lua_setfield( L, -2, kBuildQueueKey );

	lua_pushstring( L, fBuildBucket.GetString() );
	lua_setfield( L, -2, kBuildBucketKey );

	// Plugins
	lua_getfield( L, LUA_REGISTRYINDEX, kPluginsMetadataKey );
	lua_setfield( L, -2, "plugins" );
}

void
DeviceBuildData::GetJSON( String& result ) const
{
	lua_State *L = fL;

	Rtt_LUA_STACK_GUARD( fL );

	Lua::RegisterModuleLoader( L, "dkjson", Lua::Open< luaload_dkjson > );
	Lua::RegisterModuleLoader( L, "lpeg", luaopen_lpeg );
	Lua::PushModule( L, "json" ); // push 'json'

	// Call: 'json.encode( t )'
	lua_getfield( L, -1, "encode" ); // push 'json.encode'
	PushTable( L ); // push 't'
	Lua::DoCall( L, 1, 1 );

	const char *jsonValue = lua_tostring( L, -1 );
	Rtt_TRACE_SIM( ( "DeviceBuildData: %s\n", jsonValue ) );
	result.Set( jsonValue );

	lua_pop( L, 2 );
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_AppPackagerFactory.h"

#include "Rtt_NxSAppPackager.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

#if defined(CORONABUILDER_NXS)

// Read a string field from the lua args table; returns NULL if missing or non-string.
static const char *
GetLuaStringField( lua_State *L, int index, const char *key )
{
	lua_getfield( L, index, key );
	const char *result = lua_isstring( L, -1 ) ? lua_tostring( L, -1 ) : NULL;
	return result;
}

static bool
GetLuaBoolField( lua_State *L, int index, const char *key, bool fallback )
{
	lua_getfield( L, index, key );
	bool result = fallback;
	if ( lua_isboolean( L, -1 ) )
	{
		result = lua_toboolean( L, -1 ) != 0;
	}
	return result;
}

AppPackagerParams*
AppPackagerFactory::CreatePackagerParamsNxS(
	lua_State *L,
	int index,
	TargetDevice::Platform targetPlatform,
	TargetDevice::Version targetPlatformVersion,
	const char *appName,
	const char *version,
	const char *certificatePath,
	const char *projectPath,
	const char *dstPath,
	const char *sdkPath,
	const char *customBuildId,
	const char *templateType ) const
{
	AppPackagerParams *result = NULL;

	if ( targetPlatform != TargetDevice::kNxSPlatform )
	{
		Rtt_ASSERT_NOT_REACHED();
		return NULL;
	}

	// WARNING: bump lua_checkstack if you add more lua_getfield() calls.
	lua_checkstack( L, 8 );
	int top = lua_gettop( L );

	// NxS-specific fields read from the args lua table.
	// nxTemplate defaults to %CORONA_PATH%\Resources\nxtemplate when not provided.
	const char *nmetaPath    = GetLuaStringField( L, index, "nmetaPath" );
	const char *sdkRoot      = GetLuaStringField( L, index, "sdkRoot" );
	const char *nxTemplate   = GetLuaStringField( L, index, "nxTemplate" );
	const char *productId    = GetLuaStringField( L, index, "productId" );
	const char *appPackage   = GetLuaStringField( L, index, "appPackage" );
	bool isDistributionBuild = GetLuaBoolField( L, index, "publishable", false );

	// sdkRoot fallback: %NINTENDO_SDK_ROOT% env var if not in args.
	String sdkRootEnv;
	if ( ! sdkRoot || ! *sdkRoot )
	{
		const char *envSdkRoot = getenv( "NINTENDO_SDK_ROOT" );
		if ( envSdkRoot && *envSdkRoot )
		{
			sdkRootEnv.Set( envSdkRoot );
			sdkRoot = sdkRootEnv.GetString();
		}
	}

	// nxTemplate fallback: %CORONA_PATH%\Resources\nxtemplate
	String nxTemplateDefault;
	if ( ! nxTemplate || ! *nxTemplate )
	{
		const char *coronaPath = getenv( "CORONA_PATH" );
		if ( coronaPath && *coronaPath )
		{
			nxTemplateDefault.Set( coronaPath );
			// Strip any trailing slash before appending.
			size_t len = strlen( nxTemplateDefault.GetString() );
			if ( len > 0 )
			{
				char last = nxTemplateDefault.GetString()[ len - 1 ];
				if ( last == '\\' || last == '/' )
				{
					// Rtt::String doesn't expose a Truncate helper; rebuild without trailing sep.
					char *mutable_copy = strdup( nxTemplateDefault.GetString() );
					mutable_copy[ len - 1 ] = '\0';
					nxTemplateDefault.Set( mutable_copy );
					free( mutable_copy );
				}
			}
			nxTemplateDefault.Append( "\\Resources\\nxtemplate" );
			nxTemplate = nxTemplateDefault.GetString();
		}
	}

	CHECK_VALUE( nmetaPath, "nmetaPath" );
	CHECK_VALUE( sdkRoot, "sdkRoot (set NINTENDO_SDK_ROOT or pass via lua args)" );
	CHECK_VALUE( nxTemplate, "nxTemplate (pass via lua args or set CORONA_PATH)" );
	CHECK_VALUE( productId, "productId" );
	CHECK_VALUE( appPackage, "appPackage" );

	// identity / provisionFile aren't used by NxS but the parent constructor expects them.
	const char *identity = "";
	const char *provisionFile = "";

	// targetDevice is a Nintendo-specific value; default to 0 (the lua side reads
	// settings.nxs.* for any per-device behavior).
	S32 targetDevice = 0;

	result = new NxSAppPackagerParams(
		appName,
		version,
		identity,
		provisionFile,
		projectPath,
		dstPath,
		nmetaPath,
		sdkRoot,
		targetPlatform,
		targetPlatformVersion,
		targetDevice,
		customBuildId,
		productId,
		appPackage,
		isDistributionBuild,
		nxTemplate );

	lua_settop( L, top );

	return result;
}

#endif // defined(CORONABUILDER_NXS)

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

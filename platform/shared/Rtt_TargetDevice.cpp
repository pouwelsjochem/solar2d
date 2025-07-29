//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"


#include "Core/Rtt_String.h"
#include "Rtt_LuaFile.h"
#include "Rtt_String.h"
#ifdef Rtt_AUTHORING_SIMULATOR
#include "CoronaLua.h"
#endif
#include <string.h>
#include <stdlib.h>

#ifndef PATH_MAX
#define PATH_MAX	4096
#endif

#include "Rtt_TargetDevice.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

// The following finalizer class and static member variable will automatically delete this class' data on app exit.
class StaticTargetDeviceFinalizer
{
	public:
		StaticTargetDeviceFinalizer() {}
		~StaticTargetDeviceFinalizer()
		{
			TargetDevice::ReleaseAllSkins();
		}
};
static StaticTargetDeviceFinalizer sTargetDeviceFinalizer;


const char *kDefaultSkinName = "min_supported_2";
    
TargetDevice::SkinSpec **TargetDevice::fSkins = NULL;
int TargetDevice::fSkinCount = 0;
TargetDevice::Skin TargetDevice::fDefaultSkinID = TargetDevice::kUnknownSkin;

#ifdef Rtt_AUTHORING_SIMULATOR
static int compar_SkinSpec(const void *item1, const void *item2)
{
    TargetDevice::SkinSpec *skin1 = *(TargetDevice::SkinSpec **) item1;
    TargetDevice::SkinSpec *skin2 = *(TargetDevice::SkinSpec **) item2;
    
    // printf("Comparing %s and %s\n", skin1->GetName(), skin2->GetName());
    
    Rtt_ASSERT( skin1 != NULL );
    Rtt_ASSERT( skin2 != NULL );

    int skinCategoryComp = strcmp(skin1->GetCategory(), skin2->GetCategory());
    
    if (skinCategoryComp == 0)
    {
        int skinNameComp = strcmp(skin1->GetName(), skin2->GetName());

        if (skinNameComp == 0)
        {
            int skinWidthComp = skin1->GetWidth() - skin2->GetWidth();

            if (skinWidthComp == 0)
            {
                return skin1->GetHeight() - skin2->GetHeight();
            }
            else
            {
                return skinWidthComp;
            }
        }
        else
        {
            return skinNameComp;
        }
    }
    else
    {
        return skinCategoryComp;
    }
}
#endif // Rtt_AUTHORING_SIMULATOR

bool
TargetDevice::Initialize( char **skinFiles, const int skinFileCount )
{
#ifdef Rtt_AUTHORING_SIMULATOR
	ReleaseAllSkins();
    if ((fSkins = (SkinSpec **) calloc(sizeof(SkinSpec*), skinFileCount)) == NULL)
    {
        Rtt_TRACE_SIM(("ERROR: Cannot allocate memory for skins in TargetDevice::Initialize()\n"));
        
        return false;
    }
    
    fSkinCount = 0;
    for (int i = 0; i < skinFileCount; i++ )
    {
        int status = 0;
        char *skinName = NULL;
        char *skinCategory = NULL;
        int skinWidth = 0;
        int skinHeight = 0;
        lua_State *L = CoronaLuaNew( kCoronaLuaFlagNone );
        status = CoronaLuaDoFile( L, skinFiles[i], 0, false );
        
        if ( 0 == status )
        {
            lua_getglobal( L, "simulator" );
            
            if ( lua_istable( L, -1 ) )
            {
                lua_getfield( L, -1, "category" );
                skinCategory = (char *) luaL_optstring( L, -1, "Untitled category" );
                lua_pop( L, 1 );

                lua_getfield( L, -1, "deviceName" );
                skinName = (char *) luaL_optstring( L, -1, "Untitled Skin" );
                lua_pop( L, 1 );

                lua_getfield( L, -1, "deviceWidth" );
                skinWidth = luaL_optint( L, -1, 0 );
                lua_pop( L, 1 );

                lua_getfield( L, -1, "deviceHeight" );
                skinHeight = luaL_optint( L, -1, 0 );
                lua_pop( L, 1 );
            }

            fSkins[fSkinCount] = new SkinSpec(skinName, skinFiles[i], skinCategory, skinWidth, skinHeight);
            ++fSkinCount;
        }
        else
        {
            CoronaLuaError(L, "invalid Lua in skin file '%s'", skinFiles[i]);
        }

        CoronaLuaDelete( L );
    }

    qsort(fSkins, fSkinCount, sizeof(TargetDevice::SkinSpec *), compar_SkinSpec);
    
	for (int i = 0; i < fSkinCount; i++)
    {
		// Rtt_TRACE_SIM(("TargetDevice::Initialize: skin: %d: %s = %s\n", i, fSkins[i]->GetCategory(), fSkins[i]->GetName()));
        
        // Remember the index of the platform's default skin in case we want a default later
        if (strcmp(fSkins[i]->GetName(), kDefaultSkinName) == 0)
        {
            fDefaultSkinID = (Skin) i;
            break;
        }
    }
#endif // Rtt_AUTHORING_SIMULATOR
    
    return true;
}

void
TargetDevice::ReleaseAllSkins()
{
#ifdef Rtt_AUTHORING_SIMULATOR
	// Do not continue if a skin collection has not been allocated.
	if (!fSkins)
	{
		return;
	}

	// First delete all of the SkinSpec objects in the collection.
	for (int index = 0; index < fSkinCount; index++)
	{
		TargetDevice::SkinSpec *skinPointer = fSkins[index];
		if (skinPointer)
		{
			delete skinPointer;
		}
	}

	// Delete the skin collection.
	free(fSkins);

	// Reset the skin collection's static variables.
	fSkins = NULL;
	fSkinCount = 0;
	fDefaultSkinID = TargetDevice::kUnknownSkin;
#endif
}

static const char kAndroidPlatformString[] = "Android";
static const char kIOSPlatformString[] = "iOS";
static const char kLinuxPlatformString[] = "LINUX";
static const char kWin32PlatformString[] = "Win32";
static const char kOSXPlatformString[] = "OSX";
static const char kTVOSPlatformString[] = "tvOS";
static const char kSwitchPlatformString[] = "NxS Switch";

const char*
TargetDevice::StringForPlatform( TargetDevice::Platform platform )
{
	const char *result = kIOSPlatformString;

	switch ( platform )
	{
		case kIPhonePlatform:
			result = kIOSPlatformString;
			break;
		case kAndroidPlatform:
			result = kAndroidPlatformString;
			break;
		case kLinuxPlatform:
			result = kLinuxPlatformString;
			break;
		case kWin32Platform:
			result = kWin32PlatformString;
			break;
		case kOSXPlatform:
			result = kOSXPlatformString;
			break;
		case kTVOSPlatform:
			result = kTVOSPlatformString;
			break;
		default:
			Rtt_ASSERT_NOT_IMPLEMENTED();
			break;
	}

	return result;
}

TargetDevice::Platform
TargetDevice::PlatformForString( const char *str )
{
	TargetDevice::Platform result = kUnknownPlatform;

	if ( str )
	{
		if ( 0 == Rtt_StringCompareNoCase( str, kAndroidPlatformString ) )
		{
			result = kAndroidPlatform;
		}
		else if ( 0 == Rtt_StringCompareNoCase( str, kLinuxPlatformString ) )
		{
			result = kLinuxPlatform;
		}
		else if ( 0 == Rtt_StringCompareNoCase( str, kIOSPlatformString ) )
		{
			result = kIPhonePlatform;
		}
		else if ( 0 == Rtt_StringCompareNoCase( str, kWin32PlatformString ) )
		{
			result = kWin32Platform;
		}
		else if ( 0 == Rtt_StringCompareNoCase( str, kOSXPlatformString ) )
		{
			result = kOSXPlatform;
		}
		else if ( 0 == Rtt_StringCompareNoCase( str, kTVOSPlatformString ) )
		{
			result = kTVOSPlatform;
		}
	}

	return result;
}

static const char kAndroidPlatformTag[] = "android";
static const char kIOSPlatformTag[] = "ios";
static const char kLinuxPlatformTag[] = "linux";
static const char kWin32PlatformTag[] = "win32";
static const char kOSXPlatformTag1[] = "osx";
static const char kOSXPlatformTag2[] = "macos";
static const char kTVOSPlatformTag[] = "tvos";
static const char kNXSPlatformTag[] = "nx64";

const char*
TargetDevice::TagForPlatform( TargetDevice::Platform platform )
{
	const char *result = kIOSPlatformTag;

	switch ( platform )
	{
		case kIPhonePlatform:
			result = kIOSPlatformTag;
			break;
		case kAndroidPlatform:
			result = kAndroidPlatformTag;
			break;
		case kLinuxPlatform:
			result = kLinuxPlatformTag;
			break;
		case kWin32Platform:
			result = kWin32PlatformTag;
			break;
		case kOSXPlatform:
			result = kOSXPlatformTag2;
			break;
		case kTVOSPlatform:
			result = kTVOSPlatformTag;
			break;
		case kNxSPlatform:
			result = kNXSPlatformTag;
		default:
			Rtt_ASSERT_NOT_IMPLEMENTED();
			break;
	}

	return result;
}

TargetDevice::Platform
TargetDevice::PlatformForTag( const char *str )
{
	TargetDevice::Platform result = kUnknownPlatform;

	if ( str )
	{
		if ( 0 == Rtt_StringCompareNoCase( str, kAndroidPlatformTag ) )
		{
			result = kAndroidPlatform;
		}
		else if ( 0 == Rtt_StringCompareNoCase( str, kLinuxPlatformTag ) )
		{
			result = kLinuxPlatform;
		}
		else if ( 0 == Rtt_StringCompareNoCase( str, kIOSPlatformTag ) )
		{
			result = kIPhonePlatform;
		}
		else if ( 0 == Rtt_StringCompareNoCase( str, kWin32PlatformTag ) )
		{
			result = kWin32Platform;
		}
		else if ( 0 == Rtt_StringCompareNoCase( str, kOSXPlatformTag1 ) ||
				  0 == Rtt_StringCompareNoCase( str, kOSXPlatformTag2 ) )
		{
			result = kOSXPlatform;
		}
		else if ( 0 == Rtt_StringCompareNoCase( str, kTVOSPlatformTag ) )
		{
			result = kTVOSPlatform;
		}
		else if (0 == Rtt_StringCompareNoCase(str, kNXSPlatformTag))
		{
			result = kNxSPlatform;
		}
	}

	return result;
}

// TODO: Load these values from JSON file
TargetDevice::Version
TargetDevice::VersionForPlatform( Platform platform )
{
	Version result = kUnknownVersion;

	switch ( platform )
	{
		case kAndroidPlatform:
			result = kAndroidOS4_0_3;
			break;
		case kOSXPlatform:
		case kIPhonePlatform:
		case kTVOSPlatform: // As of Xcode 7.1, it seems like iOS/tvOS versions are in sync
			result = kIPhoneOS9_0;
			break;
		default:
			break;
	}

	return result;
}

const char *
TargetDevice::LuaObjectFileFromSkin( int skinID )
{
    if (skinID >= fSkinCount)
    {
        return NULL;
    }
    
    if (skinID < 0 || skinID >= fSkinCount)
    {
        skinID = fDefaultSkinID;
    }
    
    return fSkins[skinID]->GetPath();
}

const char *
TargetDevice::SkinSpec::GenerateLabel( const char *path )
{
    // Find the filename part at the end of the path (sans extension)

    static char buf[PATH_MAX];
    char *retVal = NULL;

#if defined( Rtt_WIN_ENV )
    const char *lastSlash = strrchr(path, '\\');
#else
    const char *lastSlash = strrchr(path, '/');
#endif

	Rtt_ASSERT( lastSlash != NULL ); // we require full paths

    if (lastSlash != NULL)
    {
#if defined( Rtt_WIN_ENV )
        strncpy_s(buf, PATH_MAX, lastSlash+1, strlen(lastSlash+1));
#else
        strncpy(buf, lastSlash+1, PATH_MAX);
#endif

        char *lastPeriod = strrchr(buf, '.');

		Rtt_ASSERT( lastPeriod != NULL ); // we require extensions (i.e. ".lua")

        if (lastPeriod != NULL)
        {
            *lastPeriod = '\0';

            retVal = buf;
        }
    }

    return retVal;
}

const char *
TargetDevice::NameForSkin( int skinID )
{
    // Value of "deviceName"
    // This is used to iterate through all the skins
    if (skinID >= fSkinCount)
    {
        return NULL;
    }

    if (skinID < 0 || skinID >= fSkinCount)
    {
        skinID = fDefaultSkinID;
    }
    
    return fSkins[skinID]->GetName();
}

const int
TargetDevice::WidthForSkin( int skinID )
{
    if (skinID < 0 || skinID >= fSkinCount)
    {
        skinID = fDefaultSkinID;
    }

    return fSkins[skinID]->GetWidth();
}

const int
TargetDevice::HeightForSkin( int skinID )
{
    if (skinID < 0 || skinID >= fSkinCount)
    {
        skinID = fDefaultSkinID;
    }

    return fSkins[skinID]->GetHeight();
}
    
const char *
TargetDevice::CategoryForSkin( int skinID )
{
    if (skinID < 0 || skinID >= fSkinCount)
    {
        skinID = fDefaultSkinID;
    }

    return fSkins[skinID]->GetCategory();
}

TargetDevice::Skin
TargetDevice::SkinForLabel( const char* skinLabel )
{
    int result = kUnknownSkin;

	if (skinLabel != NULL)
	{
		for (int i = 0; i < fSkinCount; i++)
		{
#if defined( Rtt_WIN_ENV )
			if (_stricmp(fSkins[i]->GetLabel(), skinLabel) == 0)
#else
			if (strcasecmp(fSkins[i]->GetLabel(), skinLabel) == 0)
#endif
			{
				result = i;
				break;
			}
		}
	}

    if (result == kUnknownSkin)
    {
        Rtt_TRACE_SIM(("Warning: unknown skin label '%s'\n", skinLabel));
        
        result = fDefaultSkinID;
    }
    
	return (TargetDevice::Skin) result;
}

const char *
TargetDevice::LabelForSkin( int skinID )
{
    if (skinID < 0 || skinID >= fSkinCount)
    {
        skinID = fDefaultSkinID;
    }

    return fSkins[skinID]->GetLabel();
}

#if defined(Rtt_LINUX_ENV) && !defined(_WIN32)
static char* strcasestrForLinux(const char* s1, const char* s2)
{
	return strcasecmp(s1, s2) == 0 ? (char*) s1 : NULL;
}
#endif

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

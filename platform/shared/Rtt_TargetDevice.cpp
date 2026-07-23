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
#include <math.h>
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


const char *kDefaultSkinLabel = "720p";
    
TargetDevice::SkinSpec **TargetDevice::fSkins = NULL;
int TargetDevice::fSkinCount = 0;
TargetDevice::Skin TargetDevice::fDefaultSkinID = TargetDevice::kUnknownSkin;

#ifdef Rtt_AUTHORING_SIMULATOR
TargetDevice::DeviceDescriptor::DeviceDescriptor()
:	width( 0 ),
	height( 0 ),
	safeAreaInsetTop( 0 ),
	safeAreaInsetLeft( 0 ),
	safeAreaInsetBottom( 0 ),
	safeAreaInsetRight( 0 ),
	isProjectDevice( false )
{
}

static int
AbsoluteLuaIndex( lua_State *L, int index )
{
	return index > 0 || index <= LUA_REGISTRYINDEX ? index : lua_gettop( L ) + index + 1;
}

static bool
ReadDescriptorString(
	lua_State *L, int tableIndex, const char *key, const char *defaultValue,
	bool allowEmpty, std::string& result, std::string& errorMessage )
{
	tableIndex = AbsoluteLuaIndex( L, tableIndex );
	lua_getfield( L, tableIndex, key );
	if ( lua_isnil( L, -1 ) )
	{
		result = defaultValue ? defaultValue : "";
		lua_pop( L, 1 );
		return true;
	}

	if ( lua_type( L, -1 ) != LUA_TSTRING )
	{
		errorMessage = std::string( "'" ) + key + "' must be a string";
		lua_pop( L, 1 );
		return false;
	}

	size_t length = 0;
	const char *value = lua_tolstring( L, -1, &length );
	if ( ( !allowEmpty && 0 == length ) || strlen( value ) != length )
	{
		errorMessage = std::string( "'" ) + key + "' must be a non-empty string without null bytes";
		lua_pop( L, 1 );
		return false;
	}

	result.assign( value, length );
	lua_pop( L, 1 );
	return true;
}

static bool
ReadDescriptorInteger(
	lua_State *L, int tableIndex, const char *key, bool isRequired,
	int defaultValue, int minimum, int maximum, int& result, std::string& errorMessage )
{
	tableIndex = AbsoluteLuaIndex( L, tableIndex );
	lua_getfield( L, tableIndex, key );
	if ( lua_isnil( L, -1 ) && !isRequired )
	{
		result = defaultValue;
		lua_pop( L, 1 );
		return true;
	}

	lua_Number value = lua_type( L, -1 ) == LUA_TNUMBER ? lua_tonumber( L, -1 ) : 0.5;
	if ( value != value || floor( value ) != value || value < minimum || value > maximum )
	{
		char message[160];
		snprintf(
			message, sizeof( message ), "'%s' must be an integer between %d and %d",
			key, minimum, maximum );
		errorMessage = message;
		lua_pop( L, 1 );
		return false;
	}

	result = (int)value;
	lua_pop( L, 1 );
	return true;
}

bool
TargetDevice::LoadDeviceDescriptor(
	const char *path, DeviceDescriptor& result, std::string& errorMessage )
{
	lua_State *L = CoronaLuaNew( kCoronaLuaFlagNone );
	if ( !L )
	{
		errorMessage = "could not create a Lua state";
		return false;
	}

	int status = CoronaLuaDoFile( L, path, 0, false );
	if ( 0 != status )
	{
		errorMessage = "contains invalid Lua";
		CoronaLuaDelete( L );
		return false;
	}

	DeviceDescriptor descriptor;
	descriptor.isProjectDevice = lua_gettop( L ) > 0 && lua_istable( L, -1 );
	if ( !descriptor.isProjectDevice )
	{
		lua_getglobal( L, "simulator" );
	}
	if ( !lua_istable( L, -1 ) )
	{
		errorMessage = "must return a table or assign one to the global 'simulator'";
		CoronaLuaDelete( L );
		return false;
	}

	int descriptorIndex = AbsoluteLuaIndex( L, -1 );
	const char *defaultCategory = descriptor.isProjectDevice ? "Project" : "Untitled category";
	bool isValid =
		ReadDescriptorString(
			L, descriptorIndex, "category", defaultCategory, true,
			descriptor.category, errorMessage ) &&
		ReadDescriptorString(
			L, descriptorIndex, "deviceName", "Untitled Skin", false,
			descriptor.name, errorMessage ) &&
		ReadDescriptorString(
			L, descriptorIndex, "deviceId", NULL, false,
			descriptor.identifier, errorMessage ) &&
		ReadDescriptorInteger(
			L, descriptorIndex, "deviceWidth", true, 0, 1, 16384,
			descriptor.width, errorMessage ) &&
		ReadDescriptorInteger(
			L, descriptorIndex, "deviceHeight", true, 0, 1, 16384,
			descriptor.height, errorMessage );

	if ( isValid )
	{
		lua_getfield( L, descriptorIndex, "safeAreaInsets" );
		if ( lua_isnil( L, -1 ) )
		{
			lua_pop( L, 1 );
		}
		else if ( !lua_istable( L, -1 ) )
		{
			errorMessage = "'safeAreaInsets' must be a table";
			lua_pop( L, 1 );
			isValid = false;
		}
		else
		{
			int insetsIndex = AbsoluteLuaIndex( L, -1 );
			isValid =
				ReadDescriptorInteger(
					L, insetsIndex, "top", false, 0, 0, 16384,
					descriptor.safeAreaInsetTop, errorMessage ) &&
				ReadDescriptorInteger(
					L, insetsIndex, "left", false, 0, 0, 16384,
					descriptor.safeAreaInsetLeft, errorMessage ) &&
				ReadDescriptorInteger(
					L, insetsIndex, "bottom", false, 0, 0, 16384,
					descriptor.safeAreaInsetBottom, errorMessage ) &&
				ReadDescriptorInteger(
					L, insetsIndex, "right", false, 0, 0, 16384,
					descriptor.safeAreaInsetRight, errorMessage );
			lua_pop( L, 1 );
		}
	}

	if ( isValid &&
		( descriptor.safeAreaInsetTop + descriptor.safeAreaInsetBottom > descriptor.height ||
		  descriptor.safeAreaInsetLeft + descriptor.safeAreaInsetRight > descriptor.width ) )
	{
		errorMessage = "'safeAreaInsets' cannot exceed the device dimensions";
		isValid = false;
	}

	if ( isValid )
	{
		result = descriptor;
	}
	CoronaLuaDelete( L );
	return isValid;
}

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
        DeviceDescriptor descriptor;
        std::string errorMessage;
        if ( !LoadDeviceDescriptor( skinFiles[i], descriptor, errorMessage ) )
        {
            Rtt_LogException(
                "ERROR: Device descriptor '%s' %s",
                skinFiles[i], errorMessage.c_str() );
            continue;
        }

        TargetDevice::SkinSpec *skinSpec = new SkinSpec(
            descriptor.name.c_str(), skinFiles[i], descriptor.category.c_str(),
            descriptor.width, descriptor.height,
            descriptor.safeAreaInsetTop, descriptor.safeAreaInsetLeft,
            descriptor.safeAreaInsetBottom, descriptor.safeAreaInsetRight );
        if ( descriptor.isProjectDevice )
        {
            std::string identifier = "project:";
            identifier.append(
                descriptor.identifier.empty() ? skinSpec->GetLabel() : descriptor.identifier );
            skinSpec->SetLabel( identifier.c_str() );
        }

        if ( FindSkinForLabel( skinSpec->GetLabel() ) != kUnknownSkin )
        {
            Rtt_LogException(
                "ERROR: Device descriptor '%s' has duplicate device identifier '%s'",
                skinFiles[i], skinSpec->GetLabel() );
            delete skinSpec;
            continue;
        }

        fSkins[fSkinCount] = skinSpec;
        ++fSkinCount;
    }

    qsort(fSkins, fSkinCount, sizeof(TargetDevice::SkinSpec *), compar_SkinSpec);
    
	for (int i = 0; i < fSkinCount; i++)
    {
		// Rtt_TRACE_SIM(("TargetDevice::Initialize: skin: %d: %s = %s\n", i, fSkins[i]->GetCategory(), fSkins[i]->GetName()));
        
        // Remember the index of the platform's default skin in case we want a default later
        if (strcmp(fSkins[i]->GetLabel(), kDefaultSkinLabel) == 0)
        {
            fDefaultSkinID = (Skin) i;
            break;
        }
    }

	if ((TargetDevice::kUnknownSkin == fDefaultSkinID) && (fSkinCount > 0))
	{
		fDefaultSkinID = (Skin)0;
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
		case kNxSPlatform:
			result = "Nintendo Switch";
			break;
		default:
			Rtt_ASSERT_NOT_IMPLEMENTED();
			break;
	}

	return result;
}

static const char kAndroidPlatformTag[] = "android";
static const char kIOSPlatformTag[] = "ios";
static const char kLinuxPlatformTag[] = "linux";
static const char kWin32PlatformTag[] = "win32";
static const char kOSXPlatformTag[] = "macos";
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
			result = kOSXPlatformTag;
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
		else if ( 0 == Rtt_StringCompareNoCase( str, kOSXPlatformTag ) )
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

TargetDevice::Version
TargetDevice::VersionForPlatform( Platform platform )
{
	Version result = kUnknownVersion;

	switch ( platform )
	{
		case kAndroidPlatform:
			result = kAndroidOS4_0_3;
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

const int
TargetDevice::SafeAreaInsetTopForSkin( int skinID )
{
	return skinID >= 0 && skinID < fSkinCount ? fSkins[skinID]->GetSafeAreaInsetTop() : 0;
}

const int
TargetDevice::SafeAreaInsetLeftForSkin( int skinID )
{
	return skinID >= 0 && skinID < fSkinCount ? fSkins[skinID]->GetSafeAreaInsetLeft() : 0;
}

const int
TargetDevice::SafeAreaInsetBottomForSkin( int skinID )
{
	return skinID >= 0 && skinID < fSkinCount ? fSkins[skinID]->GetSafeAreaInsetBottom() : 0;
}

const int
TargetDevice::SafeAreaInsetRightForSkin( int skinID )
{
	return skinID >= 0 && skinID < fSkinCount ? fSkins[skinID]->GetSafeAreaInsetRight() : 0;
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
TargetDevice::FindSkinForLabel( const char* skinLabel )
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

	return (TargetDevice::Skin) result;
}

TargetDevice::Skin
TargetDevice::SkinForLabel( const char* skinLabel )
{
	TargetDevice::Skin result = FindSkinForLabel( skinLabel );

    if (result == kUnknownSkin)
    {
        Rtt_TRACE_SIM(("Warning: unknown skin label '%s'\n", skinLabel));
        
        result = fDefaultSkinID;
    }
    
	return result;
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

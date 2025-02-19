//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_TVOSDevice.h"

#include "Rtt_LuaContext.h"
#include "Rtt_MPlatform.h"

#import "Rtt_AppleCallback.h"

#import "CoronaSystemResourceManager.h"
#import "CoronaViewPrivate.h"
#include "Corona/CoronaLog.h"

#import <Foundation/NSNotification.h>
#import <Foundation/NSString.h>
#import <UIKit/UIAccelerometer.h>
#import <UIKit/UIApplication.h>
#import <UIKit/UIDevice.h>
#import <AudioToolbox/AudioServices.h>
#import <CommonCrypto/CommonDigest.h>
#import <GameController/GameController.h>

#include <sys/types.h> // for sysctlbyname
#include <sys/sysctl.h> // for sysctlbyname
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_dl.h>

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

TVOSDevice::TVOSDevice( Rtt_Allocator &allocator, CoronaView *view )
:	fAllocator( allocator ),
	fView( view ),
	fTracker(),
	fInputDeviceManager( &allocator )
{
}

TVOSDevice::~TVOSDevice()
{
	const EventType kEvents[] =
	{
		kAccelerometerEvent,
		kMultitouchEvent,
	};
	const int kEventsLen = sizeof( kEvents ) / sizeof( kEvents[0] );
	for ( int i = 0; i < kEventsLen; i++ )
	{
		EventType t = kEvents[i];
		if ( DoesNotify( t ) )
		{
			EndNotifications( t );
		}
	}
	
	fView = nil; // Weak ref, so no release.
}

const char*
TVOSDevice::GetName() const
{
	return [[[UIDevice currentDevice] name] UTF8String];
}

const char*
TVOSDevice::GetManufacturer() const
{
	return [@"Apple" UTF8String];
}

const char*
TVOSDevice::GetModel() const
{
	return [[[UIDevice currentDevice] model] UTF8String];
}

MPlatformDevice::EnvironmentType
TVOSDevice::GetEnvironment() const
{
	return kDeviceEnvironment;
}

const char*
TVOSDevice::GetPlatformName() const
{
	return [[[UIDevice currentDevice] systemName] UTF8String];
}

const char*
TVOSDevice::GetPlatform() const
{
	return [@"tvos" UTF8String];
}

const char*
TVOSDevice::GetPlatformVersion() const
{
	return [[[UIDevice currentDevice] systemVersion] UTF8String];
}

const char*
TVOSDevice::GetArchitectureInfo() const
{
	size_t size;
	sysctlbyname("hw.machine", NULL, &size, NULL, 0);
	char* machine = (char*)malloc(size);
	sysctlbyname("hw.machine", machine, &size, NULL, 0);
	/* 
	   Possible values: 
	   "TVOS1,1" = TVOS 1G 
	   "TVOS1,2" = TVOS 3G 
	   "TVOS2,1" = TVOS 3GS 
	   "iPod1,1"   = iPod touch 1G 
	   "iPod2,1"   = iPod touch 2G 
	 */  
	NSString* platform = [NSString stringWithUTF8String:machine];  

	free(machine);
	return [platform UTF8String];
}

PlatformInputDeviceManager&
TVOSDevice::GetInputDeviceManager()
{
	return fInputDeviceManager;
}

static NSString *
MD5Hash( NSString *value )
{
	const char *str = [value UTF8String];

	// Create byte array of unsigned chars
	unsigned char md5Buffer[CC_MD5_DIGEST_LENGTH];

	// Create 16 byte MD5 hash value, store in buffer
	CC_MD5( str, (CC_LONG)strlen(str), md5Buffer );

	// Convert MD5 value in the buffer to NSString of hex values
	NSMutableString *output = [NSMutableString stringWithCapacity:CC_MD5_DIGEST_LENGTH * 2];
	for(int i = 0; i < CC_MD5_DIGEST_LENGTH; i++)
	{
		[output appendFormat:@"%02x",md5Buffer[i]];
	}

	return output;
}

#define UUID_USER_DEFAULTS_KEY @"CoronaID"

static NSString *
GetApprovedIdentifier()
{
    if ([[UIDevice currentDevice] respondsToSelector:@selector(identifierForVendor)])
    {
        // Return the MD5 hash of the identifierForVendor (hashed to make it backwards compatible)
        return MD5Hash([[[UIDevice currentDevice] identifierForVendor] UUIDString] );
    }
    else
    {
        // No "identifierForVendor", return remembered UUID or generate a new one
        NSString *uuidString = nil;
        
        NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
        
        if ((uuidString = [defaults objectForKey:UUID_USER_DEFAULTS_KEY]) == nil)
        {
            // No saved UUID, generate one
            CFUUIDRef uuid = CFUUIDCreate(NULL);
            
            if (uuid)
            {
                uuidString = (NSString *)CFUUIDCreateString(NULL, uuid);
                CFRelease(uuid);
                
                [defaults setObject:uuidString forKey:UUID_USER_DEFAULTS_KEY];
                [defaults synchronize];

                [uuidString autorelease];
            }
            else
            {
                // Not much we can do
                return nil;
            }
        }
        
        return MD5Hash(uuidString);
    }
}

	
const char*
TVOSDevice::GetUniqueIdentifier( IdentifierType t ) const
{
	const char *result = NULL;

	switch ( t )
	{
        case MPlatformDevice::kDeviceIdentifier:
        case MPlatformDevice::kHardwareIdentifier:
            result = [GetApprovedIdentifier() UTF8String];
            break;
        case MPlatformDevice::kMacIdentifier:
            // Apple doesn't allow this anymore 2013-08-22: result = [GetMacAddress() UTF8String];
            break;
        case MPlatformDevice::kUdidIdentifier:
            //result = "98765432109876543210";
            break;
        case MPlatformDevice::kIOSIdentifierForVendor:
            if ([[UIDevice currentDevice] respondsToSelector:@selector(identifierForVendor)])
            {
                result = [[[[UIDevice currentDevice] identifierForVendor] UUIDString] UTF8String];
            }
            break;
		default:
			break;
	}
    
	return result;
}
	
void
TVOSDevice::Vibrate() const
{
	AudioServicesPlaySystemSound( kSystemSoundID_Vibrate );
}

bool
TVOSDevice::HasEventSource( EventType type ) const
{
	bool hasEventSource = false;
	
	switch (type)
	{
		case MPlatformDevice::kAccelerometerEvent:
		case MPlatformDevice::kKeyEvent:
		case MPlatformDevice::kInputDeviceStatusEvent:
			hasEventSource = true;
			break;
		case MPlatformDevice::kMultitouchEvent:
		case MPlatformDevice::kGyroscopeEvent:
			break;
		default:
			Rtt_ASSERT_NOT_REACHED();
			break;
	}
	return hasEventSource;
}

void
TVOSDevice::BeginNotifications( EventType type ) const
{
	fTracker.BeginNotifications( type );

	CoronaSystemResourceManager *resourceManager = [CoronaSystemResourceManager sharedInstance];

	switch( type )
	{
		case MPlatformDevice::kAccelerometerEvent:
		{
			// Accelerometer events come from controllers, e.g. AppleTV remote
			break;
		}
		case MPlatformDevice::kGyroscopeEvent:
		{
			// Current gyroscope structure is not supported as of tvOS 9.1.
			CORONA_LOG_WARNING( "Gyroscope events are not supported on this platform." );
			break;
		}
		case MPlatformDevice::kMultitouchEvent:
		{
			CORONA_LOG_WARNING( "Multitouch events are not supported on this platform." );
			break;
		}
		default:
			Rtt_ASSERT_NOT_REACHED();
			break;
	}
}

void
TVOSDevice::EndNotifications( EventType type ) const
{
	fTracker.EndNotifications( type );

	CoronaSystemResourceManager *resourceManager = [CoronaSystemResourceManager sharedInstance];

	switch( type )
	{
		case MPlatformDevice::kAccelerometerEvent:
		{
			// Accelerometer events come from controllers, e.g. AppleTV remote
			break;
		}
		case MPlatformDevice::kGyroscopeEvent:
			break;
		case MPlatformDevice::kMultitouchEvent:
			break;
		default:
			Rtt_ASSERT_NOT_REACHED();
			break;
	}
}
	
bool
TVOSDevice::Activate( ActivationType key ) const
{
	if ( key == kActivateControllerUserInteraction )
	{
		GCEventViewController* controller = (GCEventViewController*)(fView.viewController.parentViewController);
		controller.controllerUserInteractionEnabled = YES;
		return true;
	}
	
	return false;
}

bool
TVOSDevice::Deactivate( ActivationType key ) const
{
	if ( key == kActivateControllerUserInteraction )
	{
		GCEventViewController* controller = (GCEventViewController*)(fView.viewController.parentViewController);
		controller.controllerUserInteractionEnabled = NO;
		return true;
	}
		
	return false;
}

bool
TVOSDevice::DoesNotify( EventType type ) const
{
	return fTracker.DoesNotify( type );
}

void
TVOSDevice::SetAccelerometerInterval( U32 frequency ) const
{
	CORONA_LOG_WARNING( "Accelerometer intervals are not supported on this platform." );
}

void
TVOSDevice::SetGyroscopeInterval( U32 frequency ) const
{
	CORONA_LOG_WARNING( "Gyroscope intervals are not supported on this platform." );
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------


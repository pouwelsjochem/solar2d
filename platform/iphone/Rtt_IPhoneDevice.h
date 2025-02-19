//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_IPhoneDevice_H__
#define _Rtt_IPhoneDevice_H__

#include "Rtt_MPlatformDevice.h"
#include "Rtt_AppleInputDeviceManager.h"

#import <UIKit/UIApplication.h>
#import <UIKit/UIDevice.h>

// ----------------------------------------------------------------------------

@class AppleCallback;
@class CMMotionManager;
@class CoronaView;

namespace Rtt
{

class IPhonePlatform;
class MCallback;

// ----------------------------------------------------------------------------

class IPhoneDevice : public MPlatformDevice
{
	public:
		typedef MPlatformDevice Super;

	public:
		IPhoneDevice( Rtt_Allocator &allocator, CoronaView *view );
		~IPhoneDevice();

	public:
		virtual const char* GetName() const;
		virtual const char* GetManufacturer() const;
		virtual const char* GetModel() const;
		virtual const char* GetUniqueIdentifier( IdentifierType t ) const;
		virtual EnvironmentType GetEnvironment() const;
		virtual const char* GetPlatformName() const;
		virtual const char* GetPlatform() const;
		virtual const char* GetPlatformVersion() const;
		virtual const char* GetArchitectureInfo() const;
		virtual PlatformInputDeviceManager& GetInputDeviceManager();
	
	public:
		virtual void Vibrate(const char * hapticType, const char* hapticStyle) const;
		virtual void Vibrate() const { Vibrate(NULL, NULL); }

	public:
		virtual void BeginNotifications( EventType type ) const;
		virtual void EndNotifications( EventType type ) const;
		virtual bool DoesNotify( EventType type ) const;
		virtual bool HasEventSource( EventType type ) const;
		virtual void SetAccelerometerInterval( U32 frequency ) const;
		virtual void SetGyroscopeInterval( U32 frequency ) const;

	private:
		Rtt_Allocator &fAllocator;
		CoronaView *fView; // Weak ref
		DeviceNotificationTracker fTracker;
		AppleInputDeviceManager fInputDeviceManager;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_IPhoneDevice_H__

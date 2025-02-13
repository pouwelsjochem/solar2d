//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_TVOSDevice_H__
#define _Rtt_TVOSDevice_H__

#include "Rtt_MPlatformDevice.h"
#include "Rtt_AppleInputDeviceManager.h"

#import <UIKit/UIApplication.h>
#import <UIKit/UIDevice.h>

// ----------------------------------------------------------------------------

@class AppleCallback;
@class CoronaView;

namespace Rtt
{

class IPhonePlatform;
class MCallback;

// ----------------------------------------------------------------------------

class TVOSDevice : public MPlatformDevice
{
	public:
		typedef MPlatformDevice Super;

	public:
		TVOSDevice( Rtt_Allocator &allocator, CoronaView *view );
		~TVOSDevice();

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
		virtual void Vibrate() const;

	public:
		virtual void BeginNotifications( EventType type ) const;
		virtual void EndNotifications( EventType type ) const;
		virtual bool DoesNotify( EventType type ) const;
		virtual bool HasEventSource( EventType type ) const;
		virtual void SetAccelerometerInterval( U32 frequency ) const;
		virtual void SetGyroscopeInterval( U32 frequency ) const;

		virtual bool Activate( ActivationType key ) const;
		virtual bool Deactivate( ActivationType key ) const;

	public:

	private:
		Rtt_Allocator &fAllocator;
		CoronaView *fView; // Weak ref
		DeviceNotificationTracker fTracker;
		AppleInputDeviceManager fInputDeviceManager;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_TVOSDevice_H__

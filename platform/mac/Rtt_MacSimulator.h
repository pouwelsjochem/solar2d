//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_MacSimulator_H__
#define _Rtt_MacSimulator_H__

#include "Rtt_PlatformSimulator.h"
#include "Rtt_AppleInputDeviceManager.h"
#include "Rtt_AppleInputHIDDeviceListener.h"
#include "Rtt_AppleInputMFIDeviceListener.h"

// ----------------------------------------------------------------------------

@class GLView;
@class NSDictionary;
@class NSMutableDictionary;
@class NSWindow;
@class NSWindowController;
@class NSString;

namespace Rtt
{

class MacViewCallback;

// ----------------------------------------------------------------------------

class MacSimulator : public PlatformSimulator
{
	public:
		typedef PlatformSimulator Super;
		typedef MacSimulator Self;

	public:
		MacSimulator();
		virtual ~MacSimulator();

	public:
		virtual void Initialize(
			const char deviceConfigFile[],
			const char resourcePath[] );
		virtual const char *GetPlatformName() const;
		virtual const char *GetPlatform() const;

	public:
		GLView* GetScreenView() const;
		NSWindow* GetWindow() const { return (NSWindow*)fWindow; }

		bool Back();
		bool SupportsBackKey();
		virtual const char *GetOSName() const;


	public:
		NSDictionary* GetProperties() const { return (NSDictionary*)fProperties; }

	public:
		// Physical pixel w,h of the device
		float GetDeviceWidth() const { return fDeviceWidth; }
		float GetDeviceHeight() const { return fDeviceHeight; }

	public:
		virtual void WillSuspend();
		virtual void DidResume();

	private:
		NSWindow* fWindow;
		NSWindowController* fWindowController;
		NSMutableDictionary* fProperties;
		float fDeviceWidth;
		float fDeviceHeight;
		bool fSupportsBackButton;
		NSString* fDeviceSkinIdentifier; // To save the scale factor for the current skin, we need to know which skin we are on.
        NSString* fDeviceName;
		MacViewCallback *fViewCallback;
		AppleInputHIDDeviceListener *fMacHidDeviceListener;
		AppleInputMFiDeviceListener	*fMacMFIListener;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_MacSimulator_H__

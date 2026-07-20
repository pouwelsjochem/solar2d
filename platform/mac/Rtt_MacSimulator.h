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
class MacGUIPlatform;

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
			bool roundedCorners,
			const char resourcePath[] );
		void Initialize(
			const char deviceName[],
			float deviceWidth,
			float deviceHeight,
			float safeAreaInsetTop,
			float safeAreaInsetLeft,
			float safeAreaInsetBottom,
			float safeAreaInsetRight,
			bool roundedCorners,
			const char resourcePath[] );
		virtual const char *GetPlatformName() const;
		virtual const char *GetPlatform() const;

	public:
		GLView* GetScreenView() const;
		NSWindow* GetWindow() const { return (NSWindow*)fWindow; }

		bool Back();


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
		void Initialize(
			MacGUIPlatform* platform, const Super::Config& config,
			bool roundedCorners, const char resourcePath[] );

	private:
		NSWindow* fWindow;
		NSWindowController* fWindowController;
		NSMutableDictionary* fProperties;
		float fDeviceWidth;
		float fDeviceHeight;
        NSString* fDeviceName;
		MacViewCallback *fViewCallback;
		AppleInputHIDDeviceListener *fMacHidDeviceListener;
		AppleInputMFiDeviceListener	*fMacMFIListener;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_MacSimulator_H__

//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#import <AppKit/NSView.h>

#import "CoronaCards/CoronaView.h"

#import "GLView.h"
#import "GLViewDelegate.h"
#import "CoronaViewRuntimeDelegate.h"

// ----------------------------------------------------------------------------

const int CoronaViewPrivateDefaultWidth = 320;
const int CoronaViewPrivateDefaultHeight = 480;

namespace Rtt
{
	class MacPlatform;
	class MacViewCallback;
	class Runtime;
	class ProjectSettings;
    class AppleInputHIDDeviceListener;
} // Rtt

@class CoronaView;
@class AppleInputMFiDeviceListener;

// ----------------------------------------------------------------------------

@interface CoronaView() <GLViewDelegate>
{
	@private
		NSString *_projectPath;
		GLView *_GLView;
		Rtt::MacPlatform *_platform;
		Rtt::Runtime *_runtime;
		Rtt::MacViewCallback *_GLViewCallback;
		Rtt::ProjectSettings *_projectSettings;
        Rtt::AppleInputHIDDeviceListener *_macHIDInputDeviceListener;
		AppleInputMFiDeviceListener *_macMFiDeviceListener;
}

@property (nonatomic, readwrite, getter=projectPath) NSString *_projectPath;
@property (nonatomic, readonly, getter=glView) GLView *_GLView;
@property (nonatomic, readwrite, getter=runtime, setter=setRuntime:) Rtt::Runtime *_runtime;
@property (nonatomic, readonly) Rtt::MacPlatform *_platform;
@property (nonatomic, readonly) Rtt::CoronaViewRuntimeDelegate *_runtimeDelegate;
@property (nonatomic, readonly, getter=projectSettings) Rtt::ProjectSettings *_projectSettings;
@property (nonatomic, readwrite, copy) NSDictionary *_launchParams;

- (id)initWithPath:(NSString *)path frame:(NSRect)frame;

@end

// ----------------------------------------------------------------------------

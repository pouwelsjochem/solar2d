//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_MacSimulator.h"

#include "Rtt_Lua.h"
#include "Rtt_MacPlatform.h"
#include "Rtt_MacViewCallback.h"
#include "Rtt_PlatformPlayer.h"


#import "AppDelegate.h"
#import "GLView.h"
#import "SkinlessSimulatorWindow.h"

#import "CoreAnimationUtilities.h" // for shake animation
#include "Display/Rtt_Display.h"
#include "Display/Rtt_Scene.h"
#include "Rtt_Runtime.h"

#import <AppKit/NSApplication.h>
#import <AppKit/NSOpenGL.h>
#import <AppKit/NSScreen.h>

#include "Rtt_MacKeyServices.h"
#import <Carbon/Carbon.h>

#if (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_5)
	//#import <Foundation/NSNumber.h>
	#import <Foundation/NSDictionary.h>
	#import <Foundation/NSKeyValueCoding.h>
	#import <Foundation/NSString.h>
#endif

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

MacSimulator::MacSimulator()
:	Super( Super::Finalizer< MacGUIPlatform > ),
	fWindow( nil ),
	fWindowController( nil ),
	fProperties( [[NSMutableDictionary alloc] init] ),
	fDeviceWidth( 0. ),
	fDeviceHeight( 0. ),
	fDeviceSkinIdentifier( nil ),
	fViewCallback( NULL ),
	fMacMFIListener(nil),
	fMacHidDeviceListener(NULL)
{
}

MacSimulator::~MacSimulator()
{
	if (fMacMFIListener)
	{
		[fMacMFIListener stop];
		[fMacMFIListener release];
	}
	if (fMacHidDeviceListener)
	{
		fMacHidDeviceListener->StopListening();
		delete fMacHidDeviceListener;
	}

	// The NSView and CALayer have a pointer to the runtime which it uses to draw/
	// Since the drawing may be asynchonous and the view release may cause dealloc to happen
	// at the end of the runloop, we need to clear the runtime variable now.
	GLView* view = [(SkinlessSimulatorWindow *)fWindow screenView];
    if (view != nil && [view runtime] != NULL)
    {
        [view runtime]->GetDisplay().Invalidate();
        [view setRuntime:NULL];
    }


    [fWindow saveFrameUsingName:fDeviceName];
	[fWindow close];
	// Fix case 40335: Simulator crashes sometime after an app with a native textfield runs (http://bugs.coronalabs.com/default.asp?40335)
	[fWindow makeFirstResponder:nil];

	[fWindowController close];
	delete fViewCallback;

	[fDeviceSkinIdentifier release];
	[fDeviceName release];
	[fProperties release];
	[fWindow release];
	// Fix case 20368: Setting focus on native keyboard crashes simulator (http://bugs.coronalabs.com/default.asp?20368)
	fWindow = nil;
	[fWindowController release];
}

void
MacSimulator::Initialize(
	const char deviceConfigFile[],
	const char resourcePath[] )
{
	using namespace Rtt;

	MacGUIPlatform* platform = new MacGUIPlatform( * this );
	platform->SetResourcePath( resourcePath );
	Super::Config config( platform->GetAllocator() );
	Super::LoadConfig( deviceConfigFile, *platform, config );
    
    if (! config.configLoaded)
    {
        return;
    }
    
	LoadBuildSettings( *platform );

	fDeviceWidth = config.deviceWidth;
	fDeviceHeight = config.deviceHeight;

	platform->GetMacDevice().SetManufacturer( [NSString stringWithExternalString:config.displayManufacturer.GetString()] );
	platform->GetMacDevice().SetModel( [NSString stringWithExternalString:config.displayName.GetString()] );

	// -------------
	AppDelegate *delegate = (AppDelegate*)[[NSApplication sharedApplication] delegate];
	
	// Store whether or not the simulated device supports the following features.
	[fProperties setValue:[NSNumber numberWithBool:config.supportsExitRequests] forKey:@"supportsExitRequests"];
	[fProperties setValue:[NSNumber numberWithBool:config.supportsBackKey] forKey:@"supportsBackKey"];
	[fProperties setValue:[NSNumber numberWithBool:config.supportsKeyEvents] forKey:@"supportsKeyEvents"];
	[fProperties setValue:[NSNumber numberWithBool:config.supportsMouse] forKey:@"supportsMouse"];
	[fProperties setValue:[NSString stringWithExternalString:config.osName] forKey:@"osName"];

	// Store the simulated device's safe screen area
	[fProperties setValue:[NSNumber numberWithFloat:config.safeScreenInsetTop] forKey:@"safeScreenInsetTop"];
	[fProperties setValue:[NSNumber numberWithFloat:config.safeScreenInsetLeft] forKey:@"safeScreenInsetLeft"];
	[fProperties setValue:[NSNumber numberWithFloat:config.safeScreenInsetBottom] forKey:@"safeScreenInsetBottom"];
	[fProperties setValue:[NSNumber numberWithFloat:config.safeScreenInsetRight] forKey:@"safeScreenInsetRight"];
	[fProperties setValue:[NSNumber numberWithFloat:config.safeLandscapeScreenInsetTop] forKey:@"safeLandscapeScreenInsetTop"];
	[fProperties setValue:[NSNumber numberWithFloat:config.safeLandscapeScreenInsetLeft] forKey:@"safeLandscapeScreenInsetLeft"];
	[fProperties setValue:[NSNumber numberWithFloat:config.safeLandscapeScreenInsetBottom] forKey:@"safeLandscapeScreenInsetBottom"];
	[fProperties setValue:[NSNumber numberWithFloat:config.safeLandscapeScreenInsetRight] forKey:@"safeLandscapeScreenInsetRight"];
	
	const char* display_name_cstr = config.displayName.GetString();
	const char* window_titlebar_name_cstr = config.windowTitleBarName.GetString();
	
	NSString* displayName = nil;
	NSString* windowTitleBarName = nil;
    
	if ( NULL != display_name_cstr )
	{
		displayName = [NSString stringWithExternalString:display_name_cstr];
	}
	if ( NULL != window_titlebar_name_cstr )
	{
		windowTitleBarName = [NSString stringWithFormat:@"%s - %.0fx%.0f", window_titlebar_name_cstr, fDeviceWidth, fDeviceHeight];
	}

	NSRect screenRect = { {0, 0}, {fDeviceWidth, fDeviceHeight} };
	GLView* screenView = [[GLView alloc] initWithFrame:screenRect];
	[screenView autorelease];
	[screenView setDelegate:delegate];

	SimulatorDeviceWindow* instanceWindow = nil;
	void (^window_close_handler)(id) = ^(id sender)
	{
		// pass the action to the app delegate which will handle the close for us
		[NSApp sendAction:@selector(close:) to:[[NSApplication sharedApplication] delegate] from:sender];
	};	

	// Need to do this BEFORE window is set up b/c window triggers other prepareOpenGL
	platform->Initialize( screenView );

	fViewCallback = new MacViewCallback( screenView ); // This is what is on the Timer loop
	Super::Initialize( platform, fViewCallback ); // Inside here, Runtime is instantiated

	//Mac simulator needs to defer the initial update and render in separate pass
	GetPlayer()->GetRuntime().SetProperty(Runtime::kDeferUpdate, true);
	GetPlayer()->GetRuntime().SetProperty(Runtime::kRenderAsync, true);

	// Chicken and egg issue between fViewCallback and this->fRuntime
	// Runtime is not valid until Super::Initialize() but we need to pass fViewCallback to Super::Initialize.
	fViewCallback->Initialize( & GetPlayer()->GetRuntime() );

    // restore the user's last setting for this skin
	// We need a placeholder name for autosave
	fDeviceName = [displayName copy];
	
	// Use this string as our current selected skin identifier which will be used as a key to save the user's set scale factor to
	fDeviceSkinIdentifier = [windowTitleBarName copy];
	    
    instanceWindow = [[SkinlessSimulatorWindow alloc] initWithScreenView:screenView viewRect:screenRect title:windowTitleBarName];
	[(SkinlessSimulatorWindow*)instanceWindow setPerformCloseBlock:window_close_handler];

	NSWindowController *windowController = [[NSWindowController alloc] initWithWindow:instanceWindow];
	[instanceWindow setDelegate:(id <NSWindowDelegate>)windowController];
    fWindow = instanceWindow;
	fWindowController = windowController;

	Runtime& runtime = GetPlayer()->GetRuntime();
	screenView.runtime = & runtime;
	
	//Initialize Joystick support
	AppleInputDeviceManager& macDeviceManager = (AppleInputDeviceManager&)runtime.Platform().GetDevice().GetInputDeviceManager();
	if (fMacMFIListener)
	{
		[fMacMFIListener stop];
		[fMacMFIListener release];
	}
	fMacMFIListener = [[AppleInputMFiDeviceListener alloc] initWithRuntime:&runtime andDeviceManager:&macDeviceManager];
	[fMacMFIListener start];
	
	if (fMacHidDeviceListener)
	{
		fMacHidDeviceListener->StopListening();
		delete fMacHidDeviceListener;
	}
	fMacHidDeviceListener = new AppleInputHIDDeviceListener();
	fMacHidDeviceListener->StartListening(&runtime, &macDeviceManager);
	
	// -------------

    [instanceWindow setFrameUsingName:fDeviceName];
    [instanceWindow makeKeyAndOrderFront:nil];

	Rtt_TRACE_SIM( ( "Loading project from:   %s\n", [[[NSString stringWithExternalString:resourcePath] stringByAbbreviatingWithTildeInPath] UTF8String] ) );
	Rtt_TRACE_SIM( ( "Project sandbox folder: %s\n", [[platform->GetSandboxPath() stringByAbbreviatingWithTildeInPath] UTF8String] ) );
}

const char *
MacSimulator::GetPlatformName() const
{
	static const char kName[] = "mac-sim";
	return kName;
}

const char *
MacSimulator::GetPlatform() const
{
	static const char kName[] = "macos";
	return kName;
}

GLView*
MacSimulator::GetScreenView() const
{
	return [(SkinlessSimulatorWindow *)fWindow screenView];
}

bool
MacSimulator::SupportsBackKey()
{
	return [(NSNumber*)[fProperties valueForKey:@"supportsBackKey"] boolValue];
}

const char *
MacSimulator::GetOSName() const
{
	// return the name of the OS this simulator is simulating
	return [[fProperties valueForKey:@"osName"] UTF8String];
}

bool
MacSimulator::Back()
{
	Runtime& runtime = GetPlayer()->GetRuntime();
	BOOL skinSupportsBackKey = SupportsBackKey();

	if ( skinSupportsBackKey && ! runtime.IsSuspended() )
	{
		// Simulate the pressing of a virtual "back" button
		short keyCode = kVK_Back;
		NSString *keyName = [MacKeyServices getNameForKey:[NSNumber numberWithInt:keyCode]];

		KeyEvent eDown(NULL,
					   KeyEvent::kDown,
					   [keyName UTF8String],
					   keyCode,
					   false,  // (modifierFlags & NSShiftKeyMask) || (modifierFlags & NSAlphaShiftKeyMask),
					   false,  // (modifierFlags & NSAlternateKeyMask),
					   false,  // (modifierFlags & NSControlKeyMask),
					   false ); // (modifierFlags & NSCommandKeyMask) );

		runtime.DispatchEvent( eDown );

		KeyEvent eUp(NULL,
					 KeyEvent::kUp,
					 [keyName UTF8String],
					 keyCode,
					 false,  // (modifierFlags & NSShiftKeyMask) || (modifierFlags & NSAlphaShiftKeyMask),
					 false,  // (modifierFlags & NSAlternateKeyMask),
					 false,  // (modifierFlags & NSControlKeyMask),
					 false ); // (modifierFlags & NSCommandKeyMask) );

		runtime.DispatchEvent( eUp );

		// Let our caller decide what to do (e.g. if the handler for the "back" button returns false, exit the simulation)
		return eUp.GetResult();
	}

	return true;
}

void
MacSimulator::WillSuspend()
{
    GLView* layerhostview = GetScreenView();

    [layerhostview suspendNativeDisplayObjects];
}

void
MacSimulator::DidResume()
{
	GLView* layerhostview = GetScreenView();

	[layerhostview resumeNativeDisplayObjects];
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------


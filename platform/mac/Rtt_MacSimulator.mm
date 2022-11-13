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
#import "SimulatorWindow.h"

#include "Display/Rtt_Display.h"
#include "Display/Rtt_Scene.h"
#include "Rtt_Runtime.h"

#import <AppKit/NSApplication.h>
#import <AppKit/NSOpenGL.h>
#import <AppKit/NSScreen.h>

#include "Rtt_AppleKeyServices.h"
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
	GLView* view = [(SimulatorWindow *)fWindow screenView];
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
	Super::LoadConfig( deviceConfigFile, config );

	LoadBuildSettings( *platform );

	fDeviceWidth = config.deviceWidth;
	fDeviceHeight = config.deviceHeight;

	// -------------
	AppDelegate *delegate = (AppDelegate*)[[NSApplication sharedApplication] delegate];
	
	NSRect screenRect = { {0, 0}, {fDeviceWidth, fDeviceHeight} };
	GLView* screenView = [[GLView alloc] initWithFrame:screenRect];
	[screenView autorelease];
	[screenView setDelegate:delegate];

	SimulatorWindow* instanceWindow = nil;
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
	fDeviceName = [[NSString stringWithExternalString:config.deviceName] copy];
	NSString* deviceNameWithResolution = [NSString stringWithFormat:@"%s - %.0fx%.0f", config.deviceName.GetString(), fDeviceWidth, fDeviceHeight];

    instanceWindow = [[SimulatorWindow alloc] initWithScreenView:screenView viewRect:screenRect title:deviceNameWithResolution];
	[instanceWindow setPerformCloseBlock:window_close_handler];

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
	return [(SimulatorWindow *)fWindow screenView];
}

bool
MacSimulator::Back()
{
	Runtime& runtime = GetPlayer()->GetRuntime();
	if ( ! runtime.IsSuspended() )
	{
		// Simulate the pressing of a virtual "back" button
		short keyCode = kVK_Back;
		NSString *keyName = [AppleKeyServices getNameForKey:[NSNumber numberWithInt:keyCode]];

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
    GLView* coronaView = GetScreenView();

    [coronaView suspendNativeDisplayObjects:YES];
}

void
MacSimulator::DidResume()
{
	GLView* coronaView = GetScreenView();

	[coronaView resumeNativeDisplayObjects];
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------


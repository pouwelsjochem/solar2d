//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#import "SkinlessSimulatorWindow.h"

#include "Core/Rtt_Build.h"

#import <Cocoa/Cocoa.h>
#import <AvailabilityMacros.h>

#import "AppDelegate.h"
#import "GLView.h"

#include "Rtt_Event.h"
#include "Rtt_Runtime.h"

#include "Rtt_MacSimulator.h"

#ifdef Rtt_DEBUG
#include "Rtt_RenderingStream.h"
#endif

// ----------------------------------------------------------------------------

@interface SkinlessSimulatorWindow()
@end

@implementation SkinlessSimulatorWindow

@synthesize windowTitle;

// Unlike the deleted skinable window, we have both a performClose handler and close.
// performClose eventually calls close, so we don't want our callback to fire twice.
// While performClose simulates hitting the red close button on the window,
// when you hit the real red close button yourself, the performClose method never gets invoked.
// So if I put the callback in performClose, I will fail to hit the clean up case when the user hits the red button.
// And if I put the callback in the close method, I hit a multiple close cycle problem because the AppDelegate is calling on the window and controller.
// The workaround seems to be to take advantage of windowShouldClose: because it gets invoked by both performClose: and the red button, 
// but does not get invoked by calling close.
- (BOOL)windowShouldClose:(id)sender
{
	if ( nil != performCloseBlock )
	{
		performCloseBlock(sender);
	}	
	return YES;
}

- (id)initWithScreenView:(GLView*)screenView
				viewRect:(NSRect)screenRect
				   title:(NSString*)title
{
	// Need to make window size larger than the view rect (e.g. large enough to hold it with the titlebar).
	NSRect frameRect = [NSWindow frameRectForContentRect:screenRect styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable];
	self = [super initWithContentRect:frameRect
							styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable
							  backing:NSBackingStoreBuffered
							 	defer:NO];
	if ( self )
	{
		fScreenView = [screenView retain];
		fScreenRect = screenRect;
		
		// Tell OpenGL we want it to use the best resolution the current display is capable of so that we take advantage of Retina screens
		[fScreenView setWantsBestResolutionOpenGLSurface:NO];

		[self setBackgroundColor:[NSColor blackColor]];

		[self setTitle:title];
		windowTitle = [title copy];
		
		[self setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
		
		NSView* contentView = [self contentView];
		[[NSApplication sharedApplication] addWindowsItem:self title:title filename:NO];
		[screenView setFrameOrigin:NSMakePoint(0, 0)];
		[contentView addSubview:screenView];
		[self setFrame:frameRect display:NO]; //Had to add this to remove weird black bar on top
	}
	return self;
}

// Toggling NSWindowStyleMaskTitled when entering and exiting fullscreen works around
// a problem where the window would grow by the height of the title bar on each
// transition (this process is completely vanilla and no custom code is involved)
- (void)windowWillEnterFullScreen:(NSNotification *)notification
{
	NSUInteger styleMask = [self styleMask];
	styleMask &= ~NSWindowStyleMaskTitled;
	[self setStyleMask:styleMask];
}

- (void)windowWillExitFullScreen:(NSNotification *)notification
{
	NSUInteger styleMask = [self styleMask];
	styleMask |= NSWindowStyleMaskTitled;
	[self setStyleMask:styleMask];
	[self setTitle:windowTitle];
}

@end

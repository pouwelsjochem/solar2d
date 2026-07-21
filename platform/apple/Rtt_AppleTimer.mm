//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_AppleTimer.h"

#import "Rtt_AppleCallback.h"

#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>

#ifdef Rtt_MAC_ENV
	#import <AppKit/AppKit.h>
#endif

#ifdef Rtt_IPHONE_ENV
	#import <UIKit/UIDevice.h>
#endif

// For background processes, we don't want to suck up the CPU resources.
// Set the timer interval to something that is low impact.
// 1 second usually is sufficient to not be noticed on the CPU monitors.
#define RTT_NICE_BACKGROUND_TIMER_INTERVAL 3.0
// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

// Timer is optimized for rendering. To avoid screen tearing and stuttering,
// timing should synchronized with the refresh rate of the display.
// 
// * (TODO:) On Mac OS 10.4 or later, use CVDisplayLink: http://developer.apple.com/mac/library/qa/qa2004/qa1385.html
// * On iPhone OS 3.1 or later, use CADisplayLink.
// 
AppleTimer::AppleTimer( MCallback& callback )
:	Super( callback ),
	fDisplayLink( nil ),
	fTimer( nil ),
	fTarget( [[AppleCallback alloc] init] ),
	fInterval( 0x8000000 )
{
	fTarget.callback = & callback;
}

AppleTimer::~AppleTimer()
{
	Stop();
	[fTarget release];
	[fDisplayLink release];
}
	
void
AppleTimer::Start()
{
	if ( IsRunning() )
	{
		return;
	}
#ifdef Rtt_IPHONE_ENV
	fDisplayLink = [CADisplayLink displayLinkWithTarget:fTarget selector:@selector(invoke:)];
	[fDisplayLink setPreferredFramesPerSecond:( fInterval < 33 ? 60 : 30 )];
	[fDisplayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
#else
	NSTimeInterval interval = ((NSTimeInterval)fInterval) / 1000.0;
	fTimer = [NSTimer
				scheduledTimerWithTimeInterval:interval
				target:fTarget
				selector:@selector(invoke:)
				userInfo:nil
				repeats:YES];
	[fTimer retain];

#ifdef Rtt_MAC_ENV
	// For single threaded apps like this one,
	// Cocoa seems to block timers or events sometimes. This can be seen
	// when I'm animating (via a timer) and you open an popup box or move a slider.
	// Apparently, sheets and dialogs can also block (try printing).
	// To work around this, Cocoa provides different run-loop modes. I need to
	// specify the modes to avoid the blockage.
	// NSDefaultRunLoopMode seems to be the default. I don't think I need to explicitly
	// set this one, but just in case, I will set it anyway.
	[[NSRunLoop currentRunLoop] addTimer:fTimer forMode:NSDefaultRunLoopMode];
	// This seems to be the one for preventing blocking on other events (popup box, slider, etc)
	[[NSRunLoop currentRunLoop] addTimer:fTimer forMode:NSEventTrackingRunLoopMode];
	// This seems to be the one for dialogs.
	[[NSRunLoop currentRunLoop] addTimer:fTimer forMode:NSModalPanelRunLoopMode];
#endif
#endif
}

void
AppleTimer::Stop()
{
#ifdef Rtt_IPHONE_ENV
	if ( fDisplayLink )
	{
		[fDisplayLink invalidate]; // implicitly releases itself
		fDisplayLink = nil;
	}
#endif
	[fTimer invalidate];
	[fTimer release];
	fTimer = nil;
}

void
AppleTimer::SetInterval( U32 milliseconds )
{
	if ( fInterval != milliseconds )
	{
		bool wasRunning = IsRunning();

		if ( wasRunning ) { Stop(); }

		fInterval = milliseconds;

		if ( wasRunning ) { Start(); }
	}
}

bool
AppleTimer::IsRunning() const
{
	return ( ( nil != fDisplayLink ) || ( true == [fTimer isValid] ) );
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

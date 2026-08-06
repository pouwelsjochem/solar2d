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
#if ! defined( Rtt_NO_GUI )
	#import <CoreVideo/CoreVideo.h>
	#include <atomic>
#endif
#endif

#ifdef Rtt_IPHONE_ENV
	#import <UIKit/UIDevice.h>
#endif

// For background processes, we don't want to suck up the CPU resources.
// Set the timer interval to something that is low impact.
// 1 second usually is sufficient to not be noticed on the CPU monitors.
#define RTT_NICE_BACKGROUND_TIMER_INTERVAL 3.0
// ----------------------------------------------------------------------------

#if defined( Rtt_MAC_ENV ) && ! defined( Rtt_NO_GUI )

// CVDisplayLink invokes its callback on a real-time background thread. This
// target stores only the latest predicted presentation time and coalesces the
// work onto the main queue, where Solar2D and AppKit are safe to invoke.
@interface AppleDisplayLinkTarget : AppleCallback
{
	std::atomic<uint64_t> fLatestHostTime;
	std::atomic<void*> fDispatchSource;
	NSTimeInterval fInterval;
	NSTimeInterval fNextTimestamp;
}

- (id)initWithCallback:(Rtt::MCallback*)callback interval:(NSTimeInterval)milliseconds;
- (void)invoke:(id)sender;
- (void)setInterval:(NSTimeInterval)milliseconds;
- (void)resetCadence;
- (void)setDispatchSource:(dispatch_source_t)source;
- (void)displayLinkOutputAtHostTime:(uint64_t)hostTime;
- (void)dispatchSourceFired;
- (void)displayLinkFired:(CADisplayLink*)displayLink;

@end

@implementation AppleDisplayLinkTarget

- (id)initWithCallback:(Rtt::MCallback*)timerCallback interval:(NSTimeInterval)milliseconds
{
	self = [super init];
	if ( self )
	{
		self.callback = timerCallback;
		fLatestHostTime.store( 0 );
		fDispatchSource.store( NULL );
		fInterval = milliseconds / 1000.0;
		fNextTimestamp = 0.0;
	}
	return self;
}

- (void)invoke:(id)sender
{
	// A coalesced main-queue tick can outlive AppleTimer. Stop() clears the
	// callback first, making that final delivery a harmless no-op.
	if ( self.callback )
	{
		[super invoke:sender];
	}
}

- (void)setInterval:(NSTimeInterval)milliseconds
{
	fInterval = milliseconds / 1000.0;
	fNextTimestamp = 0.0;
}

- (void)resetCadence
{
	fNextTimestamp = 0.0;
}

- (void)setDispatchSource:(dispatch_source_t)source
{
	fDispatchSource.store( (void*)source );
}

- (void)displayLinkOutputAtHostTime:(uint64_t)hostTime
{
	fLatestHostTime.store( hostTime, std::memory_order_relaxed );
	dispatch_source_t source = (dispatch_source_t)fDispatchSource.load( std::memory_order_acquire );
	if ( source )
	{
		dispatch_source_merge_data( source, 1 );
	}
}

- (void)invokeForTargetTimestamp:(NSTimeInterval)targetTimestamp
{
	if ( targetTimestamp <= 0.0 || fInterval <= 0.0 )
	{
		return;
	}

	// The display link always supplies a presentation-aligned timestamp. Keep
	// the configured logic cadence anchored to those timestamps, skipping work
	// instead of queuing catch-up ticks if the main thread falls behind.
	const NSTimeInterval kTolerance = 0.0005;
	if ( fNextTimestamp > 0.0 && targetTimestamp + kTolerance < fNextTimestamp )
	{
		return;
	}

	[self invoke:nil];

	if ( fNextTimestamp <= 0.0 || targetTimestamp - fNextTimestamp > fInterval * 4.0 )
	{
		fNextTimestamp = targetTimestamp + fInterval;
	}
	else
	{
		do
		{
			fNextTimestamp += fInterval;
		}
		while ( fNextTimestamp <= targetTimestamp + kTolerance );
	}
}

- (void)dispatchSourceFired
{
	uint64_t hostTime = fLatestHostTime.load( std::memory_order_relaxed );
	double hostFrequency = CVGetHostClockFrequency();
	if ( hostTime > 0 && hostFrequency > 0.0 )
	{
		[self invokeForTargetTimestamp:(NSTimeInterval)( hostTime / hostFrequency )];
	}
}

- (void)displayLinkFired:(CADisplayLink*)displayLink
{
	[self invokeForTargetTimestamp:displayLink.targetTimestamp];
}

@end

// ----------------------------------------------------------------------------

// CVDisplayLink is the compatibility path for macOS 10.15 through 13 and a
// fallback if a view-bound display link cannot be created. The NSView-bound
// CADisplayLink used on macOS 14+ automatically follows a window between displays.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

static CVReturn
AppleDisplayLinkOutputCallback(
	CVDisplayLinkRef displayLink,
	const CVTimeStamp* currentTime,
	const CVTimeStamp* outputTime,
	CVOptionFlags flagsIn,
	CVOptionFlags* flagsOut,
	void* context )
{
	AppleDisplayLinkTarget* target = (AppleDisplayLinkTarget*)context;
	uint64_t hostTime = ( outputTime->flags & kCVTimeStampHostTimeValid )
		? outputTime->hostTime
		: CVGetCurrentHostTime();
	[target displayLinkOutputAtHostTime:hostTime];
	return kCVReturnSuccess;
}

#endif // Rtt_MAC_ENV && ! Rtt_NO_GUI

namespace Rtt
{

// ----------------------------------------------------------------------------

// Timer is optimized for rendering. To avoid screen tearing and stuttering,
// timing should synchronized with the refresh rate of the display.
// 
// * On macOS 14 or later, use NSView's CADisplayLink. Earlier macOS versions
//   use CVDisplayLink.
// * On iPhone OS 3.1 or later, use CADisplayLink.
// 
AppleTimer::AppleTimer( MCallback& callback )
:	Super( callback ),
	fDisplayLink( nil ),
	fTimer( nil ),
	fTarget( [[AppleCallback alloc] init] ),
	fInterval( 0x8000000 )
#if defined( Rtt_MAC_ENV ) && ! defined( Rtt_NO_GUI )
	,fView( nil ),
	fMacDisplayLink( NULL ),
	fDisplaySource( NULL ),
	fDisplayTarget( nil ),
	fScreenObserver( nil )
#endif
{
	fTarget.callback = & callback;
}

#if defined( Rtt_MAC_ENV ) && ! defined( Rtt_NO_GUI )
AppleTimer::AppleTimer( MCallback& callback, NSView* view )
:	AppleTimer( callback )
{
	fView = [view retain];
}
#endif

AppleTimer::~AppleTimer()
{
	Stop();
#if defined( Rtt_MAC_ENV ) && ! defined( Rtt_NO_GUI )
	[fView release];
#endif
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
#if defined( Rtt_MAC_ENV ) && ! defined( Rtt_NO_GUI )
	if ( fView && StartMacDisplayLink() )
	{
		return;
	}
#endif
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
#if defined( Rtt_MAC_ENV ) && ! defined( Rtt_NO_GUI )
	StopMacDisplayLink();
#endif
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
	SetInterval( (double)milliseconds );
}

void
AppleTimer::SetInterval( double milliseconds )
{
	if ( fInterval != milliseconds )
	{
#if defined( Rtt_MAC_ENV ) && ! defined( Rtt_NO_GUI )
		if ( fDisplayTarget )
		{
			fInterval = milliseconds;
			ApplyMacDisplayLinkInterval();
			return;
		}
#endif
		bool wasRunning = IsRunning();

		if ( wasRunning ) { Stop(); }

		fInterval = milliseconds;

		if ( wasRunning ) { Start(); }
	}
}

bool
AppleTimer::IsRunning() const
{
	return ( ( nil != fDisplayLink ) ||
#if defined( Rtt_MAC_ENV ) && ! defined( Rtt_NO_GUI )
			 ( nil != fDisplayTarget ) ||
#endif
			 ( true == [fTimer isValid] ) );
}

#if defined( Rtt_MAC_ENV ) && ! defined( Rtt_NO_GUI )

bool
AppleTimer::StartMacDisplayLink()
{
	AppleDisplayLinkTarget* target = [[AppleDisplayLinkTarget alloc]
		initWithCallback:&Callback() interval:fInterval];

	if ( @available( macOS 14.0, * ) )
	{
		CADisplayLink* displayLink = [fView
			displayLinkWithTarget:target selector:@selector(displayLinkFired:)];
		if ( displayLink )
		{
			fDisplayTarget = target;
			fDisplayLink = [displayLink retain];
			ApplyMacDisplayLinkInterval();

			NSRunLoop* runLoop = [NSRunLoop currentRunLoop];
			[fDisplayLink addToRunLoop:runLoop forMode:NSDefaultRunLoopMode];
			[fDisplayLink addToRunLoop:runLoop forMode:NSEventTrackingRunLoopMode];
			[fDisplayLink addToRunLoop:runLoop forMode:NSModalPanelRunLoopMode];
			return true;
		}
	}

	dispatch_source_t source = dispatch_source_create(
		DISPATCH_SOURCE_TYPE_DATA_ADD, 0, 0, dispatch_get_main_queue() );
	CVDisplayLinkRef displayLink = NULL;
	if ( source &&
		 kCVReturnSuccess == CVDisplayLinkCreateWithActiveCGDisplays( & displayLink ) &&
		 kCVReturnSuccess == CVDisplayLinkSetOutputCallback(
			displayLink, AppleDisplayLinkOutputCallback, target ) )
	{
		fDisplayTarget = target;
		fDisplaySource = (void*)source;
		fMacDisplayLink = (void*)displayLink;
		[target setDispatchSource:source];

		dispatch_source_set_event_handler( source, ^{
			[target dispatchSourceFired];
		} );
		dispatch_resume( source );

		UpdateMacDisplayLinkScreen();

		NSWindow* window = [fView window];
		AppleTimer* timer = this;
		fScreenObserver = [[[NSNotificationCenter defaultCenter]
			addObserverForName:NSWindowDidChangeScreenNotification
			object:window
			queue:nil
			usingBlock:^(NSNotification* notification)
			{
				timer->UpdateMacDisplayLinkScreen();
			}] retain];

		if ( kCVReturnSuccess == CVDisplayLinkStart( displayLink ) )
		{
			return true;
		}

		StopMacDisplayLink();
		return false;
	}

	if ( displayLink )
	{
		CVDisplayLinkRelease( displayLink );
	}
	if ( source )
	{
		dispatch_source_cancel( source );
		dispatch_release( source );
	}
	[target release];
	return false;
}

void
AppleTimer::StopMacDisplayLink()
{
	if ( ! fDisplayTarget )
	{
		return;
	}

	fDisplayTarget.callback = NULL;

	if ( fScreenObserver )
	{
		[[NSNotificationCenter defaultCenter] removeObserver:fScreenObserver];
		[fScreenObserver release];
		fScreenObserver = nil;
	}

	if ( fMacDisplayLink )
	{
		CVDisplayLinkRef displayLink = (CVDisplayLinkRef)fMacDisplayLink;
		CVDisplayLinkStop( displayLink );
		CVDisplayLinkRelease( displayLink );
		fMacDisplayLink = NULL;
	}

	[(AppleDisplayLinkTarget*)fDisplayTarget setDispatchSource:NULL];
	if ( fDisplaySource )
	{
		dispatch_source_t source = (dispatch_source_t)fDisplaySource;
		dispatch_source_cancel( source );
		dispatch_release( source );
		fDisplaySource = NULL;
	}

	if ( fDisplayLink )
	{
		[fDisplayLink invalidate];
		[fDisplayLink release];
		fDisplayLink = nil;
	}

	[fDisplayTarget release];
	fDisplayTarget = nil;
}

void
AppleTimer::ApplyMacDisplayLinkInterval()
{
	AppleDisplayLinkTarget* target = (AppleDisplayLinkTarget*)fDisplayTarget;
	[target setInterval:fInterval];

	if ( @available( macOS 14.0, * ) )
	{
		if ( fDisplayLink && fInterval > 0.0 )
		{
			float framesPerSecond = (float)( 1000.0 / fInterval );
			[(CADisplayLink*)fDisplayLink setPreferredFrameRateRange:
				CAFrameRateRangeMake( framesPerSecond, framesPerSecond, framesPerSecond )];
		}
	}
}

void
AppleTimer::UpdateMacDisplayLinkScreen()
{
	if ( ! fMacDisplayLink )
	{
		return;
	}

	NSScreen* screen = [[fView window] screen];
	if ( ! screen )
	{
		screen = [NSScreen mainScreen];
	}
	NSNumber* screenNumber = [[screen deviceDescription] objectForKey:@"NSScreenNumber"];
	if ( ! screenNumber )
	{
		return;
	}

	CVDisplayLinkRef displayLink = (CVDisplayLinkRef)fMacDisplayLink;
	bool wasRunning = CVDisplayLinkIsRunning( displayLink );
	if ( wasRunning )
	{
		CVDisplayLinkStop( displayLink );
	}

	CVDisplayLinkSetCurrentCGDisplay(
		displayLink, (CGDirectDisplayID)[screenNumber unsignedIntValue] );
	[(AppleDisplayLinkTarget*)fDisplayTarget resetCadence];

	if ( wasRunning )
	{
		CVDisplayLinkStart( displayLink );
	}
}

#pragma clang diagnostic pop

#endif // Rtt_MAC_ENV && ! Rtt_NO_GUI

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

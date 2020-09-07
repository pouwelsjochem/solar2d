//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#import <AppKit/NSWindow.h>

#include "Core/Rtt_Build.h"

@class GLView;

// ----------------------------------------------------------------------------

@interface SimulatorDeviceWindow : NSWindow<NSWindowDelegate>
{
	GLView* fScreenView;
	void (^performCloseBlock)(id);
	NSRect fScreenRect; // Original rect
}

@property(nonatomic, readwrite, copy) NSString *saveFrameName;

@property(nonatomic,readonly,getter=screenView) GLView *fScreenView;

- (void) setPerformCloseBlock:(void (^)(id sender))block;

@end

// ----------------------------------------------------------------------------


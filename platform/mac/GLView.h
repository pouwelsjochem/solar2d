//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#import <AppKit/AppKit.h>
#import "GLViewDelegate.h"

//#import <AppKit/NSView.h>

#include "Core/Rtt_Types.h"

namespace Rtt
{
	class ApplePlatform;
	class Runtime;
} 

@class NSImage;
@class SPILDTopLayerView;

@interface GLView : NSOpenGLView
{
	Rtt::Runtime* fRuntime;
	NSPoint fStartPosition;
	float fFirstClickTime;
	NSTimeInterval fTapDelay;

	id< GLViewDelegate > fDelegate;
	U8 fNumTaps;

	NSRect nativeFrameRect; // currently only settable via initWithFrame:
	SPILDTopLayerView* suspendedOverlay;
    BOOL isReady;
	BOOL shouldInvalidate;
    NSMutableArray *fCursorRects;
	NSTrackingRectTag trackingRectTag;
	int numCursorHides;
#if Rtt_AUTHORING_SIMULATOR
	float lastTouchPressure;
	NSPoint lastPressurePoint;
#endif
}

@property (nonatomic, readwrite, getter=runtime, setter=setRuntime:) Rtt::Runtime *fRuntime;
@property (nonatomic, readwrite, getter=tapDelay, setter=setTapDelay:) NSTimeInterval fTapDelay;
@property (nonatomic, assign) CGFloat backingScaleFactor;
@property (nonatomic, assign) BOOL isReady;
@property (nonatomic, assign) BOOL sendAllMouseEvents;
@property (nonatomic, assign) BOOL inFullScreenTransition;
@property (nonatomic, assign) BOOL isResizable;
@property (nonatomic, assign) BOOL allowOverlay;
@property (nonatomic, assign) BOOL cursorHidden;
@property (nonatomic, assign) NSPoint initialLocation;

+ (NSOpenGLPixelFormat*) basicPixelFormat;

- (void)setDelegate:(id< GLViewDelegate >)delegate;

// This method will force the Corona OpenGL renderer to redraw everything.
- (void) invalidate;
- (void) restoreWindowProperties;

- (void)adjustPoint:(NSPoint*)p;
- (NSPoint)pointForEvent:(NSEvent*)event;

- (CGFloat) viewportWidth;
- (CGFloat) viewportHeight;

- (CGFloat)deviceWidth;
- (CGFloat)deviceHeight;

- (void) suspendNativeDisplayObjects:(bool) showOverlay;
- (void) resumeNativeDisplayObjects;

- (void) setCursor:(const char *) cursorName forRect:(NSRect) bounds;
- (void) hideCursor;
- (void) showCursor;
@end

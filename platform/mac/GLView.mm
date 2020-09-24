//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#import "GLView.h"
#include <OpenGL/gl.h>

#import <AppKit/NSApplication.h>
#import <AppKit/NSEvent.h>
#import <AppKit/NSOpenGL.h>
#import <AppKit/AppKit.h>
#import <Carbon/Carbon.h>

#ifdef Rtt_DEBUG
#define NSDEBUG(...) // NSLog(__VA_ARGS__)
#else
#define NSDEBUG(...)
#endif

#if (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_5)
	// From <Foundation/NSRunLoop.h>
	@interface NSObject (NSDelayedPerforming)

	- (void)performSelector:(SEL)aSelector withObject:(id)anArgument afterDelay:(NSTimeInterval)delay inModes:(NSArray *)modes;
	- (void)performSelector:(SEL)aSelector withObject:(id)anArgument afterDelay:(NSTimeInterval)delay;
	+ (void)cancelPreviousPerformRequestsWithTarget:(id)aTarget selector:(SEL)aSelector object:(id)anArgument;
	#if MAC_OS_X_VERSION_10_2 <= MAC_OS_X_VERSION_MAX_ALLOWED
	+ (void)cancelPreviousPerformRequestsWithTarget:(id)aTarget;
	#endif

	@end
#endif

#include "Rtt_AppleBitmap.h"
#include "Rtt_Event.h"

#include "Rtt_Runtime.h"
#include "Rtt_MPlatformDevice.h"
#include "Rtt_MacPlatform.h"
#include "Rtt_Display.h"
#import "SPILDTopLayerView.h"

#include "Display/Rtt_Display.h"
#include "Renderer/Rtt_Renderer.h"
#include "Rtt_RenderingStream.h"

#include "Rtt_MacKeyServices.h"

// So we can build with Xcode 8.0
#ifndef NSAppKitVersionNumber10_12
#define NSAppKitVersionNumber10_12 1504
#endif

// Container for cursor rects
@interface CursorRect : NSObject
{
}
@property (nonatomic, readwrite) NSRect rect;
@property (nonatomic, readwrite, retain) NSCursor *cursor;

- (id) initWithRect:(NSRect) rect cursor:(NSCursor *) cursor;

@end

@implementation CursorRect

- (id) initWithRect:(NSRect) aRect cursor:(NSCursor *) aCursor;
{
	self = [super init];
    
	if ( self )
	{
        _rect = aRect;
        _cursor = aCursor;
	}
    
	return self;
}

@end


@interface GLView ()
- (void)dispatchEvent:(Rtt::MEvent*)event;
@end

@implementation GLView

@synthesize fRuntime;
@synthesize isReady;
@synthesize sendAllMouseEvents;
@synthesize inFullScreenTransition;
@synthesize isResizable;
@synthesize allowOverlay;
@synthesize cursorHidden;
@synthesize initialLocation;

// pixel format definition
+ (NSOpenGLPixelFormat*) basicPixelFormat
{
    NSOpenGLPixelFormatAttribute attributes [] = {
        NSOpenGLPFANoRecovery,
		NSOpenGLPFADoubleBuffer,	// double buffered
        NSOpenGLPFADepthSize, (NSOpenGLPixelFormatAttribute)16, // 16 bit depth buffer
		NSOpenGLPFASampleBuffers, (NSOpenGLPixelFormatAttribute)1,
		NSOpenGLPFASamples,(NSOpenGLPixelFormatAttribute)4,
		
        (NSOpenGLPixelFormatAttribute)0
    };
    return [[[NSOpenGLPixelFormat alloc] initWithAttributes:attributes] autorelease];
}

- (void) setRuntime:(Rtt::Runtime *)runtime
{
	fRuntime = runtime;
}

- (id)initWithFrame:(NSRect)frameRect
{
    NSDEBUG(@"GLView: initWithFrame: %@", NSStringFromRect(frameRect));
	NSOpenGLPixelFormat * pf = [GLView basicPixelFormat];

	self = [super initWithFrame: frameRect pixelFormat: pf];
	
	if ( self )
	{
		isReady = NO;

		fRuntime = NULL;
		fDelegate = nil;
        fCursorRects = [[NSMutableArray alloc] initWithCapacity:18];

		sendAllMouseEvents = YES;
        inFullScreenTransition = NO;
        allowOverlay = YES; // this can be set to NO externally to disallow graphically showing the
                            // suspended state (e.g. when the Shift key is down)
		
		nativeFrameRect = frameRect;

		// It seems we need to set wantsLayer on macOS 10.12 or native display objects don't appear
		// (we avoid it on earlier versions because it has performance issues with OpenGL views)
		if (NSAppKitVersionNumber >= NSAppKitVersionNumber10_12)
		{
			[self setWantsLayer:YES];
		}

		// We're looking for a 10.9 API call to determine if we need to invalidate
		shouldInvalidate = [[NSApplication sharedApplication] respondsToSelector:@selector(occlusionState)];

		cursorHidden = NO;
		numCursorHides = 0;
	}
	
	return self;
}

- (void)dealloc
{
	[self setLayer:nil];
	
	fRuntime = NULL; // Don't delete. We do not own this pointer
    
    [fCursorRects release];

	[super dealloc];
}
- (void) reshape
{	
	[super reshape];
	
}
- (void) prepareOpenGL
{
    NSDEBUG(@"XXX: GLView: prepareOpenGL: fRuntime %p, self.isReady %s", fRuntime, (self.isReady ? "YES" : "NO"));
	//[super prepareOpenGL];

	[[self openGLContext] makeCurrentContext];

	Rtt::Display *display = NULL;

	if ( fRuntime != NULL && fRuntime->IsProperty(Rtt::Runtime::kIsApplicationLoaded) )
	{
		display = static_cast<Rtt::Display*>(&fRuntime->GetDisplay());
		// NSDEBUG(@"Deciding to call display->GetRenderer().ReleaseGPUResources(): display %p, GetRenderer() %p", display, (&display->GetRenderer()));
        if ( display != NULL && (&display->GetRenderer()) != NULL )
		{
			// NSDEBUG(@"Calling display->GetRenderer().ReleaseGPUResources(): display %p, GetRenderer() %p", display, (&display->GetRenderer()));
			// FIXME: this crashes in release builds because (&display->GetRenderer()) is NULL but the test above succeeds
			display->GetRenderer().ReleaseGPUResources();
		}
	}

	if (self.isReady == NO)
	{
		glClearColor( 0.0, 0.0, 0.0, 1.0 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
//        [[self openGLContext] flushBuffer];
		
		self.isReady = YES;
		
		[fDelegate didPrepareOpenGLContext:self];
	}

	if ( fRuntime != NULL && fRuntime->IsProperty(Rtt::Runtime::kIsApplicationLoaded) )
	{
		if ( display )
		{
			display->GetRenderer().Initialize();
			[self invalidate];
		}
	}
}

- (void)drawRect:(NSRect)rect
{
    if ([self inLiveResize] || self.inFullScreenTransition)
    {
        // This fixes nasty OpenGL painting artifacts when live resizing
		[self invalidate];
    }
	else if (shouldInvalidate)
	{
		// This turns out to be lightweight b/c setNeedsDisplay is called by the timer *only*
		// when the Scene has already been invalidated. We invalidate here b/c drawRect
		// can also be called by the OS in situations like dragging between multiple monitors.
		[self invalidate];
	}

	[[self openGLContext] makeCurrentContext];

	// This should be called by the layer, not NSTimer!!!
	// That's b/c the OGL context is valid and ready for new OGL commands
	if ( isReady && fRuntime != NULL && fRuntime->IsProperty(Rtt::Runtime::kIsApplicationLoaded))
	{
		fRuntime->Render();
	}
    
    [[self openGLContext] flushBuffer];
}

- (void)setDelegate:(id< GLViewDelegate >)delegate
{
	fDelegate = delegate;
}

// This method will force the Corona OpenGL renderer to redraw everything.
- (void) invalidate
{
	if ( fRuntime )
	{
		fRuntime->GetDisplay().Invalidate();
	}

	[self update];
}

- (BOOL) isOpaque
{
	return YES;
}

// Provides a slight performance benefit
- (BOOL) wantsDefaultClipping
{
	return NO;
}

// Set origin at upper left corner
- (BOOL)isFlipped
{
	return YES;
}

- (void) dispatchMouseEvent:(Rtt::MouseEvent::MouseEventType) eventType event:(NSEvent *) event
{
	using namespace Rtt;

    NSPoint p = [self pointForEvent:event];

    NSUInteger modifierFlags = [event modifierFlags];
    NSUInteger mouseButtons = [NSEvent pressedMouseButtons];
    const NSUInteger kLeftMask = 1 << 0;
    const NSUInteger kRightMask = 1 << 1;
    const NSUInteger kMiddleMask = 1 << 2;

	int clickCount = (int)event.clickCount;

    // Create the Corona mouse event
    MouseEvent mouseEvent(eventType,
                          p.x, p.y,
                          0, 0, clickCount,
                          (mouseButtons & kLeftMask),
                          (mouseButtons & kRightMask),
                          (mouseButtons & kMiddleMask),
                          (modifierFlags & NSShiftKeyMask),
                          (modifierFlags & NSAlternateKeyMask),
                          (modifierFlags & NSControlKeyMask),
                          (modifierFlags & NSCommandKeyMask) );

    [self dispatchEvent: ( & mouseEvent )];
}

- (void)dispatchEvent:(Rtt::MEvent*)e
{
	using namespace Rtt;

	// Since we defer the loading of the application in [self prepareOpenGL] we shouldn't dispatch
	// any events until it really is loaded or when the runtime is suspended
	if ( fRuntime == NULL || ! fRuntime->IsProperty(Rtt::Runtime::kIsApplicationLoaded) ||
		 fRuntime->IsSuspended())
	{
		return;
	}

	Runtime* runtime = self.runtime;
	Rtt_ASSERT( runtime );

	if ( Rtt_VERIFY( e ) )
	{
		runtime->DispatchEvent( * e );
	}
}

// TODO: This function needs to be kept in sync with PlatformSimulator::AdjustPoint(),
// and should eventually call it directly.
- (void)adjustPoint:(NSPoint*)p
{
	using namespace Rtt;

	p->x = roundf( p->x );
	p->y = roundf( p->y );
}

- (NSPoint)pointForEvent:(NSEvent*)event
{
	NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];

	[self adjustPoint:&p];
	return p;
}

static U32 *sTouchId; // any arbitrary pointer value will do

- (void)rightMouseDown:(NSEvent*)event
{
    using namespace Rtt;
    
    NSDEBUG( @"rightMouseDown: %@", event );
    
	[self dispatchMouseEvent:MouseEvent::kDown event:event];
}

- (void)rightMouseUp:(NSEvent*)event
{
    using namespace Rtt;
    
    NSDEBUG( @"rightMouseUp: %@", event );
    
	[self dispatchMouseEvent:MouseEvent::kUp event:event];
}

- (void)rightMouseDragged:(NSEvent*)event
{
    using namespace Rtt;
    
    // NSDEBUG( @"rightMouseDragged: %@", event );

	[self dispatchMouseEvent:MouseEvent::kDrag event:event];
}

- (void)mouseDown:(NSEvent*)event
{
    using namespace Rtt;

	NSPoint p = [self pointForEvent:event];
    
	// Send mouse event before the touch
	[self dispatchMouseEvent:MouseEvent::kDown event:event];
    
	fStartPosition = p;

	TouchEvent t( p.x, p.y, p.x, p.y, TouchEvent::kBegan );
	t.SetId( sTouchId );
	if ( fRuntime->Platform().GetDevice().DoesNotify( MPlatformDevice::kMultitouchEvent ) )
	{
		MultitouchEvent t2( &t, 1 );
		[self dispatchEvent: (&t2)];
	}
	else
	{
		[self dispatchEvent: (&t)];
	}
}

- (void)mouseDragged:(NSEvent*)event
{
	using namespace Rtt;

	// NSDEBUG( @"mouseDragged: %@", event );

	NSPoint p = [self pointForEvent:event];

	// Send mouse event before the touch
	[self dispatchMouseEvent:MouseEvent::kDrag event:event];

	TouchEvent t( p.x, p.y, fStartPosition.x, fStartPosition.y, TouchEvent::kMoved );
	t.SetId( sTouchId );
	if ( fRuntime->Platform().GetDevice().DoesNotify( MPlatformDevice::kMultitouchEvent ) )
	{
		MultitouchEvent t2( &t, 1 );
		[self dispatchEvent: (&t2)];
	}
	else
	{
		[self dispatchEvent: (&t)];
	}

	DragEvent e( fStartPosition.x, fStartPosition.y, p.x, p.y );
	[self dispatchEvent: (&e)];
}

- (void)mouseUp:(NSEvent*)event
{
	using namespace Rtt;

	// NSDEBUG( @"mouseUp: %@", event );

	// Send mouse event before the touch
	[self dispatchMouseEvent:MouseEvent::kUp event:event];
    
	NSPoint p = [self pointForEvent:event];

	TouchEvent t( p.x, p.y, fStartPosition.x, fStartPosition.y, TouchEvent::kEnded );
	t.SetId( sTouchId++ );
	if ( fRuntime->Platform().GetDevice().DoesNotify( MPlatformDevice::kMultitouchEvent ) )
	{
		MultitouchEvent t2( &t, 1 );
		[self dispatchEvent: (&t2)];
	}
	else
	{
		[self dispatchEvent: (&t)];
	}

//	NSDEBUG( @"mouseUp(%g,%g)", p.x, p.y );
}

- (NSRect) frame
{
    return [super frame];
}

- (void) setFrameSize:(NSSize)new_size
{
	BOOL sizeChanged = ! NSEqualSizes(new_size, nativeFrameRect.size);

	NSDEBUG(@"GLView:setFrameSize: newSize %@, frame %@ (isResizable %s, sizeChanged %s, isReady %s, fRuntime %s)",
		  NSStringFromSize(new_size), NSStringFromRect([self frame]),
		  (self.isResizable ? "YES" : "NO"),
		  (sizeChanged ? "YES" : "NO"),
		  (self.isReady ? "YES" : "NO"),
		  (fRuntime != NULL ? "YES" : "NO") );
	//NSDEBUG(@"GLView:setFrameSize: old %@, new %@", NSStringFromSize([self frame].size), NSStringFromSize(new_size));

	nativeFrameRect.size = new_size;

	[super setFrameSize:new_size];

	// Update rectangle used for mouseMoved: events
	[self removeTrackingRect:trackingRectTag];
	trackingRectTag = [self addTrackingRect:[self bounds] owner:self userData:nil assumeInside:NO];

	if (sizeChanged && self.isReady && self.runtime != NULL )
	{
		Rtt::Display *display = NULL;
		display = static_cast<Rtt::Display*>(&fRuntime->GetDisplay());

		if ( display )
		{
            display->WindowSizeChanged();
            display->Restart();
		}

		self.runtime->DispatchEvent( Rtt::ResizeEvent() );
	}

	// Prevents occasional GL buffer flashes
	[self invalidate];
}

- (CGFloat)deviceWidth
{
	// Rtt_TRACE(("deviceWidth: %g\n", nativeFrameRect.size.width ));
    return nativeFrameRect.size.width;
}

- (CGFloat)deviceHeight
{
	// Rtt_TRACE(("deviceHeight: %g\n", nativeFrameRect.size.height ));
    return nativeFrameRect.size.height;
}

- (BOOL) acceptsFirstResponder
{
	return YES;
}

- (void) suspendNativeDisplayObjects
{
#if Rtt_AUTHORING_SIMULATOR
    if (self.allowOverlay)
    {
        if ( nil == suspendedOverlay )
        {
            NSSize size = [self frame].size;
            suspendedOverlay = [[SPILDTopLayerView alloc] initWithFrame:NSMakeRect(0, 0, size.width, size.height)];
            [[suspendedOverlay progressIndicatorLayer] setAnimationTimeScaleFactor:8.0];
            [[suspendedOverlay progressIndicatorLayer] setColor:[NSColor orangeColor]];
            [self addSubview:suspendedOverlay];
			[[self window] makeFirstResponder:suspendedOverlay];
            [[suspendedOverlay progressIndicatorLayer] startProgressAnimation];
        }
    }
#endif // Rtt_AUTHORING_SIMULATOR
}

- (void) resumeNativeDisplayObjects
{
#if Rtt_AUTHORING_SIMULATOR
    if ( nil != suspendedOverlay )
    {
        [[suspendedOverlay progressIndicatorLayer] stopProgressAnimation];
        [suspendedOverlay removeFromSuperview];
        [suspendedOverlay release];
        suspendedOverlay = nil;
    }
#endif // Rtt_AUTHORING_SIMULATOR
}

// keyDown and keyUp do not trigger modifier key events (shift, control, etc.)
- (void)flagsChanged:(NSEvent *)event
{
    unsigned long mask = [MacKeyServices getModifierMaskForKey:[event keyCode]];

    // After certain actions, like using the screenshot tool, MacOS apparently triggers the "a" key event. Can't imagine anyone would like this event.
    if ( [event keyCode] != kVK_ANSI_A )
    {
        // The mask contains a few bits set. All must be set to consider the key down.
        if ( ( [event modifierFlags] & mask ) == mask)
        {
            [self dispatchKeyEvent:event withPhase:Rtt::KeyEvent::kDown];
        }
        else
        {
            [self dispatchKeyEvent:event withPhase:Rtt::KeyEvent::kUp];
        }
    }
}

- (void)keyDown:(NSEvent *)event
{
	[self dispatchKeyEvent:event withPhase:Rtt::KeyEvent::kDown];
	const char* characters = [[event characters] UTF8String];
	if (strlen(characters) > 1 || isprint(characters[0])) {
		Rtt::CharacterEvent e(NULL, characters);
		[self dispatchEvent: ( & e )];
	}
}

- (void)keyUp:(NSEvent *)event
{
	[self dispatchKeyEvent:event withPhase:Rtt::KeyEvent::kUp];
}

- (void)dispatchKeyEvent:(NSEvent *)event withPhase:(Rtt::KeyEvent::Phase)phase
{
	using namespace Rtt;
	
	NSUInteger modifierFlags = [event modifierFlags];
	unsigned short keyCode = [event keyCode];
	NSString *keyName = [MacKeyServices getNameForKey:[NSNumber numberWithInt:keyCode]];
	
	KeyEvent e(
			   NULL,
			   phase,
			   [keyName UTF8String],
			   keyCode,
			   (modifierFlags & NSShiftKeyMask) || (modifierFlags & NSAlphaShiftKeyMask),
			   (modifierFlags & NSAlternateKeyMask),
			   (modifierFlags & NSControlKeyMask),
			   (modifierFlags & NSCommandKeyMask) );
	[self dispatchEvent: ( & e )];
}

- (void)viewDidMoveToWindow
{
	// We may have called addTrackingRect: in a setFrame: call before we get here
	[self removeTrackingRect:trackingRectTag];

	// Limit mouse events to the view's bounds
	NSRect r = [self bounds];
	trackingRectTag = [self addTrackingRect:r owner:self userData:nil assumeInside:NO];
}

- (void)mouseMoved:(NSEvent *)event
{
	using namespace Rtt;

	if ( sendAllMouseEvents )
	{
        // NSDEBUG( @"mouseMoved: %@", event );
		NSPoint p = [self pointForEvent:event];
        NSUInteger modifierFlags = [event modifierFlags];

		// Raise the mouse event.
		// When button is down, mouseDragged: is called instead,
		// so pass false for button states
		MouseEvent e(MouseEvent::kMove, p.x, p.y, 0, 0, 0, false, false, false,
                     (modifierFlags & NSShiftKeyMask),
                     (modifierFlags & NSAlternateKeyMask),
                     (modifierFlags & NSControlKeyMask),
                     (modifierFlags & NSCommandKeyMask) );
		[self dispatchEvent: ( & e )];
	}
}

- (void)mouseEntered:(NSEvent *)event
{
	NSDEBUG(@"mouseEntered: cursorHidden %s (%d)", (cursorHidden ? "YES" : "NO"), numCursorHides);
	// Start dispatching mouseMoved: events
	// NSDEBUG( @"mouseEntered: %@", event );
	[[self window] setAcceptsMouseMovedEvents:YES];

	if (cursorHidden)
	{
		NSDEBUG(@"mouseEntered: actually hiding");
		[NSCursor hide];
		++numCursorHides;
	}
}

- (void)mouseExited:(NSEvent *)event
{
	NSDEBUG(@"mouseExited: cursorHidden %s (%d)", (cursorHidden ? "YES" : "NO"), numCursorHides);
	// Stop dispatching mouseMoved: events
	// NSDEBUG( @"mouseExited: %@", event );
	[[self window] setAcceptsMouseMovedEvents:NO];

	if (cursorHidden)
	{
		for (int i = 0; i < numCursorHides; i++)
		{
			[NSCursor unhide];
		}
		numCursorHides = 0;
		[NSCursor unhide];  // TODO: figure out why one more unhide is necessary for reliability
	}
}

- (void)scrollWheel:(NSEvent *)event
{
    using namespace Rtt;
    
	// NSDEBUG( @"scrollWheel: %@", event );
	NSPoint p = [self pointForEvent:event];
	NSUInteger modifierFlags = [event modifierFlags];

	// Raise the mouse event
	// The sign of the deltas is the opposite of what is expected so they are swapped
	MouseEvent e(MouseEvent::kScroll, p.x, p.y, -([event deltaX]), -([event deltaY]), 0, false, false, false,
				 (modifierFlags & NSShiftKeyMask),
				 (modifierFlags & NSAlternateKeyMask),
				 (modifierFlags & NSControlKeyMask),
				 (modifierFlags & NSCommandKeyMask) );

	[self dispatchEvent: ( & e )];
}

-(void)resetCursorRects
{
    // NSDEBUG(@"resetCursorRects: %@", fCursorRects);
    
    for (CursorRect *cr in fCursorRects)
    {
        [self addCursorRect:cr.rect cursor:cr.cursor];
    }
}

-(void) setCursor:(const char *) cursorName forRect:(NSRect) bounds
{
    // NSDEBUG(@"GLView:setCursor: %@", NSStringFromRect(bounds));

    NSCursor *cursor = [NSCursor currentSystemCursor];

    if (strcasecmp(cursorName, "arrow") == 0)
    {
        cursor = [NSCursor arrowCursor];
    }
    else if (strcasecmp(cursorName, "closedHand") == 0)
    {
        cursor = [NSCursor closedHandCursor];
    }
    else if (strcasecmp(cursorName, "openHand") == 0)
    {
        cursor = [NSCursor openHandCursor];
    }
	else if (strcasecmp(cursorName, "pointingHand") == 0)
    {
        cursor = [NSCursor pointingHandCursor];
    }
    else if (strcasecmp(cursorName, "crosshair") == 0)
    {
        cursor = [NSCursor crosshairCursor];
    }
    else if (strcasecmp(cursorName, "notAllowed") == 0)
    {
        cursor = [NSCursor operationNotAllowedCursor];
    }
	else if (strcasecmp(cursorName, "beam") == 0)
    {
        cursor = [NSCursor IBeamCursor];
    }
	else if (strcasecmp(cursorName, "resizeRight") == 0)
    {
        cursor = [NSCursor resizeRightCursor];
    }
	else if (strcasecmp(cursorName, "resizeLeft") == 0)
    {
        cursor = [NSCursor resizeLeftCursor];
    }
	else if (strcasecmp(cursorName, "resizeLeftRight") == 0)
    {
        cursor = [NSCursor resizeLeftRightCursor];
    }
	else if (strcasecmp(cursorName, "resizeUp") == 0)
    {
        cursor = [NSCursor resizeUpCursor];
    }
	else if (strcasecmp(cursorName, "resizeDown") == 0)
    {
        cursor = [NSCursor resizeDownCursor];
    }
	else if (strcasecmp(cursorName, "resizeUpDown") == 0)
    {
        cursor = [NSCursor resizeUpDownCursor];
    }
	else if (strcasecmp(cursorName, "disappearingItem") == 0)
    {
        cursor = [NSCursor disappearingItemCursor];
    }
	else if (strcasecmp(cursorName, "beamHorizontal") == 0)
    {
        cursor = [NSCursor IBeamCursorForVerticalLayout];
    }
	else if (strcasecmp(cursorName, "dragLink") == 0)
    {
        cursor = [NSCursor dragLinkCursor];
    }
	else if (strcasecmp(cursorName, "dragCopy") == 0)
    {
        cursor = [NSCursor dragCopyCursor];
    }
	else if (strcasecmp(cursorName, "contextMenu") == 0)
    {
        cursor = [NSCursor contextualMenuCursor];
    }
    else
    {
		// Remove any rect with these bounds
		int currIdx = 0;
		for (CursorRect *cr in fCursorRects)
		{
			if (NSEqualRects(cr.rect, bounds))
			{
				[fCursorRects removeObjectAtIndex:currIdx];
				[self.window invalidateCursorRectsForView:self];
				break;
			}
			++currIdx;
		}
		return;
	}

    [fCursorRects addObject:[[[CursorRect alloc] initWithRect:bounds cursor:cursor] autorelease]];
	[self.window invalidateCursorRectsForView:self];
}

- (void) hideCursor
{
	NSDEBUG(@"hideCursor: cursorHidden %s (%d)", (cursorHidden ? "YES" : "NO"), numCursorHides);

	if (! cursorHidden)
	{
		// We need to deal with whether the cursor is inside our window when
		// the API is called because we only want the cursor to not be visible
		// when inside the Corona window (its state when going in and out of
		// the window is handled by the mouseEntered/Exited handlers above)
		NSPoint screenPoint = [NSEvent mouseLocation];
		NSRect screenRect = NSMakeRect(screenPoint.x, screenPoint.y, 0, 0);
		NSRect baseRect = [self.window convertRectFromScreen:screenRect];
		NSPoint point = [self convertPoint:baseRect.origin fromView:nil];

		if ([self mouse:point inRect:[self bounds]])
		{
			NSDEBUG(@"hideCursor: actually hiding");

			[NSCursor hide];

			++numCursorHides;
		}

		cursorHidden = YES;
	}
}

- (void) showCursor
{
	NSDEBUG(@"showCursor: cursorHidden %s (%d)", (cursorHidden ? "YES" : "NO"), numCursorHides);

	// Various combinations of things (like whether the mouse cursor happens to be on
	// the area of screen where an app's window appears on startup) can make the matching
	// of NSCursor hides and unhides problematic so we track how many hides we do and
	// make sure to do that many unhides
	for (int i = 0; i < numCursorHides; i++)
	{
		[NSCursor unhide];
	}
	numCursorHides = 0;
	[NSCursor unhide];  // TODO: figure out why one more unhide is necessary for reliability

	cursorHidden = NO;
}

// Fix the view layering when the app is hidden or minaturized
// Fixes bug http://bugs.coronalabs.com/default.asp?44953
- (void) restoreWindowProperties
{
	NSArray* subviews = [self subviews];

	for (NSView* displayview in subviews)
	{
		if ([displayview respondsToSelector:@selector(setWantsLayer:)])
		{
			// Toggle wantsLayer off and on again
			[displayview setWantsLayer:NO];
			[displayview setWantsLayer:YES];
		}
	}
}

@end

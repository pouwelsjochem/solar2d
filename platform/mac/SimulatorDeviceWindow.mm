//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#import "SimulatorDeviceWindow.h"
#import "GLView.h"

#if Rtt_DEBUG
#define NSDEBUG(...) // NSLog(__VA_ARGS__)
#else
#define NSDEBUG(...) // NSLog(__VA_ARGS__)
#endif

@implementation SimulatorDeviceWindow

@synthesize saveFrameName;
@synthesize fScreenView;

- (void)dealloc
{
	[[NSApplication sharedApplication] removeWindowsItem:self];
	[fScreenView removeFromSuperview];
	// Fix bug #18412 (performs scheduled to move textfield focus were crashing on reload)
	[NSObject cancelPreviousPerformRequestsWithTarget:self];
	[fScreenView release];
	[saveFrameName release];
	[performCloseBlock release];
	[super dealloc];
}

// On 10.10, [NSWindow saveFrameUsingName:] saves the screen dimensions for neither [NSScreen frame]
// nor [NSScreen visibleFrame] (it's some other related but different size) making identifying the
// screen on reload difficult.  Here we just save [NSScreen frame] with the window coordinates thus
// overriding whatever the OS is doing and gaining consistency
- (void)saveFrameUsingName:(NSString *)frameName
{
    NSString *windowLocPrefName = [NSString stringWithFormat:@"NSWindow Frame %@", frameName];
    NSRect frame = [self frame];
    NSRect screenFrame = [[self screen] frame];

    NSString *windowLocPrefValue = [NSString stringWithFormat:@"%.0f %.0f %.0f %.0f %.0f %.0f %.0f %.0f ",
                                    frame.origin.x,frame.origin.y, frame.size.width, frame.size.height,
                                    screenFrame.origin.x, screenFrame.origin.y, screenFrame.size.width, screenFrame.size.height];

    [[NSUserDefaults standardUserDefaults] setObject:windowLocPrefValue forKey:windowLocPrefName];
}

// On 10.10 this [NSWindow orderWindow:] forces the window onto the same display as the app's main window regardless of
// the window's actual screen position.  Setting the window's position offscreen is enough to prevent this.
- (void)orderWindow:(NSWindowOrderingMode)orderingMode relativeTo:(NSInteger)otherWindowNumber
{
    NSDEBUG(@"orderWindow orderingMode %ld, otherWindowNumber %ld", (long)orderingMode, (long)otherWindowNumber);
    NSDEBUG(@"orderWindow: BEFORE frame %@", NSStringFromRect([self frame]));

    NSRect currFrame = [self frame];
    NSRect origFrame = currFrame;
    currFrame.origin.x = -5000;
    currFrame.origin.y = -5000;
    [super setFrame:currFrame display:NO];
    [super orderWindow:orderingMode relativeTo:otherWindowNumber];

    [super setFrame:origFrame display:NO];
    NSDEBUG(@"orderWindow: AFTER  frame %@", NSStringFromRect([self frame]));
}

//
// On 10.9 (Mavericks) Apple added a Mission Control setting called "Displays have separate Spaces" which,
// aside from allocating separate Spaces to each display, implements various rules that it feels apps should
// follow.  One of these is window locality: all app windows should be on the same display unless moved to
// another by the user *in this session*.  The upshot of this is that newly created windows will be created
// on which ever display the OS feels is the principal one for the app.  In the Simulator this means that if
// you put the app window on a different display it will be moved back to the "principal" display whenever
// the window is created (which happens on application launch and relaunches of the project).  Luckily all
// we need to do to get the old, desired behavior of having the app window appear wherever it was last put is to
// position the window using the saved position from the user preferences but avoiding NSWindow's methods that
// are there to parse and set the preference value (i.e. setFrameUsingName: and setFrameFromString:).  To keep
// things as simple as possible, we revert to the default behavior unless the "Displays have separate Spaces"
// system preference is set (it's interesting that the capability to query this value exists as it's of little
// interest to an application except for cases like this).
//
// Sample window frame pref value:
// "NSWindow Frame iPhone5.png" = "2001 389 195 95 1920 -280 1440 900 ";
//
- (BOOL) setFrameUsingName:(NSString *)frameName
{
    // Remember this frame's name for later in case we need to resave when the window moves
    saveFrameName = [frameName copy];
    [self setDelegate:self];

    BOOL result = NO;
    NSString *windowLocPref = [[NSUserDefaults standardUserDefaults] objectForKey:[NSString stringWithFormat:@"NSWindow Frame %@", frameName]];

    if (([windowLocPref length] > 0) &&
        ([NSScreen respondsToSelector:@selector(screensHaveSeparateSpaces)] && [NSScreen screensHaveSeparateSpaces]))
    {
        NSRect windowFrame;
        NSRect screenFrame;
        NSRect currFrame = [self frame];
        
        NSDEBUG(@"setFrameUsingName: screensHaveSeparateSpaces! %@: %@", frameName, windowLocPref);
        
        // Parse the preference value with a regular expression (it's a series of optionally-signed integers separated by white space)
        NSError *error = nil;
        NSRegularExpression *windowLocRegex = [NSRegularExpression regularExpressionWithPattern:@"([-]*\\d+)\\s+"
                                                                                        options:NSRegularExpressionCaseInsensitive
                                                                                          error:&error];
        NSArray *matches = [windowLocRegex matchesInString:windowLocPref
                                                   options:0
                                                     range:NSMakeRange(0, [windowLocPref length])];
        
        // There must be 8 captures for us to parse this pref correctly
        Rtt_ASSERT([matches count] == 8);
        if ([matches count] == 8)
        {
            // This is ugly but it merely continues the inherent knowledge of the format of the preference value
            windowFrame.origin.x = [[windowLocPref substringWithRange:[[matches objectAtIndex:0] range]] intValue];
            windowFrame.origin.y = [[windowLocPref substringWithRange:[[matches objectAtIndex:1] range]] intValue];
            windowFrame.size.width = currFrame.size.width;
            windowFrame.size.height = currFrame.size.height; 
            
            screenFrame.origin.x = [[windowLocPref substringWithRange:[[matches objectAtIndex:4] range]] intValue];
            screenFrame.origin.y = [[windowLocPref substringWithRange:[[matches objectAtIndex:5] range]] intValue];
            screenFrame.size.width = [[windowLocPref substringWithRange:[[matches objectAtIndex:6] range]] intValue];
            screenFrame.size.height = [[windowLocPref substringWithRange:[[matches objectAtIndex:7] range]] intValue];

            if (NSEqualRects(screenFrame, NSZeroRect))
            {
                // Sometimes a zero screen rect gets saved to the preferences, ignore anything saved for this device
                [self center];
                result = YES;
            }
            else
            {
                // Only attempt to use this saved position if we have an existing screen that matches the one it was on
                // when the window's position was saved
                for (NSScreen *screen in [NSScreen screens])
                {
                    // NSDEBUG(@"screen: %@", [screen deviceDescription]);
                    NSDEBUG(@"setFrameUsingName: [screen frame] %@; [screen visibleFrame] %@; window's screenFrame %@",
                            NSStringFromRect([screen frame]), NSStringFromRect([screen visibleFrame]), NSStringFromRect(screenFrame));

                    if (NSEqualRects([screen frame], screenFrame))
                    {
                        // Set the window's size and position ourselves
                        NSDEBUG(@"BEFORE: self frame: %@", NSStringFromRect([self frame]));
                        [self setFrame:windowFrame display:YES];
                        NSDEBUG(@"AFTER:  self frame: %@", NSStringFromRect([self frame]));
                        result = YES;

                        break;
                    }
                }
            }
        }
    }

     // If we haven't successfully restored the frame yet and there's a saved preference, do it the old fashioned way
    if (! result && [windowLocPref length] > 0)
    {
        result = [super setFrameUsingName:frameName];
    }

    return result;
}

// Pin the window's top-left corner during resizing (scaling)
- (void) setFrame:(NSRect)frameRect display:(BOOL)flag
{
    NSRect screenRect = [[self screen] frame];

    // Happens if they somehow move the window offscreen and on first display
    if (NSEqualRects(screenRect, NSZeroRect))
    {
        // Figure out which screen the window will be on
        for (NSScreen *screen in [NSScreen screens])
        {
            if (NSPointInRect(frameRect.origin, [screen frame]))
            {
                screenRect = [screen frame];
                break;
            }
        }
        
        // Last resort
        if (NSEqualRects(screenRect, NSZeroRect))
        {
            screenRect = [[NSScreen mainScreen] frame];
        }
    }

	[super setFrame:frameRect display:flag];
}

- (BOOL) canBecomeKeyWindow
{
	return YES;
}

- (BOOL) canBecomeMainWindow
{
	return YES;
}

- (void) setPerformCloseBlock:(void (^)(id sender))block
{
	[performCloseBlock release];
	performCloseBlock = [block copy];
}

- (void)windowDidMove:(NSNotification *)notification
{
    // We moved the window, save the new position so that it's remembered if we are suddenly terminated
    // (this is necessary because we override the default window position restoration process to get the
    // behavior we want, see saveFrameUsingName:)
    if (saveFrameName != nil)
    {
        [self saveFrameUsingName:saveFrameName];
    }
}

@end

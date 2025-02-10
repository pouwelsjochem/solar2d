//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#import "CoronaCards/CoronaView.h"
#import "AppDelegate.h"

// #define  Rtt_DEBUG  1

#ifdef Rtt_DEBUG
#define NSDEBUG(...) // NSLog(__VA_ARGS__)
#else
#define NSDEBUG(...)
#endif

@interface AppDelegate ()

@property (weak) IBOutlet NSWindow *window;
@property (nonatomic, readwrite, copy) CoronaView *coronaView;
@property (nonatomic, readwrite, copy) NSString *appPath;
@property (nonatomic, readwrite, assign) BOOL suspendWhenMinimized;
@property (nonatomic, readwrite, assign) BOOL lastSentWindowStateForeground;
@end


@implementation AppDelegate

@synthesize appPath = _appPath;
@synthesize coronaView = _coronaView;
@synthesize suspendWhenMinimized = _suspendWhenMinimized;
@synthesize lastSentWindowStateForeground;

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app
{
	return NO;
}

- (void)sentWindowForegroundEvent:(BOOL)foreground
{
	if(foreground!=self.lastSentWindowStateForeground) {
		self.lastSentWindowStateForeground = foreground;
		NSDictionary *event = @{ @"name" : @"windowState",
						 @"phase" : foreground?@"foreground":@"background" };

		[_coronaView sendEvent:event];
	}
}

// FIXME: Shouldn't need to surface GLView here
-(void)didPrepareOpenGLContext:(id)sender
{
	fGLView = sender;
}
- (id) layerHostView
{
	return fGLView;
}

- (void)awakeFromNib
{
	self.lastSentWindowStateForeground = true;
	
	[super awakeFromNib];
    
 	[_window setDelegate:self];

	_coronaView = [_window contentView];
	
	_appPath = [[self getProjectURL:nil] path];
	if ([[_appPath lastPathComponent] isEqualToString:@"main.lua"] || [[_appPath lastPathComponent] isEqualToString:@"main.lu"])
	{
		_appPath = [_appPath stringByDeletingLastPathComponent];
	}

	// Default to not showing the "Enter Full Screen" menu item until we know whether we allow fullscreen
	[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"NSFullScreenMenuItemEverywhere"];
	if (([NSEvent modifierFlags] & NSDeviceIndependentModifierFlagsMask) & NSShiftKeyMask)
	{
		NSLog(@"Resetting window position to defaults");
		[NSWindow removeFrameUsingName:_appPath];
	}

	NSDEBUG(@"AppPath: %@", _appPath);

	[[_window windowController] setShouldCascadeWindows:NO];      // Tell the controller to not cascade its windows
	[_window setFrameAutosaveName:_appPath];

	// Set the application icon for the project if it has one
	NSImage *appIcon = [[NSImage alloc] initWithContentsOfFile:[_appPath stringByAppendingPathComponent:@"Icon-osx.icns"]];
	[NSApp setApplicationIconImage:appIcon];

	// init launchargs
	NSArray *argv = [[NSProcessInfo processInfo] arguments];
	NSMutableDictionary *launchArgs = nil;
	BOOL hasPSNArg = ([argv count] > 1 && [[argv objectAtIndex:1] hasPrefix:@"-psn_"]);

	// Sometimes Finder adds a "-psn..." parameter (OS X version dependent)
	if ([argv count] > (hasPSNArg ? 2 : 1))
	{
		NSInteger firstUsefulArg = (hasPSNArg ? 2 : 1);
		NSIndexSet *indices = [[NSIndexSet alloc] initWithIndexesInRange:NSMakeRange(firstUsefulArg, [argv count] - (hasPSNArg ? 2 : 1))];

		launchArgs = [[NSMutableDictionary alloc] init];
		[launchArgs setValue:[argv objectsAtIndexes:indices] forKey:@"args"];
	}

	// Start the Corona app
	[_coronaView runWithPath:_appPath parameters:launchArgs];

	// Listen for "open URL" Apple Events (which will be sent to us if the Info.plist is
	// customized with a "CFBundleURLTypes" section)
	// NOTE: if CoronaShell has already put up an open file dialog to choose an app then
	// the event which caused CoronaShell to start running will be lost
	[[NSAppleEventManager sharedAppleEventManager]
		setEventHandler:self
			andSelector:@selector(handleURLEvent:withReplyEvent:)
		  forEventClass:kInternetEventClass
		     andEventID:kAEGetURL];

	// Make the default window background black which helps in full screen
	[_window setBackgroundColor:NSColor.blackColor];
	
	else
	{
		[_window setBackgroundColor:NSColor.clearColor];
	}
	
    _suspendWhenMinimized = [_coronaView settingsSuspendWhenMinimized];
    
    // Make the window full screen capable (this is always done because the
    // app may set fullscreen in code)
    NSUInteger windowCollectionBehavior = [_window collectionBehavior];
	windowCollectionBehavior |= NSWindowCollectionBehaviorFullScreenPrimary;
    [_window setCollectionBehavior:windowCollectionBehavior];

	NSString *windowTitle = [_coronaView settingsWindowTitle];
	if (windowTitle == nil)
	{
        if ([_appPath hasSuffix:@"/Resources/Corona"])
        {
            // We're built into an application bundle, use the app's name
            windowTitle = [[[[NSBundle mainBundle] bundlePath] lastPathComponent] stringByDeletingPathExtension];
        }
        else
        {
            // We're running a project out of a folder, use the folder name
            windowTitle = [[_appPath lastPathComponent] stringByDeletingPathExtension];
        }
	}
	[_window setTitle:windowTitle];

	// If we don't find a saved size and position for the window, set those up
	if (! [_window setFrameUsingName:_appPath force:YES])
	{
		int defaultWidth = [_coronaView settingsDefaultWindowViewWidth];
		int defaultHeight = [_coronaView settingsDefaultWindowViewHeight];

		NSRect customFrame;
		customFrame.size = NSMakeSize(defaultWidth, defaultHeight);

		// Position the window appropriately on the screen (we don't use [NSWindow center] because
		// that forces a display of the window which can mess up things like fullscreen transitions)
		NSRect screenRect = [[NSScreen mainScreen] visibleFrame];
		customFrame.origin.x = (screenRect.size.width - customFrame.size.width) / 2;
		customFrame.origin.y = (screenRect.size.height - customFrame.size.height) / 1.2;
		customFrame.size.height += 22;

		[_window setFrame:customFrame display:NO];
	}

	CoronaViewWindowMode windowMode = [_coronaView settingsDefaultWindowMode];

	BOOL storedFullscreenMode = [[NSUserDefaults standardUserDefaults] boolForKey:@"solar2D_storedFullscreenMode"];
	if (storedFullscreenMode)
	{
		windowMode = kFullscreen;
	}

	switch (windowMode)
	{
		case kNormal:
			[self setWindowStyles];
			break;
		
		case kFullscreen:
			[_window toggleFullScreen:nil];
			break;
	}

	// NSDEBUG(@"awakeFromNib: view: %@ (%@)", _coronaView, NSStringFromRect([_coronaView frame]));
}

- (void) setWindowStyles
{
	NSUInteger windowStyleMask = [_window styleMask];

	if ([_coronaView settingsIsWindowCloseButtonEnabled])
	{
		// Make the window closeable
		windowStyleMask |= NSWindowStyleMaskClosable;
	}

	if ([_coronaView settingsIsWindowMinimizeButtonEnabled])
	{
		// Make the window minimizable
		windowStyleMask |= NSWindowStyleMaskMiniaturizable;
	}

	// This triggers a window resize
	[_window setStyleMask:windowStyleMask];

	// Make the window not maximizable
	NSButton *zoomButton = [_window standardWindowButton:NSWindowZoomButton];
	[zoomButton setEnabled:NO];
}

// Displaying modal sheets causes the NSWindowZoomButton zoom button to re-enable if
// we've disabled it so we force it to the state we want here
- (NSRect)window:(NSWindow *)window willPositionSheet:(NSWindow *)sheet usingRect:(NSRect)rect
{
	NSButton *zoomButton = [_window standardWindowButton:NSWindowZoomButton];
	[zoomButton setEnabled:NO];

	return rect;
}

// Displaying modal sheets causes the NSWindowZoomButton zoom button to re-enable if
// we've disabled it so we force it to the state we want here
- (void)windowDidEndSheet:(NSNotification *)notification
{
	NSButton *zoomButton = [_window standardWindowButton:NSWindowZoomButton];
	[zoomButton setEnabled:NO];
}

- (void)windowDidResize:(NSNotification *)notification
{
	NSRect contentRect = [_window contentRectForFrameRect:[_window frame]];
	contentRect.origin = NSZeroPoint;
	[_coronaView setFrame:contentRect];
}

// Providing this method causes runtime errors to offer a "Quit" button
- (void) terminate:(id) sender
{
    [_coronaView terminate]; // Makes system events fire
    
    [NSApp terminate:sender];
}

// Close the application when the window is closed
- (void) windowWillClose:(id) sender
{
    [self terminate:sender];
}

// The following three members manage fullscreen transitions which we customize to avoid
// white flashes of uninitialized windows
- (NSSize)window:(NSWindow *)window willUseFullScreenContentSize:(NSSize)proposedSize
{
	// NSDEBUG(@"willUseFullScreenContentSize: %@", NSStringFromSize(proposedSize));
	return proposedSize;
}

- (NSArray *)customWindowsToEnterFullScreenForWindow:(NSWindow *)window
{
	if ([_window isEqual:window])
	{
		return [NSArray arrayWithObject:window];
	}

	return nil;
}

- (void)window:(NSWindow *)window startCustomAnimationToEnterFullScreenWithDuration:(NSTimeInterval)duration
{
	NSScreen *screen = [window screen];
	NSRect screenFrame = [screen frame];
	_nonFullscreenWindowFrame = [window frame];

	NSRect endFrame = screenFrame;
	endFrame.size = screenFrame.size;
	endFrame.size = [self window:window willUseFullScreenContentSize:endFrame.size];
	endFrame.origin.x += floor((NSWidth(screenFrame) - NSWidth(endFrame))/2);
	endFrame.origin.y += floor((NSHeight(screenFrame) - NSHeight(endFrame))/2);
	[window setFrame:endFrame display:YES];
}

- (NSArray *)customWindowsToExitFullScreenForWindow:(NSWindow *)window
{
	if ([_window isEqual:window])
	{
		return [NSArray arrayWithObject:window];
	}

	return nil;
}

- (void)window:(NSWindow *)window startCustomAnimationToExitFullScreenWithDuration:(NSTimeInterval)duration
{
	NSScreen *screen = [window screen];
	NSRect screenFrame = [screen frame];
	NSRect endFrame = _nonFullscreenWindowFrame;
	endFrame.size.height = fmax(endFrame.size.height, screenFrame.size.height / 4);
	endFrame.size.width = fmax(endFrame.size.width, screenFrame.size.width / 4);
	[window setFrame:endFrame display:YES];
}

- (void)windowDidEnterFullScreen:(NSNotification *)notification
{
	[self setWindowStyles];
}

- (void) notifyRuntimeError:(NSString *) mesg
{
	NSLog(@"notifyRuntimeError: %@", mesg);
}

- (void)applicationWillResignActive:(NSNotification *)notification
{
	[self sentWindowForegroundEvent:false];
}

- (void) applicationDidBecomeActive:(NSNotification *)notification
{
	[self sentWindowForegroundEvent:true];
}


- (void) performPause:(id) sender
{
	// NSDEBUG(@"performPause: %@", sender);
	NSMenuItem *item = (NSMenuItem *) sender;
	NSString *menuItemTitle = ([[item title] isEqualToString:@"Pause"]) ? @"Resume" : @"Pause";

	NSDictionary *event = @{ @"otherKey" : @"otherValue",
							 @"name" : @"pauseEvent",
							 @"phase" : @"pause" };

    [_coronaView sendEvent:event];

	if ([[item title] isEqualToString:@"Pause"])
	{
		[_coronaView suspend];
	}
	else
	{
		[_coronaView resume];
	}

	[item setTitle:menuItemTitle];
}

- (void) windowDidMiniaturize:(NSNotification *)notification
{
	[self sentWindowForegroundEvent:false];

    if (_suspendWhenMinimized)
    {
        [_coronaView suspend];
    }
}

- (void)windowDidDeminiaturize:(NSNotification *)notification
{
	[self sentWindowForegroundEvent:true];

    if (_suspendWhenMinimized)
    {
        [_coronaView resume];
    }

	[_coronaView restoreWindowProperties];
}

// Called when the window moves to a screen with different "backing properties" (i.e. retina to non-retina and vice versa)
- (void)windowDidChangeBackingProperties:(NSNotification *)notification
{
	NSDEBUG(@"+++ windowDidChangeBackingProperties: %@; screen %@: %g", NSStringFromRect([_window frame]), [[[_window screen] deviceDescription] objectForKey:@"NSDeviceSize"], [_window backingScaleFactor]);

	[_coronaView setScaleFactor:[_window backingScaleFactor]];
}

// This notification serves as a way to tell that the window is on a screen and
// that we can reliably query the screen's backingScaleFactor
- (void)windowDidChangeOcclusionState:(NSNotification *)notification
{
	if (_window.occlusionState & NSWindowOcclusionStateVisible)
	{
		[_coronaView setScaleFactor:[_window backingScaleFactor]];

		[_coronaView restoreWindowProperties];
	}
}

-(BOOL)hideHelpMenuItem
{
	return ![[NSBundle mainBundle] objectForInfoDictionaryKey:@"DeveloperURL"];
}

// Show the developer's website when the help menu item is chosen
- (void)showHelp:(id)sender
{
	NSString *devURLStr = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"DeveloperURL"];
	NSURL * helpFile = [NSURL URLWithString:devURLStr];
	[[NSWorkspace sharedWorkspace] openURL:helpFile];
}

- (NSURL *) getProjectURL:(NSURL *) startURL
{
	NSString *projectPath = [[NSUserDefaults standardUserDefaults] stringForKey:@"project"];

	// If we got a project on the command line, return that
	if (projectPath)
	{
		return [NSURL URLWithString:[[projectPath stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding] stringByExpandingTildeInPath]];
	}

	// If there's a project embedded in our bundle, return that
	projectPath = [[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"Corona"] stringByExpandingTildeInPath];

	BOOL isDir = NO;
	if ( [[NSFileManager defaultManager] fileExistsAtPath:projectPath isDirectory:&isDir] && isDir )
	{
		return [NSURL URLWithString:[projectPath stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
	}

	// Ask the user which project to open
	NSOpenPanel* panel = [NSOpenPanel openPanel];
	[panel setAllowsMultipleSelection:NO];
	[panel setCanChooseDirectories:YES];
	[panel setCanChooseFiles:NO];

	if (startURL != nil)
	{
		[panel setDirectoryURL:startURL];
	}

	NSInteger result = [panel runModal];
	[panel close];
	if (result == NSFileHandlingPanelOKButton)
	{
		return [panel directoryURL];
	}
	else
	{
		return nil;
	}
}

// Handle custom URL events defined in app's Info.plist
//
// Like so:
// CFBundleURLTypes = {
// 	{
// 		CFBundleURLName = "Local File",
// 		CFBundleURLSchemes =
// 		{
// 			"local",
// 		},
// 	},
// },
//
- (void)handleURLEvent:(NSAppleEventDescriptor*)event
		withReplyEvent:(NSAppleEventDescriptor*)replyEvent
{
	NSString* url = [[event paramDescriptorForKeyword:keyDirectObject] stringValue];

	[_coronaView handleOpenURL:url];
}

// Handle OS openfiles request.  The Corona app must specify the handled CFBundleDocumentTypes in settings.osx.plist
//
// Like so:
// CFBundleDocumentTypes = {
// {
//		CFBundleTypeExtensions =
//		{
//			"png",
//		},
//		CFBundleTypeIconFile = "app.icns",
//		CFBundleTypeName = "public.png",
//		LSHandlerRank = "Alternate",
//		LSItemContentTypes =
//		{
//			"public.png",
//		},
//	},
// },
//
-(void)application:(NSApplication *)sender openFiles:(NSArray *)filenames
{
	for (NSString *filename in filenames)
	{
		// The OS can call this entry point before things are ready to send Corona events
		// so dispatch the handleOpenURL: calls next time around the runloop
		[_coronaView performSelectorOnMainThread:@selector(handleOpenURL:) withObject:filename waitUntilDone:NO];
	}
}

- (NSWindow *) currentWindow
{
	return _window;
}

// If we don't implement this and hook it up with IB the "Close" menu item doesn't work for
// apps that start out full screen (for some unknown reason)
- (IBAction)performClose:(id)sender
{
	[[self currentWindow] close];
}
@end

//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#import "CoronaWindowController.h"

#import "CoronaViewPrivate.h"

#import "CoronaLua.h"

// Player
#include "Rtt_MacPlatform.h"
#include "Rtt_MacSimulatorServices.h"

// Modules
#include "Rtt_LuaLibSimulator.h"

// Librtt
#include "Rtt_LuaContext.h"
#include "Rtt_Runtime.h"
#include "Rtt_RuntimeDelegate.h"

// ----------------------------------------------------------------------------

@class CoronaWindowController;

namespace Rtt
{

// ----------------------------------------------------------------------------

static int
close( lua_State *L )
{
	Rtt_ASSERT( lua_islightuserdata( L, lua_upvalueindex( 1 ) ) );
	const CoronaWindowController *controller =
		(const CoronaWindowController *)lua_touserdata( L, lua_upvalueindex( 1 ) );

	[controller.window close];

	return 0;
}

static int
stopModal( lua_State *L )
{
	[NSApp stopModal];

	return 0;
}

// ----------------------------------------------------------------------------

class LuaLibScreen
{
	public:
		typedef LuaLibScreen Self;

	public:
		static const char kName[];
		static int Open( lua_State *L );
};

const char LuaLibScreen::kName[] = "screen";

// ----------------------------------------------------------------------------

int
LuaLibScreen::Open( lua_State *L )
{
	const luaL_Reg kVTable[] =
	{
		{ "close", close },
		{ "stopModal", stopModal },

		{ NULL, NULL }
	};

	Rtt_ASSERT( lua_islightuserdata( L, lua_upvalueindex( 1 ) ) );
	void *context = lua_touserdata( L, lua_upvalueindex( 1 ) );
	lua_pushlightuserdata( L, context );
	luaL_openlib( L, kName, kVTable, 1 ); // leave "simulator" on top of stack

	return 1;
}

// ----------------------------------------------------------------------------

class RuntimeDelegateWrapper : public RuntimeDelegate
{
	public:
		RuntimeDelegateWrapper( CoronaWindowController *owner );
		~RuntimeDelegateWrapper();

	public:
		virtual void DidInitLuaLibraries( const Runtime& sender ) const;
		virtual void WillLoadMain( const Runtime& sender ) const;
		virtual void DidLoadMain( const Runtime& sender ) const;
		virtual void WillLoadConfig( const Runtime& sender, lua_State *L ) const;
		virtual void InitializeConfig( const Runtime& sender, lua_State *L ) const;
		virtual void DidLoadConfig( const Runtime& sender, lua_State *L ) const;

	public:
		void SetDelegate( RuntimeDelegate *delegate );
		RuntimeDelegate * GetDelegate() { return fDelegate; }
	
	private:
		CoronaWindowController *fOwner;
		RuntimeDelegate *fDelegate;
};

// ----------------------------------------------------------------------------

RuntimeDelegateWrapper::RuntimeDelegateWrapper( CoronaWindowController *owner )
:	fOwner( owner ),
	fDelegate( NULL )
{
}

RuntimeDelegateWrapper::~RuntimeDelegateWrapper()
{
	delete fDelegate;
}

void
RuntimeDelegateWrapper::DidInitLuaLibraries( const Runtime& sender ) const
{
	if ( fDelegate )
	{
		fDelegate->DidInitLuaLibraries( sender );
	}
}

void
RuntimeDelegateWrapper::WillLoadMain( const Runtime& sender ) const
{
	// TODO: Shouldn't this be in DidInitLuaLibraries?
	lua_State *L = sender.VMContext().L();
	lua_pushlightuserdata( L, fOwner );
	LuaContext::RegisterModuleLoader( L, LuaLibScreen::kName, LuaLibScreen::Open, 1 );

	if ( fDelegate )
	{
		fDelegate->WillLoadMain( sender );
	}
}

void
RuntimeDelegateWrapper::DidLoadMain( const Runtime& sender ) const
{
	if ( fDelegate )
	{
		fDelegate->DidLoadMain( sender );
	}
}

void
RuntimeDelegateWrapper::WillLoadConfig( const Runtime& sender, lua_State *L ) const
{
	if ( fDelegate )
	{
		fDelegate->WillLoadConfig( sender, L );
	}
}

void
RuntimeDelegateWrapper::InitializeConfig( const Runtime& sender, lua_State *L ) const
{
	if ( fDelegate )
	{
		fDelegate->InitializeConfig( sender, L );
	}
}

void
RuntimeDelegateWrapper::DidLoadConfig( const Runtime& sender, lua_State *L ) const
{
	if ( fDelegate )
	{
		fDelegate->DidLoadConfig( sender, L );
	}
}

void
RuntimeDelegateWrapper::SetDelegate( RuntimeDelegate *delegate )
{
	if ( delegate != fDelegate )
	{	
		delete fDelegate;
		fDelegate = delegate;
	}
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------

@interface CoronaWindowController ()

@property (nonatomic, retain) NSString *fWindowTitle;

@end

@implementation CoronaWindowController

@synthesize fView;
@synthesize windowGoingAway;
@synthesize fWindowTitle;

- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)menuitem
{
    BOOL enable;
	
    if ([menuitem action] == @selector(close:))
    {
		enable = NO;
    }
	else
	{
		enable = [self respondsToSelector:[menuitem action]]; //handle general case.
	}
	
    return enable;
}

- (id)initWithPath:(NSString*)path width:(int)width height:(int)height title:(NSString *)windowTitle resizable:(bool) resizable
{
	using namespace Rtt;

	BOOL isDir = NO;
	
    NSUInteger styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
    if (resizable)
    {
        styleMask |= NSWindowStyleMaskResizable;
    }

	if ( [[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&isDir]
		 && isDir )
	{			
		NSRect contentRect = NSMakeRect( 0, 0, width, height );
		NSWindow* fWindow = [[NSWindow alloc] initWithContentRect:contentRect
														styleMask:styleMask
														  backing:NSBackingStoreBuffered
															defer:NO];

		// Centering the window now before we attach the window controller.
		// This seems to center the window for first time launches (new users with no saved preferences).
		// But this gets overridden by the window controller's last saved position if it exists which is what we want
		// because we want to respect the user's last position (especially in multiple display situations).
		[fWindow center];

        if (resizable)
        {
            [fWindow setShowsResizeIndicator:YES];
            [fWindow setMinSize:[fWindow contentRect].size];  // min window size is original size
            [fWindow setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary]; // turn on full-screen button
        }
        
		// Now create the WindowController with the window
		self = [super initWithWindow:fWindow];

		// WindowController should now own the window
		[fWindow release];

		if ( self )
		{
			[fWindow setReleasedWhenClosed:NO];
			[fWindow setDelegate:(id<NSWindowDelegate>)self];

            fView = [[CoronaView alloc] initWithPath:path frame:contentRect];
            
            [fView setViewDelegate:(id<CoronaViewDelegate>)self];  // DPC: ???
            [self.view.glView setIsResizable:resizable];

			fRuntimeDelegateWrapper = new RuntimeDelegateWrapper( self );

			NSView *contentView = [fWindow contentView];
			[contentView addSubview:fView];

			fIsInitialized = NO;
			fWindowTitle = windowTitle;
		}

		return self;
	}

	// Failed
	return nil;
}

- (void)didPrepare
{
    NSString* window_title = nil;

    [self.view run];
    
    [self setWindowFrameAutosaveName:[NSString stringWithFormat:@"%@-Window", fWindowTitle]];
    window_title = fWindowTitle;
    [[self window] setTitle:window_title];

    // This ensures any Lua window resize callback gets called with
    // the current window size as recovered from the user's preferences
    [self.view.glView setFrameSize:[self.view frame].size];
    [self windowWillResize:self.window toSize:[self.window frame].size];
}

- (void)dealloc
{
	delete fRuntimeDelegateWrapper;
	[fView release];
	[windowShouldCloseBlock release];
    windowShouldCloseBlock = nil;
	[windowCloseCompletionBlock release];
	windowCloseCompletionBlock = nil;

	[super dealloc];
}

- (void)willLoadApplication:(CoronaView*)sender
{
	using namespace Rtt;

	Runtime *runtime = self.view.runtime;
	runtime->SetDelegate( fRuntimeDelegateWrapper );
}

- (void)didLoadApplication:(CoronaView*)sender
{
	fIsInitialized = YES;
	[self show];
}

- (void)show
{
	if ( fIsInitialized )
	{
		[[self window] makeKeyAndOrderFront:nil];
	}
}

- (void)hide
{
	if ( fIsInitialized )
	{
		[[self window] close];
	}
}

#pragma mark Window delegate methods

// Warning: Watch out for the control flow due to overriding windowShouldClose for fadeout
// NSWindow performClose will invoke windowShouldClose which will avoid calling this method if it returns NO.
// But NSWindowController close or NSWindow close will bypass windowShouldClose and come here directly.
- (void) windowWillClose:(NSNotification*)the_notification
{
	if ( nil != windowCloseCompletionBlock )
	{
		windowCloseCompletionBlock();
	}
}

- (void) resurrectWindow
{
	if(NO == self.windowGoingAway)
	{
		return;
	}
	self.windowGoingAway = NO;
}


- (void) setWindowShouldCloseBlock:(BOOL (^)(void))block
{
	[windowShouldCloseBlock release];
	windowShouldCloseBlock = [block copy];
}

- (void) setWindowDidCloseCompletionBlock:(void (^)(void))block
{
	[windowCloseCompletionBlock release];
	windowCloseCompletionBlock = [block copy];
}

- (void) setWindowWillResizeBlock:(BOOL (^)(int oldWidth, int oldHeight, int newWidth, int newHeight))block
{
	[windowWillResizeBlock release];
	windowWillResizeBlock = [block copy];
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize
{
    NSRect windowFrame = [self.window frame];
    NSRect contentFrame = [[self.window contentView] frame];

    // NSLog(@"CoronaWindowController:windowWillResize: old %@ - new %@", NSStringFromSize(windowFrame.size), NSStringFromSize(frameSize));
    
    BOOL doResize = YES;
    if ( nil != windowWillResizeBlock )
	{
        // the Lua listener can reject the resize attempt by returning false
        doResize = windowWillResizeBlock(windowFrame.size.width, contentFrame.size.height,
                                             frameSize.width, frameSize.height);
    }
    
    if (doResize)
    {
        [fView setFrameSize:frameSize];
        return frameSize;
    }
    else
    {
        return windowFrame.size;
    }
}

// Call the resize callback for fullscreen events
- (void)windowWillEnterFullScreen:(NSNotification *)notification
{
    [fView.glView setInFullScreenTransition:YES];
}

- (void)windowDidEnterFullScreen:(NSNotification *)notification
{
    // NSLog(@"CoronaWindowController:windowDidEnterFullScreen: %@", notification);
    [fView.glView setInFullScreenTransition:NO];
    [self windowWillResize:self.window toSize:[self.window frame].size];
}

- (void)windowWillExitFullScreen:(NSNotification *)notification
{
    [fView.glView setInFullScreenTransition:YES];
}
- (void)windowDidExitFullScreen:(NSNotification *)notification
{
    // NSLog(@"CoronaWindowController:windowDidExitFullScreen: %@", notification);
    [fView.glView setInFullScreenTransition:NO];
    [self windowWillResize:self.window toSize:[self.window frame].size];
}

@end

// ----------------------------------------------------------------------------

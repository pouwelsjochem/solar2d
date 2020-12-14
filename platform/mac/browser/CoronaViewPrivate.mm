//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#import <AppKit/NSOpenGL.h>

#include "Core/Rtt_Build.h"

#import "CoronaViewPrivate.h"

#include "Rtt_MacPlatform.h"
#include "Rtt_Lua.h"
#include "Rtt_Runtime.h"
#include "Rtt_MacViewCallback.h"
#include "Rtt_Lua.h"
#include "Rtt_LuaContext.h"
#include "Rtt_Display.h"
#include "CoronaViewRuntimeDelegate.h"

#include "Rtt_ProjectSettings.h"
#include "Rtt_NativeWindowMode.h"

#include "Rtt_AppleInputDeviceManager.h"
#include "Rtt_AppleInputHIDDeviceListener.h"
#include "Rtt_AppleInputMFiDeviceListener.h"

extern "C" {
#include "lua.h"
}

#ifdef Rtt_DEBUG
#define NSDEBUG(...) NSLog(__VA_ARGS__)
#else
#define NSDEBUG(...) NSLog(__VA_ARGS__)
#endif

// ----------------------------------------------------------------------------

static int
CoronaViewListenerAdapter( lua_State *L )
{
	using namespace Rtt;

	int result = 0;

	int eventIndex = 1;
	if ( lua_istable( L, eventIndex ) )
	{
		CoronaView *view = (CoronaView *)lua_touserdata( L, lua_upvalueindex( 1 ) );
		id <CoronaViewDelegate> delegate = view.coronaViewDelegate;
		if ( [delegate respondsToSelector:@selector(coronaView:receiveEvent:)] )
		{
			NSDictionary *event = ApplePlatform::CreateDictionary( L, eventIndex );
			id value = [delegate coronaView:view receiveEvent:event];

			result = (int)ApplePlatform::Push( L, value );
		}
	}

	return result;
}

// ----------------------------------------------------------------------------

@interface CoronaView()
{
@private
	int fSuspendCount;
}


@end

@implementation CoronaView

@synthesize _projectPath;
@synthesize _GLView;
@synthesize _viewDelegate;
@synthesize _runtime;
@synthesize _platform;
@synthesize _runtimeDelegate;
@synthesize _projectSettings;
@synthesize _launchParams;


#if !defined( Rtt_AUTHORING_SIMULATOR )
Rtt_EXPORT const luaL_Reg* Rtt_GetCustomModulesList()
{
	return NULL;
}
#endif

- (NSInteger)run
{
    return [self runWithPath:_projectPath parameters:nil];
}

- (NSInteger)runWithPath:(NSString*)path parameters:(NSDictionary *)params
{
    NSDEBUG(@"CoronaView: runWithPath: %@ parameters: %@", path, params);
    using namespace Rtt;

	_launchParams = [params retain];

    NSInteger result = (NSInteger)Runtime::kSuccess;
    
    MacPlatform *platform = new MacPlatform(self);

	// Sanity check
    if (! [[NSFileManager defaultManager] isReadableFileAtPath:[path stringByAppendingPathComponent:@"main.lua"]] &&
        ! [[NSFileManager defaultManager] isReadableFileAtPath:[path stringByAppendingPathComponent:@"main.lu"]]&&
        ! [[NSFileManager defaultManager] isReadableFileAtPath:[path stringByAppendingPathComponent:@"resource.car"]])
	{
		NSTextView *accessory = [[NSTextView alloc] initWithFrame:NSMakeRect(0,0,400,15)];
		NSFont *msgFont = [NSFont userFontOfSize:[NSFont smallSystemFontSize]];
		NSFont *fixedFont = [NSFont userFixedPitchFontOfSize:[NSFont smallSystemFontSize]];
		NSDictionary *pathAttributes = [NSDictionary dictionaryWithObject:fixedFont forKey:NSFontAttributeName];
		NSString *link = @"http://docs.coronalabs.com/coronacards/osx/index.html";
		NSString *linkText = @"\n\nCoronaCards documentation";
		NSDictionary *linkAttributes = [NSDictionary dictionaryWithObject:link forKey:NSLinkAttributeName];
		NSString *msgText = @"\n\n(no main.lua found)";
		NSDictionary *msgAttributes = [NSDictionary dictionaryWithObject:msgFont forKey:NSFontAttributeName];
		[accessory insertText:[[NSAttributedString alloc] initWithString:path
															  attributes:pathAttributes]];
		[accessory insertText:[[NSAttributedString alloc] initWithString:linkText
															  attributes:linkAttributes]];
		[accessory insertText:[[NSAttributedString alloc] initWithString:msgText
															  attributes:msgAttributes]];
		[accessory setEditable:NO];
		[accessory setDrawsBackground:NO];

		NSAlert *alert = [[[NSAlert alloc] init] autorelease];
		[alert setMessageText:@"CoronaCards Error"];
		[alert setInformativeText:@"Cannot load Corona project:"];
		[alert setAccessoryView:accessory];
		[alert runModal];

		[NSApp terminate:nil];

		return Runtime::kGeneralFail;
	}

	_projectPath = [path retain];
	platform->SetResourcePath( [_projectPath UTF8String] );

	platform->Initialize( self.glView );

	_platform = platform;
	
	[self initializeRuntime];

	// Initialize Joystick Support:
	AppleInputDeviceManager& macDeviceManager = (AppleInputDeviceManager&)_runtime->Platform().GetDevice().GetInputDeviceManager();
	
    // MFI:
	_macMFiDeviceListener = [[AppleInputMFiDeviceListener alloc] initWithRuntime:_runtime andDeviceManager:&macDeviceManager];
	[_macMFiDeviceListener start];
	
	//HID
    _macHIDInputDeviceListener = new Rtt::AppleInputHIDDeviceListener;
    _macHIDInputDeviceListener->StartListening(_runtime, &macDeviceManager);
	
    return result;
}

- (void)suspend
{
	if ( _runtime )
	{
		Rtt_ASSERT( fSuspendCount >= 0 );

		++fSuspendCount;

		if ( fSuspendCount == 1 )
		{
			if ( [_coronaViewDelegate respondsToSelector:@selector(coronaViewWillSuspend:)] )
			{
				[_coronaViewDelegate coronaViewWillSuspend:self];
			}

			_runtime->Suspend();

			if ( [_coronaViewDelegate respondsToSelector:@selector(coronaViewDidSuspend:)] )
			{
				[_coronaViewDelegate coronaViewDidSuspend:self];
			}
		}
	}
}

- (void)resume
{
	if ( _runtime )
	{
		if ( fSuspendCount > 0 )
		{
			--fSuspendCount;

			if ( fSuspendCount == 0 )
			{
				if ( [_coronaViewDelegate respondsToSelector:@selector(coronaViewWillResume:)] )
				{
					[_coronaViewDelegate coronaViewWillResume:self];
				}

				_runtime->Resume();

				_runtime->GetDisplay().Invalidate();

				if ( [_coronaViewDelegate respondsToSelector:@selector(coronaViewDidResume:)] )
				{
					[_coronaViewDelegate coronaViewDidResume:self];
				}
			}
		}
	}
}

- (void)terminate
{
	// Destroy runtime first
	delete _runtime;
	_runtime = NULL;

	delete _runtimeDelegate;
	_runtimeDelegate = NULL;

	delete _platform;
	_platform = NULL;

	[_GLView removeFromSuperview];
}

- (id)sendEvent:(NSDictionary *)event
{
	using namespace Rtt;

	id result = nil;

	lua_State *L = _runtime->VMContext().L();
	Rtt_LUA_STACK_GUARD( L );

	id name = [event valueForKey:@"name"];
	if ( [name isKindOfClass:[NSString class]]
		&& [name length] > 0 )
	{
		ApplePlatform::CreateAndPushTable( L, event ); // push event
		Lua::DispatchRuntimeEvent( L, 1 );

		// NOTE: In the EventListener:dispatchEvent code, the default result
		// is 'false'. For Obj-C, we'd prefer the default to be 'nil'.
		// Thus, if we get 'false', we skip the conversion. In Lua, these are
		// morally equivalent and we probably should have set the default to be
		// nil to begin with.
		if ( 0 != lua_toboolean( L, -1 ) )
		{
			result = ApplePlatform::ToValue( L, -1 );
		}
		lua_pop( L, 1 ); // pop result
	}
	
	return result;
}

- (void) initializeRuntime
{
	 // [self initializeRuntimeWithPlatform:NULL runtimeDelegate:NULL];
	 using namespace Rtt;

	Rtt_ASSERT( ! _runtime );

	if ( Rtt_VERIFY( [self _platform] ) )
	{
		// Launch
		Runtime *runtime = new Runtime( * [self _platform], _GLViewCallback );

		if ( Rtt_VERIFY( runtime ) )
		{
			_runtime = runtime;
			_GLView.runtime = runtime;

			_runtimeDelegate = ( _runtimeDelegate ? _runtimeDelegate : new CoronaViewRuntimeDelegate( self ) );
			_runtime->SetDelegate( _runtimeDelegate );

            if ([[NSFileManager defaultManager] isReadableFileAtPath:[_projectPath stringByAppendingPathComponent:@"resource.car"]])
            {
                _runtime->SetProperty( Runtime::kIsApplicationNotArchived, false );
            }
            else
            {
                _runtime->SetProperty( Runtime::kIsApplicationNotArchived, true );
            }
			
			_runtime->SetProperty(Runtime::kIsLuaParserAvailable, true);
			_runtime->SetProperty(Runtime::kRenderAsync, true);
            _runtime->SetProperty(Runtime::kIsCoronaKit, true);

#ifndef Rtt_AUTHORING_SIMULATOR
			_runtime->SetProperty(Runtime::kIsLuaParserAvailable, true);
#endif
			_GLViewCallback->Initialize( _runtime );

			id delegate = _viewDelegate;
			if ( [delegate respondsToSelector:@selector(willLoadApplication:)] )
			{
				[delegate willLoadApplication:self];
			}

			// Load the project's "build.settings" and "config.lua" file first.
			// Used to fetch supported supported image suffix scales, and content width/height.
			_projectSettings->LoadFromDirectory([_projectPath UTF8String]);
			
			[self setFrameSize:NSMakeSize(CoronaViewPrivateDefaultWidth, CoronaViewPrivateDefaultHeight)];
		}
	}
}

- (void)didPrepareOpenGLContext:(id)sender
{
	NSDEBUG(@"CoronaView: didPrepareOpenGLContext: %@", NSStringFromRect([self frame]));

	if ([_coronaViewDelegate respondsToSelector:@selector(didPrepareOpenGLContext:)])
	{
		// Give the host a pointer to the GLView if it wants it
		[_coronaViewDelegate didPrepareOpenGLContext:_GLView];
	}


#ifdef Rtt_AUTHORING_SIMULATOR
	U32 launchOptions = Rtt::Runtime::kCoronaViewOption;
#else
	U32 launchOptions = Rtt::Runtime::kCoronaCardsOption;
#endif
    
    // FIXME: Bad things happen if the view gets too small, arbitrarily restrict it for now
    if ([[self window] minSize].width == 0 || [[self window] minSize].height == 0)
    {
        [[self window] setMinSize:NSMakeSize(50, 50)];
    }
    	
	if (  Rtt::Runtime::kSuccess == _runtime->LoadApplication( launchOptions ) )
	{
		[self willBeginRunLoop:_launchParams];

		_runtime->BeginRunLoop();

		id delegate = _viewDelegate;

		if ( [delegate respondsToSelector:@selector(didLoadApplication:)] )
		{
			[delegate didLoadApplication:self];
		}
        
		_runtime->GetDisplay().WindowSizeChanged();
	}
}

- (void)willBeginRunLoop:(NSDictionary *)params
{
	using namespace Rtt;

	lua_State *L = _runtime->VMContext().L();

	// Pass launch params to main.lua
	if ( params )
	{
		Rtt_LUA_STACK_GUARD( L );

		if ( Rtt_VERIFY( _runtime->PushLaunchArgs( true ) > 0 ) )
		{
			lua_State *L = _runtime->VMContext().L();
			ApplePlatform::CopyDictionary( L, -1, params );
			lua_pop( L, 1 );
		}
	}

	// Attach listener for "coronaView" events
	{
		Lua::AddCoronaViewListener( L, CoronaViewListenerAdapter, self );
		//		Rtt_LUA_STACK_GUARD( L );
		//
		//		// Push Runtime.addEventListener
		//		// Push Runtime
		//		Lua::PushRuntime( L );
		//		lua_getfield( L, -1, "addEventListener" );
		//		lua_insert( L, -2 ); // swap table and function
		//
		//		// Push 'coronaView'
		//		lua_pushstring( L, "coronaView" );
		//
		//		// Push 'CoronaViewListenerAdapter'
		//		lua_pushlightuserdata( L, self );
		//		lua_pushcclosure( L, CoronaViewListenerAdapter, 1 );
		//
		//		// Runtime.addEventListener( Runtime, "coronaView", CoronaViewListenerAdapter )
		//		int status = Lua::DoCall( L, 3, 0 ); Rtt_UNUSED( status );
		//		Rtt_ASSERT( 0 == status );
	}
}

- (id)init
{
    NSDEBUG(@"CoronaView: init");

    return [super init];
}

- (id)initWithPath:(NSString *)path frame:(NSRect)frame
{
    NSDEBUG(@"CoronaView: initWithPath: %@ frame: %@", path, NSStringFromRect(frame));

    self = [[CoronaView alloc] initWithFrame:frame];

    if ( self )
    {
		_projectPath = path;
    }

    return self;
}

- (id)initWithFrame:(NSRect)frameRect
{
	self = [super initWithFrame:frameRect];
	
	if ( self )
	{
        NSDEBUG(@"CoronaView: initWithFrame: %@", NSStringFromRect([self frame]));

        [self initInternals];
	}
	return self;
}

- (id)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];

    if ( self )
    {
        NSDEBUG(@"CoronaView: initWithCoder: %@", NSStringFromRect([self frame]));

        [self initInternals];
    }
    return self;
}

- (void) initInternals
{
    _GLView = [[GLView alloc] initWithFrame:[self frame]];
    [self addSubview:_GLView];
    [_GLView release];

    [_GLView setDelegate:self];

	[_GLView setWantsBestResolutionOpenGLSurface:NO];

    _platform = NULL;
    _runtime = NULL;
	fSuspendCount = 0;
    _macHIDInputDeviceListener = NULL;
	_macMFiDeviceListener = nil;

    _GLViewCallback = new Rtt::MacViewCallback( _GLView ); // This is what is on the Timer loop

	_projectSettings = new Rtt::ProjectSettings();
}

/*
- (BOOL) isFlipped
{
	return YES;
}
*/

- (void)dealloc
{
    if (_macHIDInputDeviceListener != NULL)
    {
        _macHIDInputDeviceListener->StopListening();
		delete _macHIDInputDeviceListener;
    }
	
	if (_macMFiDeviceListener)
	{
		[_macMFiDeviceListener stop];
		[_macMFiDeviceListener release];
	}

	delete _GLViewCallback;
	delete _projectSettings;

	_runtime->SetDelegate(NULL);
	delete _runtime;

	Rtt_DELETE( _platform );
	[_GLView removeFromSuperview];

	[super dealloc];
}

// ----------------------------------------------------------------------------

- (void) setFrameSize:(NSSize)newSize
{
	NSDEBUG(@"CoronaViewPrivate: setFrameSize: frame %@", NSStringFromRect([self frame]));
	NSDEBUG(@"CoronaViewPrivate: setFrameSize: old %@, new %@", NSStringFromSize([self frame].size), NSStringFromSize(newSize));
	
	[_GLView setFrameSize:newSize];
	[super setFrameSize:newSize];
}

- (void) setFrameOrigin:(NSPoint)newOrigin
{
    NSDEBUG(@"CoronaViewPrivate: setFrameOrigin: %@", NSStringFromPoint(newOrigin));

    [super setFrameOrigin:newOrigin];
}

- (void) setFrame:(NSRect)newFrame
{
	NSDEBUG(@"CoronaViewPrivate: setFrame: from %@ to %@", NSStringFromRect([self frame]), NSStringFromRect(newFrame));

	[super setFrame:newFrame];
}

- (NSRect) frame
{
	NSRect frame = [super frame];
	// NSDEBUG(@"CoronaViewPrivate: frame: %@", NSStringFromRect(frame));

	return frame;
}

// Interface for hosts to send "open URL" AppleScript events
- (void) handleOpenURL:(NSString *)urlStr
{
	if ( _runtime != NULL )
	{
		Rtt::SystemOpenEvent e( [urlStr UTF8String] );

		_runtime->DispatchEvent( e );
	}
}

#pragma mark GLView Helpers

- (void) restoreWindowProperties
{
	[_GLView restoreWindowProperties];
}

#pragma mark ProjectSettings Helpers

- (BOOL) settingsIsWindowCloseButtonEnabled
{
	return (BOOL) _projectSettings->IsWindowCloseButtonEnabled();
}

- (BOOL) settingsIsWindowMinimizeButtonEnabled
{
	return (BOOL) _projectSettings->IsWindowMinimizeButtonEnabled();
}

- (BOOL) settingsSuspendWhenMinimized
{
    return (BOOL) _projectSettings->SuspendWhenMinimized();
}

- (int) settingsMinContentWidth
{
	return (int) _projectSettings->GetMinContentWidth();
}
- (int) settingsMaxContentWidth
{
	return (int) _projectSettings->GetMaxContentWidth();
}

- (int) settingsMinContentHeight
{
	return (int) _projectSettings->GetMinContentHeight();
}
- (int) settingsMaxContentHeight
{
	return (int) _projectSettings->GetMaxContentHeight();
}

- (NSString *) settingsWindowTitle
{
	NSString * language = [[NSLocale currentLocale] objectForKey:NSLocaleLanguageCode];
	NSString *countryCode = [[NSLocale currentLocale] objectForKey:NSLocaleCountryCode];
	const char *title = _projectSettings->GetWindowTitleTextForLocale([language UTF8String], [countryCode UTF8String]);

	if (title != NULL)
	{
		return [NSString stringWithExternalString:title];
	}
	else
	{
		return nil;
	}
}

- (const CoronaViewWindowMode) settingsDefaultWindowMode
{
    CoronaViewWindowMode coronaViewWindowMode = kNormal;
    const Rtt::NativeWindowMode *nativeWindowMode = _projectSettings->GetDefaultWindowMode();
    
    if (*nativeWindowMode == Rtt::NativeWindowMode::kNormal)
    {
        coronaViewWindowMode = kNormal;
    }
    else if (*nativeWindowMode == Rtt::NativeWindowMode::kFullscreen)
    {
        coronaViewWindowMode = kFullscreen;
    }

	return coronaViewWindowMode;
}

- (int) settingsDefaultWindowViewWidth
{
	return _projectSettings->GetDefaultWindowViewWidth();
}

- (int) settingsDefaultWindowViewHeight
{
	return _projectSettings->GetDefaultWindowViewHeight();
}

// ----------------------------------------------------------------------------

@end

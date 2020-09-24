//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>

#import "CoronaViewPrivate.h"
#import "CoronaViewRuntimeDelegate.h"
#import "CoronaGyroscopeObserver.h"
#import "CoronaSystemResourceManager.h"
#import "AppleWeakProxy.h"

#ifdef Rtt_CORE_MOTION
	#import <CoreMotion/CoreMotion.h>
#endif

#include "Core/Rtt_Build.h"
#include "Core/Rtt_New.h"

#include "Display/Rtt_Display.h"
#include "Display/Rtt_StageObject.h"
#include "Rtt_AppleBitmap.h"
#include "Rtt_AppleInputDeviceManager.h"
#include "Rtt_AppleInputMFiDeviceListener.h"
#include "Rtt_IPhonePlatformBase.h"
#include "Rtt_MPlatformDevice.h"
#include "Rtt_Runtime.h"

#if defined( Rtt_IPHONE_ENV )
	#include "Rtt_IPhonePlatformCore.h"
	#include "Rtt_IPhoneTemplate.h"
#endif

#include "Rtt_Event.h"
#include "Rtt_GPU.h"
#include "Rtt_MCallback.h"

#include "CoronaEvent.h"
#include "CoronaLua.h"
#include "Rtt_LuaContext.h"

#include "Rtt_KeyName.h"
#include "Rtt_MetalAngleTypes.h"

// ----------------------------------------------------------------------------

namespace Rtt
{
	
class MCallback;

// ----------------------------------------------------------------------------
	
} // Rtt

// ----------------------------------------------------------------------------

// Creates the default platform for CoronaView
// NOTE: Default implies iOS and CoronaCards.
// NOTE: We deliberately do not have a default for non-iOS platforms
// to make the maintenance of this file (which is shared across iOS/tvOS)
// somewhat sane.
static Rtt::IPhonePlatformBase *
CreatePlatform( CoronaView *view )
{
#if defined( Rtt_IPHONE_ENV )
	return new Rtt::IPhonePlatformCore( view );
#else
	// On non-iOS platforms, you should pass in your own instance of iPhonePlatformBase
	// We are trying to keep all non-iOS related classes/etc (e.g. TVOS) out of this file.
	Rtt_ASSERT_NOT_REACHED();
	return nullptr;
#endif
}

// UITouch (CoronaViewExtensions)
// ----------------------------------------------------------------------------
#pragma mark # UITouch (CoronaViewExtensions)

@interface UITouch ( CoronaViewExtensions )

- (CGPoint)locationInCoronaView:(CoronaView*)view;

@end


@implementation UITouch ( CoronaViewExtensions )

- (CGPoint)locationInCoronaView:(CoronaView*)view
{
	using namespace Rtt;

	// This should be the only place in this file where we call locationInView:
	CGPoint result = [self locationInView:view];
	// Compiler macro is insufficient if you are building against the 4.0 SDK for new features, but
	// also want to support older OS's. You need a runtime check as well.
	if([view respondsToSelector:@selector(contentScaleFactor)])
	{
		CGFloat scale = view.contentScaleFactor;
		
		result.x *= scale;
		result.y *= scale;
	}
	
	return result;
}

@end

// CoronaViewListenerAdapter
// ----------------------------------------------------------------------------
#pragma mark # CoronaViewListenerAdapter

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


// CoronaView()
// ----------------------------------------------------------------------------
#pragma mark # CoronaView()

@interface CoronaView() <CoronaGyroscopeObserver>
{
@private
	NSString *fResourcePath;
	CFMutableDictionaryRef fTouchesData;
	CGPoint fStartTouchPosition;
	int fInhibitCount; // used by TouchInhibitor
	int fSuspendCount;
	int fLastContentHeight;
	bool fShouldInvalidate;
	bool fBeganRunLoop;

	// Gyroscope
	BOOL gyroscopeEnabled;
	U64 gyroscopePreviousTimestampCorona;
	NSTimeInterval gyroscopePreviousTimestamp;

	// Load Parameters
	NSDictionary *fParams;

	AppleInputMFiDeviceListener *fMFiInputListener;
}

@property (nonatomic, readonly) Rtt::IPhonePlatformBase *platform;
@property (nonatomic, retain) AppleWeakProxy *observerProxy;

- (void)addApplicationObserver;
- (void)removeApplicationObserver;
- (void)applicationWillResignActive:(NSNotification *)notification;
- (void)applicationDidBecomeActive:(NSNotification *)notification;

- (void)dispatchEvent:(Rtt::MEvent*)event;

- (void)pollAndDispatchMotionEvents;

@end

// CoronaView
// ----------------------------------------------------------------------------
#pragma mark # CoronaView

@implementation CoronaView

// ----------------------------------------------------------------------------

@synthesize fInhibitCount;

// Bottleneck for startup
- (void)initCommon
{
	fResourcePath = nil;
	fTouchesData = nil;
	fInhibitCount = 0;
	fSuspendCount = 0;
	_observeSuspendResume = YES;
	fLastContentHeight = -1;
	fShouldInvalidate = false;
	fParams = nil;

	_observerProxy = [[AppleWeakProxy alloc] initWithTarget:self];
	_gyroscopeObserver = (id< CoronaGyroscopeObserver >)_observerProxy;
}

- (id)initWithFrame:(CGRect)rect context:(Rtt_EAGLContext *)context
{
	if ( (self = [super initWithFrame:rect context:context]) )
	{
		[self initCommon];
	}

	return self;
}

#ifndef Rtt_MetalANGLE
- (id)initWithFrame:(CGRect)rect
{
	if ( self || (self = [self initWithFrame:rect context:nil]) )
	{
		[self initCommon];
	}

	return self;
}
#endif

- (id)initWithCoder:(NSCoder *)aDecoder
{
	if ( (self = [super initWithCoder:aDecoder]) )
	{		
		[self initCommon];
	}

	return self;
}

// Bottleneck for teardown
- (void)deallocCommon
{
	[self terminate];

	if ( fTouchesData )
	{
		[fParams release];

		CFRelease( fTouchesData );
		fTouchesData = nil;

		[fResourcePath release];
	}
}

- (void)dealloc
{
	[self deallocCommon];

	[super dealloc];
}

- (void)initializeRuntime
{
	[self initializeRuntimeWithPlatform:NULL runtimeDelegate:NULL];
}

- (void)initializeRuntimeWithPlatform:(Rtt::IPhonePlatformBase *)platform runtimeDelegate:(Rtt::CoronaViewRuntimeDelegate *)runtimeDelegate
{
	using namespace Rtt;

	if ( ! _runtime )
	{
		_platform = ( platform ? platform : CreatePlatform( self ) );
		_runtime = new Runtime( * _platform );

		_runtimeDelegate = ( runtimeDelegate ? runtimeDelegate : new CoronaViewRuntimeDelegate( self ) );
		
		_runtime->SetDelegate( _runtimeDelegate );

#ifdef Rtt_IPHONE_ENV
		bool isCoronaKit = Rtt::IPhoneTemplate::IsProperty( IPhoneTemplate::kIsCoronaKit );
#else
		bool isCoronaKit = false;
#endif
		_runtime->SetProperty( Rtt::Runtime::kIsCoronaKit, isCoronaKit );
		if ( isCoronaKit )
		{
			_runtime->SetProperty( Rtt::Runtime::kIsApplicationNotArchived, true );
		 	_runtime->SetProperty( Rtt::Runtime::kIsLuaParserAvailable, true );
		}
	}
}

- (Rtt_GLKViewController *)viewController
{
	Rtt_ASSERT( [self.delegate isKindOfClass:[UIViewController class]] );
	return (Rtt_GLKViewController *)self.delegate;
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

- (NSInteger)run
{
	return [self runWithPath:nil parameters:nil];
}

- (NSInteger)runWithPath:(NSString*)path parameters:(NSDictionary *)params
{
	using namespace Rtt;

	NSInteger result = (NSInteger)Runtime::kGeneralFail;

	[self initializeRuntime];

	if(!fMFiInputListener) {
		AppleInputDeviceManager& inputDeviceManager = (AppleInputDeviceManager&)_runtime->Platform().GetDevice().GetInputDeviceManager();
		fMFiInputListener = [[AppleInputMFiDeviceListener alloc] initWithRuntime:_runtime andDeviceManager:&inputDeviceManager];
		[fMFiInputListener start];
	}

	// Check if already launched
	if ( ! _runtime->IsProperty( Runtime::kIsApplicationExecuting ) )
	{
		[self addApplicationObserver];

		fResourcePath = [path copy];
		_platform->SetResourceDirectory( path );

		Rtt_ASSERT( self.context );

		// We need to bind to correct framebuffer so that GL commands triggered by
		// * LoadApplication()
		// * BeginRunLoop()
		// go to to the right place
		[Rtt_EAGLContext setCurrentContext:self.context];
		[self bindDrawable];

		fParams = [params retain];


		if ( ! self.beginRunLoopManually )
		{
			result = [self beginRunLoop];
		}
		else
		{
			result = Runtime::kSuccess;
		}
	}


	return result;
}

- (NSInteger)beginRunLoop
{
	using namespace Rtt;

	if ( fBeganRunLoop ) { return (NSInteger)Runtime::kGeneralFail; }

	fBeganRunLoop = true;

	NSInteger result =_runtime->LoadApplication( Runtime::kDeviceLaunchOption );
	if ( result == (NSInteger)Runtime::kSuccess )
	{
		[self willBeginRunLoop:fParams];

		_runtime->BeginRunLoop();
		
		//Forcing immediate blit (outside MGLKViewController which normally drives the display update)
		[self display];
	}

	return result;
}

- (void)suspend
{
	if ( _runtime )
	{
		Rtt_ASSERT( fSuspendCount >= 0 );

		if ( 0 == fSuspendCount++ )
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
			if ( 0 == --fSuspendCount )
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
	//Grab the view's context
	Rtt_EAGLContext *context = self.context;
	
	//Grab the active openGL context
	Rtt_EAGLContext *openGlContext = [Rtt_EAGLContext currentContext];
	
	if ( openGlContext != context)
	{
		[Rtt_EAGLContext setCurrentContext:context];
	}
	
	[self removeApplicationObserver];

	// Destroy runtime first
	fBeganRunLoop = false;
	delete _runtime;
	_runtime = NULL;

	delete _runtimeDelegate;
	_runtimeDelegate = NULL;

	delete _platform; // We assume _platform is instantiated with plain new
	_platform = NULL;

	[fMFiInputListener stop];
	[fMFiInputListener release];
	fMFiInputListener = nil;

	// Cleanup observers used by CoronaSystemResourceManager
	// Do this after _platform is deleted b/c IPhoneDevice needs to remove these observers
	[_observerProxy invalidate];
	[_observerProxy release];
	_observerProxy = nil;
	_gyroscopeObserver = nil;
	
	//Restore the context
	[Rtt_EAGLContext setCurrentContext:openGlContext];
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
		Corona::Lua::DispatchRuntimeEvent( L, 1 );

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

- (void)addApplicationObserver
{
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationWillResignActive:) name:UIApplicationWillResignActiveNotification object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationDidBecomeActive:) name:UIApplicationDidBecomeActiveNotification object:nil];
}

- (void)removeApplicationObserver
{
	[[NSNotificationCenter defaultCenter] removeObserver:self name:UIApplicationWillResignActiveNotification object:nil];
	[[NSNotificationCenter defaultCenter] removeObserver:self name:UIApplicationDidBecomeActiveNotification object:nil];
}

- (void)applicationWillResignActive:(NSNotification *)notification
{
	if ( _observeSuspendResume )
	{
		[self suspend];
	}
}

- (void)applicationDidBecomeActive:(NSNotification *)notification
{
	if ( _observeSuspendResume )
	{
		[self resume];
	}
}

- (void)dispatchEvent:(Rtt::MEvent*)e;
{
	using namespace Rtt;

	Rtt_ASSERT( _runtime );

	if ( Rtt_VERIFY( e ) )
	{
		_runtime->DispatchEvent( * e );
	}
}

static void
TouchesDataValueRelease( CFAllocatorRef allocator, const void *p )
{
	Rtt_ASSERT( p );
	free( (CGPoint*)p );
}

static Rtt::TouchEvent::Phase
UITouchPhaseToTouchEventPhase( UITouchPhase phase )
{
	Rtt::TouchEvent::Phase result;

	switch( phase )
	{
		case UITouchPhaseBegan:
			result = Rtt::TouchEvent::kBegan;
			break;
		case UITouchPhaseMoved:
			result = Rtt::TouchEvent::kMoved;
			break;
		case UITouchPhaseStationary:
			result = Rtt::TouchEvent::kStationary;
			break;
		case UITouchPhaseEnded:
			result = Rtt::TouchEvent::kEnded;
			break;
		case UITouchPhaseCancelled:
			result = Rtt::TouchEvent::kCancelled;
			break;
		default:
			Rtt_ASSERT_NOT_REACHED();
			result = Rtt::TouchEvent::kBegan;
			break;
	}

	return result;
}

static void
InitializeEvents( CoronaView *view, Rtt::TouchEvent *touchEvents, NSSet *touches, CFMutableDictionaryRef touchesData )
{
	using namespace Rtt;

	UITouch *touch = touches.anyObject;
	TouchEvent::Phase phase = UITouchPhaseToTouchEventPhase( touch.phase );

	int i = 0;
	for ( UITouch *t in touches )
	{
		CGPoint touchPoint = [t locationInCoronaView:view];
		
		CGPoint *startPoint = (CGPoint *)CFDictionaryGetValue( touchesData, t );
		if ( TouchEvent::kBegan == phase )
		{
			if ( Rtt_VERIFY( ! startPoint ) )
			{
				startPoint = (CGPoint *)malloc(sizeof(CGPoint));
				CFDictionarySetValue( touchesData, t, startPoint );
			}
			
			* startPoint = touchPoint;
		}
		
		// Just in case we somehow didn't store the startPoint
		if ( ! Rtt_VERIFY( startPoint ) )
		{
			startPoint = & touchPoint;
		}

		TouchEvent::Phase eventPhase = phase;
		if ( eventPhase >= TouchEvent::kNumPhases )
		{
			eventPhase = UITouchPhaseToTouchEventPhase( t.phase );
		}
		
		// Initialize TouchEvent (use placement new to call c-tor)
		TouchEvent *event = & ( touchEvents[i] );
		new( event ) TouchEvent( touchPoint.x, touchPoint.y, startPoint->x, startPoint->y, eventPhase );
		event->SetId( t );

		i++;
		
		// On iOS 8.1 on iPhone 6+, once CFDictionaryRemoveValue is called, the value for the key
		// is also deleted.  This is because of the TouchesDataValueRelease function we pass in
		// when creating the dictionary
		if ( TouchEvent::kEnded == phase || TouchEvent::kCancelled == phase )
		{
			CFDictionaryRemoveValue( touchesData, t );
		}
	}
}

- (void)dispatchTouches:(NSSet *)touches withEvent:(UIEvent *)event
{
	using namespace Rtt;

	Rtt_ASSERT( 0 == fInhibitCount );

	if ( ! fTouchesData )
	{
		CFDictionaryValueCallBacks valueCallbacks = { 0, NULL, & TouchesDataValueRelease, NULL, NULL };
		fTouchesData = CFDictionaryCreateMutable( NULL, 4, NULL, & valueCallbacks );
	}

	int numEvents = (int)touches.count;
	if ( numEvents > 0 )
	{
		TouchEvent *touchEvents = (TouchEvent*)malloc( sizeof( TouchEvent ) * numEvents );
		Rtt_ASSERT( touchEvents );

		InitializeEvents( self, touchEvents, touches, fTouchesData );

		MultitouchEvent t( touchEvents, numEvents );
		[self dispatchEvent: (&t)];

		free( touchEvents );
	}
}

// #define Rtt_DEBUG_TOUCH 1

#ifdef Rtt_DEBUG_TOUCH
static void
PrintTouches( NSSet *touches, const char *header )
{
	const char *prefix = "";
	if ( header )
	{
		prefix = "\t";
		Rtt_TRACE( ( "%s\n", header ) );		
	}

	for ( UITouch *t in touches )
	{
		Rtt_TRACE( ( "%sTouch(%x) phase(%d)\n", prefix, t, t.phase ) );
	}

	Rtt_TRACE( ( "\n" ) );
}
#endif

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
#ifdef Rtt_DEBUG_TOUCH
	PrintTouches( event.allTouches, "BEGAN" );
#endif

	if ( fInhibitCount > 0 ) { return; }

	UITouch *touch = touches.anyObject;
	fStartTouchPosition = [touch locationInCoronaView:self];

#ifdef Rtt_MULTITOUCH
	if ( self.multipleTouchEnabled )
	{
		[self dispatchTouches:touches withEvent:event];
	}
	else
#endif
	{
#ifdef Rtt_TVOS_ENV
		CGPoint currentTouchPosition = { [touch locationInView:nil].x - [self center].x, [touch locationInView:nil].y - [self center].y };
		Rtt::RelativeTouchEvent t( currentTouchPosition.x, currentTouchPosition.y, Rtt::TouchEvent::kBegan );
#else
		Rtt::TouchEvent t( fStartTouchPosition.x, fStartTouchPosition.y, fStartTouchPosition.x, fStartTouchPosition.y, Rtt::TouchEvent::kBegan );
#endif
		t.SetId( touch );
		[self dispatchEvent: (&t)];
	}
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
#ifdef Rtt_DEBUG_TOUCH
	PrintTouches( event.allTouches, "MOVED" );
#endif

	if ( fInhibitCount > 0 ) { return; }

	UITouch *touch = touches.anyObject;
	CGPoint currentTouchPosition;

#ifdef Rtt_MULTITOUCH
	if ( self.multipleTouchEnabled )
	{
		[self dispatchTouches:touches withEvent:event];
	}
	else
#endif
	{
#ifdef Rtt_TVOS_ENV
		currentTouchPosition = { [touch locationInView:self].x - [self center].x, [touch locationInView:self].y - [self center].y };
		Rtt::RelativeTouchEvent t( currentTouchPosition.x, currentTouchPosition.y, Rtt::TouchEvent::kMoved );
#else
		currentTouchPosition = [touch locationInCoronaView:self];
		Rtt::TouchEvent t( currentTouchPosition.x, currentTouchPosition.y, fStartTouchPosition.x, fStartTouchPosition.y, Rtt::TouchEvent::kMoved );
#endif
		t.SetId( touch );
		[self dispatchEvent: (&t)];
	}

#ifndef Rtt_TVOS_ENV
	Rtt::DragEvent e( fStartTouchPosition.x, fStartTouchPosition.y, currentTouchPosition.x, currentTouchPosition.y );
	[self dispatchEvent: (&e)];
#endif
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
#ifdef Rtt_DEBUG_TOUCH
	PrintTouches( event.allTouches, "ENDED" );
#endif

	if ( fInhibitCount > 0 ) { return; }

	UITouch *touch = touches.anyObject;

#ifdef Rtt_MULTITOUCH
	if ( self.multipleTouchEnabled )
	{
		[self dispatchTouches:touches withEvent:event];
	}
	else
#endif
	{
#ifdef Rtt_TVOS_ENV
		CGPoint currentTouchPosition = { [touch locationInView:self].x - [self center].x, [touch locationInView:self].y - [self center].y };
		Rtt::RelativeTouchEvent t( currentTouchPosition.x, currentTouchPosition.y, Rtt::TouchEvent::kEnded );
#else
		CGPoint currentTouchPosition = [touch locationInCoronaView:self];
		Rtt::TouchEvent t( currentTouchPosition.x, currentTouchPosition.y, fStartTouchPosition.x, fStartTouchPosition.y, Rtt::TouchEvent::kEnded );
#endif
		t.SetId( touch );
		[self dispatchEvent: (&t)];
	}

	// Rtt_TRACE(  ( "touch(%p)\n\tphase(%d)\n", touch, touch.phase ) );
	
#ifdef Rtt_TVOS_ENV
		// On tvOS, we provide a key event for the tap rather than a true tap event.
		// The tap count can still be obtained from ended-phase "RelativeTouchEvent" events.
		Rtt::KeyEvent e( NULL, Rtt::KeyEvent::kDown, Rtt::KeyName::kButtonZ, 0, false, false, false, false );
		[self dispatchEvent:(&e)];
#endif
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
#ifdef Rtt_DEBUG_TOUCH
	PrintTouches( event.allTouches, "CANCELLED" );
#endif

	if ( fInhibitCount > 0 ) { return; }

#ifdef Rtt_MULTITOUCH
	// Multitouch
	if ( self.multipleTouchEnabled )
	{
		[self dispatchTouches:touches withEvent:event];
	}
	else
#endif
	{
		UITouch *touch = touches.anyObject;
		
#ifdef Rtt_TVOS_ENV
		CGPoint currentTouchPosition = { [touch locationInView:self].x - [self center].x, [touch locationInView:self].y - [self center].y };
		Rtt::RelativeTouchEvent t( currentTouchPosition.x, currentTouchPosition.y, Rtt::TouchEvent::kCancelled );
#else
		CGPoint currentTouchPosition = [touch locationInCoronaView:self];
		Rtt::TouchEvent t( currentTouchPosition.x, currentTouchPosition.y, fStartTouchPosition.x, fStartTouchPosition.y, Rtt::TouchEvent::kCancelled );
#endif
		t.SetId( touch );
		[self dispatchEvent: (&t)];
	}
}

- (void)pollAndDispatchMotionEvents
{
#ifdef Rtt_CORE_MOTION
	// Gyroscope
	if ( gyroscopeEnabled )
	{
		U64 currentTimeCorona = Rtt_AbsoluteToMilliseconds( Rtt_GetAbsoluteTime() );
		CMMotionManager *motion = [CoronaSystemResourceManager sharedInstance].motionManager;

		// Don't callback more frequently than the user's gyroUpdateInterval
		NSTimeInterval updateInterval = motion.gyroUpdateInterval;
		U64 updateIntervalMS = (U64)(updateInterval * 1000);
		if ( (currentTimeCorona - gyroscopePreviousTimestampCorona) >= updateIntervalMS )
		{
			gyroscopePreviousTimestampCorona = currentTimeCorona;

			CMGyroData *gyroData = motion.gyroData;
			
			// Initial timestamp is 0, so only pay attention to subsequent data
			if ( 0.0 != gyroscopePreviousTimestamp )
			{
				Rtt::GyroscopeEvent e(
					gyroData.rotationRate.x, gyroData.rotationRate.y, gyroData.rotationRate.z,
					gyroData.timestamp - gyroscopePreviousTimestamp );
				self.runtime->DispatchEvent( e );
			}

			gyroscopePreviousTimestamp = gyroData.timestamp;
		}
	}
#endif // Rtt_CORE_MOTION
}

- (void)drawView
{
	[self pollAndDispatchMotionEvents];

	if ( _runtime )
	{
		(*_runtime)();
	}
}

- (void)didAddSubview:(UIView *)subview
{
	//It appears that adding subViews can cause the GL window to be flushed
	//This will detect adding subviews and force a re-render.
	//This seems to happen late enough in the process that we can lazilly invalidate
	//after the next DrawRect call
	[super didAddSubview:subview];
	fShouldInvalidate = true;
}

- (void)didMoveToWindow
{
	if ( [self.window.screen respondsToSelector:@selector(nativeScale)] )
	{
		self.contentScaleFactor = self.window.screen.nativeScale;
	}
}

- (void)drawRect:(CGRect)rect
{
	//This check is required due to radar://20416615
	//OpenGL commands are not permitted in background, but in some edge cases, drawRect is called on lock screen.
	if ( _runtime && ! _runtime->IsSuspended() )
	{
		[self drawView];
	}
	
	if ( fShouldInvalidate )
	{
		if ( _runtime )
		{
			_runtime->GetDisplay().Invalidate();
			fShouldInvalidate = false;
		}
	}
}

/*
- (void)flush
{

	// Flush
	Rtt_ASSERT( context == [Rtt_EAGLContext currentContext] );

	// This is a check to make sure the correct render buffer is bound.
	// Normally, this wouldn't ever happen, but there's a check here in
	// case we need it
	#if 0
	{
		// glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
		GLint currentBuffer = 0;
		glGetIntegerv( GL_RENDERBUFFER_BINDING, & currentBuffer );
		Rtt_ASSERT( (GLuint)currentBuffer == viewRenderbuffer );
	}
	#endif

	[context presentRenderbuffer:GL_RENDERBUFFER_OES];
}
*/

- (void)setFrame:(CGRect)frame
{
	[super setFrame:frame];
	fShouldInvalidate = true;
}

- (void)setBounds:(CGRect)bounds
{
	[super setBounds:bounds];
	fShouldInvalidate = true;
}

// CoronaGyroscopeObserver
// ----------------------------------------------------------------------------
#pragma mark # CoronaGyroscopeObserver

- (void)startPolling
{
	if ( ! gyroscopeEnabled )
	{
		gyroscopeEnabled = YES;
		gyroscopePreviousTimestampCorona = Rtt_AbsoluteToMilliseconds( Rtt_GetAbsoluteTime() );
		gyroscopePreviousTimestamp = 0.0;
	}
}

- (void)stopPolling
{
	gyroscopeEnabled = NO;
}

@end

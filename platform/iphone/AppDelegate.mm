//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#import "AppDelegate.h"
#import "CoronaViewPrivate.h"
#import "CoronaViewControllerPrivate.h"
#import "CoronaViewRuntimeDelegate.h"
#import "CoronaDelegate.h"

#include "Rtt_Event.h"
#include "Rtt_IPhoneDevice.h"
#include "Rtt_IPhonePlatformBase.h"
#include "Rtt_IPhonePlatform.h"
#include "Rtt_LuaContext.h"
#include "Rtt_LuaResource.h"
#include "Rtt_MPlatformDevice.h"
#include "Rtt_Runtime.h"

#include "Display/Rtt_Display.h"

#import "Rtt_PlatformOpenALPlayer.h"
#import "Rtt_PlatformAudioSessionManager.h"

#import <CoreMotion/CoreMotion.h>
#import "Rtt_AppleCallback.h"

#include "CoronaIOSLoader.h"

#include "Rtt_IPhoneTemplate.h"

#include "CoronaAppDelegate.h"
#include "CoronaDelegate.h"

#include "Rtt_IPhoneRuntimeDelegate.h"

// ----------------------------------------------------------------------------

static Class sCoronaDelegateClass = nil;

static Class CoronaGetDelegateClass()
{
	return sCoronaDelegateClass;
}

// Declaration
FOUNDATION_EXPORT void CoronaSetDelegateClass( Class c );

FOUNDATION_EXPORT void CoronaSetDelegateClass( Class c )
{
	sCoronaDelegateClass = c;
}

// ----------------------------------------------------------------------------

@implementation AppViewController

@synthesize prefersHomeIndicatorAutoHidden;
@synthesize preferredScreenEdgesDeferringSystemGestures;


- (id)initWithNibName:(NSString *)nibName bundle:(NSBundle *)nibBundle
{
	self = [super initWithNibName:nibName bundle:nibBundle];
	if ( self )
	{
		fNextResponder = nil;
	}
	return self;
}

- (void)dealloc
{
	[fNextResponder release];

	[super dealloc];
}

- (BOOL)prefersStatusBarHidden
{
    return YES;
}

- (UIResponder *)nextResponder
{
	return ( fNextResponder ? fNextResponder : [super nextResponder] );
}

- (void)setNextResponder:(UIResponder *)responder
{
	[fNextResponder release];
	fNextResponder = [responder retain];
}

- (NSUInteger) supportedInterfaceOrientations
{
	return UIInterfaceOrientationMaskLandscape;
}

@end

// ----------------------------------------------------------------------------

// Private methods shared by init/dealloc and applicationDidEnterBackground/DidBecomeActive
@interface AppDelegate ()

@property (nonatomic, retain) NSDictionary *launchOptions;

- (void)initSelf;
- (void)deallocSelf;
- (id<CoronaDelegate>) getCoronaAppDelegate;

@end


@implementation AppDelegate

@synthesize window;
@synthesize view;
@synthesize viewController;
@synthesize lastAccelerometerTimeStamp;

- (BOOL)respondsToSelector:(SEL)aSelector
{
	BOOL result = [super respondsToSelector:aSelector];

	if ( ! result )
	{
		result = [[self getCoronaAppDelegate] respondsToSelector:aSelector];
	}

	return result;
}

- (id)forwardingTargetForSelector:(SEL)aSelector
{
	return [self getCoronaAppDelegate];
}

- (Rtt::Runtime *)runtime
{
	return view.runtime;
}

- (NSUInteger)application:(UIApplication *)application supportedInterfaceOrientationsForWindow:(UIWindow *)windowArg
{
	return UIInterfaceOrientationMaskLandscape;
}

static void
SetLaunchArgs( UIApplication *application, NSDictionary *launchOptions, Rtt::Runtime *runtime )
{
	using namespace Rtt;

	lua_State *L = runtime->VMContext().L();

		if ( Rtt_VERIFY( runtime->PushLaunchArgs( true ) > 0 ) )
		{
		NSURL *url = [launchOptions objectForKey:UIApplicationLaunchOptionsURLKey];
		if ( url )
		{
			lua_pushstring( L, [[url absoluteString] UTF8String] );
			lua_setfield( L, -2, "url" );
		}

		lua_pop( L, 1 );
	}
}

- (NSInteger)runView:(CoronaView *)sender withPath:(NSString *)path parameters:(NSDictionary *)params
{
	using namespace Rtt;

	lastAccelerometerTimeStamp = 0.0;
	
	Rtt_ASSERT( sender == view );
	Rtt_ASSERT( view );
	Rtt_ASSERT( ! view.runtime );
	Rtt_ASSERT( viewController );
	Rtt_ASSERT( view == viewController.view );

	IPhonePlatform *platform = new IPhonePlatform( view );
	IPhoneRuntimeDelegate *runtimeDelegate = new IPhoneRuntimeDelegate( view, self, [self coronaDelegate] );
	[view initializeRuntimeWithPlatform:platform runtimeDelegate:runtimeDelegate];

	Runtime *runtime = view.runtime; // runtime is valid after the "initializeRuntime" call

#ifdef Rtt_CUSTOM_CODE
	runtime->SetProperty( Runtime::kIsUsingCustomCode, true );
#endif

	[self setLaunchArgs:self with:_launchOptions];
	
	// The AppDelegate will observe suspend/resume notification, so don't use default observers
	view.observeSuspendResume = NO;

	return [view runWithPath:path parameters:params];
}

- (void) setLaunchArgs:(id<CoronaRuntime>)runtime with:(NSDictionary*)launchOptions {
	using namespace Rtt;
	Runtime *rttRuntime = view.runtime;
	SetLaunchArgs( [UIApplication sharedApplication], _launchOptions, rttRuntime );
	
	NSMutableDictionary *params = [NSMutableDictionary dictionaryWithObject:view forKey:@"CoronaView"];
	if ( launchOptions )
	{
		[params setObject:launchOptions forKey:@"launchOptions"];
	}

	// This should return a CoronaAppDelegate which has the actual logic of looping through the plugin delegates
	// and asking each one to should on the correct values onto the lua stack
	id<CoronaDelegate> delegate = [self getCoronaAppDelegate];
	
	if ( [delegate respondsToSelector:@selector(execute:command:param:)] )
	{
		[delegate execute:runtime command:@"pushLaunchArgs" param:params];
	}
}

- (void)initializeRuntimeWithApplication:(UIApplication*)application options:(NSDictionary*)launchOptions
{
	Rtt_ASSERT( [UIApplication sharedApplication] == application );

	_launchOptions = [launchOptions copy];
	view.launchDelegate = self;
    if ([self runView:view withPath:nil parameters:nil] != (NSInteger)Rtt::Runtime::kSuccess)
    {
        NSLog(@"CoronaSDK: Failed to initialize Corona runtime");
    }
}

// EW: Watch out. There is a class method called initialize.
- (void)initializeWithApplication:(UIApplication*)application options:(NSDictionary*)launchOptions
{
	// Override point for customization after application launch.
	self.viewController = [[[AppViewController alloc] initWithNibName:nil bundle:nil] autorelease];
	self.viewController.wantsFullScreenLayout = YES;

	view = (CoronaView*)viewController.view; Rtt_ASSERT( view );

	[view setAutoresizingMask:UIViewAutoresizingFlexibleHeight|UIViewAutoresizingFlexibleWidth];

	// Get warnings in iOS 6 if we don't do this.
	[window setRootViewController:viewController];
	[window makeKeyAndVisible];

	[self showSplashScreen];

	Rtt::IPhoneTemplate::WillLoadMain();

	// NOTE: Only init runtime *after* all views are set up!
	[self initializeRuntimeWithApplication:application options:launchOptions];
}

// CoronaRuntime
// ----------------------------------------------------------------------------

- (UIWindow *)appWindow
{
	return window;
}

- (UIViewController *)appViewController
{
	return viewController;
}

- (lua_State *)L
{
	return view.runtime->VMContext().L();
}

- (void)suspend
{
	Rtt_ASSERT( fSuspendCount >= 0 );

	if ( 0 == fSuspendCount++ )
	{
		view.runtime->Suspend();
	}
}

- (void)resume
{
	Rtt_ASSERT( fSuspendCount > 0 );

	if ( 0 == --fSuspendCount )
	{
		view.runtime->Resume();
	}
}

// ----------------------------------------------------------------------------

// UIApplicationDelegate
// ----------------------------------------------------------------------------

- (id<CoronaDelegate>) getCoronaAppDelegate
{
	if ( view && [view runtimeDelegate] )
	{
		Rtt::CoronaViewRuntimeDelegate* coronaViewDelegate = [view runtimeDelegate];
		Rtt::IPhoneRuntimeDelegate* del = static_cast<Rtt::IPhoneRuntimeDelegate*>((coronaViewDelegate));
		return del->GetCoronaDelegate();
	}
	
	return fCoronaDelegate;
}

// For 4.2 and above support
- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation
{
	using namespace Rtt;

	BOOL result = NO;

	if ( [[self getCoronaAppDelegate] respondsToSelector:_cmd] )
	{
		result = [[self getCoronaAppDelegate] application:application openURL:url sourceApplication:sourceApplication annotation:annotation];
		}

	if ( ! result )
	{
		SystemOpenEvent e( [[url absoluteString] UTF8String] );
		Runtime *runtime = view.runtime;
		if ( Rtt_VERIFY( runtime ) )
		{
			runtime->DispatchEvent( e );
		}
	}

	return result;
}

- (void) showSplashScreen
{
	UIImage *splashImage = [UIImage imageNamed:@"_CoronaSplashScreen.png"];

	if (splashImage != nil)
	{
		// Construct a view heirarchy with a black background and the image view inset on top of that
		// (the background view is necessary because without it the areas not covered by the UIImageView
		// flash magenta in the Xcode iOS Simulator and this worries devs)

		splashView = [[UIView alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
		splashView.backgroundColor = [UIColor blackColor];

		UIImageView *splashImageView = [[[UIImageView alloc] initWithImage:splashImage] autorelease];
		splashImageView.contentMode = UIViewContentModeScaleAspectFit;
		splashImageView.backgroundColor = [UIColor blackColor];

		CGRect splashImageFrame;
		CGRect screenBounds = [[UIScreen mainScreen] bounds];

		// Make sure we never scale up the image
		if (screenBounds.size.height > splashImage.size.height && screenBounds.size.width > splashImage.size.width)
		{
			splashImageFrame.size = splashImage.size;
		}
		else
		{
			splashImageFrame.size = screenBounds.size;
		}

		splashImageView.frame = splashImageFrame;
		splashImageView.center = CGPointMake(CGRectGetMidX(screenBounds), CGRectGetMidY(screenBounds));
		[splashView addSubview:splashImageView];

		// NOTE: we need to have to have set self.window.rootViewController before we get here
		[self.window.rootViewController.view addSubview:splashView];
		[self.window.rootViewController.view bringSubviewToFront:splashView];
		splashView.layer.zPosition = MAXFLOAT;
	}
}


- (void)didLoadMain:(id<CoronaRuntime>)runtime
{
	if (splashView != nil)
	{
		CoronaView *coronaView = (CoronaView *)[self view];

		[coronaView suspend];

		NSTimeInterval timeSoFar = [NSDate timeIntervalSinceReferenceDate] - fAppLaunchTime;
		double SPLASH_TIME = 2.5;
		double pauseTime = (SPLASH_TIME - timeSoFar);
		double duration = 0.4; // to fade the splash image
		// NSLog(@"Time since app start: %g (pauseTime %g)", [NSDate timeIntervalSinceReferenceDate] - fAppLaunchTime, pauseTime);

		[UIView animateWithDuration:duration
							  delay:(pauseTime - duration)
							options:UIViewAnimationOptionCurveEaseIn
						 animations:^{
							 [splashView subviews][0].alpha = .0f;
						 } completion:^(BOOL finished){
							 [self hideSplashScreen];
						 }];
	}
}

- (void) hideSplashScreen
{
	[splashView removeFromSuperview];
	[splashView release];
	splashView = nil;

	CoronaView *coronaView = (CoronaView *)[self view];
	[coronaView resume];
	[coronaView display];  // necessary to avoid magenta flashes in the iOS Simulator
}

- (BOOL)application:(UIApplication *)application willFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
	using namespace Rtt;

	BOOL result = [CoronaAppDelegate handlesUrl:[launchOptions valueForKey:UIApplicationLaunchOptionsURLKey]];

	Class c = CoronaGetDelegateClass();
	if ( c )
	{
		fCoronaDelegate = [[c alloc] init];
	}

	// This is before the IPhoneRuntimeDelegate is created so the this should be the enterprise delegate
	// which is why we can directly return the result
	if ( [[self getCoronaAppDelegate] respondsToSelector:_cmd] )
	{
		result = [[self getCoronaAppDelegate] application:application willFinishLaunchingWithOptions:launchOptions];
	}

	return result;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
	using namespace Rtt;

	// Rtt_TRACE( ("initial accel update interval(%g)\n", [UIAccelerometer sharedAccelerometer].updateInterval ) );

	// NOTE: Default status bar style is set in Info.plist
	[self initializeWithApplication:application options:launchOptions];

	return [[self getCoronaAppDelegate] application:application didFinishLaunchingWithOptions:launchOptions];
}

- (void)applicationWillTerminate:(UIApplication *)application
{
	using namespace Rtt;
	
	[[self getCoronaAppDelegate] performSelector:_cmd withObject:application];
	
	if ( view.runtime )
	{
		[fCoronaDelegate release];
		// fCoronaDelegate is in the fCoronaDelegatesArray so it was released there
		fCoronaDelegate = nil;
		
		[view terminate];
	}
}

// If UIApplicationExitsOnSuspend is true, then iOS will call applicationWillTerminate:
// automatically. So this method, doesn't need to do anything. Also, appliationWillResignActive: 
// does *not* get called.
// (EW: I'm stepping through on iPad 2/iOS 4.3, and for UIApplicationExitsOnSuspend=true, applicationDidEnterBackground: and then applicationWillTerminate: is called.)
// If it's false, then we want to do a true suspend (no bkgd processing) in which case
// we rely on the fact that appliationWillResignActive: is called which puts Corona in 
// a suspended state, followed by a call to this method, 
// Addendum (EW): In trying to support backgrounding and screen locks, I discovered the callback behavior is non-deterministic for the call order of
// applicationWillResignActive: and applicationDidEnterBackground:. In one test where I had both audio backgrounding set and the audio session set to MediaPlayback,
// applicationDidEnterBackground: was called first for backgrounding then applicationWillResignActive:. But for a different audio session, 
// the order was reversed. This basically makes it impossible to distinguish the difference between a screen lock event and a backgrounding event which
// unfortunately have two subtly different behaviors we have to implement. In addition, we still have to deal with interruptions and termination which 
// makes the whole thing even more complex. There is no API to find out if the screen is locked. The best work-around is to test for data protection events on iOS 4,
// but this only works if the user enabled data protection on their device. So we have to make some guesses and assumptions and do some heroics to get this to work
// the way users might expect.
- (void)applicationDidEnterBackground:(UIApplication *)application
{
	[[self getCoronaAppDelegate] performSelector:_cmd withObject:application];
	appEnteredBackground = YES;
}

// If UIApplicationExitsOnSuspend is false, then iOS will call applicationWillEnterForeground:
// and then applicationDidBecomeActive:
- (void)applicationWillEnterForeground:(UIApplication *)application
{
	appEnteredBackground = NO;

	[[self getCoronaAppDelegate] performSelector:_cmd withObject:application];
}

// Note: In addition to terminating and backgrounding, this can also be called for screen lock (screen lock does not call applicationDidEnterBackground:)
// and interruptions (e.g. phone calls, alarms).
- (void)applicationWillResignActive:(UIApplication *)application
{
	using namespace Rtt;

	[[self getCoronaAppDelegate] performSelector:_cmd withObject:application];
	
	// System is suspending app, e.g. phone call
	// possible modes: screen lock, background, suspend, interruption/other-app
	
	// Because it is basically impossible to tell the difference between screen lock and backgrounding, we need to make some guesses.
	
	// Case 1: We are in an interruption. We might be able to tell we are in an interruption assuming Apple always fires our Core Audio callback first. (Apple has not documented this, I have not tested, but Core Audio team thinks this is the case.) We can query our Audio Session manager for the information.
	// Case 2: applicationDidEnterBackground did fire first and appEnteredBackground is YES in which case we know we are backgrounding.
	// Case 3: appEnteredBackground did not fire first so we can't tell what's going on. This could be screen lock, background, suspend, interruption/other-app.
	// Case 3b: 
	BOOL exits = [[[[NSBundle mainBundle] infoDictionary] objectForKey:@"UIApplicationExitsOnSuspend"] boolValue];
	BOOL wantsBackgroundProcessing;

	Runtime *runtime = view.runtime;
	PlatformAudioSessionManager *audioSessionMananger = PlatformAudioSessionManager::SharedInstance();
	
	BOOL supportsBackgroundAudio = (BOOL)audioSessionMananger->SupportsBackgroundAudio();
	BOOL supportsBackgroundLocation = [[[[NSBundle mainBundle] infoDictionary] objectForKey:@"UIBackgroundModes"] containsObject:@"location"];
	BOOL supportsBackgroundVoIP = [[[[NSBundle mainBundle] infoDictionary] objectForKey:@"UIBackgroundModes"] containsObject:@"voip"];
	BOOL allowsAudioDuringScreenLock = audioSessionMananger->AllowsAudioDuringScreenLock();
	
	// iOS currently defines 3 modes in the Info.plist under UIBackgroundModes: audio, location, and voip.
	// We don't support voip in Corona so we will ignore it.
	// So only audio and location should keep Corona from suspending. If these keys are not set, set the mode to suspend.
	if ( YES == supportsBackgroundAudio || YES == supportsBackgroundLocation || YES == supportsBackgroundVoIP )
	{
		wantsBackgroundProcessing = YES;
	}
	else
	{
		wantsBackgroundProcessing = NO;
	}
	
	if ( audioSessionMananger->IsInInterruption() )
	{
		runtime->SetSuspendOverrideProperty( Runtime::kSuspendAll, true );
	}
	else if( YES == appEnteredBackground ) // Case 2:
	{
		if( YES == exits )
		{
			// The old code suspended here so we do the same for now and wait for applicationWillTerminate: to do later handling.
			runtime->SetSuspendOverrideProperty( Runtime::kSuspendAll, true );

		}
		else
		{
			if ( YES == wantsBackgroundProcessing )
			{
				runtime->SetSuspendOverrideProperty( Runtime::kBackgroundAudio, supportsBackgroundAudio );
				runtime->SetSuspendOverrideProperty( Runtime::kBackgroundLocation, supportsBackgroundLocation );
				runtime->SetSuspendOverrideProperty( Runtime::kBackgroundVoIP, supportsBackgroundVoIP );
			}
			else
			{
				runtime->SetSuspendOverrideProperty( Runtime::kSuspendAll, true );

			}
		}
	}
	else // now we must guess: screen lock or backgrounding
	{
		// TODO: We could try to use data protection notifications as additional hints.
		// Look at applicationProtectedDataWillBecomeUnavailable:, UIApplicationProtectedDataDidBecomeAvailable and delegate callbacks
		/*
		if ( NO == [[UIApplication sharedApplication] protectedDataAvailable] ) // Never tried
		{
			runtime->Suspend( Runtime::kScreenLock );			
		}
		*/

		// If screen lock, we don't want to suspend if the user has an audio session mode that permits audio.
		// If backgrounding, we don't want to suspend if the user has specified they want audio or location backgrounding.
		// So one stategy is to look at both permits audio and wantsBackgroundProcessing...if either are true, then put in backgrounding mode.
		// Since we can't distinguish between screen lock and backgrounding, we set backgrounding
		if ( YES == wantsBackgroundProcessing || allowsAudioDuringScreenLock )
		{
			runtime->SetSuspendOverrideProperty( Runtime::kBackgroundAudio, ( supportsBackgroundAudio || allowsAudioDuringScreenLock )  );
			runtime->SetSuspendOverrideProperty( Runtime::kBackgroundLocation, supportsBackgroundLocation );
			runtime->SetSuspendOverrideProperty( Runtime::kBackgroundVoIP, supportsBackgroundVoIP );
		}
		else
		{
			runtime->SetSuspendOverrideProperty( Runtime::kSuspendAll, true );
		}
		
	}
	
	// iPad 3 bug radar://11477835
	// We're going to try destroying the framebuffers to avoid the problem.
	// I created a new set of methods in our CoronaView for suspend/resume.
	// This should probably go away when we update to use GLKit.
	CoronaView* glview = (CoronaView*)[self view];
	[glview suspend];
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
	using namespace Rtt;


	// We need this guard b/c on app launch, this method is invoked and we need
	// to distinguish between the startup case and a potential resume case.
	if ( fIsAppStarted  )
	{
		// iPad 3 bug radar://11477835
		// This will re-create the framebuffers we destroyed on suspend.
		// I created a new set of methods in our CoronaView for suspend/resume.
		// This should probably go away when we update to use GLKit.
		[view resume];
	}
	else
	{
		fIsAppStarted = YES;
	}

	[[self getCoronaAppDelegate] performSelector:_cmd withObject:application];
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
	using namespace Rtt;

	// Fire low memory warning event (this event doesn't have any fields)
	MemoryWarningEvent e;
	view.runtime->DispatchEvent( e );

	view.runtime->Collect();

	[[self getCoronaAppDelegate] performSelector:_cmd withObject:application];
}

- (void)application:(UIApplication *)application
	handleWatchKitExtensionRequest:(NSDictionary *)userInfo
	reply:(void (^)(NSDictionary *replyInfo))reply
{
	id<CoronaDelegate> target = [self getCoronaAppDelegate];
	if ( [target respondsToSelector:_cmd] )
	{
		[target application:application handleWatchKitExtensionRequest:userInfo reply:reply];
	}
}

// UIAccelerometerDelegate
// ----------------------------------------------------------------------------

- (void)accelerometer:(UIAccelerometer*)accelerometer didAccelerate:(UIAcceleration*)acceleration
{
	using namespace Rtt;

	const UIAccelerationValue kFilteringFactor = 0.1;

	UIAccelerationValue currentAccelX = acceleration.x;
	UIAccelerationValue currentAccelY = acceleration.y;
	UIAccelerationValue currentAccelZ = acceleration.z;

	UIAccelerationValue x = fGravityAccel[0];
	UIAccelerationValue y = fGravityAccel[1];
	UIAccelerationValue z = fGravityAccel[2];

	double rawAccel[3];
	rawAccel[0] = currentAccelX;
	rawAccel[1] = currentAccelY;
	rawAccel[2] = currentAccelZ;

	// Use a basic low-pass filter to keep only the gravity component of each axis.
	x = (currentAccelX * kFilteringFactor) + (x * (1.0 - kFilteringFactor));
	y = (currentAccelY * kFilteringFactor) + (y * (1.0 - kFilteringFactor));
	z = (currentAccelZ * kFilteringFactor) + (z * (1.0 - kFilteringFactor));
	fGravityAccel[0] = x;
	fGravityAccel[1] = y;
	fGravityAccel[2] = z;

	// Subtract the low-pass value from the current value to get a simplified high-pass filter
	x = currentAccelX - x;
	y = currentAccelY - y;
	z = currentAccelZ - z;
	fInstantAccel[0] = x;
	fInstantAccel[1] = y;
	fInstantAccel[2] = z;


	// Compute the magnitude of the current acceleration 
	// and if above a given threshold, it's a shake
	bool isShake = false;
	const UIAccelerationValue kShakeAccelSq = 4.0;
	UIAccelerationValue accelSq = x*x + y*y + z*z;
	if ( accelSq >= kShakeAccelSq )
	{
		const CFTimeInterval kMinShakeInterval = 0.5;

		CFTimeInterval t = CFAbsoluteTimeGetCurrent();
		isShake = ( t > fPreviousShakeTime + kMinShakeInterval );
		if ( isShake )
		{
			fPreviousShakeTime = t;
		}
	}

	// throw away first value since we don't have a delta
	if ( 0.0 == lastAccelerometerTimeStamp )
	{
		lastAccelerometerTimeStamp = acceleration.timestamp;
	}
	else
	{
		AccelerometerEvent e( fGravityAccel, fInstantAccel, rawAccel, isShake, acceleration.timestamp-lastAccelerometerTimeStamp );
		view.runtime->DispatchEvent( e );
		lastAccelerometerTimeStamp = acceleration.timestamp;
	}
}

// ----------------------------------------------------------------------------

-(id)init
{
	self = [super init];
	
	if ( self )
	{
		[self initSelf];
		appEnteredBackground = NO; // Do NOT put this in initSelf
	}

	return self;
}

- (void)dealloc
{
	[self deallocSelf];
	[super dealloc];
}


- (void)initSelf
{
	window = nil;
	view = nil;
	viewController = nil;
	fCoronaDelegate = nil;
	fSuspendCount = 0;
	for ( size_t i = 0; i < sizeof( fGravityAccel ) / sizeof( fGravityAccel[0] ); i++ )
	{
		Rtt_STATIC_ASSERT( sizeof( fGravityAccel ) == sizeof( fInstantAccel ) );
		Rtt_STATIC_ASSERT( sizeof( fGravityAccel[0] ) == sizeof( fInstantAccel[0] ) );
		fGravityAccel[i] = 0.;
		fInstantAccel[i] = 0.;
	}
	fPreviousShakeTime = 0.;
	fIsAppStarted = NO;
	fAppLaunchTime = [NSDate timeIntervalSinceReferenceDate];
}

- (void)deallocSelf
{
	[view terminate];
	Rtt_ASSERT( NULL == [view runtime] );

	[viewController release];
	[view release];
	[window release];
}

- (id<CoronaDelegate>)coronaDelegate
{
	return fCoronaDelegate;
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
	// NSLog(@"alertView:clickedButtonAtIndex: %@", alertView.title);
	
	if ([alertView.title hasSuffix:@" error"])
	{
		[NSException raise:@"Corona Runtime Error" format:@"%@", alertView.message];
	}
}

@end

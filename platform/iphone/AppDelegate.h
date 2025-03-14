//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#import <UIKit/UIKit.h>
#include <math.h>

//Must include this for isinf usage in mapkit
using namespace std;
#import <MapKit/MapKit.h>

#import "CoronaRuntime.h"
#import "CoronaViewPrivate.h"
#import "CoronaViewControllerPrivate.h"

// ----------------------------------------------------------------------------

namespace Rtt
{
	class IPhonePlatform;
	class Runtime;
}

// ----------------------------------------------------------------------------

@class AddressAnnotationWithCallout;
@class CoronaView;
@class UIViewController;
@protocol CoronaDelegate;

// ----------------------------------------------------------------------------

@interface AppViewController : CoronaViewController
{
	UIResponder *fNextResponder;
}

- (void)setNextResponder:(UIResponder *)responder;

@property (nonatomic, assign) BOOL prefersHomeIndicatorAutoHidden;
@property (nonatomic, assign) UIRectEdge preferredScreenEdgesDeferringSystemGestures;

@end

// ----------------------------------------------------------------------------

@interface AppDelegate : NSObject<
							CoronaRuntime,
							UIApplicationDelegate,
							UIAccelerometerDelegate >
{
	UIWindow *window;
	CoronaView *view;
	AppViewController *viewController;
	id<CoronaDelegate> fCoronaDelegate;
	int fSuspendCount;
	UIAccelerationValue fGravityAccel[3];
	UIAccelerationValue fInstantAccel[3];
	CFTimeInterval fPreviousShakeTime;
	BOOL fIsAppStarted;
	BOOL appEnteredBackground;
	NSTimeInterval lastAccelerometerTimeStamp;
	NSTimeInterval fAppLaunchTime;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, readonly) UIView *view;
@property (nonatomic, retain) IBOutlet UIViewController *viewController;

@property (nonatomic, assign) NSTimeInterval lastAccelerometerTimeStamp; // Needed a public variable to initialize the variable from the caller to 0 when the accelerometer starts.

- (Rtt::Runtime *)runtime;

- (id<CoronaDelegate>)coronaDelegate;

- (void)didLoadMain:(id<CoronaRuntime>)runtime;

@end


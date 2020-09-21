//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#import "CoronaCards/CoronaView.h"

#import <UIKit/UIKit.h>

#import "CoronaRuntime.h"
#include "Rtt_MetalAngleTypes.h"

// ----------------------------------------------------------------------------

@class CoronaViewController;
@protocol CoronaGyroscopeObserver;

// C++ Forward Declarations
namespace Rtt
{

class Runtime;
class CoronaViewRuntimeDelegate;
class IPhonePlatformBase;

} // Rtt

// ----------------------------------------------------------------------------

// Used in SDK/Enterprise to enable custom initialization of the Rtt::Runtime
// and Rtt::IPhonePlatformBase during relaunches
@protocol CoronaViewLaunchDelegate

@required
- (NSInteger)runView:(CoronaView *)sender withPath:(NSString *)path parameters:(NSDictionary *) params;

@end

// ----------------------------------------------------------------------------

@interface CoronaView() <>

@property (nonatomic, readonly) Rtt::Runtime *runtime;
@property (nonatomic, readonly) Rtt::CoronaViewRuntimeDelegate *runtimeDelegate;
@property (nonatomic, readonly) Rtt_GLKViewController *viewController;
@property (nonatomic, readwrite, getter=inhibitCount, setter=setInhibitCount:) int fInhibitCount;
@property (nonatomic, readwrite, getter=getForceTouchSupport, setter=setForceTouchSupport:) BOOL fSupportsForceTouch;
@property (nonatomic, assign) BOOL observeSuspendResume;
@property (nonatomic, assign) BOOL beginRunLoopManually;
@property (nonatomic, assign) id< CoronaRuntime > pluginContext; // Weak reference. CoronaViewController has the strong reference.
@property (nonatomic, assign) id< CoronaViewLaunchDelegate > launchDelegate;

@property (nonatomic, assign) id< CoronaGyroscopeObserver > gyroscopeObserver;

- (void)initializeRuntimeWithPlatform:(Rtt::IPhonePlatformBase *)platform runtimeDelegate:(Rtt::CoronaViewRuntimeDelegate *)runtimeDelegate;

- (NSInteger)runWithPath:(NSString*)path parameters:(NSDictionary *)params;

- (NSInteger)beginRunLoop;

- (void)terminate;

@end

// ----------------------------------------------------------------------------

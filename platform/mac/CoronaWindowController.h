//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#import <QuartzCore/QuartzCore.h>

#import <Cocoa/Cocoa.h>

#import "CoronaViewPrivate.h"

// ----------------------------------------------------------------------------

@class CoronaView;
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 101200
@protocol CAAnimationDelegate;
#endif

namespace Rtt
{
	class RuntimeDelegate;
	class RuntimeDelegateWrapper;
}

@interface CoronaWindowController : NSWindowController< CoronaViewControllerDelegate
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 101200
, CAAnimationDelegate
#endif
>
{
	@private
		CoronaView *fView;
		Rtt::RuntimeDelegateWrapper *fRuntimeDelegateWrapper;
		BOOL fIsInitialized;
}

@property (nonatomic, readonly, getter=view) CoronaView *fView;

- (id)initWithPath:(NSString*)path width:(int)width height:(int)height title:(NSString *)windowTitle resizable:(bool) resizable;

- (void)didPrepare;


- (void)show;
- (void)hide;


// For when window is fading out and is about to close, call this to bring it back.
- (void) resurrectWindow;
@end

// ----------------------------------------------------------------------------

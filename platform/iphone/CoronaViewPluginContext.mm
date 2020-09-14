//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#import "CoronaViewPluginContext.h"

#import "CoronaViewController.h"
#import "CoronaViewPrivate.h"
#include "Display/Rtt_Display.h"
#include "Rtt_LuaContext.h"
#include "Rtt_Runtime.h"

// ----------------------------------------------------------------------------

@interface CoronaViewPluginContext()
{
@private
	CoronaViewController *fOwner; // Weak ptr
}
@end

// ----------------------------------------------------------------------------

@implementation CoronaViewPluginContext

- (id)initWithOwner:(CoronaViewController *)owner
{
	self = [super init];
	if ( self )
	{
		fOwner = owner;
	}
	return self;
}

- (UIWindow *)appWindow
{
	return fOwner.view.window;
}

- (UIViewController *)appViewController
{
	return fOwner;
}

- (lua_State *)L
{
	CoronaView *view = (CoronaView *)fOwner.view;
	return view.runtime->VMContext().L();
}

- (void)suspend
{
	CoronaView *view = (CoronaView *)fOwner.view;
	[view suspend];
}

- (void)resume
{
	CoronaView *view = (CoronaView *)fOwner.view;
	[view resume];
}

@end

// ----------------------------------------------------------------------------


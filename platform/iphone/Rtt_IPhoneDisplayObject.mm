//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_IPhoneDisplayObject.h"
#include "Rtt_IPhonePlatformBase.h"

#import <UIKit/UIView.h>
#import <UIKit/UIScreen.h>
#import "CoronaLuaObjC.h"
#import "CoronaLuaObjC+NSObject.h"

#include "Display/Rtt_Display.h"
#include "Rtt_Runtime.h"


#include "Rtt_Lua.h"
#include "Display/Rtt_GroupObject.h"
#include "Display/Rtt_LuaLibDisplay.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

IPhoneDisplayObject::IPhoneDisplayObject( const Rect& bounds )
:	fSelfBounds( bounds ),
	fView( nil ),
	fCoronaView( nil ),
	fHidden( NO )
{
	Real w = bounds.Width();
	Real h = bounds.Height();
	Real halfW = Rtt_RealDiv2( w );
	Real halfH = Rtt_RealDiv2( h );

	// The UIView's self transform is relative to it's center, but the DisplayObject's
	// transform included that translation, so we need to factor this out during Build()
	// NOTE: The incoming bounds are in content coordinates, not native UIView coordinates,
	// so we must record these separately instead of relying on the values of [fView center]
	float fViewCenterX = bounds.xMin + halfW;
	float fViewCenterY = bounds.yMin + halfH;

	// Update DisplayObject so that it corresponds to the actual position of the UIView
	// where DisplayObject's self bounds will be centered around its local origin.
	Translate( fViewCenterX, fViewCenterY );

	// The self bounds needs to be centered around DisplayObject's local origin
	// even though UIView's bounds will not be.
	fSelfBounds.MoveCenterToOrigin();
}

IPhoneDisplayObject::~IPhoneDisplayObject()
{
	[fView removeFromSuperview];
	[fView release];
}

void
IPhoneDisplayObject::Preinitialize( const Display& display )
{
	PlatformDisplayObject::Preinitialize( display );

    // We can access the GLView at this point via the display's runtime, so save it for later
    const Rtt::IPhonePlatformBase& platform = static_cast< const Rtt::IPhonePlatformBase& >( display.GetRuntime().Platform() );
    fCoronaView = platform.GetView();
}

void
IPhoneDisplayObject::InitializeView( UIView *view )
{
	Rtt_ASSERT( ! fView );
	Rtt_ASSERT( view );

	fView = [view retain];
}

void
IPhoneDisplayObject::SetFocus()
{
	[fView becomeFirstResponder];
}

void
IPhoneDisplayObject::DidMoveOffscreen()
{
	fHidden = fView.hidden; // Store original value while offscreen
	fView.hidden = YES;		// Force view to be hidden
}

void
IPhoneDisplayObject::WillMoveOnscreen()
{
	fView.hidden = fHidden; // Restore view's hidden parameter
	fHidden = NO;			// Restore to default value
}

void
IPhoneDisplayObject::Prepare( const Display& display )
{
  	Super::Prepare( display );
    
    if ( ShouldPrepare() ) {
        Preinitialize( display );
        
        Rect screenBounds;
        GetScreenBounds( screenBounds );

        CGRect newFrame = CGRectMake( screenBounds.xMin, screenBounds.yMin, screenBounds.Width(), screenBounds.Height() );
        [fView setFrame:newFrame];

		CGPoint c;
		c.x = screenBounds.Width() / 2;
		c.y = screenBounds.Height() / 2;
		fView.center = c;
		fView.transform = CGAffineTransformIdentity;
		fView.frame = newFrame;

        if ( ! fHidden )
        {
            // Only restore the object if the user hasn't requested it hidden
            [fView setHidden:NO];
        }

    }
	
    if ( nil == [fView superview] )
    {
        [fCoronaView addSubview:fView];
    }
}

void
IPhoneDisplayObject::Translate( Real dx, Real dy )
{
	Super::Translate( dx, dy );
}

void
IPhoneDisplayObject::Draw( Renderer& renderer ) const
{
}

void
IPhoneDisplayObject::GetSelfBounds( Rect& rect ) const
{
	rect = fSelfBounds;
}

void
IPhoneDisplayObject::SetSelfBounds( Real width, Real height )
{
	if ( width > Rtt_REAL_0 )
	{
		fSelfBounds.Initialize( Rtt_RealDiv2(width), Rtt_RealDiv2(GetGeometricProperty(kHeight)) );
	}
	if ( height > Rtt_REAL_0 )
	{
		fSelfBounds.Initialize( Rtt_RealDiv2(GetGeometricProperty(kWidth)), Rtt_RealDiv2(height) );
	}

	// Causes prepare to be called which does the actual resizing
	Invalidate( kGeometryFlag | kStageBoundsFlag | kTransformFlag );
}

int
IPhoneDisplayObject::ValueForKey( lua_State *L, const char key[] ) const
{
	Rtt_ASSERT( key );

	int result = 1;

	if ( strcmp( "isVisible", key ) == 0 )
	{
		lua_pushboolean( L, ! fView.hidden );
	}
	else if ( strcmp( "alpha", key ) == 0 )
	{
		lua_pushnumber( L, fView.alpha );
	}
	else if ( strcmp( "hasBackground", key ) == 0 )
	{
		// TODO: When we allow changing bkgd colors, we should cache this as
		// a separate property which would ignore the UIColor.
		
		if (fView.backgroundColor == nil)
		{
			// We've never set "hasBackground", default is true
			lua_pushboolean( L, true );
		}
		else
		{
			UIColor *color = fView.backgroundColor;
			CGFloat alpha = CGColorGetAlpha( [color CGColor] );
			lua_pushboolean( L, alpha > FLT_EPSILON );
		}
	}
	else
	{
		result = 0;
	}

	return result;
}

bool
IPhoneDisplayObject::SetValueForKey( lua_State *L, const char key[], int valueIndex )
{
	Rtt_ASSERT( key );

	bool result = true;

	if ( strcmp( "isVisible", key ) == 0 )
	{
		fView.hidden = ! lua_toboolean( L, valueIndex );
	}
	else if ( strcmp( "alpha", key ) == 0 )
	{
		fView.alpha = lua_tonumber( L, valueIndex );
	}
	else if ( strcmp( "hasBackground", key ) == 0 )
	{
		UIColor *color = lua_toboolean( L, valueIndex ) ? [UIColor whiteColor] : [UIColor clearColor];
		fView.backgroundColor = color;
	}
	else
	{
		result = false;
	}

	return result;
}

id
IPhoneDisplayObject::GetNativeTarget() const
{
	return GetView();
}

int
IPhoneDisplayObject::GetNativeProperty( lua_State *L, const char key[] ) const
{
	id target = GetNativeTarget();
	int result = [target pushLuaValue:L forKey:@(key)];

	if ( 0 == result )
	{
		result = Super::GetNativeProperty( L, key );
	}

	return result;
}

bool
IPhoneDisplayObject::SetNativeProperty( lua_State *L, const char key[], int valueIndex )
{
	id target = GetNativeTarget();
	bool result = [target set:L luaValue:valueIndex forKey:@(key)];

	if ( ! result )
	{
		result = Super::SetNativeProperty( L, key, valueIndex );
	}

	return result;
}

CoronaView*
IPhoneDisplayObject::GetCoronaView() const
{
    Rtt_ASSERT(fCoronaView); // this is set in PreInitialize()
	return fCoronaView;
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------


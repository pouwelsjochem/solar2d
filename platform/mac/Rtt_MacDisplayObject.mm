//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_MacDisplayObject.h"
#include "Rtt_MacPlatform.h"
#include "Rtt_MacSimulator.h"

#import <AppKit/AppKit.h>
#import "AppDelegate.h"
#import "GLView.h"
#include "Rtt_Display.h"

#include <algorithm>
#include <vector>

#include "Rtt_Lua.h"
#include "Rtt_Runtime.h"
#include "Display/Rtt_LuaLibDisplay.h"
#include "Display/Rtt_StageObject.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

static std::vector<MacDisplayObject*> sBackingScaleListeners;

MacDisplayObject::MacDisplayObject( const Rect& bounds )
:	fSelfBounds( bounds),
	fView( nil ),
	fCoronaView( nil ),
	fIsHidden ( false )
{
	// Note: Setting the reference point to center is not done in any of the other implementations because
	// fSelfBounds is already centered/converted unlike this implementation.
	// This solves the problem, but will possibly be a problem if/when we support object resizing.
	// Either this code should be converted to center fSelfBounds initially or the 
	// subclass virtual function will need to account for the offset.
	
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

	sBackingScaleListeners.push_back( this );
	
}

MacDisplayObject::~MacDisplayObject()
{
	sBackingScaleListeners.erase(
		std::remove( sBackingScaleListeners.begin(), sBackingScaleListeners.end(), this ),
		sBackingScaleListeners.end() );

	[fView removeFromSuperview];
	[fView release];
	// I don't think this is working quite right. Removing an element in the middle of the list seems to keep some intermediate focus.
	// But it doesn't crash.
	RecomputeNextKeyViews();
}

void
MacDisplayObject::InitializeView( NSView *view )
{
	Rtt_ASSERT( ! fView );
	Rtt_ASSERT( view );

	fView = [view retain];
	NSRect view_frame = [view frame];
	fNSViewFrame = view_frame;
}

void
MacDisplayObject::SetFocus()
{
	[[fView window] performSelector:@selector(makeFirstResponder:) withObject:fView afterDelay:0.0];
}
void
MacDisplayObject::DidMoveOffscreen()
{
}

void
MacDisplayObject::WillMoveOnscreen()
{
}

bool
MacDisplayObject::CanCull() const
{
	// Disable culling for all native display objects.
	// Note: This is needed so that the Build() function will get called when a native object
	//       is being moved partially or completely offscreen.
	return false;
}

void
MacDisplayObject::NotifyBackingScaleChanged( float previousScale, float currentScale )
{
	if ( ( previousScale <= 0.0f ) || ( currentScale <= 0.0f ) )
	{
		return;
	}

	for ( MacDisplayObject* object : sBackingScaleListeners )
	{
		if ( object )
		{
			object->DidChangeBackingScale( previousScale, currentScale );
		}
	}
}


void
MacDisplayObject::PreInitialize( const Display& display )
{
	PlatformDisplayObject::Preinitialize( display );
    
    // We can access the GLView at this point via the display's runtime, so save it for later
    const Rtt::MacPlatform& platform = static_cast< const Rtt::MacPlatform& >( display.GetRuntime().Platform() );
    fCoronaView = platform.GetView();
}

void
MacDisplayObject::Translate( Real dx, Real dy )
{
	Super::Translate( dx, dy );
}

void
MacDisplayObject::Prepare( const Display& display )
{

	Super::Prepare( display );
	
	if ( ShouldPrepare() )
	{
		
		PreInitialize( display );
		
		Rect screenBounds;
		GetScreenBounds( screenBounds );

		NSRect r = NSMakeRect( screenBounds.xMin, screenBounds.yMin, screenBounds.Width(), screenBounds.Height() );
		CGFloat backingScaleFactor = [[fCoronaView window] backingScaleFactor];
		if (backingScaleFactor > 1.0)
		{
			r.origin.x /= backingScaleFactor;
			r.origin.y /= backingScaleFactor;
			r.size.width /= backingScaleFactor;
			r.size.height /= backingScaleFactor;
		}

		[fView setFrame:r];
		fNSViewFrame = r;
		if ( ! fIsHidden )
		{
			// Only restore the object if the user hasn't requested it hidden
			[fView setHidden:NO];
		}
		
		const Matrix& tSelf = GetMatrix();
		if ( tSelf.IsIdentity() )
		{
			// What is fSelfBounds? Is it our frame? What is the mapping from fSelfBounds to bounds or frame?
			//[fView setFrameOrigin:fNSViewFrame.origin];
			[fView setFrameOrigin:fNSViewFrame.origin];
			
		}
		else
		{
			[fView setFrameOrigin:r.origin];
			[fView setFrameSize:r.size];
		}
	}
	
	// We have a problem because this Build matrix is computed some time after the object was created.
	// If we insert the object when created, when this method is invoked shortly later, you can see
	// a visible jump in the object's position which looks bad.
	// The workaround is to defer adding the object to the view until after this method has been computed at least once.
	if ( nil == [fView superview] )
	{
		AddSubviewToCoronaView();
	}

}

void
MacDisplayObject::Draw( Renderer& renderer ) const
{
}

void
MacDisplayObject::GetSelfBounds( Rect& rect ) const
{
	rect = fSelfBounds;
}
	
void
MacDisplayObject::SetSelfBounds( Real width, Real height )
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

NSView*
MacDisplayObject::GetCoronaView()
{
    Rtt_ASSERT(fCoronaView); // this is set in PreInitialize()
	return fCoronaView;
}

void
MacDisplayObject::AddSubviewToCoronaView()
{
	GLView* coronaView = (GLView *)GetCoronaView();
	
    [coronaView addSubview:fView];

	// Setting the NSView's wantsLayer is necessary for the native controls to work with the OpenGL canvas
	[fView setWantsLayer:YES];

	// This block tries to set the nextkeyview's for all the objects in the layer host view
	RecomputeNextKeyViews();
}
	
void
MacDisplayObject::RecomputeNextKeyViews()
{
	NSView* coronaView = GetCoronaView();
	
	
	// This block tries to set the nextkeyview's for all the subviews in the layer host view.
	// This is so tabbing to the next field works.
	// This is highly dependent on the order of how the subviews were added/arranged in the subviews array.
	// Our deferred insertion technique to avoid the Build/jumping problem may cause problems in the future.
	NSView* lastview = nil;
	NSView* firstview = nil;
	for(NSView* view in [coronaView subviews])
	{
		if(nil == lastview)
		{
			lastview = view;
			firstview = view;
			continue;
		}
//		NSLog(@"lastview: %@, view:%@", lastview, view);
		
		[lastview setNextKeyView:view];
		lastview = view;
	}
	[lastview setNextKeyView:firstview];
}

void
MacDisplayObject::DidChangeBackingScale( float previousScale, float currentScale )
{
	(void)previousScale;
	(void)currentScale;
}

int
MacDisplayObject::ValueForKey( lua_State *L, const char key[] ) const
{
	Rtt_ASSERT( key );

	int result = 1;

	if ( strcmp( "isVisible", key ) == 0 )
	{
		lua_pushboolean( L, ! fIsHidden );
	}
	else if ( strcmp( "alpha", key ) == 0 )
	{
		lua_pushnumber( L, [fView alphaValue] );
	}
	else if ( strcmp( "hasBackground", key ) == 0 )
	{
		Rtt_ASSERT_NOT_IMPLEMENTED();
		lua_pushnil( L );
		/*
		// TODO: When we allow changing bkgd colors, we should cache this as
		// a separate property which would ignore the NSColor.
		NSColor *color = fView.backgroundColor;
		CGFloat alpha = CGColorGetAlpha( [color CGColor] );
		lua_pushboolean( L, alpha > FLT_EPSILON );
		*/
	}
	else
	{
		result = 0;
	}

	return result;
}

bool
MacDisplayObject::SetValueForKey( lua_State *L, const char key[], int valueIndex )
{
	Rtt_ASSERT( key );

	bool result = true;

	if ( strcmp( "isVisible", key ) == 0 )
	{
		[fView setHidden: ! lua_toboolean( L, valueIndex )];
		fIsHidden = ! lua_toboolean( L, valueIndex );
	}
	else if ( strcmp( "alpha", key ) == 0 )
	{
		[fView setAlphaValue: lua_tonumber( L, valueIndex )];
	}
	else if ( strcmp( "hasBackground", key ) == 0 )
	{
		Rtt_ASSERT_NOT_IMPLEMENTED();
		/*
		NSColor *color = lua_toboolean( L, valueIndex ) ? [NSColor whiteColor] : [NSColor clearColor];
		fView.backgroundColor = color;
		*/
	}
	else
	{
		result = false;
	}

	return result;
}
    
    
int
MacDisplayObject::setReturnKey( lua_State *L )
{
    return 0;
}

// TODO: This code won't compile b/c @try depends on C++ exceptions being enabled
// which is a pretty big change that we're not prepared to make right now.
//
// When we move this stuff into a plugin, we can explore whether this is
// feasible to isolate to the plugin itself.
//
// NOTE: Must add #import "CoronaLuaObjC+NSObject.h" in order for this code to work.
#define Rtt_NATIVE_PROPERTIES_MAC 0

#if Rtt_NATIVE_PROPERTIES_MAC
id
MacDisplayObject::GetNativeTarget() const
{
	return GetView();
}

int
MacDisplayObject::GetNativeProperty( lua_State *L, const char key[] ) const
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
MacDisplayObject::SetNativeProperty( lua_State *L, const char key[], int valueIndex )
{
	id target = GetNativeTarget();
	bool result = [target set:L luaValue:valueIndex forKey:@(key)];

	if ( ! result )
	{
		result = Super::SetNativeProperty( L, key, valueIndex );
	}

	return result;
}
#endif // Rtt_NATIVE_PROPERTIES_MAC

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

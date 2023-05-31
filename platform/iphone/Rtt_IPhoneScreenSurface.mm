//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_IPhoneScreenSurface.h"

#import <Availability.h>
#import <UIKit/UIScreen.h>
#include "Rtt_GPU.h"

#ifdef Rtt_IPHONE_ENV
#include "Rtt_IPhoneDevice.h"
#endif

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

IPhoneScreenSurface::IPhoneScreenSurface( Rtt_GLKView *view )
:	fView( view ) // NOTE: Weak ref b/c fView owns the Runtime instance that owns this.
{
	// NOTE: We assume CoronaView's didMoveToWindow is called already
	// which for iOS8, correctly sets this value to UIScreen's nativeScale value.
	fScale = view.contentScaleFactor;
}

IPhoneScreenSurface::~IPhoneScreenSurface()
{
}

void
IPhoneScreenSurface::SetCurrent() const
{
	Rtt_ASSERT( [Rtt_EAGLContext currentContext] == fView.context );
//	[MGLContext setCurrentContext:fView.context];

	//glBindFramebufferOES( GL_FRAMEBUFFER_OES, fView.viewFramebuffer );
}

void
IPhoneScreenSurface::Flush() const
{

}

S32
IPhoneScreenSurface::Width() const
{
	// Return size in pixels
	return fScale * fView.bounds.size.width;
}

S32
IPhoneScreenSurface::Height() const
{
	// Return size in pixels
	return fScale * fView.bounds.size.height;
}

S32
IPhoneScreenSurface::PointsWidth() const
{
	return fView.bounds.size.width;
}

S32
IPhoneScreenSurface::PointsHeight() const
{
	return fView.bounds.size.height;
}

Rtt_EAGLContext*
IPhoneScreenSurface::GetContext() const
{
	return fView.context;
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------


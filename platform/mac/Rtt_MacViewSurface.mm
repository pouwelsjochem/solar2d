//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_MacViewSurface.h"

#import "GLView.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

MacViewSurface::MacViewSurface( GLView* view )
:	fView( view )
{
}

MacViewSurface::~MacViewSurface()
{
//	[fView release];
}

void
MacViewSurface::SetCurrent() const
{

}
void
MacViewSurface::Flush() const
{
}

S32
MacViewSurface::Width() const
{
	return [fView deviceWidth];
}

S32
MacViewSurface::Height() const
{
	return [fView deviceHeight];
}


S32
MacViewSurface::PointsWidth() const
{
	return [fView viewportWidth];
}

S32
MacViewSurface::PointsHeight() const
{
	return [fView viewportHeight];
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------


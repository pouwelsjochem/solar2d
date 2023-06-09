//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_MacViewSurface_H__
#define _Rtt_MacViewSurface_H__

#include "Rtt_PlatformSurface.h"

#include "Rtt_GPU.h"

// ----------------------------------------------------------------------------

@class GLView;

namespace Rtt
{

// ----------------------------------------------------------------------------

class MacViewSurface : public PlatformSurface
{
	Rtt_CLASS_NO_COPIES( MacViewSurface )

	public:
		typedef PlatformSurface Super;

	public:
		MacViewSurface( GLView* view );
		virtual ~MacViewSurface();

	public:
		virtual void SetCurrent() const;
		virtual void Flush() const;

	public:
		virtual S32 Width() const;
		virtual S32 Height() const;
		
		virtual S32 PointsWidth() const;
		virtual S32 PointsHeight() const;
	private:
		GLView* fView;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_MacViewSurface_H__

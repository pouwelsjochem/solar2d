//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_IPhoneScreenSurface_H__
#define _Rtt_IPhoneScreenSurface_H__

#include "Rtt_PlatformSurface.h"
#include "Rtt_MetalAngleTypes.h"
// ----------------------------------------------------------------------------


namespace Rtt
{

// ----------------------------------------------------------------------------

class IPhoneScreenSurface : public PlatformSurface
{
	Rtt_CLASS_NO_COPIES( IPhoneScreenSurface )

	public:
		typedef PlatformSurface Super;

	public:
		IPhoneScreenSurface( Rtt_GLKView *view );
		virtual ~IPhoneScreenSurface();

	public:
		virtual void SetCurrent() const;
		virtual void Flush() const;

	public:
		virtual S32 Width() const;
		virtual S32 Height() const;

		virtual S32 PointsWidth() const;
		virtual S32 PointsHeight() const;

		virtual S32 DeviceWidth() const;
		virtual S32 DeviceHeight() const;

	public:
		Rtt_EAGLContext *GetContext() const;

	private:
		Rtt_GLKView *fView;
		float fScale;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_IPhoneScreenSurface_H__

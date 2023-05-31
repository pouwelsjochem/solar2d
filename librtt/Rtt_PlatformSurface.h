//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_PlatformSurface_H__
#define _Rtt_PlatformSurface_H__

// ----------------------------------------------------------------------------

namespace Rtt
{

class PlatformSurface;

// ----------------------------------------------------------------------------

class PlatformSurface
{
	public:
		PlatformSurface();
		virtual ~PlatformSurface() = 0;

	public:
		virtual void SetCurrent() const = 0;
		virtual void Flush() const = 0;

	public:
		// Size in pixels of underlying surface
		virtual S32 Width() const = 0;
		virtual S32 Height() const = 0;

		// Size in native (platform-specific) units. By default, this is simply
		// the result of Width() and Height(). However, on some platforms, the
		// size of the screen is in scaled pixels, e.g. on iPhone, size is defined
		// in terms of "points" not actual pixels.
		virtual S32 PointsWidth() const;
		virtual S32 PointsHeight() const;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

// TODO: Remove this when OffscreenGPUSurface is moved to a separate file
#include "Rtt_GPU.h"

namespace Rtt
{

// ----------------------------------------------------------------------------

#if ! defined( Rtt_ANDROID_ENV )

// TODO: Move to a separate file
// GPU-specific
class OffscreenGPUSurface : public PlatformSurface
{
	Rtt_CLASS_NO_COPIES( OffscreenGPUSurface )

	public:
		OffscreenGPUSurface( const PlatformSurface& parent );
		virtual ~OffscreenGPUSurface();

	public:
		virtual void SetCurrent() const;
		virtual void Flush() const;

	public:
		virtual S32 Width() const;
		virtual S32 Height() const;

	public:
		bool IsValid() const { return fTexture > 0; }

	protected:
		S32 fWidth;
		S32 fHeight;

		GLuint fFramebuffer; // FBO id
		GLuint fTexture; // texture id
};

#endif // defined( Rtt_ANDROID_ENV )

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_PlatformSurface_H__

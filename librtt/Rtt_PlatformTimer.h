//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_PlatformTimer_H__
#define _Rtt_PlatformTimer_H__

#include "Rtt_MCallback.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

class PlatformTimer
{
	Rtt_CLASS_NO_COPIES( PlatformTimer )

	public:
		PlatformTimer( MCallback& callback );
		virtual ~PlatformTimer();

	public:
		virtual void Start() = 0;
		virtual void Stop() = 0;
		virtual void SetInterval( U32 milliseconds ) = 0;
		virtual bool IsRunning() const = 0;
		// Returns the display refresh rate in Hz.
		// Overridden by platform-specific subclasses (e.g. WinTimer).
		// Defaults to 60 for platforms that do not provide a native query.
		virtual double GetRefreshRate() const { return 60.0; }
		/// <summary>
		///  <para>Controls whether the render loop syncs to the monitor refresh rate.</para>
		///  <para>
		///   When false (default), render runs at the same rate as logic (config.lua fps)
		///   and no duplicate frames are produced.
		///  </para>
		///  <para>
		///   When true, render posts VSYNC-rate ticks independent of the logic rate.
		///   Without engine-side interpolation, render-only frames are redraws of the
		///   same state � this is groundwork for a future interpolation feature.
		///  </para>
		///  <para>Overridden by platform-specific subclasses (e.g. WinTimer).</para>
		/// </summary>
		virtual void SetFrameSync(bool enabled) {}
	
	public:
		// Allow manual invocation
		Rtt_FORCE_INLINE void operator()() { fCallback(); }

	protected:
		Rtt_FORCE_INLINE MCallback& Callback() { return fCallback; }

	private:
		MCallback& fCallback;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_PlatformTimer_H__

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
#ifdef Rtt_WIN_ENV
		virtual void SetInterval(double milliseconds) { SetInterval((U32)(milliseconds + 0.5)); }
#endif
		virtual bool IsRunning() const = 0;
		// Returns the display refresh rate in Hz.
		// Overridden by platform-specific subclasses (e.g. WinTimer).
		// Defaults to 0 for platforms that do not provide a native query.
		virtual double GetRefreshRate() const { return 0.0; }

		/// <summary>
		///  Returns whether render-sync mode is enabled.
		/// </summary>
		virtual bool GetFrameSync() const { return false; }

		/// <summary>
		///  Controls whether the render loop syncs to the monitor refresh rate.
		/// </summary>
		virtual void SetFrameSync(bool enabled) {}
	
		// Returns the wall-clock time in milliseconds spent executing the most recent
		// full frame tick. Returns 0.0 on platforms that do not implement it.
		virtual double GetLastFrameWorkMs() const { return 0.0; }

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

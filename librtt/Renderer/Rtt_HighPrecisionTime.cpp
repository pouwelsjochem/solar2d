//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Config.h" // TODO: Cleanup header include dependencies
#include "Renderer/Rtt_HighPrecisionTime.h"

// ----------------------------------------------------------------------------

#if defined( Rtt_WIN_ENV )
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// On Windows, Rtt_GetAbsoluteTime() deliberately truncates to 100-microsecond
// resolution to avoid 64-bit overflow, then Rtt_AbsoluteToMilliseconds()
// performs integer division, and the result is cast to float — three layers of
// precision loss. The "high precision" wrapper was a no-op as a result.
//
// This implementation bypasses that chain entirely and queries
// QueryPerformanceCounter directly with double-precision arithmetic, giving
// accurate sub-millisecond timing for renderer statistics (preparationTime,
// renderTimeCPU, fRenderTimeGPU, and resource timing fields returned by
// display.getStatistics()).
//
// QueryPerformanceFrequency() is cached as a static since the frequency is
// guaranteed not to change while the system is running.

Rtt_AbsoluteTime
Rtt_GetPreciseAbsoluteTime()
{
    static LARGE_INTEGER freq = {};
    static bool freqCached = false;
    if (!freqCached)
    {
        ::QueryPerformanceFrequency(&freq);
        freqCached = true;
    }

    LARGE_INTEGER now;
    ::QueryPerformanceCounter(&now);
    return (Rtt_AbsoluteTime)now.QuadPart;
}

Rtt::Real
Rtt_PreciseAbsoluteToMilliseconds(Rtt_AbsoluteTime absoluteTime)
{
    static LARGE_INTEGER freq = {};
    static bool freqCached = false;
    if (!freqCached)
    {
        ::QueryPerformanceFrequency(&freq);
        freqCached = true;
    }

    return (Rtt::Real)((double)absoluteTime / (double)freq.QuadPart * 1000.0);
}

Rtt::Real
Rtt_PreciseAbsoluteToMicroseconds(Rtt_AbsoluteTime absoluteTime)
{
    static LARGE_INTEGER freq = {};
    static bool freqCached = false;
    if (!freqCached)
    {
        ::QueryPerformanceFrequency(&freq);
        freqCached = true;
    }

    return (Rtt::Real)((double)absoluteTime / (double)freq.QuadPart * 1000000.0);
}

#else

Rtt_AbsoluteTime
Rtt_GetPreciseAbsoluteTime()
{
	return Rtt_GetAbsoluteTime();
}

Rtt::Real 
Rtt_PreciseAbsoluteToMilliseconds( Rtt_AbsoluteTime absoluteTime )
{
	return static_cast<Rtt::Real>( Rtt_AbsoluteToMilliseconds( absoluteTime ) );
}

Rtt::Real 
Rtt_PreciseAbsoluteToMicroseconds( Rtt_AbsoluteTime absoluteTime )
{
	return static_cast<Rtt::Real>( Rtt_AbsoluteToMicroseconds( absoluteTime ) );
}

#endif
// ----------------------------------------------------------------------------

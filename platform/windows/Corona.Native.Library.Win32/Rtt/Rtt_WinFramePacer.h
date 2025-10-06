//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Core\Rtt_Build.h"

#include <chrono>

namespace Rtt
{

class WinFramePacer
{
	Rtt_CLASS_NO_COPIES(WinFramePacer)

	public:
		WinFramePacer();

		void ConfigureInterval(std::chrono::nanoseconds interval);
		void Reset();
		void Stop();

		std::chrono::nanoseconds WaitForNextFrame();

		std::chrono::nanoseconds GetLastFrameCost() const { return fLastFrameCost; }
		std::chrono::nanoseconds GetLastOvershoot() const { return fLastOvershoot; }

	private:
		using Clock = std::chrono::steady_clock;
		using TimePoint = Clock::time_point;

	private:
		std::chrono::nanoseconds fInterval;
		TimePoint fNextWakeAt;
		TimePoint fLastWakeAt;
		bool fIsActive;
		std::chrono::nanoseconds fLastFrameCost;
		std::chrono::nanoseconds fLastOvershoot;
};

} // namespace Rtt

//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Rtt_WinFramePacer.h"


namespace Rtt
{
	namespace
	{
		static const auto kMaxCatchUp = 4;
		static const auto kDefaultInterval = std::chrono::milliseconds(16);
	}

	WinFramePacer::WinFramePacer()
	: fInterval(kDefaultInterval)
	, fNextWakeAt(Clock::now())
	, fLastWakeAt(Clock::now())
	, fIsActive(false)
	, fLastFrameCost(std::chrono::nanoseconds::zero())
	, fLastOvershoot(std::chrono::nanoseconds::zero())
	{
	}

	void WinFramePacer::ConfigureInterval(std::chrono::nanoseconds interval)
	{
		if (interval.count() <= 0)
		{
			interval = std::chrono::milliseconds(1);
		}

		fInterval = interval;

		if (fIsActive)
		{
			fNextWakeAt = Clock::now() + fInterval;
		}
	}

	void WinFramePacer::Reset()
	{
		fIsActive = true;
		auto now = Clock::now();
		fNextWakeAt = now;
		fLastWakeAt = now;
		fLastFrameCost = std::chrono::nanoseconds::zero();
		fLastOvershoot = std::chrono::nanoseconds::zero();
	}

	void WinFramePacer::Stop()
	{
		fIsActive = false;
	}

	std::chrono::nanoseconds WinFramePacer::WaitForNextFrame()
	{
		if (!fIsActive)
		{
			Reset();
		}

		auto now = Clock::now();
		auto target = fNextWakeAt;

		if (now < target)
		{
			fLastOvershoot = std::chrono::nanoseconds::zero();
			fLastFrameCost = std::chrono::nanoseconds::zero();
			return target - now;
		}

		fLastFrameCost = now - fLastWakeAt;
		fLastOvershoot = now - target;

		if (fLastOvershoot.count() < 0)
		{
			fLastOvershoot = std::chrono::nanoseconds::zero();
		}

		auto next = target + fInterval;
		int catchUp = 0;
		while (next <= now && catchUp < kMaxCatchUp)
		{
			next += fInterval;
			catchUp++;
		}

		if (next <= now)
		{
			next = now + fInterval;
		}

		fLastWakeAt = now;
		fNextWakeAt = next;
		return std::chrono::nanoseconds::zero();
}

} // namespace Rtt

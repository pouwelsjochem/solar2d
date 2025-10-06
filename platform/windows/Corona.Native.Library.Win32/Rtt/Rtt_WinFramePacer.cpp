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

#include <thread>
#include <Windows.h>


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

	std::chrono::nanoseconds WinFramePacer::WaitForNextFrame(bool blockIfEarly)
	{
		if (!fIsActive)
		{
			Reset();
		}

		auto target = fNextWakeAt;
		auto now = Clock::now();

		if (now < target)
		{
			if (!blockIfEarly)
			{
				fLastOvershoot = std::chrono::nanoseconds::zero();
				fLastFrameCost = std::chrono::nanoseconds::zero();
				return target - now;
			}

			SleepUntil(target);
			now = Clock::now();
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

	void WinFramePacer::SleepUntil(TimePoint target)
	{
		auto now = Clock::now();

		while (now < target)
		{
			auto remaining = target - now;

			if (remaining > std::chrono::milliseconds(2))
			{
				auto sleepFor = remaining - std::chrono::milliseconds(1);
				if (sleepFor.count() > 0)
				{
					std::this_thread::sleep_for(sleepFor);
				}
			}
			else if (remaining > std::chrono::microseconds(200))
			{
				auto sleepFor = remaining - std::chrono::microseconds(100);
				if (sleepFor.count() > 0)
				{
					std::this_thread::sleep_for(sleepFor);
				}
			}
			else
			{
				while ((now = Clock::now()) < target)
				{
					::Sleep(0);
				}
				return;
			}

			now = Clock::now();
		}
	}

	std::chrono::nanoseconds WinFramePacer::TimeUntilNextFrame() const
	{
		if (!fIsActive)
		{
			return std::chrono::nanoseconds::zero();
		}

		auto now = Clock::now();
		auto remaining = fNextWakeAt - now;
		if (remaining.count() < 0)
		{
			return std::chrono::nanoseconds::zero();
		}

		return remaining;
	}

} // namespace Rtt

//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Rtt_WinTimer.h"

#include "Rtt_Runtime.h"

#include <chrono>
#include <cstdlib>
#include <Windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")


namespace Rtt
{

std::unordered_map<UINT_PTR, Rtt::WinTimer *> WinTimer::sTimerMap;
UINT_PTR WinTimer::sMostRecentTimerID;

#pragma region Constructors/Destructors
WinTimer::WinTimer(MCallback& callback, HWND windowHandle)
:	PlatformTimer(callback)
,	fWindowHandle(windowHandle)
,	fTimerPointer(0)
, fTimerID(0)
, fIntervalInMilliseconds(10)
, fNextIntervalTimeInTicks(0)
, fFramePacer()
, fCallbackRef(&callback)
, fTimerResolutionActive(false)
, fUseFramePacer(true)
{
	const char* disableFramePacer = std::getenv("SOLAR2D_DISABLE_FRAMEPACER");
	if (disableFramePacer && disableFramePacer[0] != '\0')
	{
		fUseFramePacer = false;
	}

	fFramePacer.ConfigureInterval(std::chrono::milliseconds(fIntervalInMilliseconds));
}

WinTimer::~WinTimer()
{
	Stop();
}

#pragma endregion


#pragma region Public Methods
void WinTimer::Start()
{
	// Do not continue if the timer is already running.
	if (IsRunning())
	{
		return;
	}

	if (fUseFramePacer && !fTimerResolutionActive)
	{
		if (timeBeginPeriod(1) == TIMERR_NOERROR)
		{
			fTimerResolutionActive = true;
		}
	}

	UpdateFramePacerInterval();
	if (fUseFramePacer)
	{
		fFramePacer.Reset();
	}

	fNextIntervalTimeInTicks = (S32)::GetTickCount() + (S32)fIntervalInMilliseconds;
	fTimerID = ++sMostRecentTimerID; // ID should be non-0, so pre-increment for first time

	if (fUseFramePacer)
	{
		fTimerPointer = ::SetTimer(fWindowHandle, fTimerID, 1, WinTimer::OnTimerElapsed);
	}
	else
	{
		UINT interval = fIntervalInMilliseconds;
		if (interval < 1)
		{
			interval = 1;
		}
		fTimerPointer = ::SetTimer(fWindowHandle, fTimerID, interval, WinTimer::OnTimerElapsed);
	}

	if (IsRunning())
	{
		sTimerMap[fTimerID] = this;
	}
}

void WinTimer::Stop()
{
	// Do not continue if the timer has already been stopped.
	if (IsRunning() == false)
	{
		return;
	}

	// Stop the timer.
	::KillTimer(fWindowHandle, fTimerID);

	sTimerMap.erase(fTimerID);

	fTimerPointer = 0;
	fTimerID = 0;
	fFramePacer.Stop();

	if (fTimerResolutionActive)
	{
		timeEndPeriod(1);
		fTimerResolutionActive = false;
	}
}

void WinTimer::SetInterval(U32 milliseconds)
{
	fIntervalInMilliseconds = milliseconds;

	if (fUseFramePacer)
	{
		fFramePacer.ConfigureInterval(std::chrono::milliseconds(fIntervalInMilliseconds));
	}
}

bool WinTimer::IsRunning() const
{
	return (fTimerPointer != 0);
}

void WinTimer::Evaluate()
{
	// Do not continue if the timer is not running.
	if (IsRunning() == false)
	{
		return;
	}

	if (fUseFramePacer)
	{
		auto wait = fFramePacer.WaitForNextFrame();
		if (wait.count() > 0)
		{
			return;
		}
		this->operator()();
		return;
	}

	// Do not continue if we haven't reached the scheduled time yet.
	if (CompareTicks((S32)::GetTickCount(), fNextIntervalTimeInTicks) < 0)
	{
		return;
	}

	// Schedule the next interval time.
	for (; CompareTicks((S32)::GetTickCount(), fNextIntervalTimeInTicks) > 0; fNextIntervalTimeInTicks += fIntervalInMilliseconds);

	// Invoke this timer's callback.
	this->operator()();
}

#pragma endregion


#pragma region Private Methods/Functions
VOID CALLBACK WinTimer::OnTimerElapsed(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	auto timer = sTimerMap.find(idEvent);
	if (sTimerMap.end() != timer)
	{
		timer->second->Evaluate();
	}
}

S32 WinTimer::CompareTicks(S32 x, S32 y)
{
	// Overflow will occur when flipping sign bit of largest negative number.
	// Give it a one millisecond boost before flipping the sign.
	if (0x80000000 == y)
	{
		y++;
	}

	// Compare the given tick values via subtraction. Overlow for this subtraction operation is okay.
	S32 deltaTime = x - y;
	if (deltaTime < 0)
	{
		return -1;
	}
	else if (0 == deltaTime)
	{
		return 0;
	}
	return 1;
}

#pragma endregion

void WinTimer::UpdateFramePacerInterval()
{
	if (!fUseFramePacer)
	{
		return;
	}

	std::chrono::nanoseconds interval = std::chrono::milliseconds(fIntervalInMilliseconds);

	if (fCallbackRef)
	{
		if (Runtime* runtime = dynamic_cast<Runtime*>(fCallbackRef))
		{
			double frameIntervalSeconds = runtime->GetFrameInterval();
			if (frameIntervalSeconds > 0.0)
			{
				interval = std::chrono::duration_cast<std::chrono::nanoseconds>(
					std::chrono::duration<double>(frameIntervalSeconds));
			}
		}
	}

	fFramePacer.ConfigureInterval(interval);
}

}	// namespace Rtt

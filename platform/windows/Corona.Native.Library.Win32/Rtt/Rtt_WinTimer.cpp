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
#include <windows.h>
#include <dwmapi.h>
#include <timeapi.h>
#include <algorithm>
#include <cmath>

// Required for DwmIsCompositionEnabled() used to detect whether the display-sync
// thread path is available on this system.
#pragma comment(lib, "dwmapi.lib")

// Required for timeBeginPeriod()/timeEndPeriod() which set the system timer
// resolution to 1ms, enabling accurate Sleep() granularity in the frame loop.
#pragma comment(lib, "winmm.lib")

namespace Rtt
{
	namespace
	{
		static const double kDefaultRefreshRateInHz = 60.0;
		static const double kMaxRefreshRateInHz = 240.0;

		double NormalizeRefreshRate(double refreshRate)
		{
			if (!(refreshRate > 1.0))
			{
				return kDefaultRefreshRateInHz;
			}

			refreshRate = (std::min)(refreshRate, kMaxRefreshRateInHz);

			// Common monitor refresh rates are typically reported as values such as
			// 59.94, 119.88, 143.98, etc. Snap near-integer results to the integer
			// to avoid tiny measurement variance from wobbling the frame cadence.
			double roundedRefreshRate = std::round(refreshRate);
			if (std::fabs(refreshRate - roundedRefreshRate) <= 0.25)
			{
				refreshRate = roundedRefreshRate;
			}

			return refreshRate;
		}
	}

	std::unordered_map<UINT_PTR, Rtt::WinTimer*> WinTimer::sTimerMap;
	UINT_PTR WinTimer::sMostRecentTimerID;

#pragma region Constructors/Destructors

	WinTimer::WinTimer(MCallback& callback, HWND windowHandle)
		: PlatformTimer(callback),
		fWindowHandle(windowHandle),
		fThreadHandle(nullptr),
		fStopEvent(nullptr),
		fHasRaisedTimerResolution(false),
		fRunning(false),
		fUseDwmThread(false),
		fTimerPointer(NULL),
		fTimerID(0),
		fIntervalInMilliseconds(10),
		fNextIntervalTimeInTicks(0),
		fRefreshRateUpdateRequested(true),
		fTickPending(false)
	{
		// Determine if DWM composition is available and enabled on this system.
		// If so, we use a display-sync background thread (fUseDwmThread = true)
		// which ties frame delivery to the monitor refresh cycle for smooth animation.
		// If not (e.g. remote desktop, older Windows), we fall back to the legacy
		// WM_TIMER approach which was the original behavior.
		BOOL dwmEnabled = FALSE;
		fUseDwmThread = SUCCEEDED(::DwmIsCompositionEnabled(&dwmEnabled)) && dwmEnabled;
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

		fRunning.store(true);
		fTickPending.store(false);
		fRefreshRateUpdateRequested.store(true);

		// Assign a unique timer ID and register this instance in the map regardless
		// of which timing path is used. The ID is posted as wParam in WM_CORONA_TIMER
		// (display-sync path) and passed as idEvent in WM_TIMER (legacy path), allowing
		// the respective message handlers to look up the correct WinTimer instance.
		// ID should be non-0, so pre-increment for first time.
		fTimerID = ++sMostRecentTimerID;
		sTimerMap[fTimerID] = this;

		if (fUseDwmThread)
		{
			// Display-sync thread path.
			// Force 1ms system timer resolution so Sleep() in the frame loop
			// has sufficient granularity to maintain consistent high-refresh timing.
			// Without this, Windows defaults to ~15.6ms resolution which makes
			// accurate frame pacing impossible.
			//
			// Note: fTickPending is not reset here. It is reset in
			// RuntimeEnvironment::RuntimeDelegate::DidResume() which fires
			// after every runtime resume regardless of which code path triggered it.
			// This is the only reliable reset point because the Simulator calls
			// Rtt::Runtime::Resume() directly, bypassing RuntimeEnvironment::Resume().
			::timeBeginPeriod(1);
			fHasRaisedTimerResolution.store(true);

			fStopEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
			if (fStopEvent)
			{
				fThreadHandle = ::CreateThread(
					nullptr, 0, WinTimer::TimerThreadProc, this, 0, nullptr);
			}

			if (fThreadHandle)
			{
				// Run at normal priority – the thread spends most of its time
				// sleeping and only needs brief CPU access for the spin phase.
				::SetThreadPriority(fThreadHandle, THREAD_PRIORITY_NORMAL);
				return;
			}

			// If the display-sync thread could not be created, fall back to the
			// legacy WM_TIMER path rather than leaving the runtime without ticks.
			if (fStopEvent)
			{
				::CloseHandle(fStopEvent);
				fStopEvent = nullptr;
			}
			if (fHasRaisedTimerResolution.exchange(false))
			{
				::timeEndPeriod(1);
			}
			fUseDwmThread = false;
		}

		if (false == fUseDwmThread)
		{
			// Legacy WM_TIMER path.
			// Start the timer, but with an interval faster than the configured interval.
			// We do this because Windows timers can invoke later than expected.
			// To compensate, we'll schedule when to invoke the timer's callback using "fIntervalEndTimeInTicks".
			fNextIntervalTimeInTicks = (S32)::GetTickCount() + (S32)fIntervalInMilliseconds;
			fTimerPointer = ::SetTimer(fWindowHandle, fTimerID, 10, WinTimer::OnTimerElapsed);
			if (!fTimerPointer)
			{
				// SetTimer failed – remove from map so we don't hold a dangling entry.
				sTimerMap.erase(fTimerID);
				fTimerID = 0;
				fRunning.store(false);
			}
		}
	}

	void WinTimer::Stop()
	{
		// Do not continue if the timer has already been stopped.
		if (!IsRunning())
		{
			return;
		}

		auto timerId = fTimerID;
		fRunning.store(false);
		fTickPending.store(false);

		// Always remove from the timer map regardless of which path is active.
		// This guards against message callbacks firing after Stop() has been called.
		sTimerMap.erase(timerId);
		fTimerID = 0;

		if (fStopEvent)
		{
			::SetEvent(fStopEvent);
		}
		if (fThreadHandle)
		{
			// Signal the background thread to exit its loop and wait for it to finish
			// before releasing resources. ThreadLoop() waits on fStopEvent during
			// both its sleep and spin phases, so shutdown should be prompt and
			// deterministic instead of racing object destruction.
			::WaitForSingleObject(fThreadHandle, INFINITE);
			::CloseHandle(fThreadHandle);
			fThreadHandle = nullptr;
		}
		if (fStopEvent)
		{
			::CloseHandle(fStopEvent);
			fStopEvent = nullptr;
		}
		if (fHasRaisedTimerResolution.exchange(false))
		{
			::timeEndPeriod(1);
		}

		if (fUseDwmThread)
		{
			fUseDwmThread = false;
		}
		else
		{
			// Stop the legacy Windows timer.
			if (timerId)
			{
				::KillTimer(fWindowHandle, timerId);
			}
			fTimerPointer = NULL;
		}
	}

	void WinTimer::SetInterval(U32 milliseconds)
	{
		fIntervalInMilliseconds.store(milliseconds);
	}

	bool WinTimer::IsRunning() const
	{
		if (fUseDwmThread)
		{
			// Display-sync path does not use fTimerPointer – use fRunning instead.
			return fRunning.load();
		}
		// Legacy path — timer is running if SetTimer() returned a valid handle.
		return (fTimerPointer != NULL);
	}

	void WinTimer::Evaluate()
	{
		// Do not continue if the timer is not running.
		if (!IsRunning())
		{
			return;
		}

		if (fUseDwmThread)
		{
			// Display-sync path: the frame interval was already enforced in ThreadLoop()
			// before WM_CORONA_TIMER was posted. Invoke the callback directly.
			this->operator()();
		}
		else
		{
			// Legacy path: WM_TIMER can fire late, so check the tick count manually
			// to determine if we've actually reached the scheduled interval time.

			// Do not continue if we haven't reached the scheduled time yet.
			if (CompareTicks((S32)::GetTickCount(), fNextIntervalTimeInTicks) < 0)
			{
				return;
			}

			// Schedule the next interval time.
			for (; CompareTicks((S32)::GetTickCount(), fNextIntervalTimeInTicks) > 0;
				fNextIntervalTimeInTicks += fIntervalInMilliseconds);

			// Invoke this timer's callback.
			this->operator()();
		}
	}

	void WinTimer::RequestRefreshRateUpdate()
	{
		fRefreshRateUpdateRequested.store(true);
	}

#pragma endregion


#pragma region Private Methods/Functions

	DWORD WINAPI WinTimer::TimerThreadProc(LPVOID lpParam)
	{
		static_cast<WinTimer*>(lpParam)->ThreadLoop();
		return 0;
	}

	void WinTimer::ThreadLoop()
	{
		static const double kRefreshRateProbeIntervalInSeconds = 1.0;
		static const double kRefreshRateChangeThresholdInHz = 0.5;

		LARGE_INTEGER freq, now;
		::QueryPerformanceFrequency(&freq);

		// Use the actual monitor refresh rate as the Windows runtime target frame
		// rate, capped at 240Hz. This avoids trying to approximate a lower fixed
		// fps such as 60 against arbitrary monitor refresh rates such as 165Hz.
		double refreshRate = NormalizeRefreshRate(GetRefreshRate());
		double targetFrameTime = 1.0 / refreshRate;

		LARGE_INTEGER start;
		::QueryPerformanceCounter(&start);

		double nextTick = 0.0;
		double nextRefreshRateProbeTime = 0.0;

		while (fRunning.load())
		{
			if (fStopEvent && (WAIT_OBJECT_0 == ::WaitForSingleObject(fStopEvent, 0)))
			{
				break;
			}

			::QueryPerformanceCounter(&now);
			double currentTime = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;
			double delta = currentTime - nextTick;

			if (fRefreshRateUpdateRequested.exchange(false) || (currentTime >= nextRefreshRateProbeTime))
			{
				double updatedRefreshRate = NormalizeRefreshRate(GetRefreshRate());
				nextRefreshRateProbeTime = currentTime + kRefreshRateProbeIntervalInSeconds;
				if (std::fabs(updatedRefreshRate - refreshRate) >= kRefreshRateChangeThresholdInHz)
				{
					refreshRate = updatedRefreshRate;
					targetFrameTime = 1.0 / refreshRate;
					nextTick = currentTime + targetFrameTime;
				}
			}

			// ---- SLEEP PHASE ----
			// If we are more than 1ms away from the next tick deadline, sleep for
			// most of the remaining time. We leave 1ms unslept as a buffer to
			// account for Sleep() waking up slightly late on a loaded system.
			// This keeps CPU usage low for the majority of each frame interval.
			if (delta < -0.001)
			{
				DWORD sleepMs = (DWORD)((-delta - 0.001) * 1000.0);
				if (sleepMs > 0)
				{
					if (fStopEvent && (WAIT_OBJECT_0 == ::WaitForSingleObject(fStopEvent, sleepMs)))
					{
						break;
					}
				}
				continue;
			}

			// ---- SPIN PHASE (last ~1ms before deadline) ----
			// Busy-wait with YieldProcessor() for sub-millisecond precision.
			// YieldProcessor emits a CPU pause instruction (PAUSE on x86) which
			// hints to the CPU that we are in a spin-wait loop, reducing power
			// consumption and improving performance of the surrounding pipeline
			// compared to a plain empty loop.
			while (true)
			{
				if (!fRunning.load() || (fStopEvent && (WAIT_OBJECT_0 == ::WaitForSingleObject(fStopEvent, 0))))
				{
					return;
				}

				::QueryPerformanceCounter(&now);
				currentTime = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;
				if (currentTime >= nextTick)
					break;
				::YieldProcessor();
			}

			// ---- FIRE ----
			// Fire once per actual monitor refresh tick, capped at 240Hz.
			// If the window moves to another monitor, the targetFrameTime above is
			// recalculated and subsequent ticks use that monitor's refresh cadence.
			{
				// If we are late by more than one frame, rebase the schedule to the
				// current moment instead of trying to "catch up" with a burst of frames.
				if (delta > targetFrameTime)
				{
					nextTick = currentTime;
				}

				// Only post if the previous frame has fully completed on the main
				// thread. In the display-sync path this gate is released after the
				// update/render/present sequence has finished, preventing the timer
				// thread from queueing another frame while the previous SwapBuffers()
				// call is still in flight.
				bool expected = false;
				if (fTickPending.compare_exchange_strong(expected, true))
				{
					if (!::PostMessage(fWindowHandle, WM_CORONA_TIMER, (WPARAM)fTimerID, 0))
					{
						// If the message could not be queued, release the gate so a future
						// tick can retry instead of deadlocking the frame pump permanently.
						fTickPending.store(false);
					}
				}
			}

			// Advance the deadline by exactly one display refresh interval.
			// Using addition rather than re-querying the clock prevents timing
			// drift from accumulating over many frames.
			nextTick += targetFrameTime;
		}
	}

	double WinTimer::GetRefreshRate() const
	{
		// Query the current display settings for the monitor that actually owns this
		// window. This avoids pacing against the wrong refresh rate on multi-monitor
		// systems where the primary display differs from the one hosting the game.
		DEVMODE dm = {};
		dm.dmSize = sizeof(dm);
		if (fWindowHandle)
		{
			HMONITOR monitorHandle = ::MonitorFromWindow(fWindowHandle, MONITOR_DEFAULTTONEAREST);
			if (monitorHandle)
			{
				MONITORINFOEX monitorInfo = {};
				monitorInfo.cbSize = sizeof(monitorInfo);
				if (::GetMonitorInfo(monitorHandle, &monitorInfo))
				{
					if (::EnumDisplaySettings(monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &dm))
					{
						if (dm.dmDisplayFrequency > 1)
						{
							return static_cast<double>(dm.dmDisplayFrequency);
						}
					}
				}
			}
		}

		if (::EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm))
		{
			if (dm.dmDisplayFrequency > 1)
			{
				return static_cast<double>(dm.dmDisplayFrequency);
			}
		}

		// Safe fallback — assumes 60Hz if the display settings cannot be queried.
		return 60.0;
	}

	VOID CALLBACK WinTimer::OnTimerElapsed(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
	{
		// Legacy WM_TIMER callback — only active when fUseDwmThread is false.
		// Look up the WinTimer instance by ID and ask it to evaluate whether
		// the configured interval has elapsed. The map guard prevents crashes
		// if this callback fires after Stop() has already removed the entry.
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

		// Compare the given tick values via subtraction. Overflow for this subtraction operation is okay.
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

}	// namespace Rtt

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

// Required for DwmIsCompositionEnabled() used to detect whether the display-sync
// thread path is available on this system.
#pragma comment(lib, "dwmapi.lib")

// Required for timeBeginPeriod()/timeEndPeriod() which set the system timer
// resolution to 1ms, enabling accurate Sleep() granularity in the frame loop.
#pragma comment(lib, "winmm.lib")

namespace Rtt
{

	std::unordered_map<UINT_PTR, Rtt::WinTimer*> WinTimer::sTimerMap;
	UINT_PTR WinTimer::sMostRecentTimerID;

#pragma region Constructors/Destructors

	WinTimer::WinTimer(MCallback& callback, HWND windowHandle)
		: PlatformTimer(callback),
		fWindowHandle(windowHandle),
		fThreadHandle(nullptr),
		fStopEvent(nullptr),
		fRunning(false),
		fUseDwmThread(false),
		fTimerPointer(NULL),
		fTimerID(0),
		fIntervalInMilliseconds(10),
		fNextIntervalTimeInTicks(0),
		fTickPending(false),
		fLastMessage(0),
		fFrameSync(false)
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

		fRunning = true;

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
			// has sufficient granularity to maintain consistent 60/120fps timing.
			// Without this, Windows defaults to ~15.6ms resolution which makes
			// accurate frame pacing impossible.
			//
			// Note: fTickPending is not reset here. It is reset in
			// RuntimeEnvironment::RuntimeDelegate::DidResume() which fires
			// after every runtime resume regardless of which code path triggered it.
			// This is the only reliable reset point because the Simulator calls
			// Rtt::Runtime::Resume() directly, bypassing RuntimeEnvironment::Resume().
			::timeBeginPeriod(1);

			fStopEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
			fThreadHandle = ::CreateThread(
				nullptr, 0, WinTimer::TimerThreadProc, this, 0, nullptr);

			if (fThreadHandle)
			{
				// Run at normal priority — the thread spends most of its time
				// sleeping and only needs brief CPU access for the spin phase.
				::SetThreadPriority(fThreadHandle, THREAD_PRIORITY_NORMAL);
			}
		}
		else
		{
			// Legacy WM_TIMER path.
			// Start the timer, but with an interval faster than the configured interval.
			// We do this because Windows timers can invoke later than expected.
			// To compensate, we'll schedule when to invoke the timer's callback using "fIntervalEndTimeInTicks".
			fNextIntervalTimeInTicks = (S32)::GetTickCount() + (S32)fIntervalInMilliseconds;
			fTimerPointer = ::SetTimer(fWindowHandle, fTimerID, 10, WinTimer::OnTimerElapsed);
			if (!fTimerPointer)
			{
				// SetTimer failed — remove from map so we don't hold a dangling entry.
				sTimerMap.erase(fTimerID);
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

		fRunning = false;

		// Always remove from the timer map regardless of which path is active.
		// This guards against message callbacks firing after Stop() has been called.
		sTimerMap.erase(fTimerID);
		fTimerID = 0;

		if (fUseDwmThread)
		{
			// Signal the background thread to exit its loop and wait for it to finish
			// before releasing resources. 2000ms timeout prevents an indefinite hang
			// if the thread is unresponsive.
			if (fStopEvent)
			{
				::SetEvent(fStopEvent);
			}
			if (fThreadHandle)
			{
				::WaitForSingleObject(fThreadHandle, 2000);
				::CloseHandle(fThreadHandle);
				fThreadHandle = nullptr;
			}
			if (fStopEvent)
			{
				::CloseHandle(fStopEvent);
				fStopEvent = nullptr;
			}

			// Restore the system timer resolution we raised in Start().
			::timeEndPeriod(1);
		}
		else
		{
			// Stop the legacy Windows timer.
			::KillTimer(fWindowHandle, fTimerID);
			fTimerPointer = NULL;
		}
	}

	void WinTimer::SetInterval(U32 milliseconds)
	{
		fIntervalInMilliseconds = milliseconds;
	}

	bool WinTimer::IsRunning() const
	{
		if (fUseDwmThread)
		{
			// Display-sync path does not use fTimerPointer — use fRunning instead.
			return fRunning;
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
			// Display-sync path: fLastMessage tells us what ThreadLoop posted.
			//
			// WM_CORONA_TIMER — logic tick is due. Run the full Step() + Render()
			//   via operator()() which acts as a shim for both.
			//
			// WM_CORONA_RENDER — VSYNC fired but no logic tick is due. Queue a
			//   WM_PAINT so OnPaint() redraws the last frame with the correct GL
			//   context. This keeps the display refreshing at monitor rate even
			//   when logic runs at a lower rate (e.g. 60fps logic on 120Hz display).
			//
			// fTickPending is cleared AFTER dispatch so the background thread can
			// post the next message as soon as the main thread is free. Clearing
			// before dispatch would allow a new message to be posted while the
			// callback is still executing, potentially re-introducing queue pressure
			// under heavy load.
			if (fLastMessage == WM_CORONA_TIMER)
			{
				// Full tick — run logic step then render.
				this->operator()();
			}
			else
			{
				// Queue a WM_PAINT — OnPaint() handles render with proper GL context.
				::InvalidateRect(fWindowHandle, nullptr, FALSE);
			}
			fTickPending.store(false);
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

	void WinTimer::SetFrameSync(bool enabled)
	{
		fFrameSync = enabled;
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
		LARGE_INTEGER freq, now;
		::QueryPerformanceFrequency(&freq);

		// Query the monitor refresh rate to use as the base tick interval.
		// On a 120Hz monitor this gives 8.33ms per tick. On 60Hz, 16.67ms.
		// The game's configured FPS (e.g. 60fps on a 120Hz monitor) is enforced
		// separately via the accumulator below — frames fire every Nth display tick.
		double refreshRate = GetRefreshRate();
		double targetFrameTime = 1.0 / refreshRate;

		// The Corona runtime's configured frame interval in seconds (e.g. 1/60 = 0.01667s).
		// Set externally via SetInterval() based on the fps value in config.lua.
		double intervalSeconds = static_cast<double>(fIntervalInMilliseconds) / 1000.0;

		LARGE_INTEGER start;
		::QueryPerformanceCounter(&start);

		double nextTick = 0.0;
		double accumulator = 0.0;

		while (fRunning)
		{
			::QueryPerformanceCounter(&now);
			double currentTime = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;
			double delta = currentTime - nextTick;

			// ---- SLEEP PHASE ----
			// If we are more than 1ms away from the next tick deadline, sleep for
			// most of the remaining time. We leave 1ms unslept as a buffer to
			// account for Sleep() waking up slightly late on a loaded system.
			// This keeps CPU usage low for the majority of each frame interval.
			if (delta < -0.001)
			{
				DWORD sleepMs = (DWORD)((-delta - 0.001) * 1000.0);
				if (sleepMs > 0)
					::Sleep(sleepMs);
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
				::QueryPerformanceCounter(&now);
				currentTime = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;
				if (currentTime >= nextTick)
					break;
				::YieldProcessor();
			}

			// ---- FIRE ----
			// Accumulate elapsed display ticks. Each VSYNC tick posts either
			// WM_CORONA_TIMER (logic + render) or WM_CORONA_RENDER (render only)
			// depending on whether the logic accumulator has reached the configured
			// frame interval. This decouples logic rate (config.lua fps) from
			// render rate (monitor refresh rate).
			accumulator += targetFrameTime;
			bool doStep = (accumulator >= intervalSeconds);
			if (doStep)
			{
				// Reset the accumulator to zero rather than carrying over the remainder.
				// Carrying over causes occasional early ticks — for example, on a 120Hz
				// monitor running a 60fps game, carry-over produces a frame every ~13
				// normal frames that arrives after only 8.3ms instead of 16.7ms. Although
				// framedebug does not flag these as stutters, they are displayed for only
				// one refresh cycle instead of two, creating subtle but perceptible judder
				// during smooth scrolling and camera movement.
				// Resetting to zero eliminates these early ticks entirely, producing a
				// consistent 16.67ms frame interval, ~1ms jitter, and a stable 60.0fps
				// readout. The tradeoff is a negligible long-term drift of a fraction of
				// a millisecond per session, which is completely imperceptible.
				accumulator = 0.0;
			}

			// Only post if the previous WM_CORONA_TIMER has been fully processed
			// by the main thread (i.e. Evaluate() has cleared fTickPending).
			// This one-message gate prevents timer messages from accumulating in
			// the queue under heavy load, which would starve input messages and
			// make the window unresponsive. The timing loop continues advancing
			// nextTick regardless, so no drift builds up when a tick is skipped.
			bool expected = false;
			if (fTickPending.compare_exchange_strong(expected, true))
			{
				if (doStep)
				{
					// Logic tick due — post full update + render message.
					::PostMessage(fWindowHandle, WM_CORONA_TIMER, (WPARAM)fTimerID, 0);
				}
				else if (fFrameSync)
				{
					// Render-only tick — only posted when frameSync is enabled.
					// When disabled (default), render runs at the same rate as
					// logic and no duplicate frames are produced.
					::PostMessage(fWindowHandle, WM_CORONA_RENDER, (WPARAM)fTimerID, 0);
				}
				else
				{
					// frameSync disabled — release the gate immediately so the
					// next logic tick is not blocked waiting for a render-only
					// message that was never posted.
					fTickPending.store(false);
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
		// Query the current display settings to obtain the monitor refresh rate.
		// This is used by ThreadLoop to determine the base tick interval so that
		// frame delivery is phase-locked to the display regardless of refresh rate.
		DEVMODE dm = {};
		dm.dmSize = sizeof(dm);

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
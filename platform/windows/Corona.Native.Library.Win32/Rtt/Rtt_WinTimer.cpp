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
#include <dxgi.h>
#include <timeapi.h>
#include <cmath>
#include <vector>


// Required for DwmIsCompositionEnabled() and DwmGetCompositionTimingInfo()
// used to detect DWM availability and query compositor refresh timing.
#pragma comment(lib, "dwmapi.lib")

// Required for timeBeginPeriod()/timeEndPeriod() which set the system timer
// resolution to 1ms, enabling accurate Sleep() granularity in the frame loop.
#pragma comment(lib, "winmm.lib")

// Required for IDXGIFactory/IDXGIOutput used by GetRefreshRateFromDXGI()
// to query the actual fractional monitor refresh rate directly from the
// GPU driver. More accurate than EnumDisplaySettings which truncates to
// integer Hz values.
#pragma comment(lib, "dxgi.lib")

namespace Rtt
{

	std::unordered_map<UINT_PTR, Rtt::WinTimer*> WinTimer::sTimerMap;
	UINT_PTR WinTimer::sMostRecentTimerID;

#pragma region Constructors/Destructors

	WinTimer::WinTimer(MCallback& callback, HWND windowHandle)
		: PlatformTimer(callback),
		fWindowHandle(windowHandle),
		fVsyncThreadHandle(nullptr),
		fMonitorWatchThreadHandle(nullptr),
		fStopEvent(nullptr),
		fRunning(false),
		fUseDwmThread(false),
		fTimerPointer(NULL),
		fTimerID(0),
		fIntervalInMilliseconds(10.0),
		fNextIntervalTimeInTicks(0),
		fTickPending(false),
		fFrameSync(true)
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

			fPendingRefreshRate.store(GetRefreshRate());

			fStopEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
			fVsyncThreadHandle = ::CreateThread(
				nullptr, 0, WinTimer::TimerThreadProc, this, 0, nullptr);

			if (fVsyncThreadHandle)
			{
				// Run at normal priority - the thread spends most of its time
				// sleeping and only needs brief CPU access for the spin phase.
				::SetThreadPriority(fVsyncThreadHandle, THREAD_PRIORITY_NORMAL);
			}

			// Monitor watch runs at below-normal priority - it only polls every 500ms
			// and should never compete with the vsync thread for CPU time.
			fMonitorWatchThreadHandle = ::CreateThread(
				nullptr, 0, WinTimer::MonitorWatchThreadProc, this, 0, nullptr);
			if (fMonitorWatchThreadHandle)
				::SetThreadPriority(fMonitorWatchThreadHandle, THREAD_PRIORITY_BELOW_NORMAL);
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
				// SetTimer failed - remove from map so we don't hold a dangling entry.
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

		const auto timerId = fTimerID;
		fRunning = false;

		// Always remove from the timer map regardless of which path is active.
		// This guards against message callbacks firing after Stop() has been called.
		sTimerMap.erase(timerId);

		if (fUseDwmThread)
		{
			// Signal the background thread to exit its loop and wait for it to finish
			// before releasing resources. 2000ms timeout prevents an indefinite hang
			// if the thread is unresponsive.
			if (fStopEvent)
			{
				::SetEvent(fStopEvent);
			}
			if (fVsyncThreadHandle)
			{
				::WaitForSingleObject(fVsyncThreadHandle, 2000);
				::CloseHandle(fVsyncThreadHandle);
				fVsyncThreadHandle = nullptr;
			}
			if (fMonitorWatchThreadHandle)
			{
				::WaitForSingleObject(fMonitorWatchThreadHandle, 2000);
				::CloseHandle(fMonitorWatchThreadHandle);
				fMonitorWatchThreadHandle = nullptr;
			}
			if (fStopEvent)
			{
				::CloseHandle(fStopEvent);
				fStopEvent = nullptr;
			}

			// Drain any WM_CORONA_TIMER messages the thread posted just before
			// exiting. fWindowHandle is still valid here - DestroyWindow hasn't
			// been called yet. PeekMessage filters by HWND only, not wParam.
			if (fWindowHandle)
			{
				MSG msg;
				while (::PeekMessage(&msg, fWindowHandle,
					WM_CORONA_TIMER, WM_CORONA_TIMER, PM_REMOVE))
				{
				}
			}

			fTickPending.store(false);
			fPendingRefreshRate.store(0.0);

			// Restore the system timer resolution we raised in Start().
			::timeEndPeriod(1);
		}
		else
		{
			// Stop the legacy Windows timer.
			::KillTimer(fWindowHandle, timerId);
			fTimerPointer = NULL;
		}

		fTimerID = 0;
	}

	void WinTimer::SetInterval(U32 milliseconds)
	{
		fIntervalInMilliseconds = milliseconds;
	}

	void WinTimer::SetInterval(double milliseconds)
	{
		fIntervalInMilliseconds = milliseconds;
	}

	bool WinTimer::IsRunning() const
	{
		if (fUseDwmThread)
		{
			// Display-sync path does not use fTimerPointer - use fRunning instead.
			return fRunning;
		}
		// Legacy path - timer is running if SetTimer() returned a valid handle.
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
			// Display-sync path: measure the wall-clock time spent executing the full
			// frame tick. The measurement brackets operator()() which runs Step() +
			// Render() - covering Lua logic, physics, scene traversal, command buffer
			// preparation, and GL dispatch. This gives a complete picture of CPU frame
			// work time as seen from the scheduling layer, independent of the idle time
			// the background thread spends sleeping between ticks.
			//
			// QueryPerformanceFrequency() is called each time rather than cached because
			// Evaluate() runs on the main thread and frequency queries are cheap (~1us).
			// If this becomes a concern, freq can be cached as a member alongside
			// fLastFrameWorkMs.
			//
			// fTickPending is cleared after the measurement so the result is fully
			// committed before the background thread is allowed to post the next tick.
			// Render-only ticks (vsync with no logic due) are handled separately via
			// InvalidateRect in ThreadLoop and never reach Evaluate().
			LARGE_INTEGER start, end, freq;
			::QueryPerformanceFrequency(&freq);
			::QueryPerformanceCounter(&start);

			this->operator()();

			::QueryPerformanceCounter(&end);
			fLastFrameWorkMs.store(
				(double)(end.QuadPart - start.QuadPart) / (double)freq.QuadPart * 1000.0
			);
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

	bool WinTimer::GetFrameSync() const
	{
		// Returns whether render-only vsync ticks are enabled.
		// Readable via display.getDefault("renderSync").
		return fFrameSync;
	}

	void WinTimer::SetFrameSync(bool enabled)
	{
		// Enables or disables render-only vsync ticks at runtime.
		// Settable via display.setDefault("renderSync", bool).
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

		double refreshRate = GetRefreshRate();
		double targetFrameTime = 1.0 / refreshRate;
		double intervalSeconds = fIntervalInMilliseconds.load() / 1000.0;

		LARGE_INTEGER start;
		::QueryPerformanceCounter(&start);

		double nextTick = 0.0;

		// Vsync counter drives logic ticks instead of a separate float accumulator.
		// This phase-locks logic ticks to display boundaries, eliminating the motion
		// blur caused by two independent float cadences drifting out of phase.
		// e.g. 60fps on 120Hz fires a logic tick every 2nd vsync, perfectly locked.
		int vsyncCount = 0;
		int logicInterval = (int)round(refreshRate * intervalSeconds);
		if (logicInterval < 1) logicInterval = 1;

		while (fRunning)
		{
			::QueryPerformanceCounter(&now);
			double currentTime = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;
			double delta = currentTime - nextTick;

			// ---- SLEEP PHASE ----
			if (delta < -0.001)
			{
				DWORD sleepMs = (DWORD)((-delta - 0.001) * 1000.0);
				if (sleepMs > 0)
					::Sleep(sleepMs);
				continue;
			}

			// ---- SPIN PHASE ----
			while (true)
			{
				::QueryPerformanceCounter(&now);
				currentTime = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;
				if (currentTime >= nextTick)
					break;
				::YieldProcessor();
			}

			// ---- FIRE ----
			// Check if MonitorWatchLoop has detected a monitor change since the last tick.
			// Atomically swap fPendingRefreshRate to 0.0 - a non-zero value means a new
			// rate is waiting. Applying it here keeps all timing state on this thread,
			// avoiding any need for locks around refreshRate/logicInterval/nextTick.
			double pendingRate = fPendingRefreshRate.exchange(0.0);
			if (pendingRate > 0.0 && fabs(pendingRate - refreshRate) > 1.0)
			{
				refreshRate = pendingRate;
				targetFrameTime = 1.0 / refreshRate;
				intervalSeconds = fIntervalInMilliseconds.load() / 1000.0;

				logicInterval = (int)round(refreshRate * intervalSeconds);
				if (logicInterval < 1) logicInterval = 1;

				// Reset vsync phase cleanly on monitor change.
				vsyncCount = fTickPending.load() ? (logicInterval - 1) : 0;

				// Re-anchor nextTick from now so the new cadence starts cleanly
				// rather than trying to catch up from a stale deadline.
				::QueryPerformanceCounter(&now);
				currentTime = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;
				nextTick = currentTime + targetFrameTime;
			}

			// Always re-read interval in case SetInterval() was called externally
			// (e.g. after a monitor change updates the effective fps cap).
			intervalSeconds = fIntervalInMilliseconds.load() / 1000.0;

			// Recalculate logicInterval every tick from the live interval.
			// This ensures it stays correct after SetInterval() is called by the
			// main thread in response to WM_CORONA_MONITOR_CHANGED, without needing
			// to coordinate timing between the two threads.
			int newLogicInterval = (int)round(refreshRate * intervalSeconds);
			if (newLogicInterval < 1) newLogicInterval = 1;
			if (newLogicInterval != logicInterval)
			{
				logicInterval = newLogicInterval;
				vsyncCount = 0; // reset phase cleanly on any interval change
			}

			// Logic tick - vsync-counter driven, phase-locked to display refresh.
			// Fires every Nth vsync where N = round(refreshRate * intervalSeconds).
			vsyncCount++;
			bool doStep = (vsyncCount >= logicInterval);
			if (doStep)
			{
				vsyncCount = 0;

				bool expected = false;
				if (fTickPending.compare_exchange_strong(expected, true))
				{
					// Guard against the window being destroyed between the time
					// fRunning was set to false and the thread's final iteration.
					// PostMessage on an invalid HWND is a no-op but leaves
					// fTickPending stuck at true, so reset it explicitly.
					if (::IsWindow(fWindowHandle))
						::PostMessage(fWindowHandle, WM_CORONA_TIMER, (WPARAM)fTimerID, 0);
					else
						fTickPending.store(false);
				}
			}

			// Render-only tick - call InvalidateRect directly from the thread.
			// This is thread-safe, coalescing (multiple calls before the next
			// paint are merged into one WM_PAINT), and WM_PAINT is a low-priority
			// synthesized message that Windows only delivers when the queue is
			// otherwise empty. This means render-only ticks can never starve
			// WM_TIMER, input messages, or anything else in the queue.
			// IsWindow() guards against the window being destroyed during the
			// brief window between Stop() setting fRunning=false and the thread
			// fully exiting - InvalidateRect on a destroyed HWND can corrupt
			// Win32 window state and cause the Simulator to steal foreground focus.
			if (fFrameSync && !doStep && ::IsWindow(fWindowHandle))
				::InvalidateRect(fWindowHandle, nullptr, FALSE);

			// Advance vsync deadline by exactly one display interval.
			nextTick += targetFrameTime;
		}
	}

	DWORD WINAPI WinTimer::MonitorWatchThreadProc(LPVOID lpParam)
	{
		static_cast<WinTimer*>(lpParam)->MonitorWatchLoop();
		return 0;
	}

	void WinTimer::MonitorWatchLoop()
	{
		// Snapshot the current refresh rate as our baseline for change detection.
		// GetRefreshRate() tries DXGI first, then DWM timing info, then
		// EnumDisplaySettings - so this initial read is as accurate as the
		// per-poll reads that follow.
		double lastRate = fPendingRefreshRate.load();

		// WaitForSingleObject with a 500ms timeout serves dual purpose:
		//   - Acts as the poll interval (check for monitor changes twice per second).
		//   - Exits immediately when Stop() signals fStopEvent, so shutdown
		//     never waits a full 500ms for the next poll cycle to complete.
		while (::WaitForSingleObject(fStopEvent, 500) == WAIT_TIMEOUT)
		{
			double newRate = GetRefreshRate();

			// Use a 1Hz threshold to avoid false positives from floating-point
			// variance between successive DXGI queries of the same display.
			if (fabs(newRate - lastRate) > 1.0)
			{
				lastRate = newRate;

				// Write the new rate atomically so ThreadLoop picks it up on its
				// next vsync tick without any cross-thread signaling overhead.
				fPendingRefreshRate.store(newRate);

				// Also notify the main thread so RenderSurfaceControl can update
				// its own rate cap (fPendingMonitorRefreshRate) and fire its event.
				if (::IsWindow(fWindowHandle))
				{
					::PostMessage(fWindowHandle, WM_CORONA_MONITOR_CHANGED,
						(WPARAM)(UINT)(newRate * 1000.0), (LPARAM)fTimerID);
				}
			}
		}
	}

	double WinTimer::GetRefreshRateFromDXGI() const
	{
		double result = 0.0;

		// Get the monitor the window is currently displayed on.
		// MONITOR_DEFAULTTONEAREST falls back to the nearest monitor
		// if the window is between displays or offscreen.
		HMONITOR hMonitor = ::MonitorFromWindow(fWindowHandle, MONITOR_DEFAULTTONEAREST);
		if (!hMonitor) return result;

		// Get the monitor's device name so we can match it in DXGI.
		MONITORINFOEX monitorInfo = {};
		monitorInfo.cbSize = sizeof(monitorInfo);
		if (!::GetMonitorInfo(hMonitor, &monitorInfo)) return result;

		// Walk DXGI adapters and outputs to find the one matching this monitor.
		IDXGIFactory* factory = nullptr;
		if (FAILED(::CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory)))
			return result;

		IDXGIAdapter* adapter = nullptr;
		for (UINT i = 0; factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			IDXGIOutput* output = nullptr;
			for (UINT j = 0; adapter->EnumOutputs(j, &output) != DXGI_ERROR_NOT_FOUND; ++j)
			{
				DXGI_OUTPUT_DESC desc;
				if (SUCCEEDED(output->GetDesc(&desc)))
				{
					// Match by device name to find the correct monitor.
					if (wcscmp(desc.DeviceName, monitorInfo.szDevice) == 0)
					{
						DEVMODE dm = {};
						dm.dmSize = sizeof(dm);
						if (::EnumDisplaySettings(desc.DeviceName,
							ENUM_CURRENT_SETTINGS, &dm))
						{
							UINT numModes = 0;
							DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM;
							output->GetDisplayModeList(format, 0, &numModes, nullptr);

							if (numModes > 0)
							{
								std::vector<DXGI_MODE_DESC> modes(numModes);
								output->GetDisplayModeList(format, 0,
									&numModes, modes.data());

								for (const auto& mode : modes)
								{
									if (mode.Width == dm.dmPelsWidth &&
										mode.Height == dm.dmPelsHeight &&
										mode.RefreshRate.Denominator > 0)
									{
										double hz =
											static_cast<double>(mode.RefreshRate.Numerator) /
											static_cast<double>(mode.RefreshRate.Denominator);

										if (fabs(hz - dm.dmDisplayFrequency) < 1.0)
										{
											result = hz;
											break;
										}
									}
								}
							}
						}
						output->Release();
						adapter->Release();
						factory->Release();
						return result;
					}
				}
				output->Release();
			}
			adapter->Release();
		}

		factory->Release();
		return result;
	}

	double WinTimer::GetRefreshRate() const
	{
		// Try DXGI first - most accurate, directly from GPU driver.
		// Correctly returns fractional rates like 59.94Hz that
		// EnumDisplaySettings truncates to integers.
		double hz = GetRefreshRateFromDXGI();

		if (hz > 0.0) return hz;

		// Fall back to DWM compositor timing info.
		DWM_TIMING_INFO timingInfo = {};
		timingInfo.cbSize = sizeof(timingInfo);
		if (SUCCEEDED(::DwmGetCompositionTimingInfo(nullptr, &timingInfo)))
		{
			if (timingInfo.rateRefresh.uiNumerator > 0 &&
				timingInfo.rateRefresh.uiDenominator > 0)
			{
				return static_cast<double>(timingInfo.rateRefresh.uiNumerator) /
					static_cast<double>(timingInfo.rateRefresh.uiDenominator);
			}
		}

		// Final fallback - EnumDisplaySettings integer value.
		DEVMODE dm = {};
		dm.dmSize = sizeof(dm);
		if (::EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm))
		{
			if (dm.dmDisplayFrequency > 1)
				return static_cast<double>(dm.dmDisplayFrequency);
		}

		// Safe fallback - assumes 60Hz if all queries fail.
		return 60.0;
	}

	VOID CALLBACK WinTimer::OnTimerElapsed(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
	{
		// Legacy WM_TIMER callback - only active when fUseDwmThread is false.
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

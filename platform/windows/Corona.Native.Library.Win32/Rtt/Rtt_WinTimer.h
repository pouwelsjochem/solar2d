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
#include "Rtt_PlatformTimer.h"
#include <Windows.h>
#include <dwmapi.h>
#include <unordered_map>
#include <atomic>


/// <summary>
///  Custom window message posted from the display-sync thread to the main thread.
///  WM_CORONA_TIMER triggers a full logic + render tick (Step + Render).
///  Gated by fTickPending to prevent queue flooding under load.
///  Render-only ticks (vsync with no logic due) are handled via InvalidateRect
///  called directly from the thread, which coalesces into a low-priority WM_PAINT
///  that Windows only delivers when the queue is otherwise idle.
/// </summary>
#define WM_CORONA_TIMER (WM_USER + 0x100)
#define WM_CORONA_MONITOR_CHANGED (WM_USER + 0x101)

namespace Rtt
{

	class WinTimer : public PlatformTimer
	{
		Rtt_CLASS_NO_COPIES(WinTimer)

	public:
		/// <summary>Creates a new Win32 timer.</summary>
		/// <param name="callback">The callback to be invoked every time the timer elapses.</param>
		/// <param name="windowHandle">Handle to a window or control to attach the windows timer to. Can be null.</param>
		WinTimer(MCallback& callback, HWND windowHandle);

		/// <summary>Destroys the timer and its resources. Stops the timer if currently running.</summary>
		virtual ~WinTimer();

		/// <summary>Starts the timer.</summary>
		virtual void Start() override;

		/// <summary>Stops the timer, if running.</summary>
		virtual void Stop() override;

		/// <summary>Sets the timer's interval in milliseconds. This can be applied while the timer is running.</summary>
		/// <param name="milliseconds">
		///  <para>How often the timer will "elapse" and invoke its given callback when running.</para>
		///  <para>Cannot be set less than 10 milliseconds.</para>
		/// </param>
		virtual void SetInterval(U32 milliseconds) override;
		virtual void SetInterval(double milliseconds) override;

		/// <summary>Determines if the timer is currently running.</summary>
		/// <returns>Returns true if the timer is currently running. Returns false if not started or stopped.</returns>
		virtual bool IsRunning() const override;

		/// <summary>
		///  <para>Dispatches a logic + render tick when called from the WM_CORONA_TIMER handler.</para>
		///  <para>
		///   In the display-sync path, invokes the full Step() + Render() callback via operator()(),
		///   then clears fTickPending so the background thread can post the next tick.
		///  </para>
		///  <para>
		///   In the legacy WM_TIMER path, the tick count is checked manually to
		///   determine if the configured interval has elapsed before invoking
		///   the full callback.
		///  </para>
		///  <para>This method will not do anything if the timer is not running.</para>
		/// </summary>
		void Evaluate();

		/// <summary>
		///  <para>Queries the actual display refresh rate from the DXGI graphics driver.</para>
		///  <para>
		///   Identifies the monitor the window is currently displayed on via
		///   MonitorFromWindow() and matches it against DXGI outputs by device name,
		///   ensuring the correct refresh rate is returned on multi-monitor systems.
		///  </para>
		///  <para>
		///   Returns fractional rates accurately (e.g. 59.883Hz) unlike
		///   EnumDisplaySettings which truncates to integers.
		///  </para>
		///  <para>Returns 0.0 if the query fails.</para>
		/// </summary>
		double GetRefreshRateFromDXGI() const;

		/// <summary>
		///  <para>Queries the current display refresh rate using EnumDisplaySettings.</para>
		///  <para>Used by ThreadLoop to determine the base tick interval for frame scheduling.</para>
		///  <returns>Returns the refresh rate in Hz, or 60.0 as a safe fallback.</returns>
		/// </summary>
		double GetRefreshRate() const override;

		/// <summary>
		///  <para>Returns whether render-sync mode is currently enabled.</para>
		///  <para>
		///   When true, ThreadLoop calls InvalidateRect on every vsync tick where
		///   no logic step is due, keeping the display refreshing at monitor rate.
		///   When false, the display refreshes only when a logic tick fires.
		///  </para>
		/// </summary>
		virtual bool GetFrameSync() const override;

		/// <summary>
		///  <para>Enables or disables render-sync mode at runtime.</para>
		///  <para>
		///   When enabled, ThreadLoop calls InvalidateRect on every vsync tick where
		///   no logic step is due, keeping the display refreshing at monitor rate even
		///   when logic runs at a lower rate (e.g. 60fps logic on a 120Hz display).
		///   When disabled, the display refreshes only when a logic tick fires.
		///  </para>
		///  <para>Can be called at any time, including while the timer is running.</para>
		/// </summary>
		virtual void SetFrameSync(bool enabled) override;

		// Returns true when the timer is using the DWM-synced background thread path.
		bool UsesDwmThread() const { return fUseDwmThread; }

		/// <summary>
		///  <para>Returns the wall-clock time in milliseconds spent executing the most recent full frame tick.</para>
		///  <para>See fLastFrameWorkMs for a full description of what this value covers.</para>
		///  <para>Thread-safe — uses an atomic load internally.</para>
		/// </summary>
		virtual double GetLastFrameWorkMs() const override { return fLastFrameWorkMs.load(); }

	private:
		/// <summary>
		///  <para>Static entry point for the display-sync background thread.</para>
		///  <para>Delegates immediately to ThreadLoop() on the WinTimer instance passed via lpParam.</para>
		/// </summary>
		static DWORD WINAPI TimerThreadProc(LPVOID lpParam);

		/// <summary>
		///  <para>Main loop for the display-sync background thread.</para>
		///  <para>
		///   Replaces the legacy WM_TIMER approach. Instead of relying on Windows message queue
		///   scheduling, this loop ties frame delivery directly to the monitor's refresh cycle
		///   using a high-resolution sleep/spin strategy seeded by the actual display refresh rate.
		///  </para>
		///  <para>
		///   On each vsync tick, if a logic update is due, WM_CORONA_TIMER is posted to the main
		///   thread for a full Step() + Render(). If only a visual refresh is needed,
		///   InvalidateRect is called directly — this coalesces into a single low-priority
		///   WM_PAINT that Windows delivers only when the queue is otherwise idle, ensuring
		///   render-only ticks never starve input, timers, or other real messages.
		///  </para>
		///  <para>
		///   This approach is conceptually equivalent to requestAnimationFrame in browsers —
		///   the callback fires once per display refresh cycle, phase-locked to the monitor.
		///   A 60fps project on a 120Hz or 144Hz display fires a logic tick every Nth refresh,
		///   ensuring frames always land on a display boundary rather than between refreshes.
		///  </para>
		///  <para>
		///   To preserve input responsiveness under heavy load, the loop uses fTickPending
		///   as a one-message gate. A new WM_CORONA_TIMER is only posted after the previous
		///   one has been fully processed by the main thread (i.e. after Evaluate() clears
		///   fTickPending). This prevents the queue from accumulating logic ticks when the
		///   main thread is busy, which would otherwise starve input messages.
		///  </para>
		/// </summary>
		void ThreadLoop();

		/// <summary>
		///  <para>Static entry point for the monitor watch background thread.</para>
		///  <para>Delegates immediately to MonitorWatchLoop() on the WinTimer instance passed via lpParam.</para>
		/// </summary>
		static DWORD WINAPI MonitorWatchThreadProc(LPVOID lpParam);

		/// <summary>
		///  <para>Polls the display refresh rate every 500ms to detect monitor changes.</para>
		///  <para>
		///   Runs on a dedicated background thread so the vsync loop in ThreadLoop()
		///   is never burdened with DXGI queries. When a rate change greater than 1Hz
		///   is detected, WM_CORONA_MONITOR_CHANGED is posted to the main thread.
		///  </para>
		///  <para>
		///   Uses WaitForSingleObject on fStopEvent with a 500ms timeout so it exits
		///   immediately when Stop() signals the event, with no artificial shutdown delay.
		///  </para>
		/// </summary>
		void MonitorWatchLoop();

		/// <summary>
		///  <para>Called by Windows when the legacy system timer has elapsed.</para>
		///  <para>Calls WinTimer's Evaluate() function to see if it is time to invoke its callback.</para>
		///  <para>Only used when DWM composition is not available (e.g. remote desktop, older Windows).</para>
		/// </summary>
		static VOID CALLBACK OnTimerElapsed(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

		/// <summary>
		///  <para>Compares the given tick values returned by ::GetTickCount().</para>
		///  <para>
		///   Correctly handles tick overflow where large negative numbers are considered greater than
		///   large positive numbers.
		///  </para>
		/// </summary>
		/// <param name="x">Ticks value to be compared against argument "y".</param>
		/// <param name="y">Ticks value to be compared against argument "x".</param>
		/// <returns>
		///  <para>Returns a positive value if "x" is greater than "y".</para>
		///  <para>Returns zero if "x" is equal to "y".</para>
		///  <para>Returns a negative value if "x" is less than "y".</para>
		/// </returns>
		static S32 CompareTicks(S32 x, S32 y);

		HWND fWindowHandle;

		/// <summary>
		///  Background thread handle for the display-sync (vsync) timer loop.
		///  Only valid when fUseDwmThread is true and the timer is running.
		/// </summary>
		HANDLE fVsyncThreadHandle;

		/// <summary>
		///  Background thread handle for the monitor watch loop.
		///  Polls GetRefreshRate() every 500ms and posts WM_CORONA_MONITOR_CHANGED
		///  when the window has moved to a display with a different refresh rate.
		///  Only valid when fUseDwmThread is true and the timer is running.
		/// </summary>
		HANDLE fMonitorWatchThreadHandle;

		/// <summary>
		///  Shared refresh rate written by MonitorWatchLoop when a monitor change is
		///  detected, and read by ThreadLoop on every vsync tick. Avoids any need for
		///  locking or cross-thread messages between the two background threads.
		///  Initialized to 0.0 — ThreadLoop treats any non-zero value as a pending
		///  monitor change and applies it at the top of the next fire phase.
		/// </summary>
		std::atomic<double> fPendingRefreshRate{ 0.0 };

		/// <summary>
		///  Event signaled by Stop() to request the background thread to exit cleanly.
		/// </summary>
		HANDLE fStopEvent;

		/// <summary>
		///  Indicates whether the timer is currently running.
		///  Declared volatile because it is written by the main thread (Stop())
		///  and read by the background thread (ThreadLoop()).
		/// </summary>
		volatile bool fRunning;

		/// <summary>
		///  True if the display-sync thread path is active.
		///  Set at construction time based on whether DWM composition is enabled.
		///  Falls back to false for legacy WM_TIMER behavior on older Windows or
		///  environments where DWM is unavailable (e.g. remote desktop).
		/// </summary>
		bool fUseDwmThread;

		// Legacy WM_TIMER fallback members - only used when fUseDwmThread is false.
		UINT_PTR fTimerPointer;
		UINT_PTR fTimerID;
		std::atomic<double> fIntervalInMilliseconds;
		S32 fNextIntervalTimeInTicks;

		/// <summary>
		///  When true (default), ThreadLoop calls InvalidateRect on every vsync tick
		///  where no logic step is due, keeping the display refreshing at monitor rate.
		///  When false, the display refreshes only when a logic tick fires.
		///  Can be changed at runtime via display.setDefault("renderSync", bool).
		/// </summary>
		bool fFrameSync;

		/// <summary>
		///  <para>Stores the wall-clock time in milliseconds spent executing the most recent full frame tick.</para>
		///  <para>
		///   Measured in Evaluate() from just before operator()() is called (which runs Step() + Render())
		///   to just after it returns. This captures total frame work time: Lua logic, physics, scene
		///   traversal, command buffer preparation, and GL dispatch — everything the main thread does
		///   to produce a frame.
		///  </para>
		///  <para>
		///   Unlike fPreparationTime and fRenderTimeCPU in Renderer::Statistics, which only cover
		///   portions of the render path, this value reflects the complete per-frame CPU cost
		///   as seen from the scheduling layer.
		///  </para>
		///  <para>
		///   Written atomically by the main thread in Evaluate() and read from the Lua side via
		///   GetLastFrameWorkMs(). Safe to read from any thread without locking.
		///  </para>
		///  <para>Only populated when fUseDwmThread is true (DWM display-sync path).</para>
		///  <para>Exposed to Lua via display.getStatistics() as the "frameWorkTime" field.</para>
		/// </summary>
		std::atomic<double> fLastFrameWorkMs{ 0.0 };

	public:
		/// <summary>
		///  Maps timer IDs to their WinTimer instances.
		///  Used by both the legacy WM_TIMER callback (OnTimerElapsed) and the
		///  display-sync path (WM_CORONA_TIMER handler in RenderSurfaceControl)
		///  to look up the correct WinTimer from a posted message's wParam.
		///  Declared public so RenderSurfaceControl can perform the lookup directly.
		///  Note: timer callback might be called after Stop(), so use this as a guard.
		/// </summary>
		static std::unordered_map<UINT_PTR, WinTimer*> sTimerMap;

		/// <summary>
		///  Incrementing counter used to generate unique timer IDs.
		///  Using an index rather than a pointer makes the map robust against
		///  the rare case where a new timer is allocated at the same address as a destroyed one.
		/// </summary>
		static UINT_PTR sMostRecentTimerID;

		/// <summary>
		///  Atomic flag used to gate WM_CORONA_TIMER delivery between the background
		///  thread and the main thread. Prevents the message queue from accumulating
		///  logic ticks when the main thread is busy.
		///  Declared public so RuntimeEnvironment::RuntimeDelegate::DidResume() can
		///  reset it after every runtime resume. The reset must live in DidResume()
		///  rather than RuntimeEnvironment::Resume() because the Simulator calls
		///  Rtt::Runtime::Resume() directly via GetRuntime(), bypassing
		///  RuntimeEnvironment::Resume() entirely. DidResume() is the only reliable
		///  hook point that fires after every resume regardless of call path.
		/// </summary>
		std::atomic<bool> fTickPending;
	};

}	// namespace Rtt
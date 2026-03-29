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
///  Custom window messages posted from the display-sync thread to the main thread.
///  WM_CORONA_TIMER triggers a full logic + render tick (Step + Render).
///  WM_CORONA_RENDER triggers a render-only tick, fired every VSYNC when no logic step is due.
///  Both are gated by fTickPending to prevent queue flooding under load.
/// </summary>
#define WM_CORONA_TIMER		(WM_USER + 0x100)
#define WM_CORONA_RENDER	(WM_USER + 0x101)

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

		/// <summary>Determines if the timer is currently running.</summary>
		/// <returns>Returns true if the timer is currently running. Returns false if not started or stopped.</returns>
		virtual bool IsRunning() const override;

		/// <summary>
		///  <para>Checks the last message type and dispatches accordingly.</para>
		///  <para>
		///   In the display-sync path, if the last message was WM_CORONA_TIMER,
		///   both Step() and Render() are invoked — a full logic + render tick.
		///   If the last message was WM_CORONA_RENDER, only Render() is invoked —
		///   the display refreshes without advancing game logic.
		///  </para>
		///  <para>
		///   In the legacy WM_TIMER path, the tick count is checked manually to
		///   determine if the configured interval has elapsed before invoking
		///   the full callback.
		///  </para>
		///  <para>
		///   Clears fTickPending after dispatch so the background thread can
		///   post the next message as soon as the main thread is free.
		///  </para>
		///  <para>This method will not do anything if the timer is not running.</para>
		/// </summary>
		void Evaluate();

		/// <summary>
		///  <para>Queries the current display refresh rate using EnumDisplaySettings.</para>
		///  <para>Used by ThreadLoop to determine the base tick interval for frame scheduling.</para>
		///  <returns>Returns the refresh rate in Hz, or 60.0 as a safe fallback.</returns>
		/// </summary>
		double GetRefreshRate() const override;

		/// <summary>
		///  <para>Enables or disables render-sync mode at runtime.</para>
		///  <para>
		///   When enabled, ThreadLoop posts WM_CORONA_RENDER on every VSYNC tick
		///   where no logic step is due, syncing the render rate to the monitor.
		///   When disabled, only WM_CORONA_TIMER is posted and render runs at
		///   the logic rate set by config.lua fps.
		///  </para>
		///  <para>Can be called at any time, including while the timer is running.</para>
		/// </summary>
		virtual void SetFrameSync(bool enabled) override;

	private:
		/// <summary>
		///  <para>Static entry point for the display-sync background thread.</para>
		///  <para>
		///   Delegates immediately to ThreadLoop() on the WinTimer instance passed via lpParam.
		///  </para>
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
		///   Each VSYNC tick posts either WM_CORONA_TIMER (logic + render due) or
		///   WM_CORONA_RENDER (render only) depending on whether the logic accumulator
		///   has reached the configured frame interval. This decouples the logic update
		///   rate (config.lua fps) from the render rate (monitor refresh rate).
		///  </para>
		///  <para>
		///   This approach is conceptually equivalent to requestAnimationFrame in browsers —
		///   the callback fires once per display refresh cycle, phase-locked to the monitor.
		///   A 60fps game on a 120Hz or 144Hz monitor fires every Nth refresh tick, ensuring
		///   frames always land on a display boundary rather than between refreshes.
		///  </para>
		///  <para>
		///   To preserve input responsiveness under heavy load, the loop uses fTickPending
		///   as a one-message gate. A new message is only posted after the previous one has
		///   been fully processed by the main thread (i.e. after Evaluate() clears fTickPending).
		///   When frameSync is disabled and no logic tick is due, the gate is released immediately
		///   without posting — render runs at logic rate with no duplicate frames.
		///  </para>
		/// </summary>
		void ThreadLoop();

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
		/// <param name="x">Ticks value to be compared against argument "y".</para>
		/// <param name="y">Ticks value to be compared against argument "x".</para>
		/// <returns>
		///  <para>Returns a positive value if "x" is greater than "y".</para>
		///  <para>Returns zero if "x" is equal to "y".</para>
		///  <para>Returns a negative value if "x" is less than "y".</para>
		/// </returns>
		static S32 CompareTicks(S32 x, S32 y);

		HWND fWindowHandle;

		/// <summary>
		///  Background thread handle for the display-sync timer loop.
		///  Only valid when fUseDwmThread is true and the timer is running.
		/// </summary>
		HANDLE fThreadHandle;

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

		// Legacy WM_TIMER fallback members — only used when fUseDwmThread is false.
		UINT_PTR fTimerPointer;
		UINT_PTR fTimerID;
		U32 fIntervalInMilliseconds;
		S32 fNextIntervalTimeInTicks;

		/// <summary>
		///  When true, ThreadLoop posts WM_CORONA_RENDER on every VSYNC tick
		///  regardless of whether a logic step is due, syncing the render rate
		///  to the monitor refresh rate. When false (default), only WM_CORONA_TIMER
		///  is posted — render runs at the same rate as logic.
		/// </summary>
		bool fFrameSync;

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
		///  thread and the main thread.
		///  <para>
		///   Set to true by ThreadLoop() immediately before posting either
		///   WM_CORONA_TIMER or WM_CORONA_RENDER. Cleared back to false by
		///   Evaluate() after dispatch is complete.
		///  </para>
		///  <para>
		///   This prevents the message queue from accumulating timer ticks when the
		///   main thread is busy (e.g. under heavy physics load), which would otherwise
		///   starve input messages and make the window unresponsive — a regression from
		///   the legacy WM_TIMER behavior where Windows naturally withheld timer messages
		///   until the message queue was idle.
		///  </para>
		///  <para>
		///   Declared public so RuntimeEnvironment::RuntimeDelegate::DidResume() can
		///   reset it after every runtime resume. The reset must live in DidResume()
		///   rather than RuntimeEnvironment::Resume() because the Simulator calls
		///   Rtt::Runtime::Resume() directly via GetRuntime(), bypassing
		///   RuntimeEnvironment::Resume() entirely. DidResume() is the only reliable
		///   hook point that fires after every resume regardless of call path.
		///  </para>
		/// </summary>
		std::atomic<bool> fTickPending;

		/// <summary>
		///  Stores the message ID (WM_CORONA_TIMER or WM_CORONA_RENDER) of the
		///  most recently received display-sync message. Set by RenderSurfaceControl
		///  before calling Evaluate() so the dispatch branch knows whether to run
		///  a full logic + render tick or a render-only tick.
		/// </summary>
		UINT fLastMessage;
	};

}	// namespace Rtt
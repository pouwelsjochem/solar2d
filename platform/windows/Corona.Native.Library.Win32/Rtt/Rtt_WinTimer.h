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
#include <unordered_map>


namespace Rtt
{

class WinTimer : public PlatformTimer
{
	Rtt_CLASS_NO_COPIES(WinTimer)

	public:
		/// <summary>Creates a new Win32 timer.</summary>
		/// <param name="callback">The callback to be invoked every time the timer elapses.</param>
		/// <param name="windowHandle">Handle to a window or control to attach the windows timer to. Can be null.</param>
		WinTimer(MCallback& callback, HWND windowHandle, PSRWLOCK lock);

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
		///  <para>Checks if the running timer's interval has elapsed, and if it has, invokes its callback.</para>
		///  <para>This method is provided because a Windows system timer can trigger late.</para>
		///  <para>This method will not do anything if the timer is not running.</para>
		/// </summary>
		void Evaluate();

// STEVE CHANGE
		void ThreadBody();
// /STEVE CHANGE

	private:
		/// <summary>
		///  <para>Called by Windows when the system timer has elapsed.</para>
		///  <para>Calls WinTimer's Evaluate() function to see if it is time to invoke its callback.</para>
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
		UINT_PTR fTimerPointer;
		UINT_PTR fTimerID;
		U32 fIntervalInMilliseconds;
		S32 fNextIntervalTimeInTicks;

		static std::unordered_map<UINT_PTR, WinTimer*> sTimerMap; // timer callback might be called after Stop(), so use this as a guard
		static UINT_PTR sMostRecentTimerID; // use an incrementing index as key, to be robust against the rare case that a new timer is reallocated into the same memory

// STEVE CHANGE
		void SetFPS(U32 fps);
		void ClearHandles();
		void CleanUpResources();

		enum {
			kRunning,
			kStopped,
			kLocked,
			kQuitting // timer has quit and is being destroyed (final state)
		};

		bool UsingV2API() const { return NULL != fHandles[kThread]; }

		bool Init_V2();

		void Start_V2();
		void Stop_V2(LONG stopState);
		void Evaluate_V2();

		enum {
			kThread,
			kWaitableTimer,

			kNumHandles
		};

		bool LockFromExpectedStateTo(LONG from, LONG to);
		bool TryToLockFromExpectedState(LONG from, LONG* actual = NULL);
		bool WaitWhileStopped(PSRWLOCK lock);
		void WaitForInterval(U64 beganWhen);

		INT64 fQpcPerSecond;
		double fFPSInterval;
		int fPeriodMin;

		void PreciseSleep(double seconds);

		U64 InvalidBeganTime() const { return ~0ULL; }

		PSRWLOCK fLock;
		CONDITION_VARIABLE fStartedOrStoppedCond;
		U64 fState; // n.b. mostly interpreted as a LONG
		U64 fEvaluateBeganTime;
		HANDLE fHandles[kNumHandles];
// /STEVE CHANGE
};

}	// namespace Rtt

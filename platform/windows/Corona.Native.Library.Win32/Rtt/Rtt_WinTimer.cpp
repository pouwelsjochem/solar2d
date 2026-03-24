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
// STEVE CHANGE
#include "Rtt_WinPlatform.h"
#include "Core/Rtt_Time.h"

#pragma comment(lib, "Winmm.lib")

// Precise sleep: https://github.com/blat-blatnik/Snippets/blob/main/precise_sleep.c
// see also: https://blog.bearcats.nl/perfect-sleep-function
// /STEVE CHANGE

namespace Rtt
{

std::unordered_map<UINT_PTR, Rtt::WinTimer *> WinTimer::sTimerMap;
UINT_PTR WinTimer::sMostRecentTimerID;

#pragma region Constructors/Destructors
WinTimer::WinTimer(MCallback& callback, HWND windowHandle, HWND messageOnlyWindowHandle) // <- STEVE CHANGE
:	PlatformTimer(callback),
// STEVE CHANGE
	fQpcPerSecond(0),
	fFPSInterval(0),
	fPeriodMin(0)
// /STEVE CHANGE
{
	fWindowHandle = windowHandle;
	fTimerPointer = NULL;
	fIntervalInMilliseconds = 10;
	fNextIntervalTimeInTicks = 0;
// STEVE CHANGE
	ClearHandles();

	bool useLegacyAPI = false; // TODO: version check, etc.
	if (!useLegacyAPI)
	{
		bool initedOK = Init_V2();
		if (initedOK)
		{
			fWindowHandle = messageOnlyWindowHandle;
		}
		else
		{
			CleanUpResources();
		}
	}
// /STEVE CHANGE
}

WinTimer::~WinTimer()
{
// STEVE CHANGE
	if (UsingV2API())
	{
		Stop_V2(kQuitting);
		CleanUpResources();
	}
	else
// /STEVE CHANGE
	Stop();
}

#pragma endregion

#pragma region Public Methods
void WinTimer::Start()
{
// STEVE CHANGE
	if (UsingV2API())
	{
		Start_V2();
		
		return;
	}
// /STEVE CHANGE
	// Do not continue if the timer is already running.
	if (IsRunning())
	{
		return;
	}

	// Start the timer, but with an interval faster than the configured interval.
	// We do this because Windows timers can invoke later than expected.
	// To compensate, we'll schedule when to invoke the timer's callback using "fIntervalEndTimeInTicks".
	fNextIntervalTimeInTicks = (S32)::GetTickCount() + (S32)fIntervalInMilliseconds;
	fTimerID = ++sMostRecentTimerID; // ID should be non-0, so pre-increment for first time
	fTimerPointer = ::SetTimer(fWindowHandle, fTimerID, 10, WinTimer::OnTimerElapsed);

	if (IsRunning())
	{
		sTimerMap[fTimerID] = this;
	}
}

void WinTimer::Stop()
{
// STEVE CHANGE
	if (UsingV2API())
	{
		Stop_V2(kStopped);
		
		return;
	}
// /STEVE CHANGE
	// Do not continue if the timer has already been stopped.
	if (IsRunning() == false)
	{
		return;
	}

	// Stop the timer.
	::KillTimer(fWindowHandle, fTimerID);

	sTimerMap.erase(fTimerID);

	fTimerPointer = NULL;
	fTimerID = 0;
}

void WinTimer::SetInterval(U32 milliseconds)
{
// STEVE CHANGE
	if (milliseconds & 0x8000)
	{
		Rtt_ASSERT(UsingV2API());
		Rtt_ASSERT(milliseconds > 0 && milliseconds <= 0xFFFF);

		SetFPS(milliseconds & 0x7FFF);
	}
	else
// /STEVE CHANGE
	fIntervalInMilliseconds = milliseconds;
}

bool WinTimer::IsRunning() const
{
	return (fTimerPointer != NULL);
}

void WinTimer::Evaluate()
{
// STEVE CHANGE
	if (UsingV2API())
	{
		Evaluate_V2();
		
		return;
	}
// /STEVE CHANGE
	// Do not continue if the timer is not running.
	if (IsRunning() == false)
	{
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

// STEVE CHANGE
// see links up top
void WinTimer::PreciseSleep(double seconds)
{
	LARGE_INTEGER qpc;
	::QueryPerformanceCounter(&qpc);
	INT64 targetQpc = (INT64)(qpc.QuadPart + seconds * fQpcPerSecond);

	if (fHandles[kWaitableTimer]) // Try using a high resolution timer first.
	{
		const double TOLERANCE = 0.001'02;
		INT64 maxTicks = (INT64)fPeriodMin * 9'500;
		for (;;) // Break sleep up into parts that are lower than scheduler period.
		{
			double remainingSeconds = (targetQpc - qpc.QuadPart) / (double)fQpcPerSecond;
			INT64 sleepTicks = (INT64)((remainingSeconds - TOLERANCE) * 10'000'000);
			if (sleepTicks <= 0)
				break;

			LARGE_INTEGER due;
			due.QuadPart = -(sleepTicks > maxTicks ? maxTicks : sleepTicks);
			SetWaitableTimerEx(fHandles[kWaitableTimer], &due, 0, NULL, NULL, NULL, 0);
			WaitForSingleObject(fHandles[kWaitableTimer], INFINITE);
			QueryPerformanceCounter(&qpc);
		}
	}
	else // Fallback to Sleep.
	{
		const double TOLERANCE = 0.000'02;
		double sleepMs = (seconds - TOLERANCE) * 1000 - fPeriodMin; // Sleep for 1 scheduler period less than requested.
		int sleepSlices = (int)(sleepMs / fPeriodMin);
		if (sleepSlices > 0)
			Sleep((DWORD)sleepSlices * fPeriodMin);
		QueryPerformanceCounter(&qpc);
	}

	while (qpc.QuadPart < targetQpc) // Spin for any remaining time.
	{
		YieldProcessor();
		QueryPerformanceCounter(&qpc);
	}
}

void WinTimer::SetFPS(U32 fps)
{
	fFPSInterval = 1.0 / fps;

	if (0 == fPeriodMin)
	{
		TIMECAPS caps;

		timeGetDevCaps(&caps, sizeof(TIMECAPS));

		fPeriodMin = (int)( caps.wPeriodMin );
	}

	LARGE_INTEGER qpf;

	QueryPerformanceFrequency(&qpf);

	fQpcPerSecond = qpf.QuadPart;
}

void WinTimer::ClearHandles()
{
	for (int i = 0; i < kNumHandles; i++)
	{
		fHandles[i] = NULL;
	}
}

void WinTimer::CleanUpResources()
{
	if (NULL != fHandles[kThread])
	{
		WaitForSingleObject(fHandles[kThread], INFINITE);
	}

	for (int i = 0; i < kNumHandles; i++)
	{
		if (NULL != fHandles[i])
		{
			CloseHandle(fHandles[i]);
		}
	}
	
	ClearHandles();
	
	Rtt_ASSERT(0 == fPeriodMin || kQuitting == InterlockedOr(&fState, 0));
}

static DWORD WINAPI ThreadProc(LPVOID lpParam)
{
	WinTimer* timer = (WinTimer*)lpParam;

	timer->ThreadBody();

	return 1;
}

bool WinTimer::Init_V2()
{
	fState = kStopped;
	fEvaluateBeganTime = InvalidBeganTime();

	InitializeSRWLock(&fLock);
	InitializeConditionVariable(&fStartedOrStoppedCond);

	if (false) // Windows8OrGreater()? (need to do other logic too...)
	{
		// fHandles[kWaitableTimer] = CreateWaitableTimerEx(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
		// ^^^ TODO!!! (constant not found??)
	}

	fHandles[kThread] = CreateThread(NULL, 0, ThreadProc, this, 0, NULL);

	if (NULL != fHandles[kThread])
	{
		SetThreadPriority(fHandles[kThread], THREAD_PRIORITY_HIGHEST);
	}
	else
	{
		Rtt_LogException("Failed to create WinTimer thread");

		return false;
	}

	return true;
}

void WinTimer::Start_V2()
{
	// There are two situations that might occur.
	// Usually we will have stopped some time ago, and already
	// be in the ready-and-waiting setup in WaitWhileStopped().
	// It is also possible, however, that a stop was followed
	// soon after by a start. To account for this, we lock so
	// that the race works out either way: either we enter the
	// waiting case and soon break out of it (once we wake the
	// condition), or we go right into the "running" state and
	// never wait.
	// This attempts a direct transition since no extra state
	// is contended, thus no point in an intermediate lock.)
	// This is a no-op if we're already running or quitting.
	Rtt_ASSERT(kEvaluated != InterlockedOr(&fState, 0));

	if (TransitionFromExpectedStateTo(kStopped, kRunning))
	{
		if (0 != fPeriodMin)
		{
			timeBeginPeriod(fPeriodMin);
		}

		WakeConditionVariable(&fStartedOrStoppedCond);
	}
}

void WinTimer::Stop_V2(LONG stopState)
{
	// See the comments in Start_V2(), which basically agree in purpose.
	Rtt_ASSERT(kEvaluated != InterlockedOr(&fState, 0));

	bool normalStop = LockFromExpectedStateTo(kRunning, kStopped);
	if (normalStop || (kQuitting == stopState && LockFromExpectedStateTo(kStopped, kQuitting)))
	{
		if (normalStop)
		{
			fEvaluateBeganTime = InvalidBeganTime();

			if (0 != fPeriodMin)
			{
				timeEndPeriod(fPeriodMin);
			}
		}

		UnlockTo(stopState);
		WakeConditionVariable(&fStartedOrStoppedCond);
	}
}

void WinTimer::Evaluate_V2()
{
	this->operator()();
}

// spin and check:
// * if the lock is held, pause and try again
// * if already in the "to" state, quit and return false ("didn't get locked")
// * otherwise, enter the "locked" state and return true ("got locked")
bool WinTimer::LockFromExpectedStateTo(LONG from, LONG to)
{
	Rtt_ASSERT(from != to);

	return TransitionFromExpectedStateTo(from, kLocked);
}

// as per LockFromExpectedStateTo(), but enter the "to" state rather than "locked"
bool WinTimer::TransitionFromExpectedStateTo(LONG from, LONG to)
{
	Rtt_ASSERT(from != to);

	while (true)
	{
		LONG previous = InterlockedCompareExchange(&fState, to, from);

		if (kLocked == previous)
		{
			YieldProcessor();
			
			continue;
		}

		bool didLock = (from == previous); // i.e. wasn't already "to"?
		return didLock;
	}
}

bool WinTimer::TryToLockFromExpectedState(LONG from, LONG* actual)
{
	LONG previous = InterlockedCompareExchange(&fState, kLocked, from);
	bool wantsToQuit = kQuitting == previous;

	if (NULL != actual)
	{
		*actual = previous;
	}

	return from == previous; // now locked?
}

LONG WinTimer::SleepUntilNotInState(LONG state)
{
	AcquireSRWLockExclusive(&fLock);

	while (true)
	{
		LONG actualState = InterlockedOr(&fState, 0);

		if (state != actualState)
		{
			ReleaseSRWLockExclusive(&fLock);

			return state;
		}

		SleepConditionVariableSRW(&fStartedOrStoppedCond, &fLock, INFINITE, 0);
	}
}

bool WinTimer::WaitWhileStopped()
{
	LONG actualState;

	// See the comment in Start_V2() for some related details.
	if (TryToLockFromExpectedState(kStopped, &actualState))
	{
		UnlockTo(kStopped);

		actualState = SleepUntilNotInState(kStopped); // wait for "running" or "quitting"
	}

	return kQuitting != actualState;
}

void WinTimer::WaitForInterval(U64 beganWhen)
{
	U64 now = Rtt_GetAbsoluteTime();

	Rtt_ASSERT(now >= beganWhen);

	if (fPeriodMin)
	{
		double ns = (double)(now - beganWhen);
		PreciseSleep(fFPSInterval - fmod(ns / 1'000'000, fFPSInterval));
	}
	else
	{
		DWORD leftover = fIntervalInMilliseconds - Rtt_AbsoluteToMilliseconds(now - beganWhen) % fIntervalInMilliseconds;
		Sleep(leftover);
	}
}

void WinTimer::UnlockTo(LONG state)
{
	LONG previous = InterlockedExchange(&fState, state);

	Rtt_ASSERT(kLocked == previous);
}

VOID CALLBACK WinTimer::OnTimerElapsed_V2(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	WinTimer* timer = (WinTimer*)idEvent;
	
	// Assuming we're still running, hold the lock until evaluated.
	if (timer->TryToLockFromExpectedState(kRunning))
	{
		timer->Evaluate_V2();
		timer->UnlockTo(kEvaluated);

		WakeConditionVariable(&timer->fStartedOrStoppedCond);
	}
}

void WinTimer::ThreadBody()
{
	while (true)
	{
		// A "stop" might mean a request to quit, in which case we
		// do exactly that.
		// Otherwise, it indicates something like the main window
		// being minimized. Such a situation will typically last a
		// while, and this thread would have nothing to do, so we go
		// into a wait until, say, a maximize wakes us up.
		bool stillRunning = WaitWhileStopped();
		if (!stillRunning)
		{
			return;
		}

		// We do a lock here, mindful of the (unlikely) case another
		// stop crept in, and fetch the otherwise potentially contended
		// time, indicating when the last evaluate began. This will
		// be bogus on the first frame, or if there was a stop along the
		// way. If the time is valid at this point, we do the wait.
		// It is possible a stop will occur while this is happening,
		// but this is harmless aside from the (tiny) wasted delay.
		if (TryToLockFromExpectedState(kRunning))
		{
			U64 evaluateBeganTime = fEvaluateBeganTime;

			UnlockTo(kRunning);

			if (InvalidBeganTime() != evaluateBeganTime)
			{
				WaitForInterval(evaluateBeganTime);
			}
		}

		// We do another "is running?" sanity check here, due to
		// the small but non-zero chance another stop occurred.
		if (TryToLockFromExpectedState(kRunning))
		{
			fEvaluateBeganTime = Rtt_GetAbsoluteTime();

			UnlockTo(kRunning);
			PostMessage(fWindowHandle, WM_USERMSG_KICK_FRAME, (WPARAM)this, (LPARAM)&OnTimerElapsed_V2);

			// Wait for evaluation to finish, or an intervening stop.
			SleepUntilNotInState(kRunning); // wait for "evaluated", "stopped", or "quitting"
			TransitionFromExpectedStateTo(kEvaluated, kRunning);
		}
	}
}
// /STEVE CHANGE

}	// namespace Rtt

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
WinTimer::WinTimer(MCallback& callback, HWND windowHandle, PSRWLOCK lock)
:	PlatformTimer(callback),
// STEVE CHANGE
	fQpcPerSecond(0),
	fFPSInterval(0),
	fPeriodMin(0),
	fLock(lock)
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
		if (!initedOK)
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

	TIMECAPS caps;

	timeGetDevCaps(&caps, sizeof(TIMECAPS));
	timeBeginPeriod(caps.wPeriodMin);

	fPeriodMin = (int)( caps.wPeriodMin );
	
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
		CloseHandle(fHandles[i]);
	}
	
	ClearHandles();
	
	if (0 != fPeriodMin)
	{
		timeEndPeriod(fPeriodMin);
		
		fPeriodMin = 0;
	}
}

static DWORD WINAPI ThreadProc(LPVOID lpParam)
{
	WinTimer* timer = (WinTimer*)lpParam;

	timer->ThreadBody();

	// TODO: errors...
	return 1;
}

bool WinTimer::Init_V2()
{
	fState = kStopped;
	fEvaluateBeganTime = InvalidBeganTime();

	InitializeConditionVariable(&fStartedOrStoppedCond);

	if (false) // Windows8OrGreater()? (need to do other logic too...)
	{
		// fHandles[kWaitableTimer] = CreateWaitableTimerEx(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
		// ^^^ TODO!!! (constant not found??)
	}

	fHandles[kThread] = CreateThread(NULL, 0, ThreadProc, this, 0, NULL);

	if (NULL != fHandles[kThread])
	{
		// Keep the timer responsive without starving the UI thread.
		SetThreadPriority(fHandles[kThread], THREAD_PRIORITY_ABOVE_NORMAL);
	}
	else
	{
		// TODO: error...
	
		return false;
	}

	return true;
}

void WinTimer::Start_V2()
{
	if (LockFromExpectedStateTo(kStopped, kRunning))
	{
		InterlockedExchange(&fState, kRunning);
		WakeConditionVariable(&fStartedOrStoppedCond);
	}
// TODO: resume period?
}

void WinTimer::Stop_V2(LONG stopState)
{
	bool normalStop = LockFromExpectedStateTo(kRunning, kStopped);
	if (normalStop || (kQuitting == stopState && LockFromExpectedStateTo(kStopped, kQuitting)))
	{
		if (normalStop)
		{
			fEvaluateBeganTime = InvalidBeganTime();
		}

		InterlockedExchange(&fState, stopState); // unlock or set quit state
		WakeConditionVariable(&fStartedOrStoppedCond);
	}
// TODO: restore period?
}

void WinTimer::Evaluate_V2()
{
	// Do the timer proc. Assuming Start() and Stop() only arise from
	// specific WM_* messages, and since the caller first does an "is
	// running?" check, the enclosing WM_TIMER should ensure the proc
	// completes without another Stop() along the way.
	this->operator()();
}

bool WinTimer::LockFromExpectedStateTo(LONG from, LONG to)
{
	Rtt_ASSERT(from != to);

	while (true)
	{
		LONG previous = InterlockedCompareExchange(&fState, kLocked, from);

		Rtt_ASSERT(kQuitting != previous);

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

bool WinTimer::WaitWhileStopped(PSRWLOCK lock)
{
	LONG actualState;

	if (TryToLockFromExpectedState(kStopped, &actualState))
	{
		InterlockedExchange(&fState, kStopped); // unlock and wait
		AcquireSRWLockExclusive(lock);

		do {
			SleepConditionVariableSRW(&fStartedOrStoppedCond, lock, INFINITE, 0);

			actualState = InterlockedOr(&fState, 0);
		} while (kRunning != actualState && kQuitting != actualState);

		ReleaseSRWLockExclusive(lock);
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
// TODO: instead of divide, multiply by fps interval?
		PreciseSleep(fFPSInterval - fmod(ns / 1'000'000, fFPSInterval));
	}
	else
	{
		DWORD leftover = fIntervalInMilliseconds - Rtt_AbsoluteToMilliseconds(now - beganWhen) % fIntervalInMilliseconds;
		Sleep(leftover);
	}
}

void WinTimer::ThreadBody()
{
	SRWLOCK waitWhileStoppedLock;

	InitializeSRWLock(&waitWhileStoppedLock);

	while (true)
	{
		// A "stop" either means something like minimizing the window,
		// in which case the thread should go dormant, or quitting. In
		// the former case, the situation should last a while, so we
		// wait for a start event (such as a maximize) to wake us back
		// up. If a stop happens later in the loop (and persists), it
		// will effectively no-op and roll back around here anyway.
		bool stillRunning = WaitWhileStopped(&waitWhileStoppedLock);
		if (!stillRunning)
		{
			return;
		}

		// We do a lock here, mindful of the (unlikely) case another
		// stop crept in, and fetch the otherwise potentially contended
		// time, indicating when the last Evaluate() started. This will
		// be bogus on the first frame, or if there was a stop along the
		// way. If the time is valid at this point, we do the wait.
		// It is possible a stop will occur while this is happening,
		// but this is harmless aside from the (tiny) wasted delay.
		if (TryToLockFromExpectedState(kRunning))
		{
			U64 evaluateBeganTime = fEvaluateBeganTime;

			InterlockedExchange(&fState, kRunning); // unlock and wait

			if (InvalidBeganTime() != evaluateBeganTime)
			{
				WaitForInterval(evaluateBeganTime);
			}
		}

		// We do another lock here, given the also-unlikely prospect
		// of a stop, either during the wait-for-interval or even the
		// much narrower gap since the "started" event. We opt into
		// the "stopped" events while still holding the lock, since
		// a stop might also land before WM_TIMER gets processed.
		// (There is also a guard condition for the pathological case
		// that an intermediate stop triggers, followed by another
		// start. Since the first such stop will signal the "stopped"
		// event and kill the wait, we must interpret the situation
		// as though that happened, even though the restart has us
		// arrive running in the timer, cf. OnTimerElapsed_V2().)
		AcquireSRWLockExclusive(fLock);

		if (kRunning == fState)
		{
			fEvaluateBeganTime = Rtt_GetAbsoluteTime();

			Evaluate_V2();

			// ...and wait for it to get processed. If it does get picked
			// up, i.e. a stop doesn't intervene, we can wait for Evaluate()
			// to finish without further worries.

		}

		ReleaseSRWLockExclusive(fLock);
	}
}
// /STEVE CHANGE

}	// namespace Rtt

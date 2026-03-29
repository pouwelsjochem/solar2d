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
#include <timeapi.h>

// Required for timeBeginPeriod()/timeEndPeriod() which set the system timer
// resolution to 1ms, enabling accurate Sleep() granularity in the frame loop.
#pragma comment(lib, "winmm.lib")

namespace Rtt
{

	std::unordered_map<UINT_PTR, Rtt::WinTimer*> WinTimer::sTimerMap;
	UINT_PTR WinTimer::sMostRecentTimerID;

#pragma region Constructors/Destructors

	WinTimer::WinTimer(MCallback& callback, HWND windowHandle)
	:	PlatformTimer(callback),
		fWindowHandle(windowHandle),
		fThreadHandle(nullptr),
		fStopEvent(nullptr),
		fFrameBoundaryEvent(nullptr),
		fRunning(false),
		fUseThreadedScheduler(windowHandle != nullptr),
		fTimerPointer(NULL),
		fTimerID(0),
		fIntervalInMilliseconds(10),
		fNextIntervalTimeInTicks(0),
		fLastFrameBoundaryQpc(0),
		fTickPending(false)
	{
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
		fTickPending.store(false);
		fLastFrameBoundaryQpc.store(0);

		// Assign a unique timer ID and register this instance in the map regardless
		// of which timing path is used. The ID is posted as wParam in WM_CORONA_TIMER
		// (threaded path) and passed as idEvent in WM_TIMER (legacy path), allowing
		// the respective message handlers to look up the correct WinTimer instance.
		// ID should be non-0, so pre-increment for first time.
		fTimerID = ++sMostRecentTimerID;
		sTimerMap[fTimerID] = this;

		if (fUseThreadedScheduler)
		{
			// Present-driven threaded scheduler path.
			// Force 1ms system timer resolution so Sleep() in the frame loop has
			// sufficient granularity to maintain a small lead time before the next
			// frame start. Without this, Windows defaults to ~15.6ms resolution.
			::timeBeginPeriod(1);

			fStopEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
			fFrameBoundaryEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
			if (!fStopEvent || !fFrameBoundaryEvent)
			{
				if (fFrameBoundaryEvent)
				{
					::CloseHandle(fFrameBoundaryEvent);
					fFrameBoundaryEvent = nullptr;
				}
				if (fStopEvent)
				{
					::CloseHandle(fStopEvent);
					fStopEvent = nullptr;
				}
				::timeEndPeriod(1);
				sTimerMap.erase(fTimerID);
				fTimerID = 0;
				fRunning = false;
				return;
			}

			fThreadHandle = ::CreateThread(
					nullptr, 0, WinTimer::TimerThreadProc, this, 0, nullptr);
			if (!fThreadHandle)
			{
				::CloseHandle(fFrameBoundaryEvent);
				fFrameBoundaryEvent = nullptr;
				::CloseHandle(fStopEvent);
				fStopEvent = nullptr;
				::timeEndPeriod(1);
				sTimerMap.erase(fTimerID);
				fTimerID = 0;
				fRunning = false;
				return;
			}

			// Run at normal priority. The thread mostly waits and only needs brief
			// CPU access for the final spin phase.
			::SetThreadPriority(fThreadHandle, THREAD_PRIORITY_NORMAL);
		}
		else
		{
			// Legacy WM_TIMER path.
			// Start the timer with an interval faster than the configured interval
			// because Windows timers can invoke later than expected. We compensate by
			// comparing against fNextIntervalTimeInTicks inside Evaluate().
			fNextIntervalTimeInTicks = (S32)::GetTickCount() + (S32)fIntervalInMilliseconds;
			fTimerPointer = ::SetTimer(fWindowHandle, fTimerID, 10, WinTimer::OnTimerElapsed);
			if (!fTimerPointer)
			{
				sTimerMap.erase(fTimerID);
				fTimerID = 0;
				fRunning = false;
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

		UINT_PTR timerId = fTimerID;
		fRunning = false;

		// Always remove from the timer map regardless of which path is active.
		// This guards against message callbacks firing after Stop() has been called.
		sTimerMap.erase(fTimerID);
		fTimerID = 0;

		if (fUseThreadedScheduler)
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
			if (fFrameBoundaryEvent)
			{
				::CloseHandle(fFrameBoundaryEvent);
				fFrameBoundaryEvent = nullptr;
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
			::KillTimer(fWindowHandle, timerId);
			fTimerPointer = NULL;
		}

		fTickPending.store(false);
		fLastFrameBoundaryQpc.store(0);
	}

	void WinTimer::SetInterval(U32 milliseconds)
	{
		fIntervalInMilliseconds = milliseconds;
	}

	bool WinTimer::IsRunning() const
	{
		if (fUseThreadedScheduler)
		{
			// Threaded scheduler path does not use fTimerPointer — use fRunning instead.
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

		if (fUseThreadedScheduler)
		{
			// Threaded scheduler path: pacing has already been enforced relative to the
			// last completed frame boundary, so just invoke the callback directly.
			this->operator()();
			SignalFrameBoundary();
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

	void WinTimer::SignalFrameBoundary()
	{
		if (!fUseThreadedScheduler)
		{
			return;
		}

		LARGE_INTEGER now;
		::QueryPerformanceCounter(&now);
		fLastFrameBoundaryQpc.store(now.QuadPart);
		fTickPending.store(false);

		if (fFrameBoundaryEvent)
		{
			::SetEvent(fFrameBoundaryEvent);
		}
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
		HANDLE waitHandles[] = { fStopEvent, fFrameBoundaryEvent };

		while (fRunning)
		{
			DWORD waitResult = ::WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
			if (WAIT_OBJECT_0 == waitResult)
			{
				break;
			}
			if (WAIT_OBJECT_0 + 1 != waitResult)
			{
				continue;
			}

			while (fRunning)
			{
				::ResetEvent(fFrameBoundaryEvent);

				double intervalSeconds = static_cast<double>(fIntervalInMilliseconds) / 1000.0;
				double presentLeadTime = intervalSeconds * 0.25;
				if (presentLeadTime < 0.002)
				{
					presentLeadTime = 0.002;
				}
				else if (presentLeadTime > 0.004)
				{
					presentLeadTime = 0.004;
				}

				LONGLONG startDelayQpc =
					(LONGLONG)(((intervalSeconds - presentLeadTime) * freq.QuadPart) + 0.5);
				if (startDelayQpc < 0)
				{
					startDelayQpc = 0;
				}

				LONGLONG frameBoundaryQpc = fLastFrameBoundaryQpc.load();
				LONGLONG wakeQpc = frameBoundaryQpc + startDelayQpc;
				bool shouldReanchor = false;

				while (fRunning)
				{
					::QueryPerformanceCounter(&now);
					if (now.QuadPart >= wakeQpc)
					{
						break;
					}

					LONGLONG timeUntilWakeQpc = wakeQpc - now.QuadPart;
					if (timeUntilWakeQpc > (freq.QuadPart / 1000))
					{
						DWORD sleepMs =
							(DWORD)(((timeUntilWakeQpc - (freq.QuadPart / 1000)) * 1000) / freq.QuadPart);
						DWORD sleepResult =
							::WaitForMultipleObjects(2, waitHandles, FALSE, (sleepMs > 0) ? sleepMs : 1);
						if (WAIT_OBJECT_0 == sleepResult)
						{
							return;
						}
						if (WAIT_OBJECT_0 + 1 == sleepResult)
						{
							shouldReanchor = true;
							break;
						}
						continue;
					}

					if (WAIT_OBJECT_0 == ::WaitForSingleObject(fStopEvent, 0))
					{
						return;
					}
					if (WAIT_OBJECT_0 == ::WaitForSingleObject(fFrameBoundaryEvent, 0))
					{
						shouldReanchor = true;
						break;
					}
					::YieldProcessor();
				}

				if (!fRunning)
				{
					break;
				}
				if (shouldReanchor)
				{
					continue;
				}

				bool expected = false;
				if (fTickPending.compare_exchange_strong(expected, true))
				{
					if (!::PostMessage(fWindowHandle, WM_CORONA_TIMER, (WPARAM)fTimerID, 0))
					{
						fTickPending.store(false);
					}
				}
				break;
			}
		}
	}

	VOID CALLBACK WinTimer::OnTimerElapsed(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
	{
		// Legacy WM_TIMER callback — only active when fUseThreadedScheduler is false.
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

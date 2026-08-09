//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_AppleTimer_H__
#define _Rtt_AppleTimer_H__

#include "Rtt_PlatformTimer.h"

// ----------------------------------------------------------------------------

#if defined(__cplusplus) && ! defined(__OBJC__)
class NSTimer;
class AppleCallback;
class NSObject;
#if defined( Rtt_MAC_ENV ) && ! defined( Rtt_NO_GUI )
class NSView;
#endif
#else
@class NSTimer;
@class AppleCallback;
@class NSObject;
#if defined( Rtt_MAC_ENV ) && ! defined( Rtt_NO_GUI )
@class NSView;
#endif
#endif


namespace Rtt
{

// ----------------------------------------------------------------------------

class AppleTimer : public PlatformTimer
{
	Rtt_CLASS_NO_COPIES( AppleTimer )

	public:
		typedef PlatformTimer Super;

	public:
		AppleTimer( MCallback& callback );
#if defined( Rtt_MAC_ENV ) && ! defined( Rtt_NO_GUI )
		AppleTimer( MCallback& callback, NSView* view );
#endif
		virtual ~AppleTimer();

	public:
		virtual void Start();
		virtual void Stop();
		virtual void SetInterval( U32 milliseconds );
		virtual void SetInterval( double milliseconds );
		virtual bool IsRunning() const;
#if defined( Rtt_MAC_ENV ) && ! defined( Rtt_NO_GUI )
		virtual Rtt_AbsoluteTime GetCurrentTime() const;
		virtual double GetRefreshRate() const;
		virtual double GetLastFrameWorkMs() const;
#endif

	public:
		AppleCallback* GetTarget() { return fTarget; }

#if defined( Rtt_MAC_ENV ) && ! defined( Rtt_NO_GUI )
	private:
		bool StartMacDisplayLink();
		void StopMacDisplayLink();
		void ApplyMacDisplayLinkInterval();
		void UpdateMacDisplayLinkScreen();
#endif
	
	private:
		id fDisplayLink;
		NSTimer* fTimer;
		AppleCallback* fTarget;
		double fInterval;
#if defined( Rtt_MAC_ENV ) && ! defined( Rtt_NO_GUI )
		NSView* fView;
		void* fMacDisplayLink;
		void* fDisplaySource;
		AppleCallback* fDisplayTarget;
		NSObject* fScreenObserver;
#endif
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_AppleTimer_H__

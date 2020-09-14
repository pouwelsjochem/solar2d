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
#else
@class NSTimer;
@class AppleCallback;
@class NSObject;
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
		virtual ~AppleTimer();

	public:
		virtual void Start();
		virtual void Stop();
		virtual void SetInterval( U32 milliseconds );
		virtual bool IsRunning() const;

	public:
		AppleCallback* GetTarget() { return fTarget; }
	
	private:
		id fDisplayLink;
		NSTimer* fTimer;
		AppleCallback* fTarget;
		U32 fInterval;
		bool fDisplayLinkSupported;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_AppleTimer_H__

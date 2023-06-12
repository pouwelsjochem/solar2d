//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Rtt_PlatformAudioSessionManager.h"


#include "Core/Rtt_Build.h"
#include "Rtt_PlatformAudioSessionManager.h"

#ifdef Rtt_AUDIO_SESSION_PROPERTY
	#include "Rtt_IPhoneAudioSessionManager.h"
#endif


namespace Rtt
{

PlatformAudioSessionManager* s_PlatformAudioSessionMananger = NULL;

PlatformAudioSessionManager::PlatformAudioSessionManager()
{		
}

PlatformAudioSessionManager::~PlatformAudioSessionManager()
{		
}
	
PlatformAudioSessionManager* 
PlatformAudioSessionManager::SharedInstance()
{
	if(NULL == s_PlatformAudioSessionMananger)
	{
#ifdef Rtt_AUDIO_SESSION_PROPERTY
//		s_PlatformAudioSessionMananger = Rtt_NEW( NULL, PlatformAudioSessionManager);
		s_PlatformAudioSessionMananger = IPhoneAudioSessionManager::GetInstance();
#else
		s_PlatformAudioSessionMananger = Rtt_NEW( NULL, PlatformAudioSessionManager);
#endif
	}
	return s_PlatformAudioSessionMananger;
}

void 
PlatformAudioSessionManager::Destroy()
{
	if(NULL != s_PlatformAudioSessionMananger)
	{
#ifdef Rtt_AUDIO_SESSION_PROPERTY
		IPhoneAudioSessionManager::DestroyInstance();
#else
		Rtt_DELETE( s_PlatformAudioSessionMananger );
#endif
		s_PlatformAudioSessionMananger = NULL;
	}
}


bool
PlatformAudioSessionManager::SetAudioSessionActive( bool is_active )
{
	return true;
}

bool 
PlatformAudioSessionManager::GetAudioSessionActive() const
{
	return true;
}

bool
PlatformAudioSessionManager::SupportsBackgroundAudio() const
{
	return false;
}

bool
PlatformAudioSessionManager::AllowsAudioDuringScreenLock() const
{
	return false;
}

bool
PlatformAudioSessionManager::IsInInterruption() const
{
	return false;
}
} // namespace Rtt

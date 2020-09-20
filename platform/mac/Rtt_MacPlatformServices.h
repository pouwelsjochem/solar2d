//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_MacPlatformServices_H__
#define _Rtt_MacPlatformServices_H__

#include "Rtt_MPlatformServices.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

class MacPlatformServices : public MPlatformServices
{
	Rtt_CLASS_NO_COPIES( MacPlatformServices )

	public:
		typedef MacPlatformServices Self;

	public:
		MacPlatformServices( const MPlatform& platform );

	public:
		// MPlatformServices
		virtual const MPlatform& Platform() const;
		virtual void GetPreference( const char *key, Rtt::String * value ) const;
		virtual void SetPreference( const char *key, const char *value ) const;
		virtual void GetSecurePreference( const char *key, Rtt::String * value ) const;
		virtual bool SetSecurePreference( const char *key, const char *value ) const;
		virtual bool IsInternetAvailable() const;
		virtual bool IsLocalWifiAvailable() const;
		virtual void Terminate() const;

		virtual void Sleep( int milliseconds ) const;

	private:
		const MPlatform& fPlatform;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_MacPlatformServices_H__

//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_MacConsolePlatform_H__
#define _Rtt_MacConsolePlatform_H__

#include "Rtt_ApplePlatform.h"

#include "Rtt_MacDevice.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

class MacConsoleDevice;

// ----------------------------------------------------------------------------

class MacConsolePlatform : public ApplePlatform
{
	public:
		typedef ApplePlatform Super;

	public:
		MacConsolePlatform();
		virtual ~MacConsolePlatform();


	public:
		virtual MPlatformDevice& GetDevice() const;
		virtual PlatformSurface* CreateScreenSurface() const;
		virtual PlatformSurface* CreateOffscreenSurface( const PlatformSurface& parent ) const;
    
		virtual void SaveBitmap( PlatformBitmap* bitmap, Rtt::Data<const char> & pngBytes ) const;
		virtual bool OpenURL( const char* url ) const;
		virtual int CanOpenURL( const char* url ) const;

		virtual NativeAlertRef ShowNativeAlert(
			const char *title,
			const char *msg,
			const char **buttonLabels,
			U32 numButtons,
			LuaResource* resource ) const;
		virtual void CancelNativeAlert( NativeAlertRef alert, S32 index ) const;

		virtual PlatformDisplayObject* CreateNativeWebView( const Rect& bounds ) const;
		virtual PlatformDisplayObject* CreateNativeVideo( const Rect& bounds ) const;

		virtual void RuntimeErrorNotification( const char *errorType, const char *message, const char *stacktrace ) const;

		virtual void GetSafeAreaInsetsPixels(Rtt_Real &top, Rtt_Real &left, Rtt_Real &bottom, Rtt_Real &right) const;
		
	private:
		MacConsoleDevice fDevice;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_MacConsolePlatform_H__

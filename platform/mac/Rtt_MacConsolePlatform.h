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
    
		virtual bool SaveBitmap( PlatformBitmap* bitmap, const char* filePath ) const;
		virtual bool OpenURL( const char* url ) const;
		virtual int CanOpenURL( const char* url ) const;

		virtual PlatformStoreProvider* GetStoreProvider( const ResourceHandle<lua_State>& handle ) const;

		virtual void SetIdleTimer( bool enabled ) const;
		virtual bool GetIdleTimer() const;

		virtual NativeAlertRef ShowNativeAlert(
			const char *title,
			const char *msg,
			const char **buttonLabels,
			U32 numButtons,
			LuaResource* resource ) const;
		virtual void CancelNativeAlert( NativeAlertRef alert, S32 index ) const;

		virtual void SetActivityIndicator( bool visible ) const;

		virtual PlatformDisplayObject* CreateNativeTextBox( const Rect& bounds ) const;
		virtual PlatformDisplayObject* CreateNativeTextField( const Rect& bounds ) const;
		virtual void SetKeyboardFocus( PlatformDisplayObject *textObject ) const;

        virtual PlatformFBConnect* GetFBConnect() const;

		virtual S32 GetFontNames( lua_State *L, int index ) const;

		virtual void RuntimeErrorNotification( const char *errorType, const char *message, const char *stacktrace ) const;
        virtual void SetProjectResourceDirectory( const char* filename );

		virtual void GetSafeAreaInsetsPixels(Rtt_Real &top, Rtt_Real &left, Rtt_Real &bottom, Rtt_Real &right) const;
		
	private:
		MacConsoleDevice fDevice;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_MacConsolePlatform_H__

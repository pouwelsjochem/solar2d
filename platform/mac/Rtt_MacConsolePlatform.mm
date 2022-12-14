//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_MacConsolePlatform.h"

#import <Foundation/Foundation.h>
#import "NSString+Extensions.h"

#include "Core/Rtt_String.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

static bool
MacOpenURL( const char *url )
{
	bool result = false;

	if ( url )
	{
		NSURL* urlPlatform = [NSURL URLWithString:[NSString stringWithExternalString:url]];
		OSStatus status = LSOpenCFURLRef( (CFURLRef)urlPlatform, NULL );

		// Compare with result codes:
		// http://developer.apple.com/DOCUMENTATION/Carbon/Reference/LaunchServicesReference/Reference/reference.html#//apple_ref/doc/uid/TP30000998-CH4g-BCIHFFIA
		result = Rtt_VERIFY( noErr == status );

		#ifdef Rtt_AUTHORING_SIMULATOR
		if ( ! result )
		{
			Rtt_TRACE_SIM( ( "WARNING: url( %s ) is not supported by the simulator\n", url ) );
		}
		#endif
	}

	return result;
}

// ----------------------------------------------------------------------------

MacConsolePlatform::MacConsolePlatform()
:	fDevice( GetAllocator(), NULL )
{
}

MacConsolePlatform::~MacConsolePlatform()
{
}
	
MPlatformDevice&
MacConsolePlatform::GetDevice() const
{
	return const_cast< MacConsoleDevice& >( fDevice );
}

PlatformSurface*
MacConsolePlatform::CreateScreenSurface() const
{
	return NULL;
}

PlatformSurface*
MacConsolePlatform::CreateOffscreenSurface( const PlatformSurface& ) const
{
	return NULL;
}

void
MacConsolePlatform::SaveBitmap( PlatformBitmap* bitmap, Rtt::Data<const char> & pngBytes ) const
{
}

bool
MacConsolePlatform::OpenURL( const char* url ) const
{
	return MacOpenURL( url );
}

int
MacConsolePlatform::CanOpenURL( const char* url ) const
{
	return -1;
}

NativeAlertRef
MacConsolePlatform::ShowNativeAlert(
	const char *title,
	const char *msg,
	const char **buttonLabels,
	U32 numButtons,
	LuaResource* resource ) const
{
	return NULL;
}

void
MacConsolePlatform::CancelNativeAlert( NativeAlertRef alert, S32 index ) const
{
}

PlatformDisplayObject *
MacConsolePlatform::CreateNativeWebView( const Rect& bounds ) const
{
	return NULL;
}

void
MacConsolePlatform::RuntimeErrorNotification( const char *errorType, const char *message, const char *stacktrace ) const
{
}

void MacConsolePlatform::GetSafeAreaInsetsPixels(Rtt_Real &top, Rtt_Real &left, Rtt_Real &bottom, Rtt_Real &right) const
{
	top = left = bottom = right = 0;
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Core/Rtt_String.h"

#include "Rtt_TVOSPlatform.h"
#include "Rtt_IPhoneTimer.h"

#include "Rtt_IPhoneAudioSessionManager.h"

#include "Rtt_IPhoneScreenSurface.h"

#include "Rtt_LuaLibNative.h"
#include "Rtt_LuaLibSystem.h"
#include "Rtt_LuaResource.h"

#import "CoronaViewPrivate.h"

#import <UIKit/UIApplication.h>
#import <UIKit/UIDevice.h>
#import <UIKit/UIGestureRecognizerSubclass.h>

#include "CoronaLua.h"

#include "Rtt_TouchInhibitor.h"

// ----------------------------------------------------------------------------

// Consume all touches, preventing their propagation
@interface CoronaNullGestureRecognizer : UIGestureRecognizer
@end

@implementation CoronaNullGestureRecognizer

- (instancetype)init
{
	self = [super initWithTarget:self action:@selector(handleGesture)];
	return self;
}

- (void)handleGesture
{
	// no-op
}

- (void)reset
{
	self.state = UIGestureRecognizerStatePossible;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	self.state = UIGestureRecognizerStateRecognized;
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	self.state = UIGestureRecognizerStateRecognized;
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	self.state = UIGestureRecognizerStateRecognized;
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	self.state = UIGestureRecognizerStateRecognized;
}

@end

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

TVOSPlatform::TVOSPlatform( CoronaView *view )
:	Super( view ),
	fDevice( GetAllocator(), view )
{
}

TVOSPlatform::~TVOSPlatform()
{
	[fActivityView release];
}

// =====================================================================


MPlatformDevice&
TVOSPlatform::GetDevice() const
{
	return const_cast< TVOSDevice& >( fDevice );
}

// =====================================================================

bool
TVOSPlatform::CanShowPopup( const char *name ) const
{
	return false;
}

bool
TVOSPlatform::ShowPopup( lua_State *L, const char *name, int optionsIndex ) const
{
	bool result = false;

	id<CoronaRuntime> runtime = (id<CoronaRuntime>)CoronaLuaGetContext( L );
	UIViewController* viewController = runtime.appViewController ;
	if ( viewController.presentedViewController )
	{
		Rtt_ERROR( ( "ERROR: There is already a native modal interface being displayed. The '%s' popup will not be shown.\n", name ? name : "" ) );
	}

	return result;
}

bool
TVOSPlatform::HidePopup( const char *name ) const
{
	bool result = false;

	Rtt_ASSERT_NOT_IMPLEMENTED();

	return result;
}

void
TVOSPlatform::RuntimeErrorNotification( const char *errorType, const char *message, const char *stacktrace ) const
{
    NSLog(@"Runtime Error: %s: %s\n%s", errorType, message, stacktrace);
}

static Rtt_INLINE
double DegreesToRadians( double degrees )
{
	return degrees * M_PI/180;
}

void
TVOSPlatform::SaveBitmap( PlatformBitmap* bitmap, Rtt::Data<const char> & pngBytes ) const
{
	Rtt_ASSERT( bitmap );

	const void* buffer = bitmap->GetBits( & GetAllocator() );
	size_t w = bitmap->Width();
	size_t h = bitmap->Height();
	size_t wDst = w;
	size_t hDst = h;

	size_t bytesPerPixel = PlatformBitmap::BytesPerPixel( bitmap->GetFormat() );
	size_t bytesPerRow = w*bytesPerPixel;
	NSInteger numBytes = h*bytesPerRow;

	CGBitmapInfo srcBitmapInfo = CGBitmapInfo(kCGBitmapByteOrderDefault | kCGImageAlphaLast);
	CGBitmapInfo dstBitmapInfo = kCGImageAlphaPremultipliedLast;

	CGDataProviderRef dataProvider = CGDataProviderCreateWithData( NULL, buffer, numBytes, NULL );
	CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
	CGImageRef imageRef = CGImageCreate(w, h, 8, 32, w*bytesPerPixel,
                                        colorspace, srcBitmapInfo, dataProvider,
                                        NULL, true, kCGRenderingIntentDefault);


	Rtt_ASSERT( w == CGImageGetWidth( imageRef ) );
	Rtt_ASSERT( h == CGImageGetHeight( imageRef ) );

	CGContextRef context = CGBitmapContextCreate(NULL, wDst, hDst, 8, wDst*bytesPerPixel, colorspace, dstBitmapInfo);
    CGContextTranslateCTM( context, wDst, hDst );

	CGContextDrawImage( context, CGRectMake( 0.0, 0.0, w, h ), imageRef );
	CGImageRef bitmapImageRef = CGBitmapContextCreateImage(context);
	UIImage* image = [[UIImage alloc] initWithCGImage:bitmapImageRef];
    NSData *bitmapImageRepData = UIImagePNGRepresentation( image );

	[image release];
	CGImageRelease( bitmapImageRef );
	CGColorSpaceRelease( colorspace );
	CGContextRelease( context );
	CGImageRelease( imageRef );
	CGDataProviderRelease( dataProvider );

	bitmap->FreeBits();
    pngBytes.Set((const char *)bitmapImageRepData.bytes, bitmapImageRepData.length);
}



// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_AppleBitmap.h"

#import <CoreFoundation/CFURL.h>

#ifdef Rtt_MAC_ENV
	#import <Cocoa/Cocoa.h>
	#import <ApplicationServices/ApplicationServices.h>

	#if (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_5)
		#if __LP64__
			typedef double CGFloat;// 64-bit
		#else
			typedef float CGFloat;// 32-bit
		#endif
	#endif
	// Apple is retarded about forcing you to include ridiculously large header
	// files when these are the only ones I need:
	// 
	// #include <CoreGraphics/CoreGraphics.h>
	// #include <ImageIO/ImageIO.h>
#else
	#include <CoreGraphics/CoreGraphics.h>
	#include <ImageIO/ImageIO.h>
	#import <UIKit/UIKit.h>
#endif

#include <string.h>


#ifdef Rtt_TEST_BITMAP
	#include "Rtt_Matrix.h"
	#include "Display/Rtt_Paint.h"
	#include "Display/Rtt_ShapeObject.h"
	#include "Display/Rtt_PlatformBitmap.h"
	#include "Rtt_Runtime.h"
	#include "Rtt_MPlatform.h"
#endif

#include "Display/Rtt_Display.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

static size_t
GetMaxTextureSize()
{
	return Display::GetMaxTextureSize();
}


#ifdef Rtt_TEST_BITMAP

void
AppleBitmap::Test( class Runtime& runtime )
{

}

#endif // Rtt_TEST_BITMAP

// ----------------------------------------------------------------------------

AppleBitmap::AppleBitmap()
:	fData( NULL )
{
}

AppleBitmap::~AppleBitmap()
{
	Self::FreeBits();
}

void
AppleBitmap::FreeBits() const
{
	if ( fData )
	{
		Rtt_FREE( fData );
		fData = NULL;
	}
}

// ----------------------------------------------------------------------------

static U8
GetInitialPropertiesValue()
{
	// CGBitmapContext's have to use a colorspace with premultiplied alpha for some unknown reason...
	// http://developer.apple.com/mac/library/qa/qa2001/qa1037.html
	return PlatformBitmap::kIsPremultiplied;
}

AppleFileBitmap::AppleFileBitmap( const char* inPath, bool isMask )
:	Super(),
	fImage( NULL ),
	fProperties( GetInitialPropertiesValue() ),
	fIsMask( isMask )
{
	size_t inPathLength = strlen( inPath );

	CFURLRef imageUrl = CFURLCreateFromFileSystemRepresentation( kCFAllocatorDefault, (UInt8*)inPath, inPathLength, false );
	CGImageSourceRef imageSource = NULL;

	if ( Rtt_VERIFY( imageUrl ) )
	{
		// CFShow( imageUrl );
		
		imageSource = CGImageSourceCreateWithURL( imageUrl, NULL );

		if ( Rtt_VERIFY( imageSource ) )
		{
			// CFShow( imageSource );            
			fImage = CGImageSourceCreateImageAtIndex( imageSource, 0, NULL );

			// if ( fImage ) { CFShow( fImage ); }
			CFRelease( imageSource );
		}

		CFRelease( imageUrl );
	}

	if (fImage)
	{
		Initialize();
	}
}

#if defined( Rtt_IPHONE_ENV ) || defined( Rtt_TVOS_ENV )

AppleFileBitmap::AppleFileBitmap( UIImage* image, bool isMask )
:	Super(),
	fImage( (CGImageRef)CFRetain( image.CGImage ) ),
	fProperties( GetInitialPropertiesValue() ),
	fIsMask( isMask )
{
	Initialize();
}
#endif

#ifdef Rtt_MAC_ENV
AppleFileBitmap::AppleFileBitmap( NSImage* image, bool isMask )
:	Super(),
	fImage( NULL ),
	fProperties( GetInitialPropertiesValue() ),
	fIsMask( isMask )
{
	fImage = (CGImageRef)CFRetain( [image CGImageForProposedRect:NULL context:NULL hints:nil] );
	Initialize();
}
#endif
	
AppleFileBitmap::~AppleFileBitmap()
{
	if ( fImage )
	{
		CFRelease( fImage );
	}
}

void
AppleFileBitmap::Initialize()
{
	fScale = CalculateScale();
}

#if !defined( Rtt_WEB_PLUGIN )
	// Implemented on a per-platform basis
	Rtt_EXPORT CGSize Rtt_GetDeviceSize();
#endif

float
AppleFileBitmap::CalculateScale() const
{
	float result = -1.0; // Default value to flag that no scale was applied

	if ( ! Rtt_VERIFY( fImage ) )
	{
		return result;
	}
	
	size_t w = CGImageGetWidth( fImage );
	size_t h = CGImageGetHeight( fImage );

#if !defined( Rtt_WEB_PLUGIN )
	// TODO: Rtt_GetDeviceSize() returns (0,0) if no simulator instance is active.
	// However, we have CoronaViews that are created independent of simulator,
	// so for now, we just do a cheap check against 0 to ignore. Later, we should
	// determine a way to decouple.
	CGSize deviceSize = Rtt_GetDeviceSize();
	bool isDeviceSizeValid = ( deviceSize.width > 0 && deviceSize.height > 0 ); 

	size_t wMax = isDeviceSizeValid ? NextPowerOf2( deviceSize.width ) : w;
	size_t hMax = isDeviceSizeValid ? NextPowerOf2( deviceSize.height ) : h;
#else
	// TODO: Figure out how to get "screen" bounds, i.e. content bounds in html page
	size_t wMax = w;
	size_t hMax = h;
#endif
	
	// Downscale the image if it exceeds OpenGL's maximum texture size.
	size_t maxBitmapSize = (size_t)GetMaxTextureSize();

	if ( IsProperty( kIsBitsFullResolution ) )
	{
		// We still have to scale if the bitmap is larger than the largest texture size
		if ( w <= maxBitmapSize && h <= maxBitmapSize )
		{
			return result;
		}
		else
		{
			Rtt_LogException( "WARNING: Image size (%ld,%ld) exceeds max texture dimension (%ld). Image will be resized to fit.\n", w, h, maxBitmapSize );
			wMax = maxBitmapSize;
			hMax = maxBitmapSize;
		}
	}
	else
	{
		wMax = Min( wMax, maxBitmapSize );
		hMax = Min( hMax, maxBitmapSize );
	}

	// Align longest image edge to the longest screen edge to calculate proper scale.
	// If image is landscape and screen size is portrait (or vice versa), 
	// then swap screen dimensions. 
	bool isScreenLandscape = wMax > hMax;
	if ( !isScreenLandscape )
	{
		size_t tmp = wMax;
		wMax = hMax; hMax = tmp;
	}

	if ( w > wMax || h > hMax )
	{
		float scaleW = ((float)wMax)/(float)w;
		float scaleH = ((float)hMax)/(float)h;
		result = ( scaleH < scaleW ? scaleH : scaleW );
	}

	// Return the down-scaling factor to be applied to the image.
	// Returns -1 if no scaling should be applied.
	return result;
}

void*
AppleFileBitmap::GetBitsGrayscale( Rtt_Allocator* context ) const
{
	Rtt_ASSERT( ! fData );

	void *result = NULL;

	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceGray();

	if ( colorSpace )
	{
		Rtt_ASSERT( 1 == CGColorSpaceGetNumberOfComponents( colorSpace ) );

		const size_t width = Width();
		const size_t height = Height();

		Rtt_ASSERT( CGImageGetBitsPerPixel( fImage ) >= CGImageGetBitsPerComponent( fImage ) );
//		const size_t bytesPerPixel = CGImageGetBitsPerPixel( fImage ) / CGImageGetBitsPerComponent( fImage );
		const size_t bytesPerRow = width; // * bytesPerPixel;
		const CGBitmapInfo bitmapInfo = kCGImageAlphaNone;
		void* data = Rtt_CALLOC( context, bytesPerRow, height );
		
		const size_t bitsPerChannel = 8;
		CGContextRef bmpContext = CGBitmapContextCreate(
										data,
										width, height, bitsPerChannel,
										bytesPerRow, colorSpace,
										bitmapInfo );

		if ( Rtt_VERIFY( bmpContext ) )
		{
			CGRect rect = { { 0.f, 0.f }, { (CGFloat)width, (CGFloat)height } };

			CGContextDrawImage( bmpContext, rect, fImage );
			CGContextRelease( bmpContext );
			result = data;
		}
		else
		{
			Rtt_FREE( data );
		}

		CGColorSpaceRelease( colorSpace );
	}

	return result;
}

void*
AppleFileBitmap::GetBitsColor( Rtt_Allocator* context ) const
{
	Rtt_ASSERT( ! fData );
	
	void *result = NULL;

	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();

	if ( colorSpace )
	{
		const size_t width = Width();
		const size_t height = Height();

		const size_t bytesPerRow = width << 2;
		const CGBitmapInfo bitmapInfo =
				#ifdef Rtt_MAC_ENV
					kCGImageAlphaPremultipliedFirst;
				#else
					kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big;
				#endif
		void* data = Rtt_CALLOC( context, bytesPerRow, height );
		
		const size_t bitsPerChannel = 8;
		CGContextRef bmpContext = CGBitmapContextCreate(
										data,
										width, height, bitsPerChannel,
										bytesPerRow, colorSpace,
										bitmapInfo );
		if ( Rtt_VERIFY( bmpContext ) )
		{
			CGRect rect = { { 0.f, 0.f }, { (CGFloat)width, (CGFloat)height } };
			CGContextDrawImage( bmpContext, rect, fImage );
			CGContextRelease( bmpContext );
			result = data;

// Debug pixel data
#if 0
PrintChannel( (const U8 *)result, 0, 4 );
PrintChannel( (const U8 *)result, 1, 4 );
PrintChannel( (const U8 *)result, 2, 4 );
PrintChannel( (const U8 *)result, 3, 4 );
#endif

#if 0
CGDataProviderRef provider = CGImageGetDataProvider( fImage );
if ( provider )
{
	CFDataRef imageData = CGDataProviderCopyData( provider );

	const void* bytes = CFDataGetBytePtr( imageData );
	Rtt_ASSERT( 0 == memcmp( bytes, data, bytesPerRow*height ) );

	CFRelease( imageData );
	CGDataProviderRelease( provider );
}
#endif
		}
		else
		{
			Rtt_FREE( data );
		}
		
		CGColorSpaceRelease( colorSpace );
	}
	
	return result;
}

const void*
AppleFileBitmap::GetBits( Rtt_Allocator* context ) const
{
	if ( ! fData )
	{
		// We don't support these colorspaces because we cannot create
		// a bitmap context for them
		Rtt_ASSERT( CGImageGetColorSpace( fImage ) );
		Rtt_ASSERT( kCGColorSpaceModelPattern != CGColorSpaceGetModel( CGImageGetColorSpace( fImage ) ) );

		#if 0
		{
			CGColorSpace colorSpace = CGImageGetColorSpace( fImage );
			size_t numChannels = CGColorSpaceGetNumberOfComponents( colorSpace );
			CGColorSpaceModel model = CGColorSpaceGetModel( colorSpace );
			Rtt_TRACE_SIM( ( "AppleFileBitmap::GetBits() numChannels(%d) model(%d)\n", numChannels, model ) );
		}
		#endif

		fData = ( fIsMask ? GetBitsGrayscale( context ) : GetBitsColor( context ) );
	}

	return fData;
}

#ifdef Rtt_DEBUG
void
AppleFileBitmap::PrintChannel( const U8 *bytes, int channel, U32 bytesPerPixel ) const
{
	const U32 width = Width();
	const U32 height = Height();
	const U32 bytesPerRow = width * 4;

	printf( "---------------------------------------------------\n" );
	printf( "[Channel = %d]\n", channel );

	for ( U32 j = 0; j < height; j++ )
	{
		const U8 *rowBytes = bytes + j*bytesPerRow + channel;
		for ( U32 i = 0; i < width; i++ )
		{
			printf( "%02x", *rowBytes );

			rowBytes += bytesPerPixel;
		}
		printf( "\n" );
	}

}
#endif

U32
AppleFileBitmap::Width() const
{
	U32 len = (U32) CGImageGetWidth( fImage );
	return WasScaled() ? (fScale * len) : len;
}

U32
AppleFileBitmap::Height() const
{
	U32 len = (U32) CGImageGetHeight( fImage );
	return WasScaled() ? (fScale * len) : len;
}

bool
AppleFileBitmap::WasScaled() const
{
	return (fScale > 0.);
}

Real
AppleFileBitmap::GetScale() const
{
	return WasScaled() ? fScale : Rtt_REAL_1;
}

bool
AppleFileBitmap::IsProperty( PropertyMask mask ) const
{
	return IsPropertyInternal( mask );
}

void
AppleFileBitmap::SetProperty( PropertyMask mask, bool newValue )
{
	if ( ! Super::IsPropertyReadOnly( mask ) )
	{
		const U8 p = fProperties;
		const U8 propertyMask = (U8)mask;
		fProperties = ( newValue ? p | propertyMask : p & ~propertyMask );
	}

	switch ( mask )
	{
		case kIsBitsFullResolution:
			fScale = CalculateScale();
			break;
		default:
			break;
	}
}

PlatformBitmap::Format
AppleFileBitmap::GetFormat() const
{
	if ( fIsMask )
	{
		return PlatformBitmap::kMask;
	}

#ifdef Rtt_MAC_ENV
	return PlatformBitmap::kBGRA;
#else
	return PlatformBitmap::kABGR;
#endif
}

// ----------------------------------------------------------------------------

#if defined( Rtt_IPHONE_ENV ) || defined( Rtt_TVOS_ENV )
IPhoneFileBitmap::IPhoneFileBitmap( UIImage *image, bool isMask )
:	Super( image, isMask ),
	fUIImage( [image retain] )
{
}

IPhoneFileBitmap::~IPhoneFileBitmap()
{
	// As a precaution, we autorelease to ensure that the CGImage in Super 
	// is released prior to UIImage releasing ownership of the CGImage.
	[fUIImage autorelease];
}
#endif

#ifdef Rtt_MAC_ENV
MacFileBitmap::MacFileBitmap( NSImage* image, bool isMask )
:	Super(image, isMask)
{
}	
#endif
	
// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------


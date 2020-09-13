//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////


#include "Core/Rtt_Build.h"
#include "librtt/Rtt_RenderingStream.h"
#include "AndroidZipFileEntry.h"
#include "NativeToJavaBridge.h"
#include "Rtt_AndroidBitmap.h"
#include "Display/Rtt_Display.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

AndroidBitmap::AndroidBitmap( Rtt_Allocator & context )
:	fImageData(&context)
{
}

AndroidBitmap::~AndroidBitmap()
{
}

const void * 
AndroidBitmap::GetBits( Rtt_Allocator* context ) const
{
	return (void*)fImageData.GetImageByteBuffer();
}

void 
AndroidBitmap::FreeBits() const
{
	fImageData.DestroyImageByteBuffer();
}

U32 
AndroidBitmap::Width() const
{
	return fImageData.GetWidth();
}

U32 
AndroidBitmap::Height() const
{
	return fImageData.GetHeight();
}

Real
AndroidBitmap::GetScale() const
{
	return fImageData.GetScale();
}

PlatformBitmap::Format 
AndroidBitmap::GetFormat() const
{
	// TODO: This may not be right. Looks like it *might* be kABGR,
	// but the byte buffer may also be reversed so everything cancels out!
	return kRGBA;
}

// ----------------------------------------------------------------------------

static U8
GetInitialPropertiesValue()
{
	return PlatformBitmap::kIsPremultiplied;
}

AndroidAssetBitmap::AndroidAssetBitmap( Rtt_Allocator &context, const char *filePath, NativeToJavaBridge *ntjb )
:	Super( context ),
	fProperties( GetInitialPropertiesValue() ),
	fPath( & context, filePath ),
	fImageDecoder( &context, ntjb )
{
	// Set up the image decoder.
	fImageDecoder.SetTarget(&fImageData);
	fImageDecoder.SetPixelFormatToRGBA();
	fImageDecoder.SetMaxWidth(Rtt::Display::GetMaxTextureSize());
	fImageDecoder.SetMaxHeight(Rtt::Display::GetMaxTextureSize());

	// Fetch the image's information such as pixel width, height, and orientation.
	// The decoder will write this information to the targeted "fImageData" member variable.
	fImageDecoder.SetToDecodeImageInfoOnly();
	fImageDecoder.DecodeFromFile(filePath);

	// Set up the decoder to read the image pixels for when GetBits() gets called.
	fImageDecoder.SetToDecodeAllImageData();
}

AndroidAssetBitmap::~AndroidAssetBitmap()
{
}

const void* 
AndroidAssetBitmap::GetBits( Rtt_Allocator *context ) const
{
	const void *bits = Super::GetBits(context);
	if (!bits)
	{
		fImageDecoder.DecodeFromFile(fPath.GetString());
		bits = Super::GetBits(context);
	}
	return bits;
}

AndroidImageDecoder&
AndroidAssetBitmap::ImageDecoder()
{
	return fImageDecoder;
}

U32
AndroidAssetBitmap::Width() const
{
	return Super::Width();
}

U32
AndroidAssetBitmap::Height() const
{
	return Super::Height();
}

bool
AndroidAssetBitmap::IsProperty( PropertyMask mask ) const
{
	return IsPropertyInternal( mask );
}

void
AndroidAssetBitmap::SetProperty( PropertyMask mask, bool newValue )
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
			break;
		default:
			break;
	}
}

// ----------------------------------------------------------------------------

AndroidMaskAssetBitmap::AndroidMaskAssetBitmap( Rtt_Allocator& context, const char *filePath, NativeToJavaBridge *ntjb )
:	Super( context, filePath, ntjb )
{
	ImageDecoder().SetPixelFormatToGrayscale();
}

PlatformBitmap::Format 
AndroidMaskAssetBitmap::GetFormat() const
{
	return PlatformBitmap::kMask;
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------


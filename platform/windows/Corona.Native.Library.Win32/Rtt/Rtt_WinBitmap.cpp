//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Rtt_WinBitmap.h"
#include "Core\Rtt_Build.h"
#include "Display\Rtt_Display.h"
#include "Interop\RuntimeEnvironment.h"
#include "Rtt_RenderingStream.h"
#include "WinString.h"
#include <windows.h>
#include <algorithm>
using std::min;
using std::max;
#include <gdiplus.h>


namespace Rtt
{

//
// LoadImageFromFileWithoutLocking
//
// Load a bitmap from a file without leaving the file locked until the
// bitmap is released (as happens by default for bitmaps from files).
//
// From StackOverflow: http://stackoverflow.com/questions/4978419
//
static Gdiplus::Bitmap* LoadImageFromFileWithoutLocking(const WCHAR* fileName)
{
	using namespace Gdiplus;
	Bitmap src(fileName);

	if (src.GetLastStatus() != Ok)
	{
		Rtt_TRACE(( "LoadImageFromFileWithoutLocking: failed to create bitmap for '%S'\n", fileName ));
		return 0;
	}

	Bitmap *dst = new Bitmap(src.GetWidth(), src.GetHeight(), PixelFormat32bppARGB);
	BitmapData srcData;
	BitmapData dstData;
	Gdiplus::Rect rc(0, 0, src.GetWidth(), src.GetHeight());

	if (src.LockBits(&rc, ImageLockModeRead, PixelFormat32bppARGB, &srcData) == Ok)
	{
		if (dst->LockBits(&rc, ImageLockModeWrite, PixelFormat32bppARGB, &dstData) == Ok)
		{
			uint8_t * srcBits = (uint8_t *)srcData.Scan0;
			uint8_t * dstBits = (uint8_t *)dstData.Scan0;
			unsigned int stride;

			if (srcData.Stride > 0)
			{
				stride = srcData.Stride;
			}
			else
			{
				stride = -srcData.Stride;
			}
			memcpy(dstBits, srcBits, src.GetHeight() * stride);

			dst->UnlockBits(&dstData);
		}
		src.UnlockBits(&srcData);
	}
	return dst;
}

WinBitmap::WinBitmap() 
	: fData( NULL ), fBitmap( NULL ), fLockedBitmapData( NULL )
{
}

WinBitmap::~WinBitmap()
{
	Self::FreeBits();

	if ( fBitmap ) {
		Rtt_ASSERT( fLockedBitmapData == NULL );
		delete fBitmap;
		fData = NULL;
	}
}

const void * 
WinBitmap::GetBits( Rtt_Allocator* context ) const
{
	const_cast< WinBitmap * >( this )->Lock();

	return fData; 
}

void 
WinBitmap::FreeBits() const
{
	const_cast< WinBitmap * >( this )->Unlock();
}

void
WinBitmap::Lock()
{
	if ( fBitmap == NULL )
		return;

	Gdiplus::Rect rect;

	rect.X = rect.Y = 0;
	rect.Width = fBitmap->GetWidth();
	rect.Height = fBitmap->GetHeight();

	fLockedBitmapData = new Gdiplus::BitmapData;
	Gdiplus::Status status = fBitmap->LockBits(      
		&rect,
		Gdiplus::ImageLockModeRead,
		PixelFormat32bppPARGB,
		fLockedBitmapData
	);

	fData = fLockedBitmapData->Scan0;
}

void
WinBitmap::Unlock()
{
	if ( fBitmap == NULL )
		return;

	fBitmap->UnlockBits( fLockedBitmapData );
	delete fLockedBitmapData;
	fLockedBitmapData = NULL;
	fData = NULL;
}

U32 
WinBitmap::Width() const
{
	if ( fLockedBitmapData != NULL )
		return fLockedBitmapData->Width;
	if ( fBitmap == NULL )
		return 0;
	return fBitmap->GetWidth();
}

U32 
WinBitmap::Height() const
{
	if ( fLockedBitmapData != NULL )
		return fLockedBitmapData->Height;
	if ( fBitmap == NULL )
		return 0;
	return fBitmap->GetHeight();
}


PlatformBitmap::Format 
WinBitmap::GetFormat() const
{
	return kARGB;
}


// ----------------------------------------------------------------------------

static U8
GetInitialPropertiesValue()
{
	return PlatformBitmap::kIsPremultiplied;
}

WinFileBitmap::WinFileBitmap( Rtt_Allocator &context )
#ifdef Rtt_DEBUG
	: fPath(&context)
#endif
{
	InitializeMembers();
}

WinFileBitmap::WinFileBitmap( const char * inPath, Rtt_Allocator &context )
#ifdef Rtt_DEBUG
	: fPath(&context)
#endif
{
	WinString wPath;

	// Initialize all member variables.
	InitializeMembers();

	// Load bitmap from file.
	wPath.SetUTF8( inPath );
#if defined(Rtt_AUTHORING_SIMULATOR)
	// Use a method of loading the bitmap that doesn't lock the underlying file
	Gdiplus::Bitmap *bm = LoadImageFromFileWithoutLocking(wPath.GetTCHAR());
#else
	Gdiplus::Bitmap *bm = Gdiplus::Bitmap::FromFile( wPath.GetTCHAR() );
#endif
	if ( bm != NULL && bm->GetLastStatus() == Gdiplus::Ok )
	{
		fBitmap = bm;
#ifdef Rtt_DEBUG
		fPath.Set( inPath );
#endif
	}
	else
	{
		delete bm;
	}
}

WinFileBitmap::~WinFileBitmap()
{
}

void
WinFileBitmap::InitializeMembers()
{
	fScale = 1.0;
	fProperties = GetInitialPropertiesValue();
	fBitmap = NULL;
	fLockedBitmapData = NULL;
	fData = NULL;
}

float
WinFileBitmap::CalculateScale() const
{
	return 1.0;
}

U32
WinFileBitmap::Width() const
{
	return WinBitmap::Width();
}

U32
WinFileBitmap::Height() const
{
	return WinBitmap::Height();
}

bool
WinFileBitmap::IsProperty( PropertyMask mask ) const
{
	return IsPropertyInternal( mask );
}

void
WinFileBitmap::SetProperty( PropertyMask mask, bool newValue )
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
			fScale = ( newValue ? -1.0f : CalculateScale() );
			break;
		default:
			break;
	}
}


// ----------------------------------------------------------------------------

WinFileGrayscaleBitmap::WinFileGrayscaleBitmap( const char *inPath, Rtt_Allocator &context )
	: WinFileBitmap(context)
{
	Gdiplus::Color sourceColor;
	Gdiplus::Bitmap *sourceBitmap = NULL;
	U8 *bitmapBuffer = NULL;
	WinString wPath;
	int byteCount;
	U32 xIndex;
	U32 yIndex;
	U8 grayscaleColor;

#ifdef Rtt_DEBUG
	// Store the path.
	fPath.Set( inPath );
#endif

	// Fetch the bitmap from file.
	wPath.SetUTF8( inPath );
#if defined(Rtt_AUTHORING_SIMULATOR)
	// Use a method of loading the bitmap that doesn't lock the underlying file
	sourceBitmap = LoadImageFromFileWithoutLocking(wPath.GetTCHAR());
#else
	sourceBitmap = Gdiplus::Bitmap::FromFile(wPath.GetTCHAR());
#endif

	if (sourceBitmap == NULL || sourceBitmap->GetLastStatus() != Gdiplus::Ok)
	{
		delete sourceBitmap;
		return;
	}

	// Store the bitmap's dimensions for fast retrieval.
	fWidth = sourceBitmap->GetWidth();
	fHeight = sourceBitmap->GetHeight();

	// Convert the given bitmap to an 8-bit grayscaled bitmap.
	byteCount = fWidth * fHeight;
	if (byteCount > 0)
	{
		// Calculate the pitch of the image, which is the width of the image padded to the byte packing alignment.
		U32 pitch = fWidth;
		U32 delta = fWidth % kBytePackingAlignment;
		if (delta > 0)
			pitch += kBytePackingAlignment - delta;

		// Create the 8-bit grayscaled bitmap.
		// --------------------------------------------------------------------------------------------------------
		// Microsoft GDI cannot create a grayscaled bitmap that OpenGL needs for masking.
		// GDI can only create 8-bit bitmaps with color palettes. So we have to create the bitmap binary ourselves.
		// --------------------------------------------------------------------------------------------------------
		byteCount = pitch * fHeight;
		bitmapBuffer = new U8[byteCount];
		for (yIndex = 0; yIndex < fHeight; yIndex++)
		{
			for (xIndex = 0; xIndex < pitch; xIndex++)
			{
				if (xIndex < fWidth)
				{
					// Convert the source bitmap color to grayscale.
					sourceBitmap->GetPixel(xIndex, yIndex, &sourceColor);
					grayscaleColor = (U8)(
							(0.30 * sourceColor.GetRed()) +
							(0.59 * sourceColor.GetGreen()) +
							(0.11 * sourceColor.GetBlue()));
				}
				else
				{
					// Fill the padded area of the bitmap (due to the pitch) with the color black.
					// This assumes that the user wants black on the edges. A bitmask is expected to have a
					// black border so that only its center area shows through on screen.
					grayscaleColor = 0;
				}
				bitmapBuffer[xIndex + (pitch * yIndex)] = grayscaleColor;
			}
		}

		// Set the image width to the pitch in case it is larger. Otherwise it will not be rendered correctly.
		// Ideally, you shouldn't do this because it will make the DisplayObject wider than expected by at
		// most 3 pixels (assuming the packing alignment is 4 bytes), but until the DisplayObject can compensate
		// for pitch then this will have to do for now.
		fWidth = pitch;
	}

	// The source bitmap is no longer needed.
	delete sourceBitmap;

	// Store the grayscale bitmap binary.
	// The base class will provide the bits via the inherited member variable "fData".
	fData = (void*)bitmapBuffer;
}

WinFileGrayscaleBitmap::~WinFileGrayscaleBitmap()
{
	if (fData)
	{
		delete fData;
		fData = nullptr;
	}
}

void
WinFileGrayscaleBitmap::FreeBits() const
{
	// Do not delete the grayscale bitmap until this object's destructor has been called.
	// This improves hit-test performance in "Rtt_PlatformBitmap.cpp" which tests a pixel's transparency value.
}

void
WinFileGrayscaleBitmap::Lock()
{
}

void
WinFileGrayscaleBitmap::Unlock()
{
}

U32
WinFileGrayscaleBitmap::Width() const
{
	return fWidth;
}

U32
WinFileGrayscaleBitmap::Height() const
{
	return fHeight;
}

PlatformBitmap::Format 
WinFileGrayscaleBitmap::GetFormat() const
{
	return kMask;
}

} // namespace Rtt

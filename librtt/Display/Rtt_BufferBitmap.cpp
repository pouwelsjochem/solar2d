//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_BufferBitmap.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

// TODO: Does TextureBitmap replace the need for this???
// 
BufferBitmap::BufferBitmap( Rtt_Allocator* allocator, size_t w, size_t h, PlatformBitmap::Format format )
:	fData( Rtt_MALLOC( allocator, w*h*PlatformBitmap::BytesPerPixel( format ) ) ),
	fWidth( (U32) w ),
	fHeight( (U32) h ),
	fProperties( 0 ),
	fFormat( format )
{
	Rtt_ASSERT( fData );
}

BufferBitmap::~BufferBitmap()
{
	ReleaseBits();
}

void
BufferBitmap::ReleaseBits()
{
	if ( fData )
	{
		Rtt_FREE( fData );
		fData = NULL;
	}
}

const void*
BufferBitmap::GetBits( Rtt_Allocator* ) const
{
	return fData;
}

void
BufferBitmap::FreeBits() const
{
	// We don't free bits b/c if we do, we have no way to recover the data
}

U32
BufferBitmap::Width() const
{
	return fWidth;
}

U32
BufferBitmap::Height() const
{
	return fHeight;
}

PlatformBitmap::Format
BufferBitmap::GetFormat() const
{
	return (PlatformBitmap::Format)fFormat;
}

bool
BufferBitmap::IsProperty( PropertyMask mask ) const
{
	return (fProperties & mask) ? true : false;
}

void
BufferBitmap::SetProperty( PropertyMask mask, bool newValue )
{
	const U8 p = fProperties;
	const U8 propertyMask = (U8)mask;
	fProperties = ( newValue ? p | propertyMask : p & ~propertyMask );
}

void
BufferBitmap::UndoPremultipliedAlpha()
{
	// We're assuming 4 bytes (U32) per pixel.
	U32 *p = static_cast< U32 * >( WriteAccess() );

	Rtt_ASSERT( sizeof( *p ) == PlatformBitmap::BytesPerPixel( GetFormat() ) );

	int numPixels = ( Width() * Height() );

	for( int i = 0;
			i < numPixels;
			++i )
	{
		#ifdef Rtt_OPENGLES
			//RGBA
			U8 a = ((U8 *)p)[3];
		#else
			//ARGB
			U8 a = ((U8 *)p)[0];
		#endif

		if ( a > 0 && a < 255 )
		{
			#ifdef Rtt_OPENGLES
				//RGBA
				U8& r = ((U8 *)p)[0];
				U8& g = ((U8 *)p)[1];
				U8& b = ((U8 *)p)[2];
			#else
				//ARGB
				U8& r = ((U8 *)p)[1];
				U8& g = ((U8 *)p)[2];
				U8& b = ((U8 *)p)[3];
			#endif

			float invAlpha = 255.f / (float)a;
			r = invAlpha * r;
			g = invAlpha * g;
			b = invAlpha * b;
		}

		p++;
	}
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

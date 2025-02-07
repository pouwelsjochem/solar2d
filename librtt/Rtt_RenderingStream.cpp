//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////


#include "Core/Rtt_Build.h"

#include "Rtt_RenderingStream.h"

#include "Display/Rtt_PlatformBitmap.h"
#include "Display/Rtt_BufferBitmap.h"
#include "Display/Rtt_Paint.h"
#include "Rtt_PlatformSurface.h"
#include "Display/Rtt_VertexCache.h"
#include "Renderer/Rtt_RenderTypes.h"

#if defined( Rtt_WIN_DESKTOP_ENV )
	#if defined(Rtt_LINUX_ENV)
		#ifdef _WIN32
			#include <GL/glew.h>
		#else
			#include <GL/gl.h>
			#include <GL/glext.h>
		#endif
	#else
	#include "glext.h"
	static PFNGLACTIVETEXTUREPROC glActiveTexture;
	static PFNGLCLIENTACTIVETEXTUREPROC glClientActiveTexture;
	#endif
#endif

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

#define Rtt_PAINT_TRIANGLES( expr ) (expr)

// ----------------------------------------------------------------------------

int
RenderingStream::GetMaxTextureUnits()
{
	GLint maxTextureUnits = 2;
#if defined( Rtt_ANDROID_ENV )
	// For some reason GL_MAX_TEXTURE_IMAGE_UNITS is not defined on Android...
	glGetIntegerv( GL_MAX_TEXTURE_UNITS, & maxTextureUnits );
	Rtt_ASSERT( maxTextureUnits > 1 ); // OpenGL-ES 1.x mandates at least 2
#else
	glGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS, & maxTextureUnits );
	Rtt_ASSERT( maxTextureUnits > 1 ); // OpenGL-ES 1.x mandates at least 2
#endif
	return maxTextureUnits;
}

#ifdef Rtt_REAL_FIXED
	static const GLenum kDataType = GL_FIXED;
#else
	static const GLenum kDataType = GL_FLOAT;
#endif

GLenum
RenderingStream::GetDataType()
{
	return kDataType;
}

RenderingStream::RenderingStream( Rtt_Allocator* pAllocator )
:	fProperties( 0 ),
	fDeviceWidth( -1 ), // uninitialized
	fDeviceHeight( -1 ), // uninitialized
	fContentWidth( -1 ), // uninitialized
	fContentHeight( -1 ), // uninitialized
	fScaledContentWidth( -1 ), // uninitialized
	fScaledContentHeight( -1 ), // uninitialized
	fMinContentWidth( -1 ), // uninitialized
	fMaxContentWidth( -1 ), // uninitialized
	fMinContentHeight( -1 ), // uninitialized
	fMaxContentHeight( -1 ), // uninitialized
	fScreenToContentScale( Rtt_REAL_1 ), // uninitialized
	fContentToScreenScale( 1 ), // uninitialized
	fXScreenOffset( 0 ),
	fYScreenOffset( 0 ),
	fScreenContentBounds(),
	fAllocator( pAllocator )
{
#if defined( Rtt_WIN_DESKTOP_ENV )
	if  ( glActiveTexture == NULL ) {
		glActiveTexture = (PFNGLACTIVETEXTUREPROC) wglGetProcAddress("glActiveTexture");
		glClientActiveTexture = (PFNGLCLIENTACTIVETEXTUREPROC) wglGetProcAddress("glClientActiveTexture");
	}
#endif
}

RenderingStream::~RenderingStream()
{
}

void
RenderingStream::SetContentSizeRestrictions( S32 minContentWidth, S32 maxContentWidth, S32 minContentHeight, S32 maxContentHeight )
{
	fMinContentWidth = minContentWidth;
	fMinContentHeight = minContentHeight;
	fMaxContentWidth = maxContentWidth;
	fMaxContentHeight = maxContentHeight;
}

void
RenderingStream::SetOptimalContentSize( S32 deviceWidth, S32 deviceHeight )
{
	fDeviceWidth = deviceWidth;
	fDeviceHeight = deviceHeight;

	S32 maxContentToScreenScaleForWidth = floor(Rtt_RealDiv(Rtt_IntToReal(fDeviceWidth), Rtt_IntToReal(fMinContentWidth)));
	S32 maxContentToScreenScaleForHeight = floor(Rtt_RealDiv(Rtt_IntToReal(fDeviceHeight), Rtt_IntToReal(fMinContentHeight)));
	S32 maxContentToScreenScale = Min(maxContentToScreenScaleForWidth, maxContentToScreenScaleForHeight);
	for(S32 contentToScreenScale=1; contentToScreenScale <= maxContentToScreenScale; contentToScreenScale++) {
		S32 contentWidth = floor(Rtt_RealDiv(Rtt_IntToReal(fDeviceWidth), Rtt_IntToReal(contentToScreenScale)));
		S32 contentHeight = floor(Rtt_RealDiv(Rtt_IntToReal(fDeviceHeight), Rtt_IntToReal(contentToScreenScale)));
		S32 clampedContentWidth = Min(Max(contentWidth, fMinContentWidth), fMaxContentWidth);
		S32 clampedContentHeight = Min(Max(contentHeight, fMinContentHeight), fMaxContentHeight);

		S32 xOffset = ceil(Rtt_RealDiv2(Rtt_IntToReal(fDeviceWidth - (clampedContentWidth * contentToScreenScale))));
		S32 yOffset = ceil(Rtt_RealDiv2(Rtt_IntToReal(fDeviceHeight - (clampedContentHeight * contentToScreenScale))));

		// We allow offset of 4 pixels as that allows much more devices to be more zoomed out
		if (contentToScreenScale == 1 || ((fXScreenOffset + fYScreenOffset) > 4 && (xOffset + yOffset) < (fXScreenOffset + fYScreenOffset))) {
			fXScreenOffset = xOffset;
			fYScreenOffset = yOffset;
			fContentWidth = clampedContentWidth;
			fContentHeight = clampedContentHeight;
			fContentToScreenScale = contentToScreenScale;
		}
	}

	fScreenToContentScale = Rtt_RealDiv( Rtt_REAL_1, Rtt_IntToReal(fContentToScreenScale) );
	fScaledContentWidth = fContentWidth * fContentToScreenScale;
	fScaledContentHeight = fContentHeight * fContentToScreenScale;

	// The bounds of the screen in content coordinates.
	Rect& bounds = GetScreenContentBounds();
	bounds.xMin = 0;
	bounds.yMin = 0;
	bounds.xMax = fContentWidth;
	bounds.yMax = fContentHeight;
}

GLenum
GPU_GetPixelFormat( PlatformBitmap::Format format )
{
#	ifdef Rtt_OPENGLES

		switch ( format )
		{
			case PlatformBitmap::kRGBA:
				return GL_RGBA;
			case PlatformBitmap::kMask:
				return GL_ALPHA;
			case PlatformBitmap::kRGB:
			case PlatformBitmap::kBGRA:
			case PlatformBitmap::kARGB:
			default:
				Rtt_ASSERT_NOT_IMPLEMENTED();
				return GL_ALPHA;
		}

#	else // Not Rtt_OPENGLES.

		switch ( format )
		{
			case PlatformBitmap::kBGRA:
			case PlatformBitmap::kARGB:
				return GL_BGRA;
			case PlatformBitmap::kRGBA:
				return GL_RGBA;
			case PlatformBitmap::kRGB:
				return GL_BGR;
			case PlatformBitmap::kMask:
				return GL_ALPHA;
			default:
				Rtt_ASSERT_NOT_IMPLEMENTED();
				return GL_ALPHA;
		}

#	endif // Rtt_OPENGLES
}

GLenum
GPU_GetPixelType( PlatformBitmap::Format format )
{
#	ifdef Rtt_OPENGLES

		switch( format )
		{
			case PlatformBitmap::kMask:
				return GL_UNSIGNED_BYTE;
			case PlatformBitmap::kBGRA:
			case PlatformBitmap::kARGB:
			case PlatformBitmap::kRGBA:
			default:
				Rtt_ASSERT_NOT_IMPLEMENTED();
				return GL_UNSIGNED_BYTE;
		}

#	else // Not Rtt_OPENGLES.

		switch( format )
		{
			case PlatformBitmap::kBGRA:
				#ifdef Rtt_BIG_ENDIAN
					return GL_UNSIGNED_INT_8_8_8_8_REV;
				#else
					return GL_UNSIGNED_INT_8_8_8_8;
				#endif
			case PlatformBitmap::kARGB:
				#ifdef Rtt_BIG_ENDIAN
					return GL_UNSIGNED_INT_8_8_8_8;
				#else
					return GL_UNSIGNED_INT_8_8_8_8_REV;
				#endif
			case PlatformBitmap::kRGBA:
				#ifdef Rtt_BIG_ENDIAN
					return GL_UNSIGNED_INT_8_8_8_8_REV;
				#else
					return GL_UNSIGNED_INT_8_8_8_8;
				#endif
			case PlatformBitmap::kMask:
				return GL_UNSIGNED_BYTE;
			default:
				Rtt_ASSERT_NOT_IMPLEMENTED();
				return GL_UNSIGNED_BYTE;
		}

#	endif // Rtt_OPENGLES
}

// Performs a screen capture and outputs the image to the given "outBuffer" bitmap.
void
RenderingStream::CaptureFrameBuffer( BufferBitmap& outBuffer, S32 xScreen, S32 yScreen, S32 wScreen, S32 hScreen )
{
//	GLint x = Rtt_RealToInt( bounds.xMin );
//	GLint y = Rtt_RealToInt( bounds.yMin );
//	GLint w = Rtt_RealToInt( bounds.xMax ) - x;
//	GLint h = Rtt_RealToInt( bounds.yMax ) - y;
#ifdef Rtt_OPENGLES
	const GLenum kFormat = GL_RGBA;
	const GLenum kType = GL_UNSIGNED_BYTE;
#else
	PlatformBitmap::Format format = outBuffer.GetFormat();
	const GLenum kFormat = GPU_GetPixelFormat( format );
	GLenum kType = GPU_GetPixelType( format );
#endif

	glReadPixels( xScreen,
					yScreen,
					wScreen,
					hScreen,
					kFormat,
					kType,
					outBuffer.WriteAccess() );
}

// ----------------------------------------------------------------------------

void
RenderingStream::ContentToScreenUnrounded( float& x, float& y ) const
{
	float w = 0;
	float h = 0;
	ContentToScreenUnrounded(x, y, w, h);
}

void
RenderingStream::ContentToScreenUnrounded( float& x, float& y, float& w, float& h ) const
{
	Rtt_Real xScreen = GetSx();
	x = Rtt_RealDiv(GetXOriginOffset() + x, xScreen);
	w = Rtt_RealDiv(w, xScreen);

	Rtt_Real yScreen = GetSy();
	y = Rtt_RealDiv(GetYOriginOffset() + y, yScreen);
	h = Rtt_RealDiv(h, yScreen);
}

// Converts the given content coordinates to pixel coordinates.
void
RenderingStream::ContentToScreen( S32& x, S32& y ) const
{
	S32 w = 0;
	S32 h = 0;
	ContentToScreen(x, y, w, h);
}

/// Converts the given content coordinates to screen coordinates with origin at top left corner of screen.
void
RenderingStream::ContentToScreen( S32& x, S32& y, S32& w, S32& h ) const
{
	Rtt_Real screenToContentScale = GetScreenToContentScale();
	x = Rtt_RealToInt(Rtt_RealDiv(Rtt_IntToReal(x), screenToContentScale) + GetXScreenOffset() + Rtt_REAL_HALF);
	w = Rtt_RealToInt(Rtt_RealDiv(Rtt_IntToReal(w), screenToContentScale) + Rtt_REAL_HALF);
	y = Rtt_RealToInt(Rtt_RealDiv(Rtt_IntToReal(y), screenToContentScale) + GetYScreenOffset() + Rtt_REAL_HALF);
	h = Rtt_RealToInt(Rtt_RealDiv(Rtt_IntToReal(h), screenToContentScale) + Rtt_REAL_HALF);
}

void
RenderingStream::ContentToScreen( Rtt_Real& x, Rtt_Real& y, Rtt_Real& w, Rtt_Real& h ) const
{
	Rtt_Real screenToContentScale = GetScreenToContentScale();
	x = Rtt_RealToInt(Rtt_RealDiv(GetXScreenOffset() + Rtt_IntToReal(x), screenToContentScale));
	w = Rtt_RealToInt(Rtt_RealDiv(Rtt_RealToFloat(w), screenToContentScale));
	y = Rtt_RealToInt(Rtt_RealDiv(GetYScreenOffset() + Rtt_IntToReal(y), screenToContentScale));
	h = Rtt_RealToInt(Rtt_RealDiv(Rtt_RealToFloat(h), screenToContentScale));

}

void
RenderingStream::ContentToPixels( S32& x, S32& y ) const
{
	S32 w = 0;
	S32 h = 0;
	ContentToPixels(x, y, w, h);
}

/// Converts the given content coordinates to pixel coordinates relative to OpenGL's origin.
void
RenderingStream::ContentToPixels( S32& x, S32& y, S32& w, S32& h ) const
{
	// First convert content coordinates to screen coordinates.
	ContentToScreen(x, y, w, h);	

	// Flip the coordinates relative to OpenGL's origin, which is typically the bottom left corner.
	if (IsProperty(RenderingStream::kFlipHorizontalAxis))
	{
		y = ((S32)fDeviceHeight - y) - h;
	}
}

void
RenderingStream::SetProperty( U32 mask, bool value )
{
	const U8 p = fProperties;
	fProperties = ( value ? p | mask : p & ~mask );
}

// ----------------------------------------------------------------------------
} // namespace Rtt
// ----------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_GPUStream.h"

#include "Display/Rtt_PlatformBitmap.h"
#include "Display/Rtt_BufferBitmap.h"
#include "Display/Rtt_Paint.h"
//#include "Rtt_GPU.h"
#include "Rtt_FillTesselatorStream.h"
#include "Rtt_PlatformSurface.h"
//#include "Rtt_VertexArray.h"
#include "Display/Rtt_VertexCache.h"
#include "Renderer/Rtt_RenderTypes.h"

#if defined( Rtt_WIN_DESKTOP_ENV )
	#if defined(Rtt_EMSCRIPTEN_ENV)
		#include <GL/glew.h>
	#elif defined(Rtt_LINUX_ENV)
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
GPUStream::GetMaxTextureUnits()
{
	GLint maxTextureUnits = 2;
#if defined( Rtt_ANDROID_ENV )
	// For some reason GL_MAX_TEXTURE_IMAGE_UNITS is not defined on Android...
	glGetIntegerv( GL_MAX_TEXTURE_UNITS, & maxTextureUnits );
	Rtt_ASSERT( maxTextureUnits > 1 ); // OpenGL-ES 1.x mandates at least 2
#elif !defined( Rtt_WIN_PHONE_ENV ) && !defined( Rtt_EMSCRIPTEN_ENV )
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

#ifdef Rtt_DEBUG
	static int sTextureStackSize = -1;
#endif

GLenum
GPUStream::GetDataType()
{
	return kDataType;
}

GPUStream::GPUStream( Rtt_Allocator* pAllocator )
:	Super(),
	fCurrentPaint( NULL ),
	fTextureCoords( NULL ),
	fNumTextureCoords( 0 ),
	fRotation( 0 ),
	fIsTexture( false ),
	fAlpha( 0xFF ),
	fAutoRotate( 0 ),
	fTextureStackSize( 0 ),
	fTextureStackDepth( 0 ),
	fTextureStackNumActiveFrames( 0 ),
	fWindowWidth( 0 ),
	fWindowHeight( 0 ),
	fRenderedContentWidth( 0 ),
	fRenderedContentHeight( 0 ),
	fOrientedContentWidth( 0 ),
	fOrientedContentHeight( 0 ),
	fXVertexOffset( Rtt_REAL_0 ),
	fYVertexOffset( Rtt_REAL_0 ),
	fTextureFunction( kTextureFunctionUnknown ),
	fClearR( 0.f ),
	fClearG( 0.f ),
	fClearB( 0.f ),
	fClearA( 1.f ),
	fAllocator( pAllocator )
{
	memset( & fColor, 0xFF, sizeof( fColor ) );

#if defined( Rtt_WIN_DESKTOP_ENV )
	if  ( glActiveTexture == NULL ) {
		glActiveTexture = (PFNGLACTIVETEXTUREPROC) wglGetProcAddress("glActiveTexture");
		glClientActiveTexture = (PFNGLCLIENTACTIVETEXTUREPROC) wglGetProcAddress("glClientActiveTexture");
	}
#endif

	// Init mask pointer values to NULL
	memset( fTextureStack, 0, sizeof( fTextureStack[0] ) * kMaxTextureStackDepth );

#ifdef Rtt_DEBUG
	if ( sTextureStackSize < 0 ) { sTextureStackSize = fTextureStackSize; }
#endif
}

GPUStream::~GPUStream()
{
}

void
GPUStream::Initialize(
				const PlatformSurface& surface,
				DeviceOrientation::Type contentOrientation )
{
	if ( Rtt_VERIFY( ! IsProperty( kInitialized ) ) )
	{
		Super::Initialize( surface, contentOrientation );

		fWindowWidth = surface.Width();
		fWindowHeight = surface.Height();

		// Must start with physical device width and height, not window w/h which
		// could have been zoomed in/out, when calculating the rendered content w/h.
		S32 w = surface.DeviceWidth();
		S32 h = surface.DeviceHeight();

		// Super::Initialize() already swapped GetSx/Sy() based on contentOrientation
		// so we need to swap w,h before applying the scale.
		// if ( contentOrientation != DeviceOrientation::kUpright )

		/// Only swap when the content and surface orientations differ
		//DeviceOrientation::Type surfaceOrientation = surface.GetOrientation();
		//S32 angle = DeviceOrientation::CalculateRotation( contentOrientation, surfaceOrientation );
		
		//#ifdef RTT_SURFACE_ROTATION
		//if ( 90 == angle || -90 == angle )
		{
			// Swap w,h for landscape. The original values are from the physical
			// dimensions, which we assume is portrait.
			if ( DeviceOrientation::IsSideways( contentOrientation ) )
			{
				Swap( w, h );
			}
		}
		//#endif

		Real sx = GetSx();
		if ( ! Rtt_RealIsOne( sx ) )
		{
			w = Rtt_RealToInt( Rtt_RealMul( sx, Rtt_IntToReal( w ) ) );
		}

		Real sy = GetSy();
		if ( ! Rtt_RealIsOne( sy ) )
		{
			h = Rtt_RealToInt( Rtt_RealMul( sy, Rtt_IntToReal( h ) ) );
		}

		fRenderedContentWidth = w;
		fRenderedContentHeight = h;
		fOrientedContentWidth = w;
		fOrientedContentHeight = h;

		// Initialize offsets. We need this init'd before PrepareToRender() calls Reshape()
		// so we can properly offset the status bar in the Corona simulator
		// UpdateOffsets( w, h ); TODO: Jochem: Re-enable?
	}
}

void
GPUStream::PrepareToRender()
{
	if ( Rtt_VERIFY( IsProperty( kInitialized ) ) )
	{
		Reshape( fRenderedContentWidth, fRenderedContentHeight );
	}
}

S32
GPUStream::GetRenderedContentWidth() const
{
	Rtt_ASSERT( fRenderedContentWidth > 0 );
	return fRenderedContentWidth;
}

S32
GPUStream::GetRenderedContentHeight() const
{
	Rtt_ASSERT( fRenderedContentHeight > 0 );
	return fRenderedContentHeight;
}

Rtt_Real
GPUStream::ViewableContentWidth() const
{
	// The viewable content width is the smaller of the rendered content width
	// or the content width itself. The rendered width is the window width 
	// in *scaled* units as determined by UpdateContentScale().
	// 
	// Depending on the relationship of aspect ratios between the window and
	// the content, the rendered width might be larger (kLetterbox)
	// the same (kFillStretch), or smaller (kFillEven).
	S32 renderedContentWidth = GetRenderedContentWidth();
	S32 contentWidth = ContentWidth();
	return Min( contentWidth, renderedContentWidth );
}

Rtt_Real
GPUStream::ViewableContentHeight() const
{
	// See comment in ViewableContentWidth()
	S32 renderedContentHeight = GetRenderedContentHeight();
	S32 contentHeight = ContentHeight();
	return Min( contentHeight, renderedContentHeight );
}

Rtt_Real
GPUStream::ActualContentWidth() const
{
	Rtt_Real result = Rtt_REAL_0;

	switch ( GetScaleMode() )
	{
		// Fills the screen, so it's same as viewable bounds,
		// i.e. the intersection of screen with stretched content
		case Display::kZoomEven:
			result = ViewableContentWidth();
			break;

		// Extra border, so calculate from device width in pixels
		case Display::kLetterbox:
		{
			DeviceOrientation::Type currentOrientation = Super::GetContentOrientation();
			if ( DeviceOrientation::IsSideways( currentOrientation ) )
			{
				result = DeviceHeight() * GetSy();
			}
			else
			{
				result = DeviceWidth() * GetSx();
			}
			break;
		}
		// Stretches content to fill screen so same value
		case Display::kZoomStretch:
		case Display::kAdaptive:
			result = ContentWidth();
			break;

		// Content takes on screen bounds
		case Display::kNone:
		default:
			Rtt_ASSERT( ScreenWidth() == DeviceWidth() );
			result = DeviceWidth();
			break;
	}

	return result;
}

Rtt_Real
GPUStream::ActualContentHeight() const
{
	Rtt_Real result = Rtt_REAL_0;

	switch ( GetScaleMode() )
	{
		// Fills the screen, so it's same as viewable bounds,
		// i.e. the intersection of screen with stretched content
		case Display::kZoomEven:
			result = ViewableContentHeight();
			break;

		// Extra border, so calculate from device width in pixels
		case Display::kLetterbox:
		{
			DeviceOrientation::Type currentOrientation = Super::GetContentOrientation();
			if ( DeviceOrientation::IsSideways( currentOrientation ) )
			{
				result = DeviceWidth() * GetSx();
			}
			else
			{
				result = DeviceHeight() * GetSy();
			}
			break;
		}
		// Stretches content to fill screen so same value
		case Display::kZoomStretch:
		case Display::kAdaptive:
			result = ContentHeight();
			break;

		// Content takes on screen bounds
		case Display::kNone:
		default:
			Rtt_ASSERT( ScreenHeight() == DeviceHeight() );
			result = DeviceHeight();
			break;
	}

	return result;
}

Real
GPUStream::CalculateAlignmentOffset( Alignment alignment, Real contentLen, Real windowLen )
{
	Real result = Rtt_REAL_0;

	switch ( alignment )
	{
		case kAlignmentLeft:
			Rtt_STATIC_ASSERT( kAlignmentTop == kAlignmentLeft );
			result = - Rtt_RealDiv2( windowLen );
			break;
		case kAlignmentCenter:
			result = - Rtt_RealDiv2( contentLen );
			break;
		case kAlignmentRight:
			Rtt_STATIC_ASSERT( kAlignmentBottom == kAlignmentRight );
			result = - Rtt_RealDiv2( Rtt_RealMul2( contentLen ) - windowLen );
			break;
		default:
			Rtt_ASSERT_NOT_REACHED();
			break;
	}

	return result;
}

void
GPUStream::UpdateOffsets( S32 renderedContentWidth, S32 renderedContentHeight )
{
	S32 w = renderedContentWidth;
	S32 h = renderedContentHeight;
	S32 contentW = ContentWidth();
	S32 contentH = ContentHeight();

	Alignment xAlign = GetXAlign();
	Alignment yAlign = GetYAlign();

	// When the launch orientation and current orientation are perpendicular
	// (one is sideways and one is vertical), then swap so that x/yAlign are
	// oriented the same as w/h and contentW/H.
	DeviceOrientation::Type launchOrientation = GetLaunchOrientation();
	DeviceOrientation::Type orientation = Super::GetContentOrientation();
	if ( DeviceOrientation::IsSideways( launchOrientation ) != DeviceOrientation::IsSideways( orientation ) )
	{
		Swap( xAlign, yAlign );
	}

	// Location of origin
	Real xOrigin = - Rtt_RealDiv2( Rtt_IntToReal( w ) );
	Real yOrigin = - Rtt_RealDiv2( Rtt_IntToReal( h ) ); 

	fXVertexOffset = CalculateAlignmentOffset( xAlign, contentW, w );
	fYVertexOffset = CalculateAlignmentOffset( yAlign, contentH, h );

	// Update origin offset
	Real x = fXVertexOffset - xOrigin;
	Real y = fYVertexOffset - yOrigin;

#ifdef RTT_SURFACE_ROTATION
	// For no auto-rotation, the screen origin is pinned to a physical location
	// on the device skin. In this case, if angle is 90 or -90, we need to swap 
	// x,y to get the origin offset relative to that pinned location. For other
	// angles the x,y values happen to be correct. For auto-rotation, the x,y 
	// values are likewise correct.
	DeviceOrientation::Type angleOrientation = DeviceOrientation::OrientationForAngle( fRotation );
	if ( DeviceOrientation::IsSideways( angleOrientation ) )
	{
		Rtt_ASSERT( ! fAutoRotate );
		Swap( x, y );
	}
#endif

	SetXOriginOffset( x );
	SetYOriginOffset( y );

//	Rtt_TRACE( ( "origin offset (x,y) = (%g,%g) [%d]\n", x, y, fRotation) );

#ifdef RTT_SURFACE_ROTATION
	// Ensure proper w,h is used for screen content bounds
	if ( DeviceOrientation::IsSideways( angleOrientation ) )
	{
		Rtt_ASSERT( ! fAutoRotate );
		Swap( w, h );
	}
#endif
	// The bounds of the screen in content coordinates.
	Rect& bounds = GetScreenContentBounds();
	bounds.xMin = -GetXOriginOffset();
	bounds.yMin = -GetYOriginOffset();
	bounds.xMax = w - GetXOriginOffset();
	bounds.yMax = h - GetYOriginOffset();
}

void
GPUStream::Reshape( S32 renderedContentWidth, S32 renderedContentHeight )
{
	UpdateOffsets( renderedContentWidth, renderedContentHeight );
	
	#ifdef RTT_SURFACE_ROTATION
		if ( fRotation )
		{
			Real xOffset = GetXOriginOffset();
			Real yOffset = GetYOriginOffset();

			// For no auto-rotation, the screen origin is pinned to a physical location
			// on the device skin that's not the same as the surface origin. In this case, 
			// if angle is 90 or -90, we need to swap x,y to get the correct screen offset.
			// For other angles and also for auto-rotation, the x,y values happen to be correct.
			DeviceOrientation::Type angleOrientation = DeviceOrientation::OrientationForAngle( fRotation );
			if ( DeviceOrientation::IsSideways( angleOrientation ) )
			{
				Rtt_ASSERT( ! fAutoRotate );
				Swap( xOffset, yOffset );
			}

			Real halfW = Rtt_RealDiv2( Rtt_IntToReal( renderedContentWidth ) ) - xOffset;
			Real halfH = Rtt_RealDiv2( Rtt_IntToReal( renderedContentHeight ) ) - yOffset;

			//Rtt_glTranslate( halfW, halfH, Rtt_REAL_0 );

			// For landscape, swap w,h
			if ( ! DeviceOrientation::IsAngleUpsideDown( fRotation ) )
			{
				Real tmp = halfW;
				halfW = halfH;
				halfH = tmp;
			}

			//glRotatef( Rtt_IntToReal( fRotation ), 0., 0., 1. );
			//Rtt_glTranslate( -halfW, -halfH, Rtt_REAL_0 );
		}
	#endif
}

bool
GPUStream::UpdateContentOrientation( DeviceOrientation::Type newOrientation )
{
	DeviceOrientation::Type current = Super::GetContentOrientation();
	bool result = ( newOrientation != current );

	if ( result )
	{
		// We swap scale factors below so inhibit swapping in call to super method
		Rtt_ASSERT( ! IsProperty( kInhibitSwap ) );
		SetProperty( kInhibitSwap, true );
		Super::SetContentOrientation( newOrientation );
		SetProperty( kInhibitSwap, false );

		S32 w = fRenderedContentWidth;
		S32 h = fRenderedContentHeight;

		// Swap w,h if current is portrait and desired is landscape (or vice versa)
		if ( Super::ShouldSwap( current, newOrientation ) )
		{
			Swap( w, h );
			SwapContentSize();
			SwapContentScale();
			// Don't swap vertex offsets or origin offsets.
			// These are updated via a call to Reshape() which calls UpdateOffsets()
		}

		fRenderedContentWidth = w;
		fRenderedContentHeight = h;
		fOrientedContentWidth = w;
		fOrientedContentHeight = h;
	}

	return result;
}

// Used to support auto-rotate on devices. For simulator, use SetOrientation().
// 
// You should not call this directly. Call Runtime::SetContentOrientation() instead.
// This function updates the content orientation (swapping various values as necessary)
// and then updates the projection matrix/viewing frustum, including rotating if
// the content and surface orientations differ
void
GPUStream::SetContentOrientation( DeviceOrientation::Type newOrientation )
{
	Rtt_ASSERT( newOrientation >= DeviceOrientation::kUnknown && newOrientation <= DeviceOrientation::kNumTypes );

	if ( UpdateContentOrientation( newOrientation ) )
	{
		Reshape( fRenderedContentWidth, fRenderedContentHeight );
	}
}

#ifdef RTT_SURFACE_ROTATION

// Used to support changing Corona view orientation in the simulator, where the window
// remains upright, even though we try to simulate a physically rotated device window.
// 
// When autoRotate is true, that means we want the origin/coordinate system
// of the content to be upright, i.e. the origin is at the top-left of whatever
// orientation the screen is in.
// When it's false, then the content will rotate along with the screen, i.e. the 
// content's origin stays fixed *relative* to the screen as it rotates.
void
GPUStream::SetOrientation( DeviceOrientation::Type newOrientation, bool autoRotate )
{
	bool shouldReshape = UpdateContentOrientation( newOrientation );

	Rtt_ASSERT( DeviceOrientation::IsInterfaceOrientation( newOrientation ) );

	DeviceOrientation::Type oldSurfaceValue = Super::GetSurfaceOrientation();
	if ( newOrientation != oldSurfaceValue )
	{
		shouldReshape = true;
		SetSurfaceOrientation( newOrientation );

		// When mode is not auto-rotate, we want to rotate the model view matrix to preserve
		// the orientation of the content relative to oldSurfaceValue.
		if ( ! autoRotate )
		{
			// Relative to previous orientation
			S32 angle = fRotation - DeviceOrientation::CalculateRotation( oldSurfaceValue, newOrientation );;
			if ( angle > 270 ) { angle -= 360; }
			else if ( angle < -270 ) { angle += 360; }

			fRotation = angle;
		}
		else
		{
			// When we auto-rotate, we don't want any relative orientation angle
			// between content and surface.
			fRotation = 0;
		}

		if ( Super::ShouldSwap( oldSurfaceValue, newOrientation ) )
		{
			Swap( fWindowWidth, fWindowHeight );
		}
	}

	if ( shouldReshape )
	{
		Rtt_ASSERT( ! fAutoRotate );
		fAutoRotate = autoRotate;
		Reshape( fRenderedContentWidth, fRenderedContentHeight );
		fAutoRotate = false;
	}
}

DeviceOrientation::Type
GPUStream::GetRelativeOrientation() const
{
	S8 result = Super::GetRelativeOrientation();

	if ( 0 != fRotation )
	{
		S8 delta = 0;
		switch ( DeviceOrientation::OrientationForAngle( fRotation ) )
		{
			case DeviceOrientation::kSidewaysLeft:
				delta = -1;
				break;
			case DeviceOrientation::kSidewaysRight:
				delta = 1;
				break;
			case DeviceOrientation::kUpsideDown:
				delta = 2;
				break;
			default:
				Rtt_ASSERT_NOT_REACHED();
				break;
		}

		result += delta;
		if ( result > DeviceOrientation::kSidewaysLeft )
		{
			result -= DeviceOrientation::kSidewaysLeft;
		}
		else if ( result < DeviceOrientation::kUpright )
		{
			result += DeviceOrientation::kSidewaysLeft;
		}
	}

	return (DeviceOrientation::Type)result;
}

#endif

#if 0
void
GPUStream::SetBlendMode( RenderTypes::BlendMode mode )
{
	// Compare requested blend mode with current blend mode. Only set when
	// different to minimize gl state changes.
	if ( GetBlendMode() != mode )
	{
		Super::SetBlendMode( mode );

		GLenum sfactor = GL_SRC_ALPHA;
		GLenum dfactor = GL_ONE_MINUS_SRC_ALPHA;

		switch ( mode )
		{
			// Normal: pixel = src*alphaSrc + dst*(1-srcAlpha)
			case RenderTypes::kNormalNonPremultiplied:
				// (R,G,B,A) = (rSrc,gSrc,bSrc,aSrc)*aSrc + (rDst,gDst,bDst,aDst)*(1-aSrc)
				//                                     ^                              ^
				//                                GL_SRC_ALPHA      GL_ONE_MINUS_SRC_ALPHA
				sfactor = GL_SRC_ALPHA;
				dfactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case RenderTypes::kNormalPremultiplied:
				// (R,G,B,A) = (aSrc*[rSrc,gSrc,bSrc],aSrc)*1 + (rDst,gDst,bDst,aDst)*(1-aSrc)
				//                                          ^                             ^
				//                                        GL_ONE        GL_ONE_MINUS_SRC_ALPHA
				sfactor = GL_ONE;
				dfactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			// Additive: pixel = src + dst
			case RenderTypes::kAdditiveNonPremultiplied:
				// (R,G,B,A) = (rSrc,gSrc,bSrc,aSrc)*aSrc + (rDst,gDst,bDst,aDst)*1
				//                                    ^                           ^
				//                               GL_SRC_ALPHA                GL_ONE
				sfactor = GL_SRC_ALPHA;
				dfactor = GL_ONE;
				break;
			case RenderTypes::kAdditivePremultiplied:
				// (R,G,B,A) = (aSrc*[rSrc,gSrc,bSrc],aSrc)*1 + (rDst,gDst,bDst,aDst)*1
				//                                          ^                         ^
				//                                        GL_ONE                 GL_ONE
				sfactor = GL_ONE;
				dfactor = GL_ONE;
				break;
			// Screen: pixel = 1 - (1-src)*(1-dst) = src + dst*(1-src)
			case RenderTypes::kScreenNonPremultiplied:
				// (R,G,B,A) = Src + (1 - Src) * Dst
				// (R,G,B,A) = (rSrc,gSrc,bSrc,aSrc)*aSrc + (rDst,gDst,bDst,aDst)*((1 - Src)*aSrc)
				//                                    ^                           ^^^^^^^^^^^^^^^^
				//                         GL_SRC_ALPHA  GL_ONE_MINUS_SRC_COLOR_TIMES_SRC_ALPHA (?)
				sfactor = GL_SRC_ALPHA;
				dfactor = GL_ONE_MINUS_SRC_COLOR;
				break;
			case RenderTypes::kScreenPremultiplied:
				// (R,G,B,A) = Src + (1 - Src) * Dst
				// (R,G,B,A) = (aSrc*[rSrc,gSrc,bSrc],aSrc)*1 + (rDst,gDst,bDst,aDst)*(1 - aSrc*[rSrc,gSrc,bSrc],aSrc)
				//                                          ^                         ^
				//                                        GL_ONE      GL_ONE_MINUS_SRC_COLOR
				sfactor = GL_ONE;
				dfactor = GL_ONE_MINUS_SRC_COLOR;
				break;
			// Multiply: pixel = src * dst
			case RenderTypes::kMultiplyNonPremultiplied:
				// (R,G,B,A) = {foreground}*(rDst,gDst,bDst,aDst) + (rDst,gDst,bDst,aDst)*(1-aSrc)
				//                                    ^                           ^
				//                               GL_DST_COLOR          GL_ONE_MINUS_SRC_ALPHA
				sfactor = GL_DST_COLOR;
				dfactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case RenderTypes::kMultiplyPremultiplied:
				// (R,G,B,A) = {foreground}*(rDst,gDst,bDst,aDst) + (rDst,gDst,bDst,aDst)*(1-aSrc)
				//                                    ^                           ^
				//                               GL_DST_COLOR          GL_ONE_MINUS_SRC_ALPHA
				sfactor = GL_DST_COLOR;
				dfactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case RenderTypes::kDstAlphaZero:
				sfactor = GL_ZERO;
				dfactor = GL_ZERO;
				break;
			case RenderTypes::kDstAlphaAccumulate:
				sfactor = GL_ONE;
				dfactor = GL_ONE;
				break;
			case RenderTypes::kDstAlphaModulateSrc:
				sfactor = GL_DST_ALPHA;
				dfactor = GL_ZERO;
				break;
			case RenderTypes::kDstAlphaBlit:
				sfactor = GL_DST_ALPHA;
				dfactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			default:
				Rtt_ASSERT_NOT_REACHED();
				break;
		}

		glBlendFunc( sfactor, dfactor );
	}
}
#endif

void
GPUStream::SetClearColor( const Paint& paint )
{
#if 0
	RGBA rgba = paint.GetRGBA();
	GLfloat kInv255 = 1.0f / 255.0f;
	fClearR = kInv255 * rgba.r;
	fClearG = kInv255 * rgba.g;
	fClearB = kInv255 * rgba.b;
	fClearA = kInv255 * rgba.a;
	glClearColor( fClearR, fClearG, fClearB, fClearA );
#endif
}

RGBA
GPUStream::GetClearColor()
{
	RGBA color;
	color.r = (U8)(fClearR * 255.0f);
	color.g = (U8)(fClearG * 255.0f);
	color.b = (U8)(fClearB * 255.0f);
	color.a = (U8)(fClearA * 255.0f);
	return color;
}

void
GPUStream::SetTextureCoordinates( const Vertex2 *coords, S32 numCoords )
{
	fTextureCoords = coords;
	fNumTextureCoords = numCoords;
}

int
GPUStream::GetTextureDepth() const
{
	return GetActiveTextureCount();
}

const BitmapPaint*
GPUStream::GetBitmap() const
{
	Rtt_ASSERT( fCurrentPaint->IsType( Paint::kBitmap ) == fIsTexture );
	const BitmapPaint *result = NULL;
	if ( fIsTexture )
	{
		result = (const BitmapPaint*)fCurrentPaint;
	}
	else if ( GetTextureDepth() > 0 )
	{
		result = fTextureStack[0].paint;
	}

	return result;
}

U8
GPUStream::GetAlpha() const
{
	return fAlpha;
}

U8
GPUStream::SetAlpha( U8 newValue, bool accumuluate )
{
	U8 result = fAlpha;

	if ( accumuluate )
	{
		if ( newValue < 0xFF )
		{
			fAlpha = (((U16)result) * newValue) >> 8;
		}

		// when newValue is 0xFF, complement is 0; in that case, no need to multiply
//		U32 inverse = ~newValue;
//		fAlpha = ( inverse ? (((U16)result) * newValue) >> 8 : newValue );
	}
	else
	{
		fAlpha = newValue;
	}

	return result;
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
GPUStream::CaptureFrameBuffer( BufferBitmap& outBuffer, S32 xScreen, S32 yScreen, S32 wScreen, S32 hScreen )
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

class BitmapContext
{
	Rtt_CLASS_NO_DYNAMIC_ALLOCATION
	Rtt_CLASS_NO_COPIES( BitmapContext )

	public:
		BitmapContext( bool isBitmap, int numActiveTextures, const Vertex2 *texVertices, const Vertex2 *dstVertices );
		~BitmapContext();

	private:
		bool fIsBitmap; // true if fCurrentPaint is a bitmap; false otherwise
		int fNumActiveTextures; // index of current active texture unit
};

// ----------------------------------------------------------------------------

/// Converts the given content coordinates to pixel coordinates relative to OpenGL's origin.
void
GPUStream::ContentToPixels( S32& x, S32& y, S32& w, S32& h ) const
{
	// First convert content coordinates to screen coordinates.
	ContentToScreen(x, y, w, h);
	
	// Rotate screen coordinates to match OpenGL's orientation.
	S32 screenWidth = ScreenWidth();
	S32 screenHeight = ScreenHeight();
	DeviceOrientation::Type orientation = GetRelativeOrientation();
	if (DeviceOrientation::IsSideways(orientation))
	{
		// Swap coordinates if hardware was rotated 90 or 270 degrees.
		Swap(x, y);
		Swap(w, h);
		Swap(screenWidth, screenHeight);
	}
	if (((fRotation - 90) % 180) == 0)
	{
		// Swap coordinates if simulator window was rotated 90 or 270 degrees.
		// This will undo the above swap on a simulator with fixed orientation and sideways hardware.
		Swap(x, y);
		Swap(w, h);
		Swap(screenWidth, screenHeight);
	}
	if ((DeviceOrientation::kUpsideDown == orientation) ||
	    (DeviceOrientation::kSidewaysRight == orientation))
	{
		x = (screenWidth - x) - w;
	}
	if ((DeviceOrientation::kUpsideDown == orientation) ||
	    (DeviceOrientation::kSidewaysLeft == orientation))
	{
		y = (screenHeight - y) - h;
	}
	
	// Next, convert screen coordinates to pixel coordinates.
	// Pixel coordinates will only differ when running on a simulator that is zoomed-in/out.
	Rtt_Real xScale = Rtt_RealDiv(Rtt_IntToReal(fWindowWidth), Rtt_IntToReal(screenWidth));
	Rtt_Real yScale = Rtt_RealDiv(Rtt_IntToReal(fWindowHeight), Rtt_IntToReal(screenHeight));
	x = Rtt_RealToInt(Rtt_RealMul(Rtt_IntToReal(x), xScale) + Rtt_REAL_HALF);
	y = Rtt_RealToInt(Rtt_RealMul(Rtt_IntToReal(y), yScale) + Rtt_REAL_HALF);
	w = Rtt_RealToInt(Rtt_RealMul(Rtt_IntToReal(w), xScale) + Rtt_REAL_HALF);
	h = Rtt_RealToInt(Rtt_RealMul(Rtt_IntToReal(h), yScale) + Rtt_REAL_HALF);

	// Flip the coordinates relative to OpenGL's origin, which is typically the bottom left corner.
	if (IsProperty(RenderingStream::kFlipVerticalAxis))
	{
		x = ((S32)fWindowWidth - x) - w;
	}
	if (IsProperty(RenderingStream::kFlipHorizontalAxis))
	{
		y = ((S32)fWindowHeight - y) - h;
	}
}
// ----------------------------------------------------------------------------
} // namespace Rtt
// ----------------------------------------------------------------------------
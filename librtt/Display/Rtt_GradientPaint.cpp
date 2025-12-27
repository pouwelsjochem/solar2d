//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Display/Rtt_GradientPaint.h"

#include "Display/Rtt_BufferBitmap.h"
#include "Display/Rtt_Display.h"
#include "Display/Rtt_DisplayObject.h"
#include "Display/Rtt_GradientPaintAdapter.h"
#include "Display/Rtt_TextureFactory.h"
#include "Display/Rtt_TextureResource.h"
#include "Core/Rtt_Math.h"
#include "Core/Rtt_Real.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

GradientPaint::Direction
GradientPaint::StringToDirection( const char *str )
{
	Direction result = kDefaultDirection;

	if ( str )
	{
		if ( 0 == strcmp( str, "up" ) )
		{
			result = kUpDirection;
		}
		else if ( 0 == strcmp( str, "left" ) )
		{
			result = kLeftDirection;
		}
		else if ( 0 == strcmp( str, "right" ) )
		{
			result = kRightDirection;
		}
	}

	return result;
}

GradientPaint::Mode
GradientPaint::StringToMode( const char *str )
{
	Mode result = kDefaultMode;

	if ( str )
	{
		if ( 0 == strcmp( str, "smooth" ) )
		{
			result = kSmoothMode;
		}
	}

	return result;
}

// ----------------------------------------------------------------------------

namespace { // anonymous
	enum {
		kBufferWidth = 1,
		kBufferHeight = 32,
		kEnd = 0,
		kStart = kBufferHeight - 1
	};
}

// Create an 1x2 bitmap
struct PremultipliedColor
{
	float r;
	float g;
	float b;
	float a;
};

static PremultipliedColor
ToPremultiplied( Color color )
{
	ColorUnion c;
	c.pixel = color;
	float a = c.rgba.a;
	float alpha = a / 255.f;
	PremultipliedColor result;
	result.r = c.rgba.r * alpha;
	result.g = c.rgba.g * alpha;
	result.b = c.rgba.b * alpha;
	result.a = a;
	return result;
}

static Color
ToColor( const PremultipliedColor& color )
{
	ColorUnion c;
	c.rgba.r = color.r;
	c.rgba.g = color.g;
	c.rgba.b = color.b;
	c.rgba.a = color.a;
	return c.pixel;
}

static BufferBitmap *
NewBufferBitmap(
	Rtt_Allocator *pAllocator,
	Color start,
	Color end,
	Rtt_Real colorMidPoint,
	GradientPaint::Direction direction,
	GradientPaint::Mode mode )
{
	const PlatformBitmap::Format kFormat = PlatformBitmap::kRGBA;

	BufferBitmap *result =
		Rtt_NEW( pAllocator, BufferBitmap( pAllocator, kBufferWidth, kBufferHeight, kFormat ) );
	result->SetProperty( PlatformBitmap::kIsPremultiplied, true );

	Color *pixels = (Color *)result->WriteAccess();

	PremultipliedColor color0 = ToPremultiplied( start );
	PremultipliedColor color1 = ToPremultiplied( end );
	ColorUnion startStraight;
	ColorUnion endStraight;
	startStraight.pixel = start;
	endStraight.pixel = end;

	float midPoint = Clamp( colorMidPoint, Rtt_REAL_0, Rtt_REAL_1 );
	bool hasMidPoint = ( midPoint > 0.f && midPoint < 1.f );

	// Rtt_TRACE( ( "(r,g,b,a) = (%g, %g, %g, %g)\n", r1, g1, b1, a1 ) );

	// Interpolate across the gradient, biasing the midpoint if requested.
	for ( int i = 0, iMax = (int)kBufferHeight - 1; i <= iMax; i++ )
	{
		float t = ((float)i) / iMax;
		float s = 1.f - t; // 0 at start, 1 at end
		float factor = s;
		if ( hasMidPoint )
		{
			if ( s <= midPoint )
			{
				factor = 0.5f * ( s / midPoint );
			}
			else
			{
				factor = 0.5f + 0.5f * ( ( s - midPoint ) / ( 1.f - midPoint ) );
			}
		}
        
		bool isSmoothMode = ( GradientPaint::kSmoothMode == mode );
		if ( isSmoothMode )
		{
			float eased = powf( t, 1.18f ) * 0.9f;
			eased = Clamp( eased, 0.f, 1.f );
			float alpha = 1.f - eased;
			alpha = alpha * alpha; // -- quadratic ease-out
			factor = alpha;
		}
        
		float invFactor = 1.f - factor;
		PremultipliedColor blended;
        
		if ( isSmoothMode )
		{
			float a = startStraight.rgba.a * invFactor + endStraight.rgba.a * factor;
			float r = startStraight.rgba.r * invFactor + endStraight.rgba.r * factor;
			float g = startStraight.rgba.g * invFactor + endStraight.rgba.g * factor;
			float b = startStraight.rgba.b * invFactor + endStraight.rgba.b * factor;
			float premulScale = a / 255.f;
			blended.r = r * premulScale;
			blended.g = g * premulScale;
			blended.b = b * premulScale;
			blended.a = a;
		}
		else
		{
			blended.r = color0.r * invFactor + color1.r * factor;
			blended.g = color0.g * invFactor + color1.g * factor;
			blended.b = color0.b * invFactor + color1.b * factor;
			blended.a = color0.a * invFactor + color1.a * factor;
		}

		pixels[i] = ToColor( blended );
		// Rtt_TRACE( ( "[%d] (r,g,b,a) = (%g, %g, %g, %g)\n", i, r, g, b, a ) );
	}

	// Rtt_TRACE( ( "(r,g,b,a) = (%g, %g, %g, %g)\n", r0, g0, b0, a0 ) );

	const Real kScale = ((float)(kBufferHeight - 1)) / kBufferHeight; // 0.5;
	result->SetNormalizationScaleY( kScale );

	return result;
}

// ----------------------------------------------------------------------------

GradientPaint *
GradientPaint::New( TextureFactory& factory, Color start, Color end, Direction direction, Rtt_Real angle, Mode mode )
{
	Rtt_Allocator *allocator = factory.GetDisplay().GetAllocator();
	BufferBitmap *bitmap = NewBufferBitmap( allocator, start, end, Rtt_REAL_HALF, direction, mode );

	SharedPtr< TextureResource > resource = factory.FindOrCreate( bitmap, true );
	Rtt_ASSERT( resource.NotNull() );

	GradientPaint *result = Rtt_NEW( allocator, GradientPaint( resource, angle ) );

	return result;
}

GradientPaint *
GradientPaint::New( TextureFactory& factory, Color start, Color end, Rtt_Real colorMidPoint, Direction direction, Rtt_Real angle, Mode mode )
{
	Rtt_Allocator *allocator = factory.GetDisplay().GetAllocator();
	BufferBitmap *bitmap = NewBufferBitmap( allocator, start, end, colorMidPoint, direction, mode );

	SharedPtr< TextureResource > resource = factory.FindOrCreate( bitmap, true );
	Rtt_ASSERT( resource.NotNull() );

	GradientPaint *result = Rtt_NEW( allocator, GradientPaint( resource, angle ) );

	return result;
}

// ----------------------------------------------------------------------------

GradientPaint::GradientPaint( const SharedPtr< TextureResource >& resource, Rtt_Real angle )
:	Super( resource )
{
	if(!Rtt_RealIsZero(angle))
	{
		Transform & t = GetTransform();
		t.SetProperty(kRotation, angle);
	}
	Initialize( kGradient );
	
	Invalidate( Paint::kTextureTransformFlag );
}

const Paint*
GradientPaint::AsPaint( Super::Type type ) const
{
	return ( kGradient == type || kBitmap == type ? this : NULL );
}

const MLuaUserdataAdapter&
GradientPaint::GetAdapter() const
{
	return GradientPaintAdapter::Constant();
}

Color
GradientPaint::GetStart() const
{
	BufferBitmap *bufferBitmap = static_cast< BufferBitmap * >( GetBitmap() );

	const Color *pixels = (const Color *)bufferBitmap->ReadAccess();

	return pixels[kStart];
}

Color
GradientPaint::GetEnd() const
{
	BufferBitmap *bufferBitmap = static_cast< BufferBitmap * >( GetBitmap() );

	const Color *pixels = (const Color *)bufferBitmap->ReadAccess();

	return pixels[kEnd];
}

void
GradientPaint::SetStart( Color color )
{
	BufferBitmap *bufferBitmap = static_cast< BufferBitmap * >( GetBitmap() );

	Color *pixels = (Color *)bufferBitmap->WriteAccess();

	pixels[kStart] = color;

	GetTexture()->Invalidate(); // Force Renderer to update GPU texture

	GetObserver()->InvalidateDisplay(); // Force reblit
}

void
GradientPaint::SetEnd( Color color )
{
	BufferBitmap *bufferBitmap = static_cast< BufferBitmap * >( GetBitmap() );

	Color *pixels = (Color *)bufferBitmap->WriteAccess();

	pixels[kEnd] = color;

	GetTexture()->Invalidate(); // Force Renderer to update GPU texture

	GetObserver()->InvalidateDisplay(); // Force reblit
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

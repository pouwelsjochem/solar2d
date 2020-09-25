//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Display/Rtt_Tesselator.h"

#include "Rtt_Matrix.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

// Unit circle
const Vertex2
Tesselator::kUnitCircleVertices[] =
{
	{ Rtt_REAL_0, Rtt_REAL_0 },
	{ Rtt_REAL_1, Rtt_REAL_0 },
	{ Rtt_REAL_0, Rtt_REAL_1 },
	{ Rtt_REAL_NEG_1, Rtt_REAL_0 },
	{ Rtt_REAL_0, Rtt_REAL_NEG_1 },
	{ Rtt_REAL_1, Rtt_REAL_0 }
};

// ----------------------------------------------------------------------------

Tesselator::Tesselator()
:	fMaxSubdivideDepth( 0 )
{
}

Tesselator::~Tesselator()
{
}

void
Tesselator::MoveTo( ArrayVertex2& vertices, const Vertex2& p )
{
	Rtt_ASSERT_NOT_IMPLEMENTED();
}

void
Tesselator::ApplyTransform( ArrayVertex2& vertices, const Vertex2& origin )
{
	if ( ! Rtt_RealIsZero( origin.x ) || ! Rtt_RealIsZero( origin.y ) )
	{
		Vertex2_Translate( vertices.WriteAccess(), vertices.Length(), origin.x, origin.y );
	}
}

static const Rtt_Real kUnitCircleScaleFactor[] =
{
#ifdef Rtt_REAL_FIXED
// TODO: will need better precision than this...
	0x16a09,
	0x1d906,
	0x1f629,
	0x1fd88,
	0x1ff62,
	0x1ffd8,
	0x1fff6,
	0x1fffd,
	0x1ffff,
	0x1ffff,
	0x1ffff,
	0x1ffff,
	0x20000,
	0x20000
#else
	1.414213562373095f,
	1.847759065022573f,
	1.961570560806461f,
	1.990369453344394f,
	1.997590912410345f,
	1.999397637392408f,
	1.999849403678289f,
	1.999962350565202f,
	1.999990587619152f,
	1.999997646903404f,
	1.999999411725764f,
	1.999999852931436f,
	1.999999963232858f,
	1.999999990808215f
#endif
};

// Subdivide circular sector --- assumes unit circle centered at (0,0)
// http://mathworld.wolfram.com/CircularSector.html
//
// Tesselation is a triangle fan of points along the circumference,
// but it's ordered as a triangle strip. This is why after each point along
// the circumference, the origin is appended to create degenerate tri's.
// This makes it easy to use in rounded rects.
//
// Use depth -1 to insert p1 at the first recursion. This will recurse to the
// same depth as passing depth = 0.
void
Tesselator::SubdivideCircleSector(
	ArrayVertex2& vertices, const Vertex2& p1, const Vertex2& p2, int depth )
{
	const Vertex2 kOrigin = { Rtt_REAL_0, Rtt_REAL_0 };

	// TODO: PERFORMANCE
	// Precalculate vertices on unit circle and group by depth.
	// These calculations are always the same, so we should just append them to 
	// the vertex array
	const int kSubdivideDepth = fMaxSubdivideDepth;

	const Rtt_Real x1 = p1.x;
	const Rtt_Real y1 = p1.y;
	const Rtt_Real x2 = p2.x;
	const Rtt_Real y2 = p2.y;

	const Rtt_Real d0 = kUnitCircleScaleFactor[depth];
	const Rtt_Real xm0 = Rtt_RealDiv( (x1+x2), d0 );
	const Rtt_Real ym0 = Rtt_RealDiv( (y1+y2), d0 );

	const Vertex2 m0 = { xm0, ym0 };

	++depth;
	bool shouldSubdivideAgain = ( depth < kSubdivideDepth );

	if ( shouldSubdivideAgain )
	{
		SubdivideCircleSector( vertices, p1, m0, depth );
	}

	vertices.Append( m0 );
	vertices.Append( kOrigin );

	if ( shouldSubdivideAgain )
	{
		SubdivideCircleSector( vertices, m0, p2, depth );
	}

	vertices.Append( p2 );
	vertices.Append( kOrigin );
}

/// Gets the log2 of the given value.
/// Note: This function was taken from internal Lua function luaO_log2(), source file "lobject.c",
///       which we cannot access if Lua is provided in library form.
static int
GetLog2(unsigned int x)
{
	static const unsigned char log_2[256] = {
		0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
	};
	int l = -1;
	while (x >= 256) { l += 8; x >>= 8; }
	return l + log_2[x];
}

static int
DepthForRadius( Real radius )
{
	Rtt_ASSERT( radius > Rtt_REAL_0 );

	int r = Rtt_RealToInt( radius );

	// Quarter arc is pi*r/2 < r
	// And each depth halves the segment length, so at a depth of log2( r ),
	// the segment length is about a pixel
	int result = GetLog2( r );

	// For large circles, reduce subdivision depth
	if ( radius > 7 )
	{
		--result;
	}

	if ( result <= 0 ) { result = 1; }

	// Max radius of 256 should satisfy most screen sizes, for now...
	if ( result > 8 ) { result = 8; }

	return result;
}

void
Tesselator::AppendCircle( ArrayVertex2& vertices, Real radius, U32 options )
{
	// TODO: Remove the assumption that fVertices is empty
	Rtt_ASSERT( vertices.Length() == 0 );

	// By default, SubdivideCircleSector() does not add the first point,
	// so we add this in manually.
	vertices.Append( kUnitCircleVertices[1] ); // (-1,0)
	vertices.Append( kUnitCircleVertices[0] ); //  (0,0)

	fMaxSubdivideDepth = DepthForRadius( radius );
	SubdivideCircleSector( vertices, kUnitCircleVertices[1], kUnitCircleVertices[2], 0 );
	SubdivideCircleSector( vertices, kUnitCircleVertices[2], kUnitCircleVertices[3], 0 );
	SubdivideCircleSector( vertices, kUnitCircleVertices[3], kUnitCircleVertices[4], 0 );
	SubdivideCircleSector( vertices, kUnitCircleVertices[4], kUnitCircleVertices[5], 0 );
	fMaxSubdivideDepth = 0;

	if ( ! (options&kNoScale) )
	{
		Vertex2_Scale( vertices.WriteAccess(), vertices.Length(), radius, radius );
	}
}

void
Tesselator::AppendRect( ArrayVertex2& vertices, Real halfW, Real halfH )
{
	// Append in tri-strip order
	const Vertex2 bottomLeft =	{ -halfW, -halfH };
	vertices.Append( bottomLeft );

	const Vertex2 upperLeft =	{ -halfW, halfH };
	vertices.Append( upperLeft );

	const Vertex2 bottomRight =	{ halfW, -halfH };
	vertices.Append( bottomRight );

	const Vertex2 upperRight =	{ halfW, halfH };
	vertices.Append( upperRight );
}

void
Tesselator::MoveCenterToOrigin( ArrayVertex2& vertices, Vertex2 currentCenter )
{
	for ( int i = 0, iMax = vertices.Length(); i < iMax; i++ )
	{
		Vertex2& v = vertices[i];
		v.x -= currentCenter.x;
		v.y -= currentCenter.y;
	}
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------


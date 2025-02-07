//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Display/Rtt_RectPath.h"

//#include "Display/Rtt_BitmapPaint.h"
//#include "Display/Rtt_DisplayObject.h"
#include "Display/Rtt_ShapeAdapterRect.h"
#include "Display/Rtt_TesselatorRect.h"
#include "Renderer/Rtt_Geometry_Renderer.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

RectPath *
RectPath::NewRect( Rtt_Allocator *pAllocator, Real width, Real height )
{
	TesselatorRect *tesselator = Rtt_NEW( pAllocator, TesselatorRect( width, height ) );
	RectPath *result = Rtt_NEW( pAllocator, RectPath( pAllocator, tesselator ) );
	result->SetAdapter( & ShapeAdapterRect::Constant() );
	result->SetDelegate( result );
	return result;
}

// ----------------------------------------------------------------------------

// NOT USED: static const S32 kRectPathNumVertices = sizeof(Quad) / sizeof(Vertex2);

RectPath::RectPath( Rtt_Allocator* pAllocator, TesselatorShape *tesselator )
:	Super( pAllocator, tesselator )
{
	fFillGeometry->Resize( 4, false );
	SetProperty( kIsRectPath, true );
}

void
RectPath::Update( RenderData& data, const Matrix& srcToDstSpace )
{
	TesselatorRect *tesselator = (TesselatorRect *)GetTesselator();
	if ( ! tesselator->HasOffset() )
	{
		Super::Update( data, srcToDstSpace );
	}
	else
	{
		UpdateFill( data, srcToDstSpace );
	}
}

void
RectPath::UpdateGeometry(
	Geometry& dst, const VertexCache& src, const Matrix& srcToDstSpace, U32 flags ) const
{
	if ( 0 == flags ) { return; }

	const ArrayVertex2& vertices = src.Vertices();
	const ArrayVertex2& texVertices = src.TexVertices();
	U32 numVertices = vertices.Length();

	if ( dst.GetVerticesAllocated() < numVertices )
	{
		dst.Resize( numVertices, false );
	}
	Geometry::Vertex *dstVertices = dst.GetVertexData();

	bool updateVertices = ( flags & kVerticesMask );
	bool updateTexture = ( flags & kTexVerticesMask );

	Rtt_ASSERT( ! updateTexture || ( vertices.Length() == texVertices.Length() ) );

	const TesselatorRect *tesselator = NULL;
	bool hasOffset = false;
	if ( updateTexture )
	{
		tesselator = (const TesselatorRect *)GetTesselator();
		hasOffset = tesselator->HasOffset();
	}

    const ArrayFloat * floatArray = src.ExtraFloatArray( ZKey() );
    Rtt_ASSERT( ! floatArray || ( floatArray->Length() == vertices.Length() ) );
    const float zero = 0.f, * zsource = floatArray ? floatArray->ReadAccess() : &zero;
    size_t step = floatArray ? 1U : 0U;

    for ( U32 i = 0, iMax = vertices.Length(); i < iMax; i++, zsource += step )
	{
		Rtt_ASSERT( i < dst.GetVerticesAllocated() );

		Geometry::Vertex& dst = dstVertices[i];

		if ( updateVertices )
		{
			Vertex2 v = vertices[i];
			srcToDstSpace.Apply( v );

			dst.x = v.x;
			dst.y = v.y;
            dst.z = *zsource;
		}

		if ( updateTexture )
		{
			if ( ! hasOffset )
			{
				dst.u = texVertices[i].x;
				dst.v = texVertices[i].y;
				dst.q = 1.f;
			}
			else
			{
				Real c = tesselator->GetCoefficient( i );
				dst.u = c * texVertices[i].x;
				dst.v = c * texVertices[i].y;
				dst.q = c;
			}
		}
	}

	dst.SetVerticesUsed( numVertices );
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------


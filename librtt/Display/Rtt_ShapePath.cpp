//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Display/Rtt_ShapePath.h"

#include "Display/Rtt_BitmapPaint.h"
#include "Display/Rtt_DisplayObject.h"
#include "Display/Rtt_Paint.h"
#include "Display/Rtt_ShapeAdapterCircle.h"
#include "Display/Rtt_ShapeAdapterPolygon.h"
#include "Display/Rtt_ShapeAdapterRoundedRect.h"
#include "Display/Rtt_ShapeAdapterMesh.h"
#include "Display/Rtt_TesselatorCircle.h"
#include "Display/Rtt_TesselatorPolygon.h"
#include "Display/Rtt_TesselatorRoundedRect.h"
#include "Display/Rtt_TesselatorShape.h"
#include "Renderer/Rtt_Geometry_Renderer.h"
#include "Renderer/Rtt_Renderer.h"

#include "Rtt_LuaAux.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

ShapePath *
ShapePath::NewRoundedRect( Rtt_Allocator *pAllocator, Real width, Real height, Real radius )
{
	TesselatorRoundedRect *tesselator = Rtt_NEW( pAllocator, TesselatorRoundedRect( width, height, radius ) );
	ShapePath *result = Rtt_NEW( pAllocator, ShapePath( pAllocator, tesselator ) );
	result->SetAdapter( & ShapeAdapterRoundedRect::Constant() );
	return result;
}

ShapePath *
ShapePath::NewCircle( Rtt_Allocator *pAllocator, Real radius )
{
	TesselatorCircle *tesselator = Rtt_NEW( pAllocator, TesselatorCircle( radius ) );
	ShapePath *result = Rtt_NEW( pAllocator, ShapePath( pAllocator, tesselator ) );
	result->SetAdapter( & ShapeAdapterCircle::Constant() );
	return result;
}

ShapePath *
ShapePath::NewPolygon( Rtt_Allocator *pAllocator )
{
	TesselatorPolygon *tesselator = Rtt_NEW( pAllocator, TesselatorPolygon( pAllocator ) );
	ShapePath *result = Rtt_NEW( pAllocator, ShapePath( pAllocator, tesselator ) );
	result->SetAdapter( & ShapeAdapterPolygon::Constant() );
	return result;
}
	
	
ShapePath *
ShapePath::NewMesh( Rtt_Allocator *pAllocator, Geometry::PrimitiveType meshType )
{
	TesselatorMesh *tesselator = Rtt_NEW( pAllocator, TesselatorMesh( pAllocator, meshType ) );
	ShapePath *result = Rtt_NEW( pAllocator, ShapePath( pAllocator, tesselator ) );
	result->SetAdapter( & ShapeAdapterMesh::Constant() );
	return result;
}

// ----------------------------------------------------------------------------

ShapePath::ShapePath( Rtt_Allocator *pAllocator, TesselatorShape *tesselator )
:	Super( pAllocator ),
	fFillGeometry( Rtt_NEW( pAllocator, Geometry( pAllocator, tesselator->GetFillPrimitive(), 0, 0, tesselator->GetFillPrimitive() == Geometry::kIndexedTriangles ) ) ),
	fFillSource( pAllocator ),
	fIndexSource( pAllocator ),
	fTesselator( tesselator ),
	fDelegate( NULL )
{
	Rtt_ASSERT( fTesselator );
}

ShapePath::~ShapePath()
{
	DisplayObject *observer = GetObserver();
	if ( observer )
	{
		observer->QueueRelease( fFillGeometry );
	}

	Rtt_DELETE( fTesselator );
}

void
ShapePath::CalculateUV( ArrayVertex2& texVertices, Paint *paint, bool canTransformTexture )
{
	Transform t;

	if ( canTransformTexture
			|| ! paint->IsValid( Paint::kTextureTransformFlag ) )
	{
		paint->SetValid( Paint::kTextureTransformFlag );
		paint->UpdateTransform( t );

		const PlatformBitmap *bitmap = paint->GetBitmap();
		if ( bitmap )
		{
			fTesselator->SetNormalizationScaleX( bitmap->GetNormalizationScaleX() );
			fTesselator->SetNormalizationScaleY( bitmap->GetNormalizationScaleY() );
		}
	}

	texVertices.Clear();
	fTesselator->GenerateFillTexture( texVertices, t );
	paint->ApplyPaintUVTransformations( texVertices );
}

void
ShapePath::TesselateFill()
{
	Rtt_ASSERT( HasFill() );

	Paint *paint = GetFill();

	bool canTransformTexture = paint->CanTransform();
	
	if ( ! IsValid( kFillSource ) )
	{
		fFillSource.Vertices().Clear();
		fTesselator->GenerateFill( fFillSource.Vertices() );
		SetValid( kFillSource );

		if ( canTransformTexture )
		{
			Invalidate( kFillSourceTexture );
		}

		// Force renderdata update
		Invalidate( kFill );

		// Force per-vertex color data update
		GetObserver()->Invalidate( DisplayObject::kColorFlag );
	}
	
	if ( !IsValid(kFillSourceIndices) )
	{
		fIndexSource.Clear();
		fTesselator->GenerateFillIndices(fIndexSource);
		SetValid( kFillSourceIndices );
		
		Invalidate( kFillIndices );
	}

	if ( ! IsValid( kFillSourceTexture ) )
	{
		CalculateUV( fFillSource.TexVertices(), paint, canTransformTexture );

		SetValid( kFillSourceTexture );

		// Force renderdata update
		Invalidate( kFillTexture );

		Rtt_ASSERT( fFillSource.Vertices().Length() == fFillSource.TexVertices().Length() );
	}
}

void
ShapePath::UpdateFill( RenderData& data, const Matrix& srcToDstSpace )
{
	if ( HasFill() )
	{
		TesselateFill();

		// The flags here are for a common helper (UpdateGeometry)
		// which is agnostic to fill, so we have to map
		// the fill-specific flags to generic flags (e.g. kVerticesMask)
		U32 flags = 0;
		if ( ! IsValid( kFill ) )
		{
			flags |= kVerticesMask;
		}
		if ( ! IsValid( kFillTexture ) )
		{
			flags |= kTexVerticesMask;
		}
		if ( ! IsValid( kFillIndices ) )
		{
			flags |= kIndicesMask;
		}

		if ( ! fDelegate )
		{
			UpdateGeometry( *fFillGeometry, fFillSource, srcToDstSpace, flags, &fIndexSource );
		}
		else
		{
			fDelegate->UpdateGeometry( * fFillGeometry, fFillSource, srcToDstSpace, flags );
		}
		data.fGeometry = fFillGeometry;

		SetValid( kFill | kFillTexture | kFillIndices );
	}
}

void
ShapePath::Update( RenderData& data, const Matrix& srcToDstSpace )
{
	Super::Update( data, srcToDstSpace );

	UpdateFill( data, srcToDstSpace );
}

void
ShapePath::UpdateResources( Renderer& renderer ) const
{
	if ( HasFill() && IsFillVisible() && fFillGeometry->GetStoredOnGPU() )
	{
		renderer.QueueUpdate( fFillGeometry );
	}
}

void
ShapePath::Translate( Real dx, Real dy )
{
	Super::Translate( dx, dy );

	Geometry::Vertex *fillVertices = fFillGeometry->GetVertexData();
	for ( int i = 0, iMax = fFillGeometry->GetVerticesUsed(); i < iMax; i++ )
	{
		Geometry::Vertex& v = fillVertices[i];
		v.x += dx;
		v.y += dy;
	}
}

void
ShapePath::GetSelfBounds( Rect& rect ) const
{
	fTesselator->GetSelfBounds( rect );
}

bool
ShapePath::SetSelfBounds( Real width, Real height )
{
	bool result = fTesselator->SetSelfBounds( width, height );

	if ( result )
	{
		Invalidate( kFillSource );
	}

	return result;
}

void
ShapePath::GetTextureVertices( ArrayVertex2& texVertices )
{
	Rtt_ASSERT( HasFill() );

	Paint *paint = GetFill();

	CalculateUV( texVertices, paint, paint->CanTransform() );
}

Rect
ShapePath::GetTextureExtents( const ArrayVertex2& texVertices ) const
{
	Rect extents;

	for (S32 i = 0, iMax = texVertices.Length(); i < iMax; ++i)
	{
		extents.Union(texVertices[i]);
	}

	return extents;
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------


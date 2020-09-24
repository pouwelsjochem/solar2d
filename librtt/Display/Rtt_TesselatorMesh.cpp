//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Display/Rtt_TesselatorMesh.h"

#include "Rtt_Matrix.h"
#include "Rtt_Transform.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

TesselatorMesh::TesselatorMesh( Rtt_Allocator *allocator, Geometry::PrimitiveType meshType )
:	Super()
,	fUVs( allocator )
,	fMesh( allocator )
,	fIndices( allocator )
,	fSelfBounds()
,	fIsMeshValid( false )
,	fMeshType(meshType)
,   fVertexOffset(kVertexOrigin)
{
}

void
TesselatorMesh::GenerateFill( ArrayVertex2& vertices )
{
	Update();
	for (int i=0; i<fMesh.Length(); i++) {
		vertices.Append(fMesh[i]);
	}
}
	
void
TesselatorMesh::GenerateFillIndices( ArrayIndex& indicies )
{
	for (int i=0; i<fIndices.Length(); i++) {
		indicies.Append(fIndices[i]);
	}
}
	
void
TesselatorMesh::GenerateFillTexture( ArrayVertex2& texCoords, const Transform& t )
{
	Update();
	for (int i=0; i<fUVs.Length(); i++) {
		texCoords.Append(fUVs[i]);
	}

	if( !t.IsIdentity() )
	{
		Matrix m;
		m.Translate( -Rtt_REAL_HALF, -Rtt_REAL_HALF );
		m.Scale( t.GetSx(), t.GetSy() );
		m.Rotate( -t.GetRotation() );
		m.Translate( t.GetX(), t.GetY() );
		m.Translate( Rtt_REAL_HALF, Rtt_REAL_HALF );
		m.Apply(texCoords.WriteAccess(), texCoords.Length());
	}
}

void
TesselatorMesh::GetSelfBounds( Rect& rect )
{
	Update();
	rect = fSelfBounds;
}

Geometry::PrimitiveType
TesselatorMesh::GetFillPrimitive() const
{
	return fMeshType;
}

void
TesselatorMesh::Invalidate()
{
	fIsMeshValid = false;
}

void
TesselatorMesh::Update()
{
	if (!fIsMeshValid)
	{
		fIsMeshValid = true;
		
		fSelfBounds.SetEmpty();
		for (int i=0; i<fMesh.Length(); i++)
		{
			fSelfBounds.Union( fMesh[i] );
		}
		
		if ( fUVs.Length() != fMesh.Length() )
		{
			fUVs.Empty();
			fUVs.Reserve( fMesh.Length() );
			Real invW = 0;
			if (!Rtt_RealIsZero(fSelfBounds.Width()))
			{
				invW = Rtt_RealDiv(1, fSelfBounds.Width());
			}
			Real invH = 0;
			if (!Rtt_RealIsZero(fSelfBounds.Height()))
			{
				invH = Rtt_RealDiv(1, fSelfBounds.Height());
			}
			for (int i=0; i<fMesh.Length(); i++)
			{
				Vertex2 v = fMesh[i];
				v.x = ( v.x - fSelfBounds.xMin ) * invW;
				v.y = ( v.y - fSelfBounds.yMin ) * invH;
				fUVs.Append(v);
			}
		}
	}
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------


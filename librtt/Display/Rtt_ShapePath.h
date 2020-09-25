//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_ShapePath_H__
#define _Rtt_ShapePath_H__

#include "Display/Rtt_ClosedPath.h"
#include "Display/Rtt_DisplayTypes.h"
#include "Display/Rtt_VertexCache.h"
#include "Display/Rtt_TesselatorShape.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

class MShapePathDelegate
{
	public:
		virtual void UpdateGeometry(
			Geometry& dst,
			const VertexCache& src,
			const Matrix& srcToDstSpace,
			U32 flags ) const = 0;
};

class ShapePath : public ClosedPath
{
	public:
		typedef ClosedPath Super;

	public:
		static ShapePath *NewCircle( Rtt_Allocator *pAllocator, Real radius );

	public:
		ShapePath( Rtt_Allocator *pAllocator, TesselatorShape *tesselator );
		virtual ~ShapePath();

	protected:
		void TesselateFill();

		void UpdateFill( RenderData& data, const Matrix& srcToDstSpace );

		void CalculateUV( ArrayVertex2& texVertices, Paint *paint, bool canTransformTexture );
	public:
		virtual void Update( RenderData& data, const Matrix& srcToDstSpace );
		virtual void UpdateResources( Renderer& renderer ) const;
		virtual void Translate( Real dx, Real dy );
		virtual void GetSelfBounds( Rect& rect ) const;

		void GetTextureVertices( ArrayVertex2& texVertices);
		Rect GetTextureExtents( const ArrayVertex2& texVertices ) const;
	public:
		virtual bool SetSelfBounds( Real width, Real height );

	public:
		TesselatorShape *GetTesselator() { return fTesselator; }
		const TesselatorShape *GetTesselator() const { return fTesselator; }

		const MShapePathDelegate *GetDelegate() const { return fDelegate; }
		void SetDelegate( const MShapePathDelegate *delegate ) { fDelegate = delegate; }

	protected:
		Geometry *fFillGeometry;
		ArrayIndex fIndexSource;
		VertexCache fFillSource;
		TesselatorShape *fTesselator;
		const MShapePathDelegate *fDelegate;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_ShapePath_H__
